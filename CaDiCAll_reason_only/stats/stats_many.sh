#!/usr/bin/env bash

TIMEOUT=60  # seconds


shrink=false
dir=""

while getopts "sd:" option; do
  case "$option" in
    s)
      shrink=true
      ;;
    d)
      dir="$OPTARG"
      ;;
    *)
      echo "USAGE: ./stats_avg.sh [-s] [-d <cnf_dir>]"
      exit 1
      ;;
  esac
done

# Base command
BASE_SCRIPT=(./stats_one.sh)

if $shrink; then
    echo "stats_one.sh -s"
    BASE_SCRIPT+=( -s )
fi

# Validate directory if given
if [[ -n $dir ]]; then
    if [[ ! -d $dir ]]; then
        echo "Error: '$dir' is not a directory"
        exit 1
    fi
    shopt -s nullglob
    cnf_files=( "$dir"/*.cnf )
    shopt -u nullglob

    if (( ${#cnf_files[@]} == 0 )); then
        echo "Error: no .cnf files found in '$dir'"
        exit 1
    fi
fi

declare -A sum_time sum_pct cur_time cur_pct

metrics=(
    cadicall_check_literal
    cadicall_implicant_shrinking
    cadicall_push
    cadicall_pop
    cadicall_highest_dl_to_flip
    cadicall_cb_check_found_model
    cadicall_notify_assignment
    cadicall_notify_backtrack
    cadicall_notify_new_decision_level
    cadicall_cb_decide
    cadicall_forced_backtrack_model_found
    cadicall_cb_propagate
    cadicall_cb_add_reason_clause_lit
)

optional_metrics=(
    cadicall_check_literal
    cadicall_implicant_shrinking
)

runs=0

run_once() {
    local cmd=("$@")

    bad_run=0
    zero_run=1
    started=0

    for m in "${metrics[@]}"; do
        unset cur_time[$m]
        unset cur_pct[$m]
    done

    while read -r line; do
        # Wait for first empty line
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
    # done < <("${cmd[@]}")
    done < <(timeout $TIMEOUT "${cmd[@]}")
    if (( ${PIPESTATUS[0]} == 124 )); then
        echo "Command timed out after $TIMEOUT seconds"
        return 1
    fi



    if (( bad_run )) || (( zero_run )); then
        return 1
    fi

    ((runs++))

    for m in "${metrics[@]}"; do
        sum_time[$m]=$(awk "BEGIN {print ${sum_time[$m]:-0} + ${cur_time[$m]:-0}}")
        sum_pct[$m]=$(awk "BEGIN {print ${sum_pct[$m]:-0} + ${cur_pct[$m]:-0}}")
    done

    # printf "\033[H\033[J"
    echo
    echo "--- running average after $runs runs ---"

    for m in "${metrics[@]}"; do
        avg_time=$(awk "BEGIN {print ${sum_time[$m]} / $runs}")
        avg_pct=$(awk "BEGIN {print ${sum_pct[$m]} / $runs}")

        printf "%-35s %8.2f %8.2f\n" \
            "$m:" \
            "$avg_time" \
            "$avg_pct"
    done

    return 0
}

# =========================
# Main control flow
# =========================

if [[ -n $dir ]]; then
    # Run once per CNF file
    for cnf in "${cnf_files[@]}"; do
        echo "Processing: $cnf"
        run_once "${BASE_SCRIPT[@]}" -c "$cnf"
    done
else
    # Infinite loop (original behavior)
    while true; do
        run_once "${BASE_SCRIPT[@]}"
    done
fi
