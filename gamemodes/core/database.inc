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

// Добавя колона, ако я няма (за бази от по-стари версии).
stock Database_AddColumn(const table[], const column[], const definition[])
{
    new q[256], DBResult:r, bool:exists = false, name[64];
    format(q, sizeof (q), "PRAGMA table_info(%s)", table);
    r = DB_ExecuteQuery(gDatabase, "%s", q);
    if (r != DBResult:0) {
        new rows = DB_GetRowCount(r);
        for (new i = 0; i < rows; i++) {
            DB_GetFieldStringByName(r, "name", name, sizeof (name));
            if (strcmp(name, column, true) == 0) {
                exists = true;
                break;
            }
            DB_SelectNextRow(r);
        }
        DB_FreeResultSet(r);
    }
    if (!exists) {
        format(q, sizeof (q), "ALTER TABLE %s ADD COLUMN %s %s", table, column, definition);
        Database_Exec(q);
        printf("[DB] Added column %s.%s", table, column);
    }
}

// Служебни стойности (еднократни миграции и т.н.)
stock Database_GetMeta(const key[])
{
    new q[128], value = 0;
    format(q, sizeof (q), "SELECT value FROM server_meta WHERE key = '%s'", key);
    new DBResult:r = DB_ExecuteQuery(gDatabase, "%s", q);
    if (r != DBResult:0) {
        if (DB_GetRowCount(r) > 0) value = DB_GetFieldIntByName(r, "value");
        DB_FreeResultSet(r);
    }
    return value;
}

stock Database_SetMeta(const key[], value)
{
    new q[160];
    format(q, sizeof (q), "INSERT OR REPLACE INTO server_meta (key, value) VALUES ('%s', %d)", key, value);
    Database_Exec(q);
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

    // --- Колони, добавени след етап 1 ---
    Database_AddColumn("accounts", "garage_city", "INTEGER NOT NULL DEFAULT 0");
    Database_AddColumn("accounts", "starter_given", "INTEGER NOT NULL DEFAULT 0");

    // --- Камиони на играчите ---
    q[0] = EOS;
    strcat(q, "CREATE TABLE IF NOT EXISTS player_vehicles (");
    strcat(q, "id INTEGER PRIMARY KEY AUTOINCREMENT,");
    strcat(q, "owner_id INTEGER NOT NULL,");
    strcat(q, "slot INTEGER NOT NULL,");
    strcat(q, "model INTEGER NOT NULL,");
    strcat(q, "color1 INTEGER NOT NULL DEFAULT 1,");
    strcat(q, "color2 INTEGER NOT NULL DEFAULT 1,");
    strcat(q, "health REAL NOT NULL DEFAULT 1000,");
    strcat(q, "created_at INTEGER NOT NULL DEFAULT 0)");
    Database_Exec(q);
    Database_Exec("CREATE INDEX IF NOT EXISTS idx_pveh_owner ON player_vehicles(owner_id)");

    Database_Exec("CREATE TABLE IF NOT EXISTS server_meta (key TEXT PRIMARY KEY, value INTEGER NOT NULL DEFAULT 0)");

    // --- Входове на гаражите (по град) ---
    Database_Exec("CREATE TABLE IF NOT EXISTS garage_entrances (city INTEGER PRIMARY KEY, x REAL NOT NULL, y REAL NOT NULL, z REAL NOT NULL, a REAL NOT NULL)");

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
