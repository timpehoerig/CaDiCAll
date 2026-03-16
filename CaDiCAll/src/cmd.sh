#!/usr/bin/env bash

if [ $# -ne 1 ]; then
    echo "Usage: $0 <cnf>"
    exit 1
fi

cnf="$1"

last_line=$(./cadicall -c "$cnf" | tail -n 1 | tr -d '[:space:]')

if [ "$last_line" = "0" ]; then
    echo "0"
    exit 0
else
    echo "1"
    exit 1
fi