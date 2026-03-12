#!/bin/bash

tmp_cadicall_negated_models=./tmp_cadicall_negated_models.txt

path_fuzzed_cnf=./tmp/tmp_fuzzed.cnf
path_out_bc=./tmp/tmp_terminal_out.txt
path_out_dualiza=./tmp/tmp_out_dualiza.txt

mkdir -p ./tmp/

echo "fuzz > $path_fuzzed_cnf"
../../cnfuzz/cnfuzz --tiny > $path_fuzzed_cnf

echo "cadicall > $path_out_bc"
# running cadicall with --shrink may lead to a different count of models
../src/cadicall -c -r $path_fuzzed_cnf > $path_out_bc

mv $tmp_cadicall_negated_models ./tmp/

echo "dualiza > $path_out_dualiza"
../../dualiza/dualiza $path_fuzzed_cnf > $path_out_dualiza

num_bc=$(grep -A 1 "NUMBER SATISFYING ASSIGNMENTS" $path_out_bc | tail -n 1)

num_dualiza=$(grep -v '^[[:space:]]*$' $path_out_dualiza | tail -n 1)

if [ "$num_bc" = "$num_dualiza" ]; then
    echo "eq $num_bc"
else
    echo "neq $num_bc vs $num_dualiza"
fi
