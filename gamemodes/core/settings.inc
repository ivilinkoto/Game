// Настройки на сървъра, които се сменят от играта (пазят се в базата).

stock Settings_Init()
{
    Database_Exec("CREATE TABLE IF NOT EXISTS settings (skey TEXT PRIMARY KEY, value INTEGER NOT NULL)");
}

stock Settings_GetInt(const key[], defaultValue)
{
    new q[128], value = defaultValue;
    format(q, sizeof (q), "SELECT value FROM settings WHERE skey = '%s' LIMIT 1", key);
    new DBResult:result = Database_Query(q);
    if (result != DBResult:0) {
        if (DB_GetRowCount(result) > 0) value = DB_GetFieldIntByName(result, "value");
        DB_FreeResultSet(result);
    }
    return value;
}

stock Settings_SetInt(const key[], value)
{
    new q[128];
    format(q, sizeof (q), "INSERT OR REPLACE INTO settings (skey, value) VALUES ('%s', %d)", key, value);
    Database_Exec(q);
}
