// Админ система. Нива: 1 Хелпър, 2 Модератор, 3 Админ, 4 Старши админ, 5 Собственик.
// Първия собственик го правиш с RCON: /rcon login [парола], после /makeadmin [твоето ID] 5

#define ADMIN_HELPER     1
#define ADMIN_MODERATOR  2
#define ADMIN_ADMIN      3
#define ADMIN_SENIOR     4
#define ADMIN_OWNER      5

stock Admin_GetRankName(level, dest[], size = sizeof (dest))
{
    switch (level) {
        case ADMIN_HELPER:    format(dest, size, "Хелпър");
        case ADMIN_MODERATOR: format(dest, size, "Модератор");
        case ADMIN_ADMIN:     format(dest, size, "Админ");
        case ADMIN_SENIOR:    format(dest, size, "Старши админ");
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

// Не може да се действа срещу админ с по-високо или равно ниво (освен собственик).
stock bool:Admin_CanTarget(playerid, targetid)
{
    if (playerid == targetid) return true;
    if (PlayerData[playerid][pAdmin] >= ADMIN_OWNER) return true;
    if (PlayerData[targetid][pAdmin] >= PlayerData[playerid][pAdmin]) {
        SendError(playerid, "Не можеш да използваш това срещу админ с равно или по-високо ниво.");
        return false;
    }
    return true;
}

stock Admin_Broadcast(colour, const msg[])
{
    for (new i = 0; i < MAX_PLAYERS; i++) {
        if (IsPlayerConnected(i) && PlayerData[i][pLogged] && PlayerData[i][pAdmin] > 0) {
            SendClientMessage(i, colour, "%s", msg);
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

// ============================ Команди ============================

CMD:ahelp(playerid, params[])
{
    #pragma unused params
    new lvl = PlayerData[playerid][pAdmin];
    if (!Admin_Check(playerid, ADMIN_HELPER)) return 1;

    SendClientMessage(playerid, COLOR_ADMIN, "______________ Админ команди ______________");
    SendClientMessage(playerid, COLOR_WHITE, "[1] /a /kick /goto /gethere /admins");
    if (lvl >= ADMIN_MODERATOR) SendClientMessage(playerid, COLOR_WHITE, "[2] /ban /sethp /setskin /veh /dv");
    if (lvl >= ADMIN_ADMIN)     SendClientMessage(playerid, COLOR_WHITE, "[3] /unban /ann");
    if (lvl >= ADMIN_SENIOR)    SendClientMessage(playerid, COLOR_WHITE, "[4] /givemoney");
    if (lvl >= ADMIN_OWNER)     SendClientMessage(playerid, COLOR_WHITE, "[5] /makeadmin /setvip /addcoins");
    return 1;
}

// Админ чат
CMD:a(playerid, params[])
{
    if (!Admin_Check(playerid, ADMIN_HELPER)) return 1;
    if (IsNull(params)) return SendUsage(playerid, "/a [текст]");

    new rank[32], msg[200];
    Admin_GetRankName(PlayerData[playerid][pAdmin], rank);
    format(msg, sizeof (msg), "[A] %s %s: %s", rank, PlayerData[playerid][pName], params);
    Admin_Broadcast(COLOR_ADMIN, msg);
    return 1;
}

CMD:kick(playerid, params[])
{
    if (!Admin_Check(playerid, ADMIN_HELPER)) return 1;

    new targetid, reason[96];
    if (!GetParamRest(params, 1, reason)) return SendUsage(playerid, "/kick [ID/име] [причина]");
    if (!GetParamPlayer(playerid, params, 0, targetid)) return 1;
    if (!Admin_CanTarget(playerid, targetid)) return 1;

    SendClientMessageToAll(COLOR_ADMIN, "Админ %s изгони %s. Причина: %s",
        PlayerData[playerid][pName], PlayerData[targetid][pName], reason);
    Log_Add("kick", PlayerData[playerid][pName], PlayerData[targetid][pName], reason);
    KickEx(targetid);
    return 1;
}

// /ban [ID] [дни, 0 = завинаги] [причина]
CMD:ban(playerid, params[])
{
    if (!Admin_Check(playerid, ADMIN_MODERATOR)) return 1;

    new targetid, days, reason[96];
    if (!GetParamInt(params, 1, days) || !GetParamRest(params, 2, reason) || days < 0) {
        return SendUsage(playerid, "/ban [ID/име] [дни (0 = завинаги)] [причина]");
    }
    if (!GetParamPlayer(playerid, params, 0, targetid)) return 1;
    if (targetid == playerid) return SendError(playerid, "Не можеш да баннеш себе си.");
    if (!Admin_CanTarget(playerid, targetid)) return 1;

    new ip[16];
    GetPlayerIp(targetid, ip, sizeof (ip));
    Bans_Add(PlayerData[targetid][pName], ip, PlayerData[playerid][pName], reason, days);

    if (days == 0) {
        SendClientMessageToAll(COLOR_ADMIN, "Админ %s банна %s завинаги. Причина: %s",
            PlayerData[playerid][pName], PlayerData[targetid][pName], reason);
    } else {
        SendClientMessageToAll(COLOR_ADMIN, "Админ %s банна %s за %d дни. Причина: %s",
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

    SendClientMessage(playerid, COLOR_ADMIN, "Банът на %s е премахнат.", name);
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
    SendClientMessage(playerid, COLOR_ADMIN, "Телепортира се до %s.", PlayerData[targetid][pName]);
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
    SendClientMessage(targetid, COLOR_ADMIN, "Админ %s те телепортира до себе си.", PlayerData[playerid][pName]);
    SendClientMessage(playerid, COLOR_ADMIN, "Телепортира %s до себе си.", PlayerData[targetid][pName]);
    return 1;
}

CMD:sethp(playerid, params[])
{
    if (!Admin_Check(playerid, ADMIN_MODERATOR)) return 1;

    new targetid, hp;
    if (!GetParamInt(params, 1, hp) || hp < 0 || hp > 100) return SendUsage(playerid, "/sethp [ID/име] [0-100]");
    if (!GetParamPlayer(playerid, params, 0, targetid)) return 1;

    SetPlayerHealth(targetid, float(hp));
    SendClientMessage(playerid, COLOR_ADMIN, "Зададе %d HP на %s.", hp, PlayerData[targetid][pName]);
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
    SendClientMessage(playerid, COLOR_ADMIN, "Смени скина на %s на %d.", PlayerData[targetid][pName], skin);
    return 1;
}

CMD:givemoney(playerid, params[])
{
    if (!Admin_Check(playerid, ADMIN_SENIOR)) return 1;

    new targetid, amount;
    if (!GetParamInt(params, 1, amount)) return SendUsage(playerid, "/givemoney [ID/име] [сума (може и отрицателна)]");
    if (!GetParamPlayer(playerid, params, 0, targetid)) return 1;

    GivePlayerCash(targetid, amount);
    SendClientMessage(playerid, COLOR_ADMIN, "Даде $%d на %s.", amount, PlayerData[targetid][pName]);
    SendClientMessage(targetid, COLOR_ADMIN, "Админ %s ти даде $%d.", PlayerData[playerid][pName], amount);

    new details[32];
    format(details, sizeof (details), "$%d", amount);
    Log_Add("givemoney", PlayerData[playerid][pName], PlayerData[targetid][pName], details);
    Accounts_Save(targetid, false);
    return 1;
}

// Създава временна админ кола (изчезва при изход или нова /veh).
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
    }

    new Float:x, Float:y, Float:z, Float:a;
    GetPlayerPos(playerid, x, y, z);
    GetPlayerFacingAngle(playerid, a);

    new vehicleid = CreateVehicle(model, x, y, z + 0.5, a, c1, c2, -1);
    if (vehicleid == INVALID_VEHICLE_ID || vehicleid == 0) {
        PlayerData[playerid][pAdminVehicle] = INVALID_VEHICLE_ID;
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
    // Ако е нечия админ кола – изчистваме връзката
    for (new i = 0; i < MAX_PLAYERS; i++) {
        if (PlayerData[i][pAdminVehicle] == vehicleid) {
            PlayerData[i][pAdminVehicle] = INVALID_VEHICLE_ID;
        }
    }
    DestroyVehicle(vehicleid);
    SendClientMessage(playerid, COLOR_ADMIN, "Превозното средство е премахнато.");
    return 1;
}

CMD:ann(playerid, params[])
{
    if (!Admin_Check(playerid, ADMIN_ADMIN)) return 1;
    if (IsNull(params)) return SendUsage(playerid, "/ann [текст]");
    SendClientMessageToAll(COLOR_ORANGE, "[Обявление] %s", params);
    return 1;
}

// /makeadmin [ID] [0-5] – собственик или RCON админ
CMD:makeadmin(playerid, params[])
{
    if (PlayerData[playerid][pAdmin] < ADMIN_OWNER && !IsPlayerAdmin(playerid)) {
        return SendError(playerid, "Нямаш права за тази команда.");
    }

    new targetid, level;
    if (!GetParamInt(params, 1, level) || level < 0 || level > ADMIN_OWNER) {
        return SendUsage(playerid, "/makeadmin [ID/име] [0-5]");
    }
    if (!GetParamPlayer(playerid, params, 0, targetid)) return 1;

    PlayerData[targetid][pAdmin] = level;
    Accounts_Save(targetid, false);

    new rank[32];
    Admin_GetRankName(level, rank);
    SendClientMessage(targetid, COLOR_ADMIN, "%s те направи %s (ниво %d).", PlayerData[playerid][pName], rank, level);
    SendClientMessage(playerid, COLOR_ADMIN, "%s вече е %s (ниво %d).", PlayerData[targetid][pName], rank, level);

    new details[16];
    format(details, sizeof (details), "ниво %d", level);
    Log_Add("makeadmin", PlayerData[playerid][pName], PlayerData[targetid][pName], details);
    return 1;
}

// /setvip [ID] [1-3] [дни] – ръчно даване (напр. награда за събитие)
CMD:setvip(playerid, params[])
{
    if (!Admin_Check(playerid, ADMIN_OWNER)) return 1;

    new targetid, level, days;
    if (!GetParamInt(params, 1, level) || !GetParamInt(params, 2, days) || level < 0 || level > VIP_GOLD || days < 0) {
        return SendUsage(playerid, "/setvip [ID/име] [0-3] [дни]  (0 = премахва VIP)");
    }
    if (!GetParamPlayer(playerid, params, 0, targetid)) return 1;

    if (level == VIP_NONE || days == 0) {
        PlayerData[targetid][pVip] = VIP_NONE;
        PlayerData[targetid][pVipExpire] = 0;
        Accounts_Save(targetid, false);
        SendClientMessage(playerid, COLOR_ADMIN, "Премахна VIP на %s.", PlayerData[targetid][pName]);
    } else {
        Vip_Give(targetid, level, days);
        new name[16];
        Vip_GetName(level, name);
        SendClientMessage(targetid, COLOR_VIP, "Получи VIP %s за %d дни!", name, days);
        SendClientMessage(playerid, COLOR_ADMIN, "Даде VIP %s за %d дни на %s.", name, days, PlayerData[targetid][pName]);
    }

    new details[32];
    format(details, sizeof (details), "VIP %d, %d дни", level, days);
    Log_Add("setvip", PlayerData[playerid][pName], PlayerData[targetid][pName], details);
    return 1;
}

CMD:addcoins(playerid, params[])
{
    if (!Admin_Check(playerid, ADMIN_OWNER)) return 1;

    new targetid, amount;
    if (!GetParamInt(params, 1, amount)) return SendUsage(playerid, "/addcoins [ID/име] [брой (може и отрицателен)]");
    if (!GetParamPlayer(playerid, params, 0, targetid)) return 1;

    PlayerData[targetid][pCoins] += amount;
    if (PlayerData[targetid][pCoins] < 0) PlayerData[targetid][pCoins] = 0;
    Accounts_Save(targetid, false);

    SendClientMessage(playerid, COLOR_ADMIN, "%s вече има %d монети.", PlayerData[targetid][pName], PlayerData[targetid][pCoins]);

    new details[32];
    format(details, sizeof (details), "%d монети", amount);
    Log_Add("addcoins", PlayerData[playerid][pName], PlayerData[targetid][pName], details);
    return 1;
}
