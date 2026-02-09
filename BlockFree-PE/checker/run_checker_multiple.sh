#!/bin/bash

verified_count=0

while true; do
    output="$(./run_checker.sh)"

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
