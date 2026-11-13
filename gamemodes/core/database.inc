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

    Database_Exec("CREATE INDEX IF NOT EXISTS idx_accounts_referrer ON accounts (referrer_id)");

    print("[DB] Connected.");
    return true;
}

stock Database_Close()
{
    if (gDatabase != DB:0) {
        DB_Close(gDatabase);
        gDatabase = DB:0;
    }
}
