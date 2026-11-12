// Пари (на сървъра, защита срещу money hack), заплата и нива.

// Винаги ползвай тези функции вместо GivePlayerMoney!
stock GivePlayerCash(playerid, amount)
{
    PlayerData[playerid][pMoney] += amount;
    SyncPlayerMoney(playerid);
}

stock SetPlayerCash(playerid, amount)
{
    PlayerData[playerid][pMoney] = amount;
    SyncPlayerMoney(playerid);
}

stock GetPlayerCash(playerid)
{
    return PlayerData[playerid][pMoney];
}

stock SyncPlayerMoney(playerid)
{
    ResetPlayerMoney(playerid);
    GivePlayerMoney(playerid, PlayerData[playerid][pMoney]);
}

// Всяка секунда: ако клиентът показва различна сума (чийт), връщаме истинската.
stock Economy_OnTick()
{
    for (new i = 0; i < MAX_PLAYERS; i++) {
        if (!IsPlayerConnected(i) || !PlayerData[i][pLogged]) continue;
        if (GetPlayerMoney(i) != PlayerData[i][pMoney]) {
            SyncPlayerMoney(i);
        }
    }
}

stock Economy_GetExpNeeded(level)
{
    return level * EXP_PER_LEVEL;
}

// Всяка изиграна минута
stock Economy_OnPlayerMinute(playerid)
{
    PlayerData[playerid][pPlayedMinutes]++;
    PlayerData[playerid][pPaydayMinutes]++;

    if (PlayerData[playerid][pPaydayMinutes] >= PAYDAY_MINUTES) {
        PlayerData[playerid][pPaydayMinutes] = 0;
        Economy_PayDay(playerid);
    }
}

stock Economy_PayDay(playerid)
{
    new salary = PAYDAY_BASE + PlayerData[playerid][pLevel] * PAYDAY_PER_LEVEL;
    new bonusPercent = Vip_GetPaydayBonus(playerid);
    new bonus = salary * bonusPercent / 100;

    PlayerData[playerid][pBank] += salary + bonus;
    PlayerData[playerid][pExp]++;

    new s1[16], s2[16], s3[16];
    FormatNumber(salary, s1);
    FormatNumber(bonus, s2);
    FormatNumber(PlayerData[playerid][pBank], s3);

    SendClientMessage(playerid, COLOR_WHITE, "|___________ {33CC33}ЗАПЛАТА{FFFFFF} ___________|");
    SendClientMessage(playerid, COLOR_WHITE, "  Заплата: {33CC33}$%s", s1);
    if (bonus > 0) {
        SendClientMessage(playerid, COLOR_WHITE, "  VIP бонус (+%d%%): {FFD700}$%s", bonusPercent, s2);
    }
    SendClientMessage(playerid, COLOR_WHITE, "  Баланс в банката: {33CC33}$%s", s3);

    new needed = Economy_GetExpNeeded(PlayerData[playerid][pLevel]);
    if (PlayerData[playerid][pExp] >= needed) {
        PlayerData[playerid][pExp] = 0;
        PlayerData[playerid][pLevel]++;
        SetPlayerScore(playerid, PlayerData[playerid][pLevel]);
        SendClientMessage(playerid, COLOR_YELLOW, "Поздравления! Вече си ниво %d.", PlayerData[playerid][pLevel]);
        GameTextForPlayer(playerid, "~y~LEVEL UP!", 4000, 1);
    } else {
        SendClientMessage(playerid, COLOR_GREY, "  Опит: %d/%d до следващо ниво.", PlayerData[playerid][pExp], needed);
    }

    Accounts_Save(playerid, false);
}
