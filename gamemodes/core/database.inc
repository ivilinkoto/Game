// SQLite база данни (вградена в open.mp).
// Файл: Server/scriptfiles/hotfix.db

new DB:gDatabase = DB:0;

// Заявка без резултат.
stock Database_Exec(const query[])
{
    new DBResult:result = DB_ExecuteQuery(gDatabase, "%s", query);
    if (result != DBResult:0) {
        DB_FreeResultSet(result);
    }
}

// Заявка с резултат. Извикващият трябва да извика DB_FreeResultSet!
stock DBResult:Database_Query(const query[])
{
    return DB_ExecuteQuery(gDatabase, "%s", query);
}

stock bool:Database_Init()
{
    gDatabase = DB_Open(DATABASE_FILE);
    if (gDatabase == DB:0) {
        printf("[DB] Cannot open %s! Shutting down.", DATABASE_FILE);
        SendRconCommand("exit");
        return false;
    }

    Database_Exec("PRAGMA journal_mode = WAL");

    new q[1500];

    // --- Акаунти ---
    q[0] = EOS;
    strcat(q, "CREATE TABLE IF NOT EXISTS accounts (");
    strcat(q, "id INTEGER PRIMARY KEY AUTOINCREMENT,");
    strcat(q, "name TEXT NOT NULL UNIQUE COLLATE NOCASE,");
    strcat(q, "password TEXT NOT NULL,");
    strcat(q, "salt TEXT NOT NULL,");
    strcat(q, "email TEXT NOT NULL DEFAULT '',");
    strcat(q, "gender INTEGER NOT NULL DEFAULT 0,");
    strcat(q, "age INTEGER NOT NULL DEFAULT 0,");
    strcat(q, "source TEXT NOT NULL DEFAULT '',");
    strcat(q, "referrer_id INTEGER NOT NULL DEFAULT 0,");
    strcat(q, "promo_code TEXT NOT NULL DEFAULT '',");
    strcat(q, "skin INTEGER NOT NULL DEFAULT 26,");
    strcat(q, "money INTEGER NOT NULL DEFAULT 0,");
    strcat(q, "level INTEGER NOT NULL DEFAULT 1,");
    strcat(q, "admin INTEGER NOT NULL DEFAULT 0,");
    strcat(q, "pos_x REAL NOT NULL DEFAULT 0,");
    strcat(q, "pos_y REAL NOT NULL DEFAULT 0,");
    strcat(q, "pos_z REAL NOT NULL DEFAULT 0,");
    strcat(q, "pos_a REAL NOT NULL DEFAULT 0,");
    strcat(q, "interior INTEGER NOT NULL DEFAULT 0,");
    strcat(q, "virtual_world INTEGER NOT NULL DEFAULT 0,");
    strcat(q, "register_ip TEXT NOT NULL DEFAULT '',");
    strcat(q, "last_ip TEXT NOT NULL DEFAULT '',");
    strcat(q, "registered_at INTEGER NOT NULL DEFAULT 0,");
    strcat(q, "last_login INTEGER NOT NULL DEFAULT 0)");
    Database_Exec(q);

    // --- Промокодове ---
    // max_uses = 0 -> без лимит, expires = 0 -> не изтича
    q[0] = EOS;
    strcat(q, "CREATE TABLE IF NOT EXISTS promo_codes (");
    strcat(q, "code TEXT PRIMARY KEY COLLATE NOCASE,");
    strcat(q, "money INTEGER NOT NULL DEFAULT 0,");
    strcat(q, "max_uses INTEGER NOT NULL DEFAULT 0,");
    strcat(q, "uses INTEGER NOT NULL DEFAULT 0,");
    strcat(q, "expires INTEGER NOT NULL DEFAULT 0,");
    strcat(q, "active INTEGER NOT NULL DEFAULT 1,");
    strcat(q, "created_at INTEGER NOT NULL DEFAULT 0)");
    Database_Exec(q);

    // --- Банове (expire = 0 -> завинаги) ---
    q[0] = EOS;
    strcat(q, "CREATE TABLE IF NOT EXISTS bans (");
    strcat(q, "id INTEGER PRIMARY KEY AUTOINCREMENT,");
    strcat(q, "name TEXT NOT NULL COLLATE NOCASE,");
    strcat(q, "ip TEXT NOT NULL DEFAULT '',");
    strcat(q, "admin TEXT NOT NULL DEFAULT '',");
    strcat(q, "reason TEXT NOT NULL DEFAULT '',");
    strcat(q, "date INTEGER NOT NULL DEFAULT 0,");
    strcat(q, "expire INTEGER NOT NULL DEFAULT 0)");
    Database_Exec(q);

    // --- Лог на важни действия (админи, пари) ---
    q[0] = EOS;
    strcat(q, "CREATE TABLE IF NOT EXISTS logs (");
    strcat(q, "id INTEGER PRIMARY KEY AUTOINCREMENT,");
    strcat(q, "type TEXT NOT NULL,");
    strcat(q, "actor TEXT NOT NULL DEFAULT '',");
    strcat(q, "target TEXT NOT NULL DEFAULT '',");
    strcat(q, "details TEXT NOT NULL DEFAULT '',");
    strcat(q, "date INTEGER NOT NULL DEFAULT 0)");
    Database_Exec(q);

    // --- Нови колони в стари таблици (за бази, създадени от по-стара версия) ---
    Database_EnsureColumn("accounts", "exp", "INTEGER NOT NULL DEFAULT 0");
    Database_EnsureColumn("accounts", "played_minutes", "INTEGER NOT NULL DEFAULT 0");
    Database_EnsureColumn("accounts", "payday_minutes", "INTEGER NOT NULL DEFAULT 0");
    Database_EnsureColumn("accounts", "admin_password", "TEXT NOT NULL DEFAULT ''");
    Database_EnsureColumn("accounts", "admin_salt", "TEXT NOT NULL DEFAULT ''");

    Database_Exec("CREATE INDEX IF NOT EXISTS idx_accounts_referrer ON accounts (referrer_id)");
    Database_Exec("CREATE INDEX IF NOT EXISTS idx_bans_name ON bans (name)");
    Database_Exec("CREATE INDEX IF NOT EXISTS idx_bans_ip ON bans (ip)");

    print("[DB] Connected.");
    return true;
}

// Добавя колона, ако я няма.
stock Database_EnsureColumn(const table[], const column[], const definition[])
{
    new q[256], name[64], bool:exists = false;
    format(q, sizeof (q), "PRAGMA table_info(%s)", table);
    new DBResult:result = Database_Query(q);
    if (result != DBResult:0) {
        if (DB_GetRowCount(result) > 0) {
            do {
                DB_GetFieldStringByName(result, "name", name);
                if (!strcmp(name, column, true)) {
                    exists = true;
                    break;
                }
            } while (DB_SelectNextRow(result));
        }
        DB_FreeResultSet(result);
    }
    if (!exists) {
        format(q, sizeof (q), "ALTER TABLE %s ADD COLUMN %s %s", table, column, definition);
        Database_Exec(q);
        printf("[DB] Added column %s.%s", table, column);
    }
}

// Записва действие в таблица logs.
stock Log_Add(const type[], const actor[], const target[], const details[])
{
    new eActor[64], eTarget[64], eDetails[256], q[512];
    SQL_Escape(actor, eActor);
    SQL_Escape(target, eTarget);
    SQL_Escape(details, eDetails);
    format(q, sizeof (q),
        "INSERT INTO logs (type, actor, target, details, date) VALUES ('%s', '%s', '%s', '%s', %d)",
        type, eActor, eTarget, eDetails, gettime());
    Database_Exec(q);
}

stock Database_Close()
{
    if (gDatabase != DB:0) {
        DB_Close(gDatabase);
        gDatabase = DB:0;
    }
}
