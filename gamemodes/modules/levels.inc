// Активно време, PayDay и нива.
//
// PayDay не дава пари (парите се печелят от работи и бизнеси), а само опит.
// Време се брои само когато играчът НЕ е AFK (виж modules/afk.inc).

stock Levels_GetExpNeeded(level)
{
    return level * EXP_PER_LEVEL;
}

// Всяка минута
stock Levels_OnPlayerMinute(playerid)
{
    if (Afk_IsAFK(playerid)) return;

    PlayerData[playerid][pPlayedMinutes]++;
    PlayerData[playerid][pPaydayMinutes]++;

    if (PlayerData[playerid][pPaydayMinutes] >= PAYDAY_MINUTES) {
        PlayerData[playerid][pPaydayMinutes] = 0;
        Levels_PayDay(playerid);
    }
}

stock Levels_PayDay(playerid)
{
    PlayerData[playerid][pExp] += PAYDAY_EXP;

    SendClientMessage(playerid, COLOR_BRAND, "________________ {FFFFFF}PAYDAY {FF6A00}________________");
    SendClientMessage(playerid, COLOR_WHITE, "Получи {FFD54A}+%d опит{FFFFFF} за час активна игра.", PAYDAY_EXP);

    new needed = Levels_GetExpNeeded(PlayerData[playerid][pLevel]);
    if (PlayerData[playerid][pExp] >= needed) {
        PlayerData[playerid][pExp] -= needed;
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
