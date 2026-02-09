#!/usr/bin/env bash

dir="$1"

if [[ -z "$dir" ]]; then
    echo "Usage: $0 <folder>"
    exit 1
fi

if [[ ! -d "$dir" ]]; then
    echo "Error: '$dir' is not a directory"
    exit 1
fi

shopt -s nullglob

for file in "$dir"/*.cnf; do
    out="$(./run_checker.sh -c "$file")"
    last_line="$(printf '%s\n' "$out" | tail -n 1)"
    echo "$last_line - $file"
done
