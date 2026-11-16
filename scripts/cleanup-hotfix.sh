#!/usr/bin/env bash
# Еднократно премахва всичко от стария HOTFIX ONLINE от VPS-а и пуска CARGO STATE.
#
# Пуска се като root:  bash /home/omp/hotfix/scripts/cleanup-hotfix.sh
#
# Какво прави:
#   1. Спира сървъра.
#   2. Архивира старите файлове на HOTFIX (gamemode, база) в /root и ги изтрива от сървъра.
#   3. Оправя config.json (и server.cfg, ако има) – име CARGO STATE, главен скрипт cargo.
#   4. Преименува хранилището /home/omp/hotfix -> /home/omp/cargo и пренасочва командата deploy.
#   5. Компилира и стартира CARGO STATE (чрез deploy).
#   6. Показва дали някъде още е останало "hotfix".
set -euo pipefail

main() {
    local SERVER_DIR OLD_REPO NEW_REPO BRANCH
    SERVER_DIR="${SERVER_DIR:-/home/omp/Server}"
    OLD_REPO="/home/omp/hotfix"
    NEW_REPO="/home/omp/cargo"
    BRANCH="ccr-9762dda4-kqdp20"

    if [ "$(id -u)" != "0" ]; then
        echo "Пусни скрипта като root."
        exit 1
    fi

    echo "==> Спиране на сървъра"
    systemctl stop omp || true

    echo "==> Стари файлове на HOTFIX"
    local files=()
    local f
    for f in "$SERVER_DIR"/gamemodes/hotfix.* "$SERVER_DIR"/scriptfiles/hotfix.* \
             "$SERVER_DIR"/filterscripts/*hotfix* "$SERVER_DIR"/scriptfiles/*hotfix*; do
        [ -e "$f" ] && files+=("$f")
    done
    # Без дубликати
    if [ "${#files[@]}" -gt 0 ]; then
        mapfile -t files < <(printf '%s\n' "${files[@]}" | sort -u)
        local backup
        backup="/root/hotfix-backup-$(date +%Y%m%d-%H%M%S).tar.gz"
        tar -czf "$backup" "${files[@]}" 2>/dev/null
        rm -f "${files[@]}"
        printf '    изтрито: %s\n' "${files[@]}"
        echo "    архив (за всеки случай): $backup"
        echo "    ако не ти трябва:  rm $backup"
    else
        echo "    няма"
    fi

    echo "==> Настройки на сървъра"
    python3 - "$SERVER_DIR" <<'PY'
import json, os, re, sys
server = sys.argv[1]

path = os.path.join(server, "config.json")
if os.path.exists(path):
    with open(path, encoding="utf-8") as f:
        cfg = json.load(f)
    cfg["name"] = "CARGO STATE"
    cfg["language"] = "Bulgarian"
    pawn = cfg.setdefault("pawn", {})
    pawn["main_scripts"] = ["cargo 1"]
    side = pawn.get("side_scripts", [])
    pawn["side_scripts"] = [s for s in side if "hotfix" not in s.lower()]
    plugins = pawn.setdefault("legacy_plugins", [])
    for plugin in ("streamer", "samp_bcrypt"):
        if plugin not in plugins:
            plugins.append(plugin)
    with open(path, "w", encoding="utf-8") as f:
        json.dump(cfg, f, indent=4, ensure_ascii=False)
    print("    config.json: име CARGO STATE, главен скрипт cargo")

path = os.path.join(server, "server.cfg")
if os.path.exists(path):
    with open(path, encoding="utf-8", errors="replace") as f:
        text = f.read()
    text = re.sub(r"(?m)^hostname .*$", "hostname CARGO STATE", text)
    text = re.sub(r"(?m)^gamemode0 .*$", "gamemode0 cargo 1", text)
    with open(path, "w", encoding="utf-8") as f:
        f.write(text)
    print("    server.cfg: име CARGO STATE, gamemode0 cargo")
PY

    echo "==> Хранилище"
    if [ -d "$OLD_REPO/.git" ] && [ ! -e "$NEW_REPO" ]; then
        mv "$OLD_REPO" "$NEW_REPO"
        echo "    $OLD_REPO -> $NEW_REPO"
    elif [ -d "$NEW_REPO/.git" ]; then
        echo "    вече е в $NEW_REPO"
        [ -d "$OLD_REPO" ] && echo "    ВНИМАНИЕ: $OLD_REPO също съществува – провери го ръчно"
    else
        echo "    Не намерих хранилището нито в $OLD_REPO, нито в $NEW_REPO"
        exit 1
    fi
    chown -R omp:omp "$NEW_REPO"

    echo "==> Команда deploy"
    local target="$NEW_REPO/scripts/deploy.sh"
    local cmd
    cmd="$(command -v deploy || true)"
    if [ -z "$cmd" ]; then
        cmd="/usr/local/bin/deploy"
    fi
    if [ -L "$cmd" ] || [ ! -e "$cmd" ]; then
        ln -sfn "$target" "$cmd"
        echo "    $cmd -> $target"
    elif grep -q "$OLD_REPO" "$cmd"; then
        sed -i "s#$OLD_REPO#$NEW_REPO#g" "$cmd"
        echo "    $cmd: пътят е сменен на $NEW_REPO"
    else
        ln -sfn "$target" "$cmd"
        echo "    $cmd -> $target"
    fi

    echo "==> Компилиране и стартиране на CARGO STATE"
    BRANCH="$BRANCH" bash "$target"

    echo "==> Проверка за останали следи от hotfix"
    local leftovers
    leftovers="$( {
        grep -ril hotfix "$SERVER_DIR"/config.json "$SERVER_DIR"/server.cfg 2>/dev/null || true
        ls -d "$SERVER_DIR"/gamemodes/hotfix* "$SERVER_DIR"/scriptfiles/hotfix* 2>/dev/null || true
        grep -ril hotfix /etc/systemd/system/omp.service /etc/systemd/system/omp.service.d 2>/dev/null || true
        crontab -l 2>/dev/null | grep -i hotfix || true
        [ -d "$OLD_REPO" ] && echo "$OLD_REPO" || true
    } )"
    if [ -n "$leftovers" ]; then
        echo "    Още има:"
        echo "$leftovers" | sed 's/^/      /'
    else
        echo "    Чисто – няма нищо от HOTFIX."
    fi
    echo
    echo "Главен скрипт: $(python3 -c "import json;print(json.load(open('$SERVER_DIR/config.json'))['pawn']['main_scripts'])" 2>/dev/null || echo '?')"
    echo "Ако в списъка със сървъри още пише HOTFIX ONLINE – обнови списъка (името се кешира в клиента)."
}

main "$@"
exit
