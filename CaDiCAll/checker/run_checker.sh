#!/bin/bash

echo "clean all tmps"
rm -f tmp*

# paths
tmp_cadicall_negated_models=./tmp_cadicall_negated_models.txt

# default path for cnf
path_fuzzed_cnf=./tmp_fuzzed.cnf

fuzz=true
shrink=false

while getopts "c:s" option; do
  case "$option" in
    c)
      path_fuzzed_cnf="$OPTARG"
      fuzz=false
      ;;
    s)
      shrink=true
      echo "Script: Allow models to be shrunken"
      ;;
    *)
      echo "This is a script for running the checker"
      echo
      echo "USAGE: ./run_schecker.sh [-c <path_to_cnf>] [-s]"
      echo
      echo "-c <path_to_cnf>    Uses the given cnf"
      echo "-s                  Allow shrunken models"
      echo
      echo "If no cnf is provided, a random cnf is fuzzed with cnfuzz (--tiny option is on)"
      exit 1
      ;;
  esac
done

echo "Script: make all"
make -C ../

if $fuzz; then
    echo "Script: fuzz cnf into $path_fuzzed_cnf"
    ../../cnfuzz/cnfuzz --tiny > $path_fuzzed_cnf 
fi

if $shrink; then
  echo "Script: run cadicall -s"
  ../src/cadicall -s $path_fuzzed_cnf
else
  echo "Script: run cadicall"
  ../src/cadicall $path_fuzzed_cnf
fi

if $shrink; then
  echo "Script: run checker -s"
  ./checker -s $path_fuzzed_cnf $tmp_cadicall_negated_models
else
  echo "Script: run checker"
  ./checker $path_fuzzed_cnf $tmp_cadicall_negated_models
fi
