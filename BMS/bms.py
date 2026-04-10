from summarize import read_all_BMS, BMS, BM
import matplotlib.pyplot as plt
import argparse


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


def scatter(path: str, all_bms: dict[str, BMS]):

    x, y = list(all_bms.keys())

    plt.xlabel(x)
    plt.ylabel(y)

    plt.xscale("log")
    plt.yscale("log")

    xs = [bm.time for bm in all_bms[x].values()]
    ys = [bm.time for bm in all_bms[y].values()]


    plt.scatter(
        xs, ys,
        marker="*",
        color="blue",
    )
    plt.plot([min(xs), max(xs)], [min(ys), max(ys)], "k-", alpha=0.5)
    plt.plot([max(xs), max(xs)], [min(ys), max(ys)], "r-", alpha=0.5)
    plt.savefig(path, dpi=300)
    plt.show()


def plot(path: str, all_bms: dict[str, BMS], min: int = 450, max: int = 500):
    for bms in BENCHMARKS:

        (x, y, name) = get_bms(list(all_bms[bms].values()), bms)
        plt.plot(
            x, y,
            linestyle="-",
            color=name[:-1],
            marker=name[-1],
            markersize=4,
            label=bms
        )

    plt.plot([5000, 5000], [min, max], "r-", alpha=0.2)

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


def parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Run BMS with optional configurations"
    )

    # Required arguments
    parser.add_argument("bms_name", help="Name of the benchmark set")
    parser.add_argument("filename", help="Input file")

    parser.add_argument("--min", type=int, default=0, help="Minimum time for plot")
    parser.add_argument("--max", type=int, default=500, help="Maximum time for plot")

    parser.add_argument("-s", "--scatter", action="store_true", help="Use scatter plot instead of line plot")

    parser.add_argument("-p", "--plot", nargs="*", default=COMBS, help="s fr ...], default = all")

    return parser


# python3 bms.py bms_name filename [comb]*
if __name__ == "__main__":

    COMBS = [
        "sr",
        "s",
        "sfr",
        "",
        "r",
        "sf",
        "fr",
        "f"
    ]

    args = parser().parse_args()

    if args.scatter:
        BENCHMARKS = [f"cadicall{f"-{bm}" if bm != "" else ""}" for bm in args.plot]
        if len(args.plot) < 2:
            BENCHMARKS = ["tabularallsat"] + BENCHMARKS

        all_bms = read_all_BMS(f"./{args.bms_name}/", BENCHMARKS)
        all_bms_sorted = sorted(all_bms)
        scatter(f"../tex/figures/{args.filename}.png", all_bms)
    else:
        BENCHMARKS = ["tabularallsat"] + [f"cadicall{f"-{bm}" if bm != "" else ""}" for bm in args.plot]

        all_bms = read_all_BMS(f"./{args.bms_name}/", BENCHMARKS)
        all_bms_sorted = sorted(all_bms)
        plot(f"../tex/figures/{args.filename}.png", all_bms, args.min, args.max)
