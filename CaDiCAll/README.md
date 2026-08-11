# CaDiCAll - A model enumerator without Blocking Clauses using IPASIR-UP

## IMPORTANT

This uses CaDiCaL as an underlying solver. However, there must be made certain changes to CaDiCaL itself to work. Thus, this repo includes a version of CaDiCaL that contains all those changes. If you want to use a different version, you must add the changes yourself. All changes and their reasons are described in the thesis.

## USAGE:

1. Compile CaDiCaL:

Move to 'cadical' and run `./configure $$ make`

2. Compile CaDiCAll:

Run `make` on this level to compile all (CaDiCAll and checker), run `make clean` to clean up all files produced by `make` and the execution of either.

## USAGE: CaDiCAll:

Move to `src`.

Run `./cadicall --help`.
