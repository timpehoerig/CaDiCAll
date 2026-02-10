#!/usr/bin/env bash

tmp_wbcp_negated_models=./tmp_wbcp_negated_models.txt

path_fuzzed_cnf=./tmp/tmp_fuzzed.cnf
path_out_wbc=./tmp/tmp_terminal_out.txt

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
  echo "Script: run wbcp_enum -s"
  ../src/wbcp_enum -p -s $path_fuzzed_cnf > $path_out_wbc
else
  echo "Script: run wbcp_enum"
  ../src/wbcp_enum -p $path_fuzzed_cnf > $path_out_wbc
fi

mv "$tmp_wbcp_negated_models" ./tmp/ 2>/dev/null

echo

# List of event names
events=(
    wbc_check_literal
    wbc_implicant_shrinking
    wbc_push
    wbc_pop
    wbc_highest_dl_to_flip
    wbc_cb_check_found_model
    wbc_notify_assignment
    wbc_notify_backtrack
    wbc_notify_new_decision_level
    wbc_notify_backtrack
    wbc_cb_decide
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
        ' "$path_out_wbc"
    )
done

# Print results
for event in "${events[@]}"; do
    printf "%-30s %8s %8s\n" \
        "$event:" \
        "${time[$event]}" \
        "${percent[$event]}"
done

