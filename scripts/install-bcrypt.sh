#!/usr/bin/env bash
# Инсталира плъгина samp-bcrypt (github.com/Sreyas-Sreelal/samp-bcrypt)
# в open.mp сървъра. deploy го пуска сам, ако плъгинът липсва.
#
# Ръчно (като root):  bash scripts/install-bcrypt.sh
# Конкретна версия:   BCRYPT_VERSION=0.4.1 bash scripts/install-bcrypt.sh
set -euo pipefail

SERVER_DIR="${SERVER_DIR:-/home/omp/Server}"
VERSION="${BCRYPT_VERSION:-latest}"
REPO_API="https://api.github.com/repos/Sreyas-Sreelal/samp-bcrypt/releases"

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

if [ "$VERSION" = "latest" ]; then
    API="$REPO_API/latest"
else
    API="$REPO_API/tags/$VERSION"
fi

URLS="$(curl -fsSL "$API" | grep -o '"browser_download_url": *"[^"]*"' | sed 's/.*"\(http[^"]*\)"$/\1/')"
SO_URL="$(echo "$URLS" | grep -E '\.so$' | head -1 || true)"
ZIP_URL="$(echo "$URLS" | grep -iE 'linux' | grep -E '\.(zip|tar\.gz)$' | head -1 || true)"

mkdir -p "$SERVER_DIR/plugins"
if [ -n "$SO_URL" ]; then
    echo "    изтегляне: $SO_URL"
    curl -fsSL -o "$TMP/samp_bcrypt.so" "$SO_URL"
    SO="$TMP/samp_bcrypt.so"
elif [ -n "$ZIP_URL" ]; then
    echo "    изтегляне: $ZIP_URL"
    command -v unzip >/dev/null || apt-get -y install unzip
    mkdir -p "$TMP/x"
    case "$ZIP_URL" in
        *.zip) curl -fsSL -o "$TMP/a.zip" "$ZIP_URL" && unzip -q "$TMP/a.zip" -d "$TMP/x" ;;
        *)     curl -fsSL -o "$TMP/a.tgz" "$ZIP_URL" && tar -xzf "$TMP/a.tgz" -C "$TMP/x" ;;
    esac
    SO="$(find "$TMP/x" -name '*.so' | head -1)"
else
    echo "Не намерих файл за Linux в $API"
    echo "$URLS"
    exit 1
fi

if [ -z "${SO:-}" ] || [ ! -f "$SO" ]; then
    echo "Не намерих samp_bcrypt.so"
    exit 1
fi

install -m 755 "$SO" "$SERVER_DIR/plugins/samp_bcrypt.so"
chown omp:omp "$SERVER_DIR/plugins" "$SERVER_DIR/plugins/samp_bcrypt.so"

MISSING="$(ldd "$SERVER_DIR/plugins/samp_bcrypt.so" 2>/dev/null | grep 'not found' || true)"
if [ -n "$MISSING" ]; then
    echo "ВНИМАНИЕ: липсващи библиотеки за samp_bcrypt.so:"
    echo "$MISSING"
    exit 1
fi
echo "    samp-bcrypt е инсталиран."
