import networkx as nx
import matplotlib.pyplot as plt
from networkx.drawing.nx_pydot import graphviz_layout


def parse_stack_blocks(filename):
    stacks = []

    with open(filename) as f:
        lines = f.readlines()

    i = 0
    while i < len(lines):
        if lines[i].startswith("c stack:"):

            val_line = None
            for j in range(i + 1, min(i + 6, len(lines))):
                if lines[j].startswith("c val"):
                    val_line = lines[j]
                    break

            if val_line:
                parts = val_line.split("|")[1:]
                stack = []

                for p in parts:
                    p = p.strip()
                    if not p:
                        continue
                    stack.append(int(p.split()[0]))

                stacks.append(stack)

        i += 1

    return stacks


def longest_common_prefix(a, b):
    i = 0
    while i < min(len(a), len(b)) and a[i] == b[i]:
        i += 1
    return i


def build_tree(stacks):

    G = nx.DiGraph()

    root = "root"
    G.add_node(root, label="root")

    node_counter = 0

    prev_stack = []
    prev_nodes = [root]

    for stack in stacks:

        # find backtracking point
        lcp = longest_common_prefix(prev_stack, stack)

        current_nodes = prev_nodes[: lcp + 1]

        parent = current_nodes[-1]

        # create nodes for new suffix
        for val in stack[lcp:]:
            node_counter += 1
            node_id = f"n{node_counter}"

            G.add_node(node_id, label=str(val))
            G.add_edge(parent, node_id)

            parent = node_id
            current_nodes.append(node_id)

        prev_stack = stack
        prev_nodes = current_nodes

    return G


def plot_tree(G):

    pos = graphviz_layout(G, prog="dot")
    labels = nx.get_node_attributes(G, "label")

    plt.figure(figsize=(12, 10))

    nx.draw(
        G,
        pos,
        labels=labels,
        node_size=2000,
        node_color="lightblue",
        arrows=True,
        font_size=10
    )

    plt.title("SAT Search Tree")
    plt.show()


def main():
    filename = "tmp_out.txt"

    stacks = parse_stack_blocks(filename)
    print("Parsed stacks:", stacks)

    G = build_tree(stacks)
    plot_tree(G)


if __name__ == "__main__":
    main()