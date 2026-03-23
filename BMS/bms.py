from summarize import read_all_BMS, BMS


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
            if count == -1:  # or "-s" in solver:
                continue
            if count != solvers["tabularallsat"]:
                if bm_name not in out:
                    out[bm_name] = {"tabularallsat": solvers["tabularallsat"]}
                out[bm_name][solver] = count
    return out


if __name__ == "__main__":
    all_bms = read_all_BMS("./")
    for name, bms in check(inverse(all_bms)).items():
        print(name, bms)
