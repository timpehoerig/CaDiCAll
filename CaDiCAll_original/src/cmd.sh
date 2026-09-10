#!/usr/bin/env bash

cnf="$1"


# run CaDiCAll
out=$(./cadicall -s "$cnf")

last_line=$(../checker/checker -s "$cnf" tmp_cadicall_negated_models.txt | tail -n 1)

if [ "$last_line" = "s PROBLEM" ]; then
    exit 1
else
    exit 0
fi
