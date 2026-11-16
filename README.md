# CARGO STATE

Truck simulator сървър за SA-MP / open.mp – целият San Andreas като една логистична мрежа.
Играе се еднакво добре на PC и на мобилни устройства: всичко е на бутон **Y** или в менюта.

## Спецификации

| Файл | Статус |
|---|---|
| [docs/specs/mvp.md](docs/specs/mvp.md) | Основа + Trucking MVP – одобрена. Етап 1 (основа) е готов, етап 2 (превози) предстои. |

## Код

| Файл | За какво е |
|---|---|
| `gamemodes/cargo.pwn` | Главният файл на gamemode-а |
| `gamemodes/core/config.inc` | Настройки: начални пари, спаун, нива, CB канали |
| `gamemodes/core/` | Цветове, диалози, данни на играча, база, команди, пароли (BCrypt) |
| `gamemodes/modules/auth.inc` | Регистрация (парола → пол → скин) и вход |
| `gamemodes/modules/money.inc` | Пари само на сървъра (`Money_Give` / `Money_Take` / `Money_Set`) |
| `gamemodes/modules/levels.inc` | Опит и нива 1–5 (`Level_GiveExp`) |
| `gamemodes/modules/hud.inc` | HUD: пари, скорост, състояние на двигателя |
| `gamemodes/modules/chat.inc` | Общ чат, `/pm`, `/r`, CB радио (`/cb`, `/cbch`) |
| `gamemodes/modules/menu.inc` | Главно меню (Y или `/menu`): работа, GPS, профил, CB, настройки, помощ |
| `gamemodes/modules/gps.inc` | GPS по категории (`/gps`, админ: `/gpsadd`, `/gpslist`, `/gpsdel`) |
| `gamemodes/modules/admin.inc` | Админ нива, админ парола, банове, `/veh`, пари (`/ahelp`) |
| `scripts/build.sh` | Компилиране (UTF-8 → Windows-1251 → `cargo.amx`) |
| `scripts/deploy.sh` | Обновяване на сървъра |

Текстът в HUD-а е на латиница – шрифтовете на SA не показват кирилица. Чатът и диалозите са на български.

## Сървър (VPS)

open.mp е в `/home/omp/Server` и работи като услуга `omp`. Хранилището е клонирано в `/home/omp/cargo`.

**Еднократно – премахване на HOTFIX ONLINE и пускане на CARGO STATE** (като root):

```bash
cd /home/omp/hotfix && runuser -u omp -- git fetch origin ccr-9762dda4-kqdp20 && runuser -u omp -- git reset --hard FETCH_HEAD && bash scripts/cleanup-hotfix.sh
```

Скриптът спира сървъра, архивира и изтрива старите файлове на HOTFIX (архивът е в `/root`),
оправя `config.json`, мести хранилището в `/home/omp/cargo`, пренасочва `deploy` и стартира CARGO STATE.
След това обновяването е само `deploy`.

| Какво | Команда |
|---|---|
| Обновяване | `deploy` |
| Състояние | `systemctl status omp --no-pager` |
| Рестарт | `systemctl restart omp` |
| Лог на живо | `journalctl -u omp -f` |

## Първи собственик

1. В `/home/omp/Server/config.json` трябва да има RCON парола: `"rcon": { "password": "..." }`.
2. В играта: `/rcon login ПАРОЛАТА`, после `/makeadmin [твоето ID] 5`.
3. `/alogin` – при първото влизане всеки админ създава своя админ парола.

Админ нива: 1 Хелпър, 2 Модератор, 3 Администратор, 4 Главен администратор, 5 Собственик.

GPS точките (индустрии, бензиностанции, депа) се добавят от админ на място: `/gpsadd [категория] [име]`.

## Кредити

- Streamer плъгин – samp-incognito ([GitHub](https://github.com/samp-incognito/samp-streamer-plugin))
- samp-bcrypt – Sreyas-Sreelal ([GitHub](https://github.com/Sreyas-Sreelal/samp-bcrypt), MIT)
