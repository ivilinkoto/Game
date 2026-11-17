#!/usr/bin/env bash
# Компилира gamemodes/cargo.pwn -> $SERVER_DIR/gamemodes/cargo.amx
#
# Изходният код е в UTF-8, а SA-MP клиентът показва кирилица в Windows-1251,
# затова компилираме конвертирано копие на кода.
set -euo pipefail

REPO_DIR="$(cd "$(dirname "$(readlink -f "$0")")/.." && pwd)"
SERVER_DIR="${SERVER_DIR:-/home/omp/Server}"
QAWNO="$SERVER_DIR/qawno"
GM="cargo"

SRC="$(mktemp -d)"
trap 'rm -rf "$SRC"' EXIT

cd "$REPO_DIR/gamemodes"
find . -type f \( -name '*.pwn' -o -name '*.inc' \) | while read -r f; do
    mkdir -p "$SRC/$(dirname "$f")"
    iconv -f UTF-8 -t CP1251 "$f" > "$SRC/$f"
done

# Компилаторът търси #include "..." първо в текущата папка, затова компилираме
# ОТ конвертираното копие – иначе модулите се взимат от оригиналите в UTF-8.
cd "$SRC"

# Маршрутът по картата се включва само ако GPS плъгинът е на сървъра
# (без него сървърът не би заредил скрипта). FORCE_ROUTE=1 – за проверка в CI.
EXTRA=()
if [ -f "$SERVER_DIR/plugins/GPS.so" ] || [ "${FORCE_ROUTE:-0}" = "1" ]; then
    EXTRA+=("ROUTE_ENABLED=1")
    echo "Маршрут по картата: включен (GPS плъгин)"
fi

# Компилираме във временен файл, за да не се развали работещият .amx при грешка.
LD_LIBRARY_PATH="$QAWNO" "$QAWNO/pawncc" "$SRC/$GM.pwn" \
    "-i$QAWNO/include" "-i$SRC/include" "-i$SRC" \
    "-o$SRC/$GM.amx" \
    -d3 '-;+' -O1 "${EXTRA[@]}"

install -m 644 "$SRC/$GM.amx" "$SERVER_DIR/gamemodes/$GM.amx"
echo "Компилирано: $SERVER_DIR/gamemodes/$GM.amx"
