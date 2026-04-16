from summarize import read_all_BMS, BMS, BM
import matplotlib.pyplot as plt
import argparse


def get_bms(bms: list[BM], name: str) -> tuple[list[float], list[int], str]:
    #y = sorted(map(lambda bm: bm.time, bms))
    y = sorted(bms, key=lambda bm: bm.time)
    y_sum: list[float] = list()
    for yi in y:
        if yi.time >= 5000 or yi.real >= 5000:
            continue
        y_sum.append(yi.time)
    x = list(range(len(y_sum)))
    return y_sum, x, get_single(name)


def get_single(name: str) -> str:
    if name.startswith("tabularallsat"):
        return "darkred*"
    if name.startswith("dualiza"):
        return "crimsons"
    name = "" if "-" not in name else name[name.index("-", len(name) - 4):]
    if "sfr" in name:
        return C[0] + "*"
    elif "sr" in name:
        return "orange*"
    elif "sf" in name:
        return C[1] + "d"
    elif "fr" in name:
        return C[2] + "^"
    elif "s" in name:
        return "goldd"
    elif "f" in name:
        return C[3] + "o"
    elif "r" in name:
        return "skyblue^"
    else:
        return "limes"


C = ["#EE82EE",  # violet
     "#DA70D6",  # orchid
     "#BA55D3",  # medium orchid
     "#8A2BE2"]  # blue violet


def scatter(path: str, all_bms: dict[str, BMS], color_ref: str):

    x, y = list(all_bms.keys())

    plt.xlabel(x)
    plt.ylabel(y)

    plt.xscale("log")
    plt.yscale("log")

    xs = [bm.time for bm in all_bms[x].values()]
    ys = [bm.time for bm in all_bms[y].values()]

    c = get_single(color_ref)[:-1]

    plt.scatter(
        xs, ys,
        marker="o",
        color=c,
    )

    # dia
    plt.plot([0, max(*xs, *ys)], [0, max(*xs, *ys)], "k-", alpha=0.5)

    # cut off
    plt.plot([5000, 5000], [0, 5000], "r-", alpha=0.5)
    plt.plot([0, 5000], [5000, 5000], "r-", alpha=0.5)

    plt.axis("equal")

    ax = plt.gca()
    ax.set_box_aspect(1)

    plt.savefig(path, dpi=300)
    plt.show()


def plot(path: str, all_bms: dict[str, BMS], min: int = 450, maxx: int = 500):

    amount: int = 0

    for bms in BENCHMARKS:

        amount = max(amount, len(all_bms[bms].values()))

        (x, y, name) = get_bms(list(all_bms[bms].values()), bms)
        plt.plot(
            x, y,
            linestyle="-",
            color=name[:-1],
            marker=name[-1],
            markersize=4,
            label=bms
        )

    # plt.plot([5000, 5000], [min, max], "r-", alpha=0.2)

    plt.ylim(min, maxx)
    plt.xlabel("time (s)")
    plt.ylabel(f"benchmarks (n = {amount})")

    # plt.yscale("log")

    handles, labels = plt.gca().get_legend_handles_labels()

    def sort_key(h):
        y = h.get_ydata()
        x = h.get_xdata()
        return (y[-1], x[-1])  # (height, x-value)

    # Sort: highest y first, but for ties smaller x first
    handles, labels = zip(*sorted(
        zip(handles, labels),
        key=lambda hl: (hl[0].get_ydata()[-1], -hl[0].get_xdata()[-1]),
        reverse=True
    ))

    plt.legend(
        handles,
        labels,
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
        "t",
        "d",
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

    BENCHMARKS = []
    for bm in args.plot:
        if bm == "t":
            BENCHMARKS.append("tabularallsat")
        elif bm == "d":
            BENCHMARKS.append("dualiza")
        elif bm == "":
            BENCHMARKS.append("cadicall")
        else:
            BENCHMARKS.append(f"cadicall-{bm}")

    all_bms = read_all_BMS(f"./{args.bms_name}/", BENCHMARKS)
    all_bms_sorted = sorted(all_bms)

    if args.scatter:
        scatter(f"../tex/figures/{args.filename}.png", all_bms, BENCHMARKS[0])
    else:
        plot(f"../tex/figures/{args.filename}.png", all_bms, args.min, args.max)
