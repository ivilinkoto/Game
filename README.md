# HOTFIX ONLINE

Лайт RP сървър за мобилни играчи (SA-MP / open.mp), Los Santos.

## Работен процес

1. Кодът се пише тук, в `gamemodes/` (UTF-8).
2. На VPS-а се пуска `deploy` (като root). Командата изтегля кода, компилира го и рестартира сървъра.

| Файл | За какво е |
|---|---|
| `gamemodes/hotfix.pwn` | Главният файл на gamemode-а |
| `gamemodes/core/` | Настройки, цветове, данни на играча, база данни |
| `gamemodes/modules/auth.inc` | Регистрация и вход |
| `gamemodes/modules/skin_select.inc` | Избор на скин след регистрация |
| `gamemodes/modules/promo.inc` | Промокодове |
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

## Промокодове

Засега се създават от VPS-а (еднократно: `apt -y install sqlite3`):

```bash
sqlite3 /home/omp/Server/scriptfiles/hotfix.db "INSERT INTO promo_codes (code, money, max_uses) VALUES ('START', 10000, 100);"
```

`max_uses = 0` е без лимит. `expires` е unix време (0 = не изтича).

## Предстои

- Награда за поканилия и поканения (полето `referrer_id` вече се записва)
- Админ команди за промокодове
