from summarize import read_all_BMS, BMS, BM
import matplotlib.pyplot as plt
import sys


def get_bms(bms: list[BM], name: str) -> tuple[list[float], list[int], str]:
    y = sorted(map(lambda bm: bm.time, bms))
    y_sum: list[float] = list()
    for yi in y:
        y_sum.append(yi)
    x = list(range(len(y)))
    return y_sum, x, get_single(name)


def get_single(name: str) -> str:
    if name.startswith("tabularallsat"):
        return "crimson*"
    name = "" if "-" not in name else name[name.index("-", len(name) - 4):]
    if "sfr" in name:
        return "m*"
    elif "sr" in name:
        return "orange*"
    elif "sf" in name:
        return "md"
    elif "fr" in name:
        return "m*"
    elif "s" in name:
        return "goldd"
    elif "f" in name:
        return "ms"
    elif "r" in name:
        return "skyblue*"
    else:
        return "limes"


def plot(path: str, all_bms: dict[str, BMS], min: int = 450, max: int = 500):
    for bms in sorted(all_bms):

        (x, y, name) = get_bms(list(all_bms[bms].values()), bms)
        plt.plot(
            x, y,
            linestyle="-",
            color=name[:-1],
            marker=name[-1],
            markersize=4,
            label=bms
        )

    plt.ylim(min, max)
    plt.xlabel("time")
    plt.ylabel("benchmarks")
    plt.legend(
        fontsize=8,      # text size
        markerscale=1,    # marker size in legend
        handlelength=2    # length of line in legend
    )
    plt.savefig(path, dpi=300)
    plt.show()


# python3 bms.py bms_name filename [comb]*
if __name__ == "__main__":

    COMBS = [
        "",
        "f",
        "fr",
        "r",
        "s",
        "sf",
        "sfr",
        "sr"
    ]

    args = sys.argv[1:]

    name_bms = args[0]

    file_name = args[1]

    min = int(args[2]) if len(args) > 2 else 0
    max = int(args[3]) if len(args) > 2 else 500

    combs = args[4:] if len(args) > 4 else COMBS

    BENCHMARKS = [f"cadicall{f"-{bm}" if bm != "" else ""}" for bm in combs] + ["tabularallsat"]

    all_bms = read_all_BMS(f"./{name_bms}/", BENCHMARKS)
    all_bms_sorted = sorted(all_bms)

    plot(f"../tex/figures/{file_name}.png", all_bms, min, max)
