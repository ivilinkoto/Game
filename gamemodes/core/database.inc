// SQLite база данни (вградена в open.mp, без плъгини).
// Файлът е scriptfiles/rp.db. По-късно може да се мигрира към MySQL.

new DB:gDatabase = DB:0;

// Изпълнява заявка, която не връща резултат.
stock Database_Exec(const query[])
{
    new DBResult:result = DB_ExecuteQuery(gDatabase, "%s", query);
    if (result != DBResult:0) {
        DB_FreeResultSet(result);
    }
}

// Изпълнява заявка с резултат. Извикващият трябва да извика DB_FreeResultSet!
stock DBResult:Database_Query(const query[])
{
    return DB_ExecuteQuery(gDatabase, "%s", query);
}

stock bool:Database_Init()
{
    gDatabase = DB_Open(DATABASE_FILE);
    if (gDatabase == DB:0) {
        printf("[БАЗА] Не може да се отвори %s! Сървърът спира.", DATABASE_FILE);
        SendRconCommand("exit");
        return false;
    }

    Database_Exec("PRAGMA journal_mode = WAL");
    Database_Exec("PRAGMA foreign_keys = ON");

    new q[1024];

    // --- Акаунти ---
    q[0] = EOS;
    strcat(q, "CREATE TABLE IF NOT EXISTS accounts (");
    strcat(q, "id INTEGER PRIMARY KEY AUTOINCREMENT,");
    strcat(q, "name TEXT NOT NULL UNIQUE COLLATE NOCASE,");
    strcat(q, "password TEXT NOT NULL,");
    strcat(q, "salt TEXT NOT NULL,");
    strcat(q, "gender INTEGER NOT NULL DEFAULT 0,");
    strcat(q, "money INTEGER NOT NULL DEFAULT 0,");
    strcat(q, "bank INTEGER NOT NULL DEFAULT 0,");
    strcat(q, "level INTEGER NOT NULL DEFAULT 1,");
    strcat(q, "exp INTEGER NOT NULL DEFAULT 0,");
    strcat(q, "skin INTEGER NOT NULL DEFAULT 26,");
    strcat(q, "admin INTEGER NOT NULL DEFAULT 0,");
    strcat(q, "vip INTEGER NOT NULL DEFAULT 0,");
    strcat(q, "vip_expire INTEGER NOT NULL DEFAULT 0,");
    strcat(q, "coins INTEGER NOT NULL DEFAULT 0,");
    strcat(q, "played_minutes INTEGER NOT NULL DEFAULT 0,");
    strcat(q, "payday_minutes INTEGER NOT NULL DEFAULT 0,");
    strcat(q, "pos_x REAL NOT NULL DEFAULT 0,");
    strcat(q, "pos_y REAL NOT NULL DEFAULT 0,");
    strcat(q, "pos_z REAL NOT NULL DEFAULT 0,");
    strcat(q, "pos_a REAL NOT NULL DEFAULT 0,");
    strcat(q, "interior INTEGER NOT NULL DEFAULT 0,");
    strcat(q, "virtual_world INTEGER NOT NULL DEFAULT 0,");
    strcat(q, "register_ip TEXT,");
    strcat(q, "last_ip TEXT,");
    strcat(q, "register_date INTEGER NOT NULL DEFAULT 0,");
    strcat(q, "last_login INTEGER NOT NULL DEFAULT 0)");
    Database_Exec(q);

    // --- Банове (expire = 0 означава завинаги) ---
    q[0] = EOS;
    strcat(q, "CREATE TABLE IF NOT EXISTS bans (");
    strcat(q, "id INTEGER PRIMARY KEY AUTOINCREMENT,");
    strcat(q, "name TEXT NOT NULL COLLATE NOCASE,");
    strcat(q, "ip TEXT,");
    strcat(q, "admin TEXT,");
    strcat(q, "reason TEXT,");
    strcat(q, "date INTEGER NOT NULL DEFAULT 0,");
    strcat(q, "expire INTEGER NOT NULL DEFAULT 0)");
    Database_Exec(q);

    // --- Дарения ---
    // Уеб магазинът САМО вмъква редове тук (processed = 0).
    // Сървърът ги обработва и зачислява монетите.
    // processed: 0 = чака, 1 = зачислено, 2 = няма такъв акаунт
    q[0] = EOS;
    strcat(q, "CREATE TABLE IF NOT EXISTS donations (");
    strcat(q, "id INTEGER PRIMARY KEY AUTOINCREMENT,");
    strcat(q, "account_name TEXT NOT NULL COLLATE NOCASE,");
    strcat(q, "coins INTEGER NOT NULL,");
    strcat(q, "provider TEXT,");
    strcat(q, "transaction_id TEXT UNIQUE,");
    strcat(q, "created_at INTEGER NOT NULL DEFAULT 0,");
    strcat(q, "processed INTEGER NOT NULL DEFAULT 0,");
    strcat(q, "processed_at INTEGER NOT NULL DEFAULT 0)");
    Database_Exec(q);

    // --- Лог на админ действия и покупки ---
    q[0] = EOS;
    strcat(q, "CREATE TABLE IF NOT EXISTS logs (");
    strcat(q, "id INTEGER PRIMARY KEY AUTOINCREMENT,");
    strcat(q, "type TEXT NOT NULL,");
    strcat(q, "actor TEXT,");
    strcat(q, "target TEXT,");
    strcat(q, "details TEXT,");
    strcat(q, "date INTEGER NOT NULL DEFAULT 0)");
    Database_Exec(q);

    Database_Exec("CREATE INDEX IF NOT EXISTS idx_bans_name ON bans (name)");
    Database_Exec("CREATE INDEX IF NOT EXISTS idx_bans_ip ON bans (ip)");
    Database_Exec("CREATE INDEX IF NOT EXISTS idx_donations_pending ON donations (processed)");

    print("[БАЗА] Свързан успешно.");
    return true;
}

stock Database_Close()
{
    if (gDatabase != DB:0) {
        DB_Close(gDatabase);
        gDatabase = DB:0;
    }
}

// Записва действие в таблица logs.
stock Log_Add(const type[], const actor[], const target[], const details[])
{
    new eType[32], eActor[64], eTarget[64], eDetails[256], q[512];
    SQL_Escape(type, eType);
    SQL_Escape(actor, eActor);
    SQL_Escape(target, eTarget);
    SQL_Escape(details, eDetails);
    format(q, sizeof (q),
        "INSERT INTO logs (type, actor, target, details, date) VALUES ('%s', '%s', '%s', '%s', %d)",
        eType, eActor, eTarget, eDetails, gettime());
    Database_Exec(q);
}
