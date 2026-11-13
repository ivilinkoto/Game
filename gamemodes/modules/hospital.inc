// Смърт и болница.

stock Hospital_OnPlayerDeath(playerid)
{
    if (!PlayerData[playerid][pLogged]) return;
    PlayerData[playerid][pDied] = true;
    SetSpawnInfo(playerid, NO_TEAM, PlayerData[playerid][pSkin], HOSPITAL_X, HOSPITAL_Y, HOSPITAL_Z, HOSPITAL_A);
}

// Извиква се от OnPlayerSpawn. Връща true, ако играчът идва от смърт.
stock bool:Hospital_OnPlayerSpawn(playerid)
{
    if (!PlayerData[playerid][pDied]) return false;
    PlayerData[playerid][pDied] = false;

    SetPlayerInterior(playerid, 0);
    SetPlayerVirtualWorld(playerid, 0);
    SetPlayerHealth(playerid, 100.0);

    new fee = HOSPITAL_FEE;
    if (fee > PlayerData[playerid][pMoney]) fee = PlayerData[playerid][pMoney];
    if (fee > 0) Money_Give(playerid, -fee);

    SendClientMessage(playerid, COLOR_WHITE, "Събуждаш се в болница {FFD54A}All Saints{FFFFFF}. Сметката е {FF4040}$%d{FFFFFF}.", fee);
    return true;
}
