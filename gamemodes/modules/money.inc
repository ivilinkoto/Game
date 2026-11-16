// Пари – само на сървъра. Клиентските пари се изравняват всяка секунда,
// така че "напомпани" пари от чийт не значат нищо.

stock Money_Get(playerid)
{
    return PlayerData[playerid][pMoney];
}

stock Money_Set(playerid, amount)
{
    PlayerData[playerid][pMoney] = amount;
    Money_Sync(playerid);
    Hud_UpdateMoney(playerid);
    Money_Save(playerid);
}

stock Money_Give(playerid, amount)
{
    Money_Set(playerid, PlayerData[playerid][pMoney] + amount);
}

// Взима пари само ако ги има. Връща true при успех.
stock bool:Money_Take(playerid, amount)
{
    if (amount < 0 || PlayerData[playerid][pMoney] < amount) return false;
    Money_Set(playerid, PlayerData[playerid][pMoney] - amount);
    return true;
}

stock Money_Sync(playerid)
{
    if (GetPlayerMoney(playerid) != PlayerData[playerid][pMoney]) {
        ResetPlayerMoney(playerid);
        GivePlayerMoney(playerid, PlayerData[playerid][pMoney]);
    }
}

stock Money_Save(playerid)
{
    if (!PlayerData[playerid][pLogged] || PlayerData[playerid][pID] <= 0) return;
    new q[96];
    format(q, sizeof (q), "UPDATE accounts SET money = %d WHERE id = %d", PlayerData[playerid][pMoney], PlayerData[playerid][pID]);
    Database_Exec(q);
}
