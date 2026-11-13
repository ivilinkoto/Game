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
| `gamemodes/core/commands.inc` | Команден процесор (`CMD:име(playerid, params[])`) |
| `gamemodes/modules/menu.inc` | Главно меню (бутон Y или `/menu`) |
| `gamemodes/modules/admin.inc` | Админ нива, команди и банове |
| `gamemodes/modules/chat.inc` | RP чат |
| `gamemodes/modules/money.inc` | Пари на сървъра (`Money_Give` / `Money_Set`) |
| `gamemodes/modules/levels.inc` | Активно време, PayDay (опит), нива |
| `gamemodes/modules/afk.inc` | AFK система |
| `gamemodes/modules/hospital.inc` | Смърт, болница, вход/изход |
| `gamemodes/maps/` | Карти (обекти през Streamer) |
| `scripts/install-streamer.sh` | Инсталира Streamer плъгина (deploy го пуска сам, ако липсва) |
| `scripts/install-bcrypt.sh` | Инсталира samp-bcrypt (deploy го пуска сам, ако липсва) |
| `gamemodes/core/passwords.inc` | Пароли с BCrypt (асинхронно) + прехвърляне на стари SHA-256 пароли |
| `gamemodes/include/` | Външни include файлове (samp_bcrypt.inc) |
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

## Първи собственик

1. В `/home/omp/Server/config.json` задай парола в `"rcon": { "password": "..." }` и рестартирай (`systemctl restart omp`).
2. В играта: `/rcon login ПАРОЛАТА`, после `/makeadmin [твоето ID] 5`.
3. `/alogin` – при първото влизане всеки админ сам създава своя админ парола. Без нея админ командите не работят.

Админ нива: 1 Хелпър, 2 Модератор, 3 Администратор, 4 Главен администратор, 5 Собственик.

## Промокодове

Засега се създават от VPS-а (еднократно: `apt -y install sqlite3`):

```bash
sqlite3 /home/omp/Server/scriptfiles/hotfix.db "INSERT INTO promo_codes (code, money, max_uses) VALUES ('START', 10000, 100);"
```

`max_uses = 0` е без лимит. `expires` е unix време (0 = не изтича).

## Предстои

- Награда за поканилия и поканения (полето `referrer_id` вече се записва)
- Админ команди за промокодове

## Кредити

- Интериор на болница All Saints – AdamsLT ([SA-MP форум](https://sampforum.blast.hk/showthread.php?tid=500284))
- Streamer плъгин – samp-incognito ([GitHub](https://github.com/samp-incognito/samp-streamer-plugin))
- samp-bcrypt – Sreyas-Sreelal ([GitHub](https://github.com/Sreyas-Sreelal/samp-bcrypt), MIT)
