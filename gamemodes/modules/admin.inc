// Админ система и банове.
//
// Нива: 1 Хелпър, 2 Модератор, 3 Администратор, 4 Главен администратор, 5 Собственик.
// Първия собственик го правиш с RCON:
//     /rcon login [паролата от config.json]
//     /makeadmin [твоето ID] 5

#define ADMIN_HELPER     1
#define ADMIN_MODERATOR  2
#define ADMIN_ADMIN      3
#define ADMIN_HEAD       4
#define ADMIN_OWNER      5

stock Admin_GetRankName(level, dest[], size = sizeof (dest))
{
    switch (level) {
        case ADMIN_HELPER:    format(dest, size, "Хелпър");
        case ADMIN_MODERATOR: format(dest, size, "Модератор");
        case ADMIN_ADMIN:     format(dest, size, "Администратор");
        case ADMIN_HEAD:      format(dest, size, "Главен администратор");
        case ADMIN_OWNER:     format(dest, size, "Собственик");
        default:              format(dest, size, "Играч");
    }
}

// Проверява нивото и пише грешка, ако не стига.
stock bool:Admin_Check(playerid, level)
{
    if (PlayerData[playerid][pAdmin] >= level) return true;
    SendError(playerid, "Нямаш права за тази команда.");
    return false;
}

// Не може срещу админ с равно или по-високо ниво (освен собственик и себе си).
stock bool:Admin_CanTarget(playerid, targetid)
{
    if (playerid == targetid || PlayerData[playerid][pAdmin] >= ADMIN_OWNER) return true;
    if (PlayerData[targetid][pAdmin] >= PlayerData[playerid][pAdmin]) {
        SendError(playerid, "Не можеш да го направиш на админ с равно или по-високо ниво.");
        return false;
    }
    return true;
}

stock Admin_Broadcast(const msg[])
{
    for (new i = 0; i < MAX_PLAYERS; i++) {
        if (IsPlayerConnected(i) && PlayerData[i][pLogged] && PlayerData[i][pAdmin] > 0) {
            SendClientMessage(i, COLOR_ADMIN, "%s", msg);
        }
    }
}

stock Admin_OnPlayerDisconnect(playerid)
{
    if (PlayerData[playerid][pAdminVehicle] != INVALID_VEHICLE_ID) {
        DestroyVehicle(PlayerData[playerid][pAdminVehicle]);
        PlayerData[playerid][pAdminVehicle] = INVALID_VEHICLE_ID;
    }
}

// ============================================================
//  Банове
// ============================================================

// При свързване. Връща true, ако играчът е баннат (и подготвя текста за прозореца).
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

    new admin[MAX_PLAYER_NAME + 1], reason[128], expire, term[64];
    DB_GetFieldStringByName(result, "admin", admin);
    DB_GetFieldStringByName(result, "reason", reason);
    expire = DB_GetFieldIntByName(result, "expire");
    DB_FreeResultSet(result);

    if (expire == 0) {
        format(term, sizeof (term), C_RED "завинаги");
    } else {
        format(term, sizeof (term), C_GOLD "още около %d ч.", (expire - gettime()) / 3600 + 1);
    }

    format(PlayerData[playerid][pBanInfo], 256,
        C_WHITE "Админ: " C_GOLD "%s\n" C_WHITE "Причина: " C_GOLD "%s\n" C_WHITE "Срок: %s",
        admin, reason, term);
    PlayerData[playerid][pBanned] = true;
    return true;
}

stock Bans_ShowDialog(playerid)
{
    new body[768];
    format(body, sizeof (body),
        C_RED "Достъпът ти до HOTFIX ONLINE е ограничен.\n\n" \
        "%s\n\n" \
        C_GREY "Ако смяташ, че е грешка, пиши на администрацията в Discord: %s",
        PlayerData[playerid][pBanInfo], SERVER_DISCORD);
    ShowPlayerDialog(playerid, DIALOG_INFO_KICK, DIALOG_STYLE_MSGBOX, C_BRAND "HOTFIX ONLINE " C_WHITE "| Бан", "%s", "Изход", "", body);
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

// Връща true, ако е имало активен бан.
stock bool:Bans_Remove(const name[])
{
    new eName[64], q[256], count = 0;
    SQL_Escape(name, eName);
    format(q, sizeof (q),
        "SELECT COUNT(*) AS c FROM bans WHERE name = '%s' AND (expire = 0 OR expire > %d)", eName, gettime());
    new DBResult:result = Database_Query(q);
    if (result != DBResult:0) {
        count = DB_GetFieldIntByName(result, "c");
        DB_FreeResultSet(result);
    }
    if (count == 0) return false;

    // Пазим историята – само прекратяваме бана.
    format(q, sizeof (q),
        "UPDATE bans SET expire = %d WHERE name = '%s' AND (expire = 0 OR expire > %d)", gettime(), eName, gettime());
    Database_Exec(q);
    return true;
}

// ============================================================
//  Команди
// ============================================================

CMD:ahelp(playerid, params[])
{
    #pragma unused params
    if (!Admin_Check(playerid, ADMIN_HELPER)) return 1;
    Admin_ShowHelp(playerid, false);
    return 1;
}

stock Admin_ShowHelp(playerid, bool:fromMenu)
{
    new lvl = PlayerData[playerid][pAdmin], body[1500], rank[32];
    Admin_GetRankName(lvl, rank);
    format(body, sizeof (body), C_WHITE "Твоят ранг: " C_BRAND "%s" C_WHITE " (ниво %d)\n\n", rank, lvl);

    strcat(body, C_GOLD "Хелпър (1)\n" C_WHITE "/a [текст] - админ чат\n/kick [ID] [причина]\n/goto [ID]   /gethere [ID]\n\n");
    if (lvl >= ADMIN_MODERATOR) strcat(body, C_GOLD "Модератор (2)\n" C_WHITE "/ban [ID] [дни, 0 = завинаги] [причина]\n/sethp [ID] [0-100]   /setskin [ID] [скин]\n/veh [модел] - временна кола   /dv - махни кола\n\n");
    if (lvl >= ADMIN_ADMIN)     strcat(body, C_GOLD "Администратор (3)\n" C_WHITE "/unban [Име_Фамилия]\n/ann [текст] - обявление до всички\n\n");
    if (lvl >= ADMIN_HEAD)      strcat(body, C_GOLD "Главен администратор (4)\n" C_WHITE "/givemoney [ID] [сума]\n\n");
    if (lvl >= ADMIN_OWNER)     strcat(body, C_GOLD "Собственик (5)\n" C_WHITE "/makeadmin [ID] [0-5]\n");

    if (fromMenu) {
        ShowPlayerDialog(playerid, DIALOG_MENU_BACK, DIALOG_STYLE_MSGBOX, C_BRAND "HOTFIX ONLINE " C_WHITE "| Админ команди", "%s", "Назад", "", body);
    } else {
        ShowPlayerDialog(playerid, DIALOG_INFO, DIALOG_STYLE_MSGBOX, C_BRAND "HOTFIX ONLINE " C_WHITE "| Админ команди", "%s", "Затвори", "", body);
    }
}

// Админ чат
CMD:a(playerid, params[])
{
    if (!Admin_Check(playerid, ADMIN_HELPER)) return 1;
    if (IsNull(params)) return SendUsage(playerid, "/a [текст]");

    new rank[32], msg[200];
    Admin_GetRankName(PlayerData[playerid][pAdmin], rank);
    format(msg, sizeof (msg), "[A] %s %s: %s", rank, PlayerData[playerid][pName], params);
    Admin_Broadcast(msg);
    return 1;
}

CMD:kick(playerid, params[])
{
    if (!Admin_Check(playerid, ADMIN_HELPER)) return 1;

    new targetid, reason[96];
    if (!GetParamRest(params, 1, reason)) return SendUsage(playerid, "/kick [ID/име] [причина]");
    if (!GetParamPlayer(playerid, params, 0, targetid)) return 1;
    if (!Admin_CanTarget(playerid, targetid)) return 1;

    SendClientMessageToAll(COLOR_ADMIN, "[Админ] %s изгони %s. Причина: %s",
        PlayerData[playerid][pName], PlayerData[targetid][pName], reason);
    Log_Add("kick", PlayerData[playerid][pName], PlayerData[targetid][pName], reason);
    KickEx(targetid);
    return 1;
}

CMD:ban(playerid, params[])
{
    if (!Admin_Check(playerid, ADMIN_MODERATOR)) return 1;

    new targetid, days, reason[96];
    if (!GetParamInt(params, 1, days) || !GetParamRest(params, 2, reason) || days < 0 || days > 3650) {
        return SendUsage(playerid, "/ban [ID/име] [дни, 0 = завинаги] [причина]");
    }
    if (!GetParamPlayer(playerid, params, 0, targetid)) return 1;
    if (targetid == playerid) return SendError(playerid, "Не можеш да баннеш себе си.");
    if (!Admin_CanTarget(playerid, targetid)) return 1;

    new ip[16];
    GetPlayerIp(targetid, ip, sizeof (ip));
    Bans_Add(PlayerData[targetid][pName], ip, PlayerData[playerid][pName], reason, days);

    if (days == 0) {
        SendClientMessageToAll(COLOR_ADMIN, "[Админ] %s банна %s завинаги. Причина: %s",
            PlayerData[playerid][pName], PlayerData[targetid][pName], reason);
    } else {
        SendClientMessageToAll(COLOR_ADMIN, "[Админ] %s банна %s за %d дни. Причина: %s",
            PlayerData[playerid][pName], PlayerData[targetid][pName], days, reason);
    }

    new details[128];
    format(details, sizeof (details), "%d дни: %s", days, reason);
    Log_Add("ban", PlayerData[playerid][pName], PlayerData[targetid][pName], details);
    KickEx(targetid);
    return 1;
}

CMD:unban(playerid, params[])
{
    if (!Admin_Check(playerid, ADMIN_ADMIN)) return 1;

    new name[MAX_PLAYER_NAME + 1];
    if (!GetParam(params, 0, name)) return SendUsage(playerid, "/unban [Име_Фамилия]");
    if (!Bans_Remove(name)) return SendError(playerid, "Няма активен бан за това име.");

    SendClientMessage(playerid, COLOR_ADMIN, "[Админ] Банът на %s е премахнат.", name);
    Log_Add("unban", PlayerData[playerid][pName], name, "");
    return 1;
}

CMD:goto(playerid, params[])
{
    if (!Admin_Check(playerid, ADMIN_HELPER)) return 1;

    new targetid;
    if (IsNull(params)) return SendUsage(playerid, "/goto [ID/име]");
    if (!GetParamPlayer(playerid, params, 0, targetid)) return 1;

    new Float:x, Float:y, Float:z;
    GetPlayerPos(targetid, x, y, z);
    SetPlayerInterior(playerid, GetPlayerInterior(targetid));
    SetPlayerVirtualWorld(playerid, GetPlayerVirtualWorld(targetid));
    SetPlayerPos(playerid, x + 1.0, y, z);
    SendClientMessage(playerid, COLOR_ADMIN, "[Админ] Телепортира се до %s.", PlayerData[targetid][pName]);
    return 1;
}

CMD:gethere(playerid, params[])
{
    if (!Admin_Check(playerid, ADMIN_HELPER)) return 1;

    new targetid;
    if (IsNull(params)) return SendUsage(playerid, "/gethere [ID/име]");
    if (!GetParamPlayer(playerid, params, 0, targetid)) return 1;

    new Float:x, Float:y, Float:z;
    GetPlayerPos(playerid, x, y, z);
    SetPlayerInterior(targetid, GetPlayerInterior(playerid));
    SetPlayerVirtualWorld(targetid, GetPlayerVirtualWorld(playerid));
    SetPlayerPos(targetid, x + 1.0, y, z);
    SendClientMessage(targetid, COLOR_ADMIN, "[Админ] %s те телепортира до себе си.", PlayerData[playerid][pName]);
    SendClientMessage(playerid, COLOR_ADMIN, "[Админ] Телепортира %s до себе си.", PlayerData[targetid][pName]);
    return 1;
}

CMD:sethp(playerid, params[])
{
    if (!Admin_Check(playerid, ADMIN_MODERATOR)) return 1;

    new targetid, hp;
    if (!GetParamInt(params, 1, hp) || hp < 0 || hp > 100) return SendUsage(playerid, "/sethp [ID/име] [0-100]");
    if (!GetParamPlayer(playerid, params, 0, targetid)) return 1;

    SetPlayerHealth(targetid, float(hp));
    SendClientMessage(playerid, COLOR_ADMIN, "[Админ] Зададе %d HP на %s.", hp, PlayerData[targetid][pName]);
    return 1;
}

CMD:setskin(playerid, params[])
{
    if (!Admin_Check(playerid, ADMIN_MODERATOR)) return 1;

    new targetid, skin;
    if (!GetParamInt(params, 1, skin) || skin < 0 || skin > 311 || skin == 74) {
        return SendUsage(playerid, "/setskin [ID/име] [0-311]");
    }
    if (!GetParamPlayer(playerid, params, 0, targetid)) return 1;

    PlayerData[targetid][pSkin] = skin;
    SetPlayerSkin(targetid, skin);
    SendClientMessage(playerid, COLOR_ADMIN, "[Админ] Смени скина на %s на %d.", PlayerData[targetid][pName], skin);
    Auth_SaveAccount(targetid);
    return 1;
}

CMD:givemoney(playerid, params[])
{
    if (!Admin_Check(playerid, ADMIN_HEAD)) return 1;

    new targetid, amount;
    if (!GetParamInt(params, 1, amount)) return SendUsage(playerid, "/givemoney [ID/име] [сума] (може и минус)");
    if (!GetParamPlayer(playerid, params, 0, targetid)) return 1;

    Money_Give(targetid, amount);
    SendClientMessage(playerid, COLOR_ADMIN, "[Админ] Даде $%d на %s.", amount, PlayerData[targetid][pName]);
    SendClientMessage(targetid, COLOR_ADMIN, "[Админ] %s ти даде $%d.", PlayerData[playerid][pName], amount);

    new details[32];
    format(details, sizeof (details), "$%d", amount);
    Log_Add("givemoney", PlayerData[playerid][pName], PlayerData[targetid][pName], details);
    Auth_SaveAccount(targetid);
    return 1;
}

// Временна админ кола (изчезва при нова /veh или при излизане).
CMD:veh(playerid, params[])
{
    if (!Admin_Check(playerid, ADMIN_MODERATOR)) return 1;

    new model, c1 = -1, c2 = -1;
    if (!GetParamInt(params, 0, model) || model < 400 || model > 611) {
        return SendUsage(playerid, "/veh [модел 400-611] [цвят1] [цвят2]");
    }
    GetParamInt(params, 1, c1);
    GetParamInt(params, 2, c2);

    if (PlayerData[playerid][pAdminVehicle] != INVALID_VEHICLE_ID) {
        DestroyVehicle(PlayerData[playerid][pAdminVehicle]);
        PlayerData[playerid][pAdminVehicle] = INVALID_VEHICLE_ID;
    }

    new Float:x, Float:y, Float:z, Float:a;
    GetPlayerPos(playerid, x, y, z);
    GetPlayerFacingAngle(playerid, a);

    new vehicleid = CreateVehicle(model, x, y, z + 0.5, a, c1, c2, -1);
    if (vehicleid == INVALID_VEHICLE_ID || vehicleid == 0) {
        return SendError(playerid, "Колата не може да бъде създадена.");
    }
    SetVehicleVirtualWorld(vehicleid, GetPlayerVirtualWorld(playerid));
    LinkVehicleToInterior(vehicleid, GetPlayerInterior(playerid));
    SetVehicleNumberPlate(vehicleid, "ADMIN");
    PutPlayerInVehicle(playerid, vehicleid, 0);
    PlayerData[playerid][pAdminVehicle] = vehicleid;
    return 1;
}

CMD:dv(playerid, params[])
{
    #pragma unused params
    if (!Admin_Check(playerid, ADMIN_MODERATOR)) return 1;
    if (!IsPlayerInAnyVehicle(playerid)) return SendError(playerid, "Не си в превозно средство.");

    new vehicleid = GetPlayerVehicleID(playerid);
    for (new i = 0; i < MAX_PLAYERS; i++) {
        if (IsPlayerConnected(i) && PlayerData[i][pAdminVehicle] == vehicleid) {
            PlayerData[i][pAdminVehicle] = INVALID_VEHICLE_ID;
        }
    }
    DestroyVehicle(vehicleid);
    SendClientMessage(playerid, COLOR_ADMIN, "[Админ] Превозното средство е премахнато.");
    return 1;
}

CMD:ann(playerid, params[])
{
    if (!Admin_Check(playerid, ADMIN_ADMIN)) return 1;
    if (IsNull(params)) return SendUsage(playerid, "/ann [текст]");
    SendClientMessageToAll(COLOR_BRAND, "[Обявление] {FFFFFF}%s", params);
    return 1;
}

// Собственик или RCON админ
CMD:makeadmin(playerid, params[])
{
    if (PlayerData[playerid][pAdmin] < ADMIN_OWNER && !IsPlayerAdmin(playerid)) {
        return SendError(playerid, "Нямаш права за тази команда.");
    }

    new targetid, level;
    if (!GetParamInt(params, 1, level) || level < 0 || level > ADMIN_OWNER) {
        return SendUsage(playerid, "/makeadmin [ID/име] [0-5]  (0 = премахва)");
    }
    if (!GetParamPlayer(playerid, params, 0, targetid)) return 1;

    PlayerData[targetid][pAdmin] = level;
    Auth_SaveAccount(targetid);

    new rank[32];
    Admin_GetRankName(level, rank);
    SendClientMessage(targetid, COLOR_ADMIN, "[Админ] %s ти даде ранг %s (ниво %d). Команди: /ahelp", PlayerData[playerid][pName], rank, level);
    if (targetid != playerid) {
        SendClientMessage(playerid, COLOR_ADMIN, "[Админ] %s вече е %s (ниво %d).", PlayerData[targetid][pName], rank, level);
    }

    new details[16];
    format(details, sizeof (details), "ниво %d", level);
    Log_Add("makeadmin", PlayerData[playerid][pName], PlayerData[targetid][pName], details);
    return 1;
}
