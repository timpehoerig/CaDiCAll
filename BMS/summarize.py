from BM import BM, BMS
import os


BENCHMARKS = [
    "cadicall-24-03",
    "cadicall-24-03-f",
    "cadicall-24-03-fr",
    "cadicall-24-03-r",
    "cadicall-24-03-s",
    "cadicall-24-03-sf",
    "cadicall-24-03-sfr",
    "cadicall-24-03-sr",
    "tabularallsat",
]


def get_one(lst: list[str], one: str) -> float:
    for line in lst:
        if one in line:
            return float(line.split("\t")[-1].split(" ")[0])
    else:
        return -1


def read_err(path: str) -> dict[str, float]:
    out: dict[str, float] = dict()
    with open(path, 'r') as file:
        content = file.read()
        lines = content.split("\n")

        out["time"] = get_one(lines, "time:")
        out["real"] = get_one(lines, "real:")
        out["space"] = get_one(lines, "space:")

        out["tlim"] = get_one(lines, "time limit:")
        out["rlim"] = get_one(lines, "real time limit:")
        out["slim"] = get_one(lines, "space limit:")

        out["status"] = float("segmentation fault" in content)

    return out


def read_log(path: str) -> int:
    with open(path, 'r') as file:
        content = file.read().split("\n")
        if "tabularallsat" in path:
            idx = content.index("s MODEL COUNT")
        else:
            idx = content.index("NUMBER SATISFYING ASSIGNMENTS")
        return int(content[idx + 1])


def read_BM(name: str) -> BM:
    stats = read_err(f"{name}.err")
    if stats["time"] >= stats["tlim"] or stats["real"] >= stats["rlim"] or stats["space"] >= stats["slim"]:
        count = -1
    elif stats["status"] == 1:  # segmentation fault
        count = -2
    else:
        count = read_log(f"{name}.log")
    return BM(count, *stats.values())


def read_BMS(path_to_dir: str) -> BMS:
    """
    Given a directory with benchmarks, returns a dictionary mapping names to stats
    """
    out: BMS = dict()
    with open(os.path.join(path_to_dir, "benchmarks")) as file:
        for line in file:
            bm = line.split("\t")[-1].strip()

            if bm in ["results-BC.csv.xz"]:
                continue

            out[bm] = read_BM(os.path.join(path_to_dir, bm))

    return out


def read_all_BMS(directory: str) -> dict[str, BMS]:
    out: dict[str, BMS] = dict()
    for dir in os.listdir(directory):
        if dir not in BENCHMARKS:
            continue
        out[dir] = read_BMS(os.path.join(directory, dir))
    return out


if __name__ == "__main__":
    bms = read_all_BMS("./")
    print(bms.keys())
