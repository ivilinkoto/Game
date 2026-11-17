#!/usr/bin/env bash
# Инсталира плъгина за маршрути samp-gps-plugin (github.com/AmyrAhmady/samp-gps-plugin)
# в open.mp сървъра: plugins/GPS.so и scriptfiles/GPS.dat. deploy го пуска сам, ако липсва.
#
# Ръчно (като root):  bash scripts/install-gps.sh
# Друга версия:       GPS_VERSION=1.4.0 bash scripts/install-gps.sh
set -euo pipefail

SERVER_DIR="${SERVER_DIR:-/home/omp/Server}"
VERSION="${GPS_VERSION:-1.4.1}"
API="https://api.github.com/repos/AmyrAhmady/samp-gps-plugin/releases/tags/$VERSION"

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
mkdir -p "$TMP/x"

URLS="$(curl -fsSL "$API" | grep -o '"browser_download_url": *"[^"]*"' | sed 's/.*"\(http[^"]*\)"$/\1/')"
if [ -z "$URLS" ]; then
    echo "Не намерих файлове в изданието $VERSION ($API)"
    exit 1
fi
echo "    файлове в изданието $VERSION:"
echo "$URLS" | sed 's/^/      /'

# Сваляме файловете за Linux и GPS.dat (пропускаме Windows .dll)
while read -r url; do
    [ -z "$url" ] && continue
    name="$(basename "$url")"
    case "$name" in
        *.dll) continue ;;
    esac
    curl -fsSL -o "$TMP/$name" "$url"
    case "$name" in
        *.zip)
            command -v unzip >/dev/null || apt-get -y install unzip
            unzip -qo "$TMP/$name" -d "$TMP/x"
            ;;
        *.tar.gz|*.tgz)
            tar -xzf "$TMP/$name" -C "$TMP/x"
            ;;
        *)
            cp "$TMP/$name" "$TMP/x/"
            ;;
    esac
done <<< "$URLS"

SO="$(find "$TMP/x" -type f -name '*.so' | head -1)"
DAT="$(find "$TMP/x" -type f -iname 'GPS.dat' | head -1)"
if [ -z "$SO" ] || [ -z "$DAT" ]; then
    echo "В изданието липсва .so (Linux) или GPS.dat. Намерени файлове:"
    find "$TMP/x" -type f | sed 's/^/      /'
    exit 1
fi

mkdir -p "$SERVER_DIR/plugins" "$SERVER_DIR/scriptfiles"
install -m 755 "$SO" "$SERVER_DIR/plugins/GPS.so"
install -m 644 "$DAT" "$SERVER_DIR/scriptfiles/GPS.dat"
chown omp:omp "$SERVER_DIR/plugins/GPS.so" "$SERVER_DIR/scriptfiles/GPS.dat"

MISSING="$(ldd "$SERVER_DIR/plugins/GPS.so" 2>/dev/null | grep 'not found' || true)"
if [ -n "$MISSING" ]; then
    echo "ВНИМАНИЕ: липсващи библиотеки за GPS.so:"
    echo "$MISSING"
    rm -f "$SERVER_DIR/plugins/GPS.so"
    exit 1
fi
echo "    GPS плъгинът е инсталиран (plugins/GPS.so, scriptfiles/GPS.dat)."
