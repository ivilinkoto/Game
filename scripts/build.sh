#!/usr/bin/env bash
# Компилира gamemodes/rp.pwn -> gamemodes/rp.amx (Linux/macOS)
#
# Изходният код е в UTF-8, но SA-MP клиентът показва кирилица само в
# Windows-1251. Затова първо конвертираме копие на кода в build/src.
#
# Променливи (по избор):
#   PAWNCC      път до pawncc   (по подразбиране: ../server/qawno/pawncc)
#   OMP_INCLUDE път до include  (по подразбиране: ../server/qawno/include)
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PAWNCC="${PAWNCC:-$ROOT/../server/qawno/pawncc}"
OMP_INCLUDE="${OMP_INCLUDE:-$ROOT/../server/qawno/include}"
SRC="$ROOT/build/src"

if [ ! -x "$PAWNCC" ]; then
    echo "Не намирам pawncc: $PAWNCC"
    echo "Задай пътя: PAWNCC=/път/до/pawncc OMP_INCLUDE=/път/до/include $0"
    exit 1
fi

rm -rf "$SRC"
mkdir -p "$SRC"

cd "$ROOT/gamemodes"
find . -type f \( -name '*.pwn' -o -name '*.inc' \) | while read -r f; do
    mkdir -p "$SRC/$(dirname "$f")"
    iconv -f UTF-8 -t CP1251 "$f" > "$SRC/$f"
done

"$PAWNCC" "$SRC/rp.pwn" \
    "-i$OMP_INCLUDE" "-i$SRC" \
    "-o$ROOT/gamemodes/rp.amx" \
    -d3 '-;+' -O1

echo "Готово: gamemodes/rp.amx"
