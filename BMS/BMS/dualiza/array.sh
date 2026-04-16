#!/bin/sh
#SBATCH --job-name=BMSdualiza
#SBATCH --array=1-8
#SBATCH --partition=cpu-single
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=64
#SBATCH --time=99
#SBATCH --mem=236G
delta=64
msg () {
  printf "c array.sh: %-20s %s\n" "$1" "$2"
}
msg 'start time' "`date`"
msg 'job id' "$SLURM_ARRAY_JOB_ID"
msg 'task id' "$SLURM_ARRAY_TASK_ID"
end=`expr $SLURM_ARRAY_TASK_ID \* $delta + 1`
start=`expr $end - $delta`
limit=`expr 501 + 1`
[ $end -gt $limit ] && end=$limit
msg 'start index' $start
msg 'end index' $end
index=$start
while [ $index -lt $end ]
do
  msg index $index
  benchmark=`awk '$1 == '$index' { print $2 }' benchmarks`
  msg benchmark "$benchmark"
  ./run.sh $index &
  index=`expr $index + 1`
  sleep 1
done
wait
msg 'end time' "`date`"
