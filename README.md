# HOTFIX ONLINE

Лайт RP сървър за мобилни играчи (SA-MP / open.mp), Los Santos.

## Работен процес

1. Кодът се пише тук, в `gamemodes/` (UTF-8).
2. На VPS-а се пуска `deploy` (като root). Командата изтегля кода, компилира го и рестартира сървъра.

| Файл | За какво е |
|---|---|
| `gamemodes/hotfix.pwn` | Главният файл на gamemode-а |
| `scripts/build.sh` | Компилиране (UTF-8 → Windows-1251 → `hotfix.amx`) |
| `scripts/deploy.sh` | Обновяване на сървъра |

## Сървър

- open.mp е в `/home/omp/Server` и работи като услуга `omp` под потребител `omp`
- Хранилището е клонирано в `/home/omp/hotfix`

| Какво | Команда |
|---|---|
| Обновяване | `deploy` |
| Състояние | `systemctl status omp --no-pager` |
| Рестарт | `systemctl restart omp` |
| Лог на живо | `journalctl -u omp -f` |
