#!/usr/bin/env bash

shrink=false

while getopts "s" option; do
  case "$option" in
    s) shrink=true ;;
    *)
      echo "USAGE: ./stats_avg.sh [-s]"
      exit 1
      ;;
  esac
done

SCRIPT=(./stats.sh)

if $shrink; then
    echo "stats.sh -s"
    SCRIPT+=( -s )
fi


declare -A sum_time sum_pct cur_time cur_pct

metrics=(
    wbc_check_literal
    wbc_implicant_shrinking
    wbc_push
    wbc_pop
    wbc_highest_dl_to_flip
    wbc_cb_check_found_model
    wbc_notify_assignment
    wbc_notify_backtrack
    wbc_notify_new_decision_level
    wbc_cb_decide
    wbc_forced_backtrack_model_found
)

# Metrics allowed to be missing when -s is NOT used
optional_metrics=(
    wbc_check_literal
    wbc_implicant_shrinking
)

runs=0

while true; do
    bad_run=0
    zero_run=1
    started=0

    for m in "${metrics[@]}"; do
        unset cur_time[$m]
        unset cur_pct[$m]
    done

    while read -r line; do
        # Wait until first empty line
        if (( ! started )); then
            [[ -z $line ]] && started=1
            continue
        fi

        # name:   (no numbers)
        if [[ $line =~ ^([a-z_]+):[[:space:]]*$ ]]; then
            name="${BASH_REMATCH[1]}"

            if ! $shrink; then
                for opt in "${optional_metrics[@]}"; do
                    [[ $name == "$opt" ]] && continue 2
                done
            fi

            bad_run=1
            continue
        fi

        # name: time percent
        if [[ $line =~ ^([a-z_]+):[[:space:]]+([0-9.]+)[[:space:]]+([0-9.]+) ]]; then
            name="${BASH_REMATCH[1]}"
            time="${BASH_REMATCH[2]}"
            pct="${BASH_REMATCH[3]}"

            cur_time[$name]=$time
            cur_pct[$name]=$pct

            if [[ $time != 0 || $pct != 0 ]]; then
                zero_run=0
            fi
        fi
    done < <("${SCRIPT[@]}")

    if (( bad_run )) || (( zero_run )); then
        continue
    fi

    ((runs++))

    for m in "${metrics[@]}"; do
        sum_time[$m]=$(awk "BEGIN {print ${sum_time[$m]:-0} + ${cur_time[$m]:-0}}")
        sum_pct[$m]=$(awk "BEGIN {print ${sum_pct[$m]:-0} + ${cur_pct[$m]:-0}}")
    done

    printf "\033[H\033[J"
    echo "--- running average after $runs runs ---"

    for m in "${metrics[@]}"; do
        avg_time=$(awk "BEGIN {print ${sum_time[$m]} / $runs}")
        avg_pct=$(awk "BEGIN {print ${sum_pct[$m]} / $runs}")

        printf "%-35s %8.2f %8.2f\n" \
            "$m:" \
            "$avg_time" \
            "$avg_pct"
    done
done
