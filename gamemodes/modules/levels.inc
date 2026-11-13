// Активно време, заплата и нива.
//
// Време се брои само когато играчът е активен:
//  - клиентът праща данни (играта не е минимизирана/на пауза)
//  - героят се е движил през последните AFK_IDLE_MINUTES минути

stock Levels_GetExpNeeded(level)
{
    return level * EXP_PER_LEVEL;
}

stock Levels_GetSalary(playerid)
{
    return PAYDAY_BASE + PlayerData[playerid][pLevel] * PAYDAY_PER_LEVEL;
}

stock bool:Levels_IsAFK(playerid)
{
    if (GetTickCount() - PlayerData[playerid][pLastUpdateTick] > AFK_PAUSE_MS) return true;
    return (PlayerData[playerid][pIdleMinutes] >= AFK_IDLE_MINUTES);
}

// Извиква се от OnPlayerUpdate (много често – само записва времето).
stock Levels_OnPlayerUpdate(playerid)
{
    PlayerData[playerid][pLastUpdateTick] = GetTickCount();
}

// Всяка минута
stock Levels_OnPlayerMinute(playerid)
{
    // Движил ли се е през последната минута?
    new Float:x, Float:y, Float:z;
    GetPlayerPos(playerid, x, y, z);
    new Float:dx = x - PlayerData[playerid][pIdleX],
        Float:dy = y - PlayerData[playerid][pIdleY],
        Float:dz = z - PlayerData[playerid][pIdleZ];
    if (dx * dx + dy * dy + dz * dz < 1.0) {
        PlayerData[playerid][pIdleMinutes]++;
    } else {
        PlayerData[playerid][pIdleMinutes] = 0;
    }
    PlayerData[playerid][pIdleX] = x;
    PlayerData[playerid][pIdleY] = y;
    PlayerData[playerid][pIdleZ] = z;

    if (Levels_IsAFK(playerid)) return;

    PlayerData[playerid][pPlayedMinutes]++;
    PlayerData[playerid][pPaydayMinutes]++;

    if (PlayerData[playerid][pPaydayMinutes] >= PAYDAY_MINUTES) {
        PlayerData[playerid][pPaydayMinutes] = 0;
        Levels_PayDay(playerid);
    }
}

stock Levels_PayDay(playerid)
{
    new salary = Levels_GetSalary(playerid);
    Money_Give(playerid, salary);
    PlayerData[playerid][pExp]++;

    new s1[16], s2[16];
    FormatNumber(salary, s1);
    FormatNumber(PlayerData[playerid][pMoney], s2);

    SendClientMessage(playerid, COLOR_BRAND, "________________ {FFFFFF}ЗАПЛАТА {FF6A00}________________");
    SendClientMessage(playerid, COLOR_WHITE, "Заплата: {33CC66}$%s{FFFFFF}  |  В брой: {33CC66}$%s", s1, s2);

    new needed = Levels_GetExpNeeded(PlayerData[playerid][pLevel]);
    if (PlayerData[playerid][pExp] >= needed) {
        PlayerData[playerid][pExp] = 0;
        PlayerData[playerid][pLevel]++;
        SetPlayerScore(playerid, PlayerData[playerid][pLevel]);
        SendClientMessage(playerid, COLOR_GOLD, "Поздравления! Вече си ниво %d.", PlayerData[playerid][pLevel]);
        GameTextForPlayer(playerid, "~y~LEVEL UP!", 4000, 1);
    } else {
        SendClientMessage(playerid, COLOR_GREY, "Опит: %d / %d до ниво %d.",
            PlayerData[playerid][pExp], needed, PlayerData[playerid][pLevel] + 1);
    }
    Auth_SaveAccount(playerid);
}
