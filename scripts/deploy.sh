#!/usr/bin/env bash
# Обновява сървъра: изтегля кода от GitHub, компилира и рестартира.
# Пуска се като root:  deploy
set -euo pipefail

# Целият код е във функция, защото git обновява и този файл,
# докато се изпълнява – bash трябва първо да го прочете целия.
main() {
    local REPO_DIR SERVER_DIR BRANCH
    REPO_DIR="$(cd "$(dirname "$(readlink -f "$0")")/.." && pwd)"
    SERVER_DIR="${SERVER_DIR:-/home/omp/Server}"
    BRANCH="${BRANCH:-ccr-887af6ee-7dr19j}"

    if [ "${DEPLOY_REEXEC:-}" != "1" ]; then
        echo "==> Изтегляне на кода ($BRANCH)"
        runuser -u omp -- git -C "$REPO_DIR" fetch --quiet origin "$BRANCH"
        runuser -u omp -- git -C "$REPO_DIR" reset --hard --quiet "origin/$BRANCH"
        runuser -u omp -- git -C "$REPO_DIR" log -1 --format='    %h %s'
        # Пускаме наново вече обновената версия на този скрипт.
        DEPLOY_REEXEC=1 exec bash "$REPO_DIR/scripts/deploy.sh" "$@"
    fi

    if [ ! -f "$SERVER_DIR/plugins/streamer.so" ] || [ ! -f "$SERVER_DIR/qawno/include/streamer.inc" ]; then
        echo "==> Инсталиране на Streamer"
        SERVER_DIR="$SERVER_DIR" bash "$REPO_DIR/scripts/install-streamer.sh"
    fi

    if [ ! -f "$SERVER_DIR/plugins/samp_bcrypt.so" ]; then
        echo "==> Инсталиране на samp-bcrypt"
        SERVER_DIR="$SERVER_DIR" bash "$REPO_DIR/scripts/install-bcrypt.sh"
    fi

    echo "==> Компилиране"
    SERVER_DIR="$SERVER_DIR" runuser -u omp -- bash "$REPO_DIR/scripts/build.sh"

    echo "==> Настройки на сървъра"
    python3 - "$SERVER_DIR/config.json" <<'PY'
import json, sys
path = sys.argv[1]
with open(path, encoding="utf-8") as f:
    cfg = json.load(f)
before = json.dumps(cfg, sort_keys=True)
cfg["name"] = "HOTFIX ONLINE"
cfg["language"] = "Bulgarian"
pawn = cfg.setdefault("pawn", {})
pawn["main_scripts"] = ["hotfix 1"]
plugins = pawn.setdefault("legacy_plugins", [])
for plugin in ("streamer", "samp_bcrypt"):
    if plugin not in plugins:
        plugins.append(plugin)
if json.dumps(cfg, sort_keys=True) != before:
    with open(path, "w", encoding="utf-8") as f:
        json.dump(cfg, f, indent=4, ensure_ascii=False)
    print("    config.json е обновен")
else:
    print("    без промени")
PY

    echo "==> Рестартиране"
    systemctl restart omp
    sleep 3
    if systemctl is-active --quiet omp; then
        echo "==> ГОТОВО. Сървърът работи."
    else
        echo "==> ГРЕШКА: сървърът не тръгна. Последни редове от лога:"
        journalctl -u omp -n 30 --no-pager
        exit 1
    fi
}

main "$@"
exit
