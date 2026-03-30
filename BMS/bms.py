from summarize import read_all_BMS, BMS, BM
import matplotlib.pyplot as plt


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


def get_bms(bms: list[BM], name: str) -> tuple[list[float], list[int], str]:
    y = sorted(map(lambda bm: bm.time, bms))
    y_sum: list[float] = list()
    for yi in y:
        y_sum.append(yi)
    x = list(range(len(y)))
    return y_sum, x, get_single(name)


def get_single(name: str) -> str:
    if name.startswith("tabularallsat"):
        return "c*"
    name = name[name.index("-", len(name) - 4):]
    if "sfr" in name:
        return "k*"
    elif "sr" in name:
        return "m*"
    elif "sf" in name:
        return "kd"
    elif "fr" in name:
        return "k*"
    elif "s" in name:
        return "md"
    elif "f" in name:
        return "ks"
    elif "r" in name:
        return "r*"
    else:
        return "gs"


if __name__ == "__main__":
    all_bms = read_all_BMS("./")
    for i, bms in enumerate(sorted(all_bms)):
        (x, y, name) = get_bms(list(all_bms[bms].values()), bms)
        plt.plot(
            x, y,
            linestyle="-",
            marker=name[1],
            color=name[0],
            markersize=8,
            label=bms
        )

    plt.ylim(450, 500)
    plt.xlabel("time")
    plt.ylabel("benchmarks")
    plt.legend()
    plt.show()
