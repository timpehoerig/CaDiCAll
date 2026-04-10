from BM import BM, BMS
import os


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
            if "s MODEL COUNT" not in content:
                return -1
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


def read_all_BMS(directory: str, bmss: list[str]) -> dict[str, BMS]:
    out: dict[str, BMS] = dict()
    for dir in os.listdir(directory):
        if dir not in bmss:
            continue
        out[dir] = read_BMS(os.path.join(directory, dir))
    return out


def inverse(all_bms: dict[str, BMS]):
    out: dict[str, dict[str, int]] = dict()
    for benchmark_set in all_bms:
        for name, benchmark in all_bms[benchmark_set].items():
            if name not in out:
                out[name] = dict()
            out[name][benchmark_set] = benchmark.count
    return out


def check(d: dict[str, dict[str, int]]) -> dict[str, dict[str, int]]:
    out: dict[str, dict[str, int]] = dict()
    for bm_name, solvers in d.items():
        for solver, count in solvers.items():
            # if count == -1:  # or "-s" in solver:
            #    continue
            if count != solvers["tabularallsat"]:
                if bm_name not in out:
                    out[bm_name] = {"tabularallsat": solvers["tabularallsat"]}
                out[bm_name][solver] = count
    return out


if __name__ == "__main__":
    bms = read_all_BMS("./mc2025/", ["tabularallsat", "cadicall-sfr", "cadicall-sr", "cadicall-sf", "cadicall-fr", "cadicall-s", "cadicall-f", "cadicall-r", "cadicall"])
    print(bms.keys())

    d = inverse(bms)

    d = check(d)

    for k, v in d.items():
        print(k, v)
