#!/usr/bin/env bash
# Инсталира Streamer плъгина (github.com/samp-incognito/samp-streamer-plugin)
# в open.mp сървъра. deploy го пуска сам, ако плъгинът липсва.
#
# Ръчно (като root):  bash scripts/install-streamer.sh
# Конкретна версия:   STREAMER_VERSION=v2.9.6 bash scripts/install-streamer.sh
set -euo pipefail

SERVER_DIR="${SERVER_DIR:-/home/omp/Server}"
VERSION="${STREAMER_VERSION:-latest}"
REPO_API="https://api.github.com/repos/samp-incognito/samp-streamer-plugin/releases"

command -v unzip >/dev/null || apt-get -y install unzip

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

if [ "$VERSION" = "latest" ]; then
    API="$REPO_API/latest"
else
    API="$REPO_API/tags/$VERSION"
fi

URL="$(curl -fsSL "$API" | grep -o '"browser_download_url": *"[^"]*\.zip"' | head -1 | sed 's/.*"\(http[^"]*\)"$/\1/')"
if [ -z "$URL" ]; then
    echo "Не намерих архив на Streamer в $API"
    exit 1
fi

echo "    изтегляне: $URL"
curl -fsSL -o "$TMP/streamer.zip" "$URL"
unzip -q "$TMP/streamer.zip" -d "$TMP/x"

SO="$(find "$TMP/x" -name 'streamer.so' | head -1)"
INC="$(find "$TMP/x" -name 'streamer.inc' | head -1)"
if [ -z "$SO" ] || [ -z "$INC" ]; then
    echo "В архива липсва streamer.so или streamer.inc"
    exit 1
fi

mkdir -p "$SERVER_DIR/plugins"
install -m 755 "$SO" "$SERVER_DIR/plugins/streamer.so"
install -m 644 "$INC" "$SERVER_DIR/qawno/include/streamer.inc"
chown omp:omp "$SERVER_DIR/plugins" "$SERVER_DIR/plugins/streamer.so" "$SERVER_DIR/qawno/include/streamer.inc"

MISSING="$(ldd "$SERVER_DIR/plugins/streamer.so" 2>/dev/null | grep 'not found' || true)"
if [ -n "$MISSING" ]; then
    echo "ВНИМАНИЕ: липсващи библиотеки за streamer.so:"
    echo "$MISSING"
    exit 1
fi
echo "    Streamer е инсталиран."
