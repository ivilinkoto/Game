// SQLite база данни (вградена в open.mp).
// Файл: Server/scriptfiles/cargo.db

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
    strcat(q, "gender INTEGER NOT NULL DEFAULT 0,");
    strcat(q, "skin INTEGER NOT NULL DEFAULT 133,");
    strcat(q, "money INTEGER NOT NULL DEFAULT 0,");
    strcat(q, "exp INTEGER NOT NULL DEFAULT 0,");
    strcat(q, "level INTEGER NOT NULL DEFAULT 1,");
    strcat(q, "playtime INTEGER NOT NULL DEFAULT 0,");
    strcat(q, "admin INTEGER NOT NULL DEFAULT 0,");
    strcat(q, "admin_password TEXT NOT NULL DEFAULT '',");
    strcat(q, "pos_x REAL NOT NULL DEFAULT 0,");
    strcat(q, "pos_y REAL NOT NULL DEFAULT 0,");
    strcat(q, "pos_z REAL NOT NULL DEFAULT 0,");
    strcat(q, "pos_a REAL NOT NULL DEFAULT 0,");
    strcat(q, "has_pos INTEGER NOT NULL DEFAULT 0,");
    strcat(q, "cb_channel INTEGER NOT NULL DEFAULT 19,");
    strcat(q, "cb_on INTEGER NOT NULL DEFAULT 1,");
    strcat(q, "hud_on INTEGER NOT NULL DEFAULT 1,");
    strcat(q, "register_ip TEXT NOT NULL DEFAULT '',");
    strcat(q, "last_ip TEXT NOT NULL DEFAULT '',");
    strcat(q, "registered_at INTEGER NOT NULL DEFAULT 0,");
    strcat(q, "last_login INTEGER NOT NULL DEFAULT 0)");
    Database_Exec(q);

    // --- Банове ---
    q[0] = EOS;
    strcat(q, "CREATE TABLE IF NOT EXISTS bans (");
    strcat(q, "id INTEGER PRIMARY KEY AUTOINCREMENT,");
    strcat(q, "name TEXT NOT NULL COLLATE NOCASE,");
    strcat(q, "reason TEXT NOT NULL DEFAULT '',");
    strcat(q, "admin_name TEXT NOT NULL DEFAULT '',");
    strcat(q, "created_at INTEGER NOT NULL DEFAULT 0,");
    strcat(q, "expires_at INTEGER NOT NULL DEFAULT 0)");   // 0 = завинаги
    Database_Exec(q);
    Database_Exec("CREATE INDEX IF NOT EXISTS idx_bans_name ON bans(name)");

    // --- GPS точки ---
    q[0] = EOS;
    strcat(q, "CREATE TABLE IF NOT EXISTS gps_points (");
    strcat(q, "id INTEGER PRIMARY KEY AUTOINCREMENT,");
    strcat(q, "category INTEGER NOT NULL DEFAULT 0,");
    strcat(q, "name TEXT NOT NULL,");
    strcat(q, "x REAL NOT NULL, y REAL NOT NULL, z REAL NOT NULL)");
    Database_Exec(q);

    print("[DB] " DATABASE_FILE " is ready.");
    return true;
}

stock Database_Close()
{
    if (gDatabase != DB:0) {
        DB_Close(gDatabase);
        gDatabase = DB:0;
    }
}
