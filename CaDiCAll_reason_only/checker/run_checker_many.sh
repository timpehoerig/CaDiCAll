#!/bin/bash

shrink=false

while getopts "s" option; do
  case "$option" in
    s)
      shrink=true
      echo "running with -s"
      ;;
    *)
      echo "This is a script for running the checker multiple times"
      echo
      echo "USAGE: ./run_schecker_multiple.sh [-s]"
      echo
      echo "-s Allow shrunken models"
      exit 1
      ;;
  esac
done

verified_count=0

while true; do

    if $shrink; then
        output="$(./run_checker.sh -s)"
    else
        output="$(./run_checker.sh)"
    fi

    # Get last and second-last lines
    last_line="$(printf '%s\n' "$output" | tail -n 1)"
    second_last_line="$(printf '%s\n' "$output" | tail -n 2 | head -n 1)"

    # Check second last line
    if [[ "$second_last_line" == *"c cnf has no models" ]]; then
        continue
    fi

    # Check last line
    if [[ "$last_line" == "s VERIFIED" ]]; then
        ((verified_count++))
        echo -ne "VERIFIED: $verified_count \r"
    else
        echo -ne "VERIFIED: $verified_count\n"
        echo "Last line is not VERIFIED: '$last_line'"
        break
    fi
done
