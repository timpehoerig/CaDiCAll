#!/usr/bin/env bash

tmp_cadicall_negated_models=./tmp_cadicall_negated_models.txt

path_fuzzed_cnf=./tmp/tmp_fuzzed.cnf
path_out_cadicall=./tmp/tmp_terminal_out.txt

mkdir -p ./tmp/


fuzz=true
shrink=false

while getopts "sc:" option; do
  case "$option" in
    c)
      path_fuzzed_cnf="$OPTARG"
      fuzz=false
      ;;
    s)
      shrink=true
      ;;
    *)
      echo "This is a script for running the stats"
      echo
      echo "USAGE: ./stats.sh [-s] [-c <path_to_cnf>]"
      echo
      echo "-c <path_to_cnf>    Uses the given cnf"
      echo "-s                  Allow shrunken models"
      echo
      echo "If no cnf is provided, a random cnf is fuzzed with cnfuzz (--tiny option is on)"
      exit 1
      ;;
  esac
done


if $fuzz; then
    echo "Script: fuzz cnf into $path_fuzzed_cnf"
    ../../cnfuzz/cnfuzz --tiny > $path_fuzzed_cnf 
fi

if $shrink; then
  echo "Script: run cadicall -s"
  ../src/cadicall -p -s $path_fuzzed_cnf > $path_out_cadicall
else
  echo "Script: run cadicall"
  ../src/cadicall -p $path_fuzzed_cnf > $path_out_cadicall
fi

mv "$tmp_cadicall_negated_models" ./tmp/ 2>/dev/null

echo

# List of event names
events=(
    cadicall_check_literal
    cadicall_implicant_shrinking
    cadicall_push
    cadicall_pop
    cadicall_highest_dl_to_flip
    cadicall_cb_check_found_model
    cadicall_notify_assignment
    cadicall_notify_backtrack
    cadicall_notify_new_decision_level
    cadicall_notify_backtrack
    cadicall_cb_decide
    cadicall_forced_backtrack_model_found
    cadicall_cb_propagate
    cadicall_cb_add_reason_clause_lit
)

# Associative arrays to store results
declare -A time
declare -A percent

# Extract data

for event in "${events[@]}"; do
    read time[$event] percent[$event] < <(
        awk -v e="$event" '
            $NF == e {
                gsub(/%/, "", $(NF-1))
                print $(NF-2), $(NF-1)
            }
        ' "$path_out_cadicall"
    )
done

# Print results
for event in "${events[@]}"; do
    printf "%-30s %8s %8s\n" \
        "$event:" \
        "${time[$event]}" \
        "${percent[$event]}"
done

