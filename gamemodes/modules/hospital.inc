// Смърт и болница – без такса.
//
// В GTA: San Andreas няма вграден болничен интериор. Засега героят се появява
// пред болница All Saints. Когато имаме карта на болничен интериор (mapping),
// сменяме HOSPITAL_X/Y/Z/A и HOSPITAL_INTERIOR в core/config.inc и
// виртуалния свят по-долу – останалото е готово.

#define HOSPITAL_VIRTUAL_WORLD  0

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

    SetPlayerInterior(playerid, HOSPITAL_INTERIOR);
    SetPlayerVirtualWorld(playerid, HOSPITAL_VIRTUAL_WORLD);
    SetPlayerHealth(playerid, 100.0);

    SendClientMessage(playerid, COLOR_WHITE, "Събуждаш се в болница {FFD54A}All Saints{FFFFFF}. Лекарите те излекуваха.");
    return true;
}
