# testing

Run `./test_one.sh` to run CaDiCAll in counting mode and comparing it to dualiza's count. Outputs `eq` if equal else `neq`.

Run `./test_many.sh` to run `test_one.sh` until one run returns `neq`.

NOTE: `neq` can also happen if the cnf is too large and CaDiCAll fails due to too much memory consumption.

## NOTE

paths are hardcoded and thus must be kept up to date
