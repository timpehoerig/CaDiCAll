import argparse
import os
from os import path
import subprocess
from enum import Enum
import shutil

from typing import Optional


# PATHS
PATH_TMP = "tmp"
PATH_DUALIZA = "../../dualiza/dualiza"
PATH_CNFUZZ = "../../cnfuzz/cnfuzz"
PATH_CADICALL = "../src/cadicall"


def fuzz(path_cnf: str, size: str = "--tiny") -> None:
    create_file(path_cnf, run_cpp(PATH_CNFUZZ, size).stdout)


def dualiza(path_cnf: str) -> int:
    result = run_cpp(PATH_DUALIZA, path_cnf)
    return int(result.stdout.split("\n")[1])


def fuzz_tiny_cnf_with_models(path_cnf: str) -> int:
    model_count: int = 0
    while model_count == 0:
        fuzz(path_cnf)
        model_count = dualiza(path_cnf)
    return model_count


def cadicall_count(*args: str) -> int:
    result = run_cpp(PATH_CADICALL, "-c", *args)
    result_lst = result.stdout.split("\n")
    idx = result_lst.index("NUMBER SATISFYING ASSIGNMENTS")
    return int(result_lst[idx + 1])


def run_once(options_lst: list[list[str]], cnf_path: Optional[str] = None) -> dict[str, int]:
    if cnf_path is None:
        cnf_path = path.join(PATH_TMP, "fuzzed.cnf")
        mc_dualiza = fuzz_tiny_cnf_with_models(cnf_path)
    else:
        mc_dualiza = dualiza(cnf_path)

    results: dict[str, int] = {"dualiza": mc_dualiza}

    for options in options_lst:
        key = "cadicall" + "".join(options).replace("-", "_")
        value = cadicall_count(*options, cnf_path)

        results[key] = value

    return results


def run_cpp(executable: str, *args: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [executable, *args],  # executable and arguments
        capture_output=True,  # capture stdout and stderr
        text=True             # return strings instead of bytes
    )


def create_dir(path: str) -> None:
    os.makedirs(path, exist_ok=True)


def create_file(path: str, content: str) -> None:
    with open(path, "w") as f:
        f.write(content)


def check_results(results: dict[str, int], stats: dict[str, int]) -> bool:
    truth = results["dualiza"]
    flag = True
    for name, result in results.items():
        if truth == result:
            if name not in stats:
                stats[name] = 0
            stats[name] += 1
        else:
            flag = False
    return flag


class Color(Enum):
    RED = "\033[91m"
    GREEN = "\033[92m"
    YELLOW = "\033[93m"
    BLUE = "\033[94m"
    MAGENTA = "\033[95m"
    CYAN = "\033[96m"
    WHITE = "\033[97m"
    RESET = "\033[0m"


class BackgroundColor(Enum):
    RED = "\033[41m"
    GREEN = "\033[42m"
    YELLOW = "\033[43m"
    BLUE = "\033[44m"
    MAGENTA = "\033[45m"
    CYAN = "\033[46m"
    WHITE = "\033[47m"
    RESET = "\033[0m"


def color(text: str, color: Color) -> str:
    return f"{color.value}{text}{Color.RESET.value}"


def background_color(text: str, color: BackgroundColor) -> str:
    return f"\033[48;5;{color.value}m{text}{Color.RESET.value}"


def pretty(file_name: str, results: dict[str, int], count: int, head: bool = True, counts: Optional[tuple[int, int]] = None) -> str:
    top: str = "count\t"
    bot: str = f"{count}\t"

    if counts is not None:
        top += "correct\tfalse\t"
        bot += f"{color(str(counts[0]), Color.GREEN)}\t{color(str(counts[1]), Color.RED)}\t"

    top += "dualiza\t"
    bot += f"{results['dualiza']}\t"

    top += "CaDiCAll:\t"
    bot += f"{color('verified', Color.GREEN) if check_results(results, {}) else color('failed  ', Color.RED)}\t"

    for name, result in results.items():
        if name == "dualiza":
            continue
        top += f"{name.replace("cadicall", "").replace("_", "-").strip()}\t"
        bot += f"{result}\t"

    top += "CNF"
    bot += file_name

    if head:
        return "\033[2J\033[H\n\n" + color(top, Color.MAGENTA) + "\n" + bot
    return bot


def parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Running CaDiCAll with all options on fuzzed or provided CNFs."
    )

    parser.add_argument(
        "-c", "--cnf",
        type=str,
        default="",
        help="Path to a CNF file."
    )

    parser.add_argument(
        "-d", "--directory",
        type=str,
        default="",
        help="Path to a directory containing CNF files."
    )

    return parser


if __name__ == "__main__":
    args = parser().parse_args()

    # create tmp dir if it does not exist
    create_dir(PATH_TMP)
    # options = [
    #     ["-f"],
    #     ["-r"],
    #     ["-s"],
    #     ["-r", "-f"],
    #     ["-r", "-s"],
    #     ["-f", "-s"],
    #     ["-f", "-s", "-r"],
    # ]

    options = [["-s"]]

    stats: dict[str, int] = dict()
    count_correct: int = 0
    count_false: int = 0
    count: int = 0

    if args.cnf != "":
        results = run_once(options, args.cnf)
        check_results(results, stats)
        print(pretty(args.cnf, results, 1))

    elif args.directory != "":
        for filename in os.listdir(args.directory):
            print(filename)
            if filename.endswith(".cnf"):
                cnf_path = path.join(args.directory, filename)
                results = run_once(options, cnf_path)
                if check_results(results, stats):
                    count_correct += 1
                else:
                    count_false += 1
                count += 1
                print(pretty(filename, results, count, True, (count_correct, count_false)))

    else:
        while True:
            results = run_once(options)
            if not check_results(results, stats):
                count_false += 1
                shutil.copy("./tmp/fuzzed.cnf", f"./failed/fuzzed_failed_{count}_{results['dualiza']}.cnf")
            else:
                count_correct += 1
            count += 1
            print(pretty("tmp_fuzzed.cnf", results, count, True, (count_correct, count_false)))
