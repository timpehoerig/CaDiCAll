from summarize import read_all_BMS, BMS
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
            if "-s" in solver:
                continue
            if count != solvers["tabularallsat"]:
                if bm_name not in out:
                    out[bm_name] = {"tabularallsat": solvers["tabularallsat"]}
                out[bm_name][solver] = count
    return out


def plot_all_solvers(data: dict[str, BMS]):
    fig, axes = plt.subplots(3, 1, figsize=(9, 11), sharex=False)

    for solver_name, solver_dict in data.items():

        if "-s" in solver_name:
            continue

        benchmarks = sorted(solver_dict.items())

        cumulative_time = []
        y_time = []
        y_real = []
        y_space = []

        total = 0.0

        for name, bm in benchmarks:
            total += bm.time
            cumulative_time.append(total)

            y_time.append(bm.time)
            y_real.append(bm.real)
            y_space.append(bm.space)

        axes[0].plot(cumulative_time, y_time, marker='o', label=solver_name)
        axes[1].plot(cumulative_time, y_real, marker='o', label=solver_name)
        axes[2].plot(cumulative_time, y_space, marker='o', label=solver_name)

    axes[0].set_ylabel("time")
    axes[0].set_title("time vs cumulative time")

    axes[1].set_ylabel("real")
    axes[1].set_title("real vs cumulative time")

    axes[2].set_ylabel("space")
    axes[2].set_xlabel("cumulative time")
    axes[2].set_title("space vs cumulative time")

    for ax in axes:
        ax.legend()

    plt.tight_layout()
    plt.show()


# usage
# plot_all_solvers(data)


# Beispielaufruf
# solver_dict = data['cadicall']
# plot_solver(solver_dict, "cadicall")


if __name__ == "__main__":
    all_bms = read_all_BMS("./")
    for name, bms in check(inverse(all_bms)).items():
        print(name, bms)
    # plot_all_solvers(all_bms)
