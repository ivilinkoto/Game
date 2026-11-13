// Пари на сървъра.
//
// Истинската сума е в PlayerData[pMoney]. Клиентът само я показва.
// Ако някой си „добави“ пари с чийт, сървърът връща истинската сума.
// ВИНАГИ ползвай Money_Give / Money_Set, никога GivePlayerMoney директно.

stock Money_Get(playerid)
{
    return PlayerData[playerid][pMoney];
}

stock Money_Give(playerid, amount)
{
    PlayerData[playerid][pMoney] += amount;
    Money_Sync(playerid);
}

stock Money_Set(playerid, amount)
{
    PlayerData[playerid][pMoney] = amount;
    Money_Sync(playerid);
}

stock Money_Sync(playerid)
{
    ResetPlayerMoney(playerid);
    GivePlayerMoney(playerid, PlayerData[playerid][pMoney]);
}

// Всяка секунда: връща истинската сума, ако клиентът показва друга.
stock Money_OnTick()
{
    for (new i = 0; i < MAX_PLAYERS; i++) {
        if (!IsPlayerConnected(i) || !PlayerData[i][pLogged]) continue;
        if (GetPlayerMoney(i) != PlayerData[i][pMoney]) {
            Money_Sync(i);
        }
    }
}
