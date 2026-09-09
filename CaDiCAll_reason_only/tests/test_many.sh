#!/usr/bin/env bash

count_nonzero=0

while true; do
    # Run the script and capture its output
    output=$(./test_one.sh)

    # Extract the last line
    last_line=$(printf "%s\n" "$output" | tail -n 1)

    # If last line starts with "eq "
    if [[ "$last_line" =~ ^eq[[:space:]]+([0-9]+)$ ]]; then
        value="${BASH_REMATCH[1]}"

        # Count non-zero eq values
        if [[ "$value" -ne 0 ]]; then
            ((count_nonzero++))
            echo " eq: $count_nonzero"
        fi

        # Continue looping
        continue
    fi

    # If last line starts with "neq" or "neg", stop and print it
    if [[ "$last_line" =~ ^ne[qg][[:space:]]+.* ]]; then
        echo ""
        echo "$last_line"
        break
    fi

    # Optional: handle unexpected output defensively
    echo "Unexpected last line:"
    echo "$last_line"
    break
done
