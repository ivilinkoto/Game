// VIP статус – купува се с монети (coins) от /shop.
// Предимствата са удобства, а не сила – за да е честно за всички.

#define VIP_NONE    0
#define VIP_BRONZE  1
#define VIP_SILVER  2
#define VIP_GOLD    3

stock Vip_GetName(level, dest[], size = sizeof (dest))
{
    switch (level) {
        case VIP_BRONZE: format(dest, size, "Bronze");
        case VIP_SILVER: format(dest, size, "Silver");
        case VIP_GOLD:   format(dest, size, "Gold");
        default:         format(dest, size, "Няма");
    }
}

// Бонус към заплатата в проценти
stock Vip_GetPaydayBonus(playerid)
{
    switch (PlayerData[playerid][pVip]) {
        case VIP_BRONZE: return 10;
        case VIP_SILVER: return 20;
        case VIP_GOLD:   return 30;
    }
    return 0;
}

stock bool:Vip_IsActive(playerid)
{
    return (PlayerData[playerid][pVip] > VIP_NONE && PlayerData[playerid][pVipExpire] > gettime());
}

stock Vip_CheckExpire(playerid)
{
    if (PlayerData[playerid][pVip] > VIP_NONE && PlayerData[playerid][pVipExpire] <= gettime()) {
        PlayerData[playerid][pVip] = VIP_NONE;
        PlayerData[playerid][pVipExpire] = 0;
        SendClientMessage(playerid, COLOR_VIP, "[VIP] Твоят VIP статус изтече. Можеш да го подновиш от /shop.");
        Accounts_Save(playerid, false);
    }
}

// Дава или удължава VIP. Ако е същото ниво – добавя дни към оставащите.
stock Vip_Give(playerid, level, days)
{
    new now = gettime();
    if (PlayerData[playerid][pVip] == level && PlayerData[playerid][pVipExpire] > now) {
        PlayerData[playerid][pVipExpire] += days * 86400;
    } else {
        PlayerData[playerid][pVip] = level;
        PlayerData[playerid][pVipExpire] = now + days * 86400;
    }
    Accounts_Save(playerid, false);
}

stock Vip_ShowLoginInfo(playerid)
{
    if (!Vip_IsActive(playerid)) return 0;
    new name[16], daysLeft = (PlayerData[playerid][pVipExpire] - gettime()) / 86400;
    Vip_GetName(PlayerData[playerid][pVip], name);
    SendClientMessage(playerid, COLOR_VIP, "[VIP] Имаш VIP %s. Остават %d дни.", name, daysLeft);
    return 1;
}

CMD:vip(playerid, params[])
{
    #pragma unused params
    if (!Vip_IsActive(playerid)) {
        SendClientMessage(playerid, COLOR_VIP, "Нямаш VIP статус. Виж /shop за повече информация.");
        return 1;
    }
    new name[16], daysLeft = (PlayerData[playerid][pVipExpire] - gettime()) / 86400;
    Vip_GetName(PlayerData[playerid][pVip], name);
    SendClientMessage(playerid, COLOR_VIP, "VIP %s | Остават: %d дни | Бонус към заплата: +%d%%",
        name, daysLeft, Vip_GetPaydayBonus(playerid));
    SendClientMessage(playerid, COLOR_WHITE, "Команди: /vc (VIP чат)");
    return 1;
}

// VIP чат
CMD:vc(playerid, params[])
{
    if (!Vip_IsActive(playerid) && PlayerData[playerid][pAdmin] == 0) {
        return SendError(playerid, "Само за VIP играчи.");
    }
    if (IsNull(params)) return SendUsage(playerid, "/vc [текст]");

    new rpName[MAX_PLAYER_NAME + 1];
    GetRPName(playerid, rpName);
    for (new i = 0; i < MAX_PLAYERS; i++) {
        if (!IsPlayerConnected(i) || !PlayerData[i][pLogged]) continue;
        if (!Vip_IsActive(i) && PlayerData[i][pAdmin] == 0) continue;
        SendClientMessage(i, COLOR_VIP, "[VIP] %s: %s", rpName, params);
    }
    return 1;
}
