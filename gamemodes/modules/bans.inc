// Банове в базата данни (по име и IP, временни или завинаги).

// Връща true и кика, ако играчът е баннат.
stock bool:Bans_CheckPlayer(playerid)
{
    new ip[16], q[384];
    GetPlayerIp(playerid, ip, sizeof (ip));

    format(q, sizeof (q),
        "SELECT admin, reason, expire FROM bans WHERE (name = '%s' OR ip = '%s') AND (expire = 0 OR expire > %d) ORDER BY id DESC LIMIT 1",
        PlayerData[playerid][pName], ip, gettime());

    new DBResult:result = Database_Query(q);
    if (result == DBResult:0) return false;

    if (DB_GetRowCount(result) == 0) {
        DB_FreeResultSet(result);
        return false;
    }

    new admin[MAX_PLAYER_NAME + 1], reason[128], expire;
    DB_GetFieldStringByName(result, "admin", admin);
    DB_GetFieldStringByName(result, "reason", reason);
    expire = DB_GetFieldIntByName(result, "expire");
    DB_FreeResultSet(result);

    SendClientMessage(playerid, COLOR_RED, "Ти си баннат от този сървър.");
    SendClientMessage(playerid, COLOR_WHITE, "Админ: {FFC400}%s{FFFFFF} | Причина: {FFC400}%s", admin, reason);
    if (expire == 0) {
        SendClientMessage(playerid, COLOR_WHITE, "Срок: {FF4040}завинаги");
    } else {
        new hoursLeft = (expire - gettime()) / 3600 + 1;
        SendClientMessage(playerid, COLOR_WHITE, "Остават приблизително {FFC400}%d{FFFFFF} часа.", hoursLeft);
    }
    SendClientMessage(playerid, COLOR_GREY, "Ако смяташ, че е грешка, пиши на %s", SERVER_WEBSITE);
    KickEx(playerid);
    return true;
}

// days = 0 -> завинаги
stock Bans_Add(const name[], const ip[], const admin[], const reason[], days)
{
    new eReason[256], q[512];
    SQL_Escape(reason, eReason);
    new expire = (days > 0) ? (gettime() + days * 86400) : 0;

    format(q, sizeof (q),
        "INSERT INTO bans (name, ip, admin, reason, date, expire) VALUES ('%s', '%s', '%s', '%s', %d, %d)",
        name, ip, admin, eReason, gettime(), expire);
    Database_Exec(q);
}

// Връща true, ако е намерен активен бан и е премахнат.
stock bool:Bans_Remove(const name[])
{
    new eName[64], q[256];
    SQL_Escape(name, eName);

    format(q, sizeof (q),
        "SELECT COUNT(*) AS c FROM bans WHERE name = '%s' AND (expire = 0 OR expire > %d)", eName, gettime());
    new DBResult:result = Database_Query(q), count = 0;
    if (result != DBResult:0) {
        count = DB_GetFieldIntByName(result, "c");
        DB_FreeResultSet(result);
    }
    if (count == 0) return false;

    // Изтича веднага (пазим историята, не трием).
    format(q, sizeof (q),
        "UPDATE bans SET expire = %d WHERE name = '%s' AND (expire = 0 OR expire > %d)", gettime(), eName, gettime());
    Database_Exec(q);
    return true;
}
