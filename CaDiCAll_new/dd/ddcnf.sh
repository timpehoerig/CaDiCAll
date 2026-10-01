#!/bin/bash
# Wird von cnfdd für jeden Minimierungs-Kandidaten aufgerufen: cnfdd_test.sh <kandidat.cnf>
# Exit 0 = "interessant" (Reduktion behalten), Exit 1 = "nicht interessant" (verwerfen)

CNF="$1"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

# Pfade zu den Binaries: per Umgebungsvariable überschreibbar, z.B.
#   CADICALL_BIN=/pfad/zu/cadicall DUALIZA_BIN=/pfad/zu/dualiza cnfdd ...
CADICALL="${CADICALL_BIN:-$ROOT/CaDiCAll/src/cadicall}"
DUALIZA="${DUALIZA_BIN:-$ROOT/dualiza/dualiza}"

# Leere CNF zählt wie "gleiches Ergebnis" -> nicht interessant
CLAUSES=$(grep -vE '^[[:space:]]*(c|p|%|$)' "$CNF" | grep -c '[0-9]')
if [ "$CLAUSES" -eq 0 ]; then
    exit 0
fi

CADICALL_COUNT=$("$CADICALL" -c "$CNF" 2>/dev/null | tail -1)
DUALIZA_COUNT=$("$DUALIZA" -c "$CNF" 2>/dev/null | tail -1)

if [ "$CADICALL_COUNT" != "$DUALIZA_COUNT" ]; then
    exit 1   # unterschiedliche Modellzahl -> interessant
else
    exit 0   # gleiche Zahl (oder leer) -> nicht interessant
fi