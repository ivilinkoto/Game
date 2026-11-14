// Болница All Saints: спаун след смърт (без такса) и вход/изход през главната врата.
// Интериорът е в maps/hospital.inc.

static gHospitalPickupOut = -1,   // отвън -> влизаш
       gHospitalPickupIn = -1;    // отвътре -> излизаш

stock Hospital_Init()
{
    Map_Hospital_Create();
    gHospitalPickupOut = CreatePickup(1318, 1, HOSPITAL_DOOR_OUT_X, HOSPITAL_DOOR_OUT_Y, HOSPITAL_DOOR_OUT_Z, HOSPITAL_WORLD);
    gHospitalPickupIn = CreatePickup(1318, 1, HOSPITAL_DOOR_IN_X, HOSPITAL_DOOR_IN_Y, HOSPITAL_DOOR_IN_Z, HOSPITAL_WORLD);
    Create3DTextLabel("{FF6A00}Болница All Saints\n{FFFFFF}Влез", COLOR_WHITE,
        HOSPITAL_DOOR_OUT_X, HOSPITAL_DOOR_OUT_Y, HOSPITAL_DOOR_OUT_Z + 0.7, 15.0, HOSPITAL_WORLD, true);
    Create3DTextLabel("{FFFFFF}Изход", COLOR_WHITE,
        HOSPITAL_DOOR_IN_X, HOSPITAL_DOOR_IN_Y, HOSPITAL_DOOR_IN_Z + 0.7, 10.0, HOSPITAL_WORLD, true);
}

// Връща true, ако пикапът е на болницата.
stock bool:Hospital_OnPickUp(playerid, pickupid)
{
    if (pickupid != gHospitalPickupOut && pickupid != gHospitalPickupIn) return false;
    if (!PlayerData[playerid][pLogged] || IsPlayerInAnyVehicle(playerid)) return true;

    // Пикапът се задейства постоянно, докато стоиш на него – пазим се от повторение.
    new now = GetTickCount();
    if (now - PlayerData[playerid][pLastTeleportTick] < 3000) return true;
    PlayerData[playerid][pLastTeleportTick] = now;

    // Входът е на изток: навътре = към по-малко X, навън = към по-голямо X.
    if (pickupid == gHospitalPickupOut) {
        SetPlayerPos(playerid, HOSPITAL_DOOR_IN_X - HOSPITAL_DOOR_STEP, HOSPITAL_DOOR_IN_Y, HOSPITAL_DOOR_IN_Z);
        SetPlayerFacingAngle(playerid, 90.0);
    } else {
        SetPlayerPos(playerid, HOSPITAL_DOOR_OUT_X + HOSPITAL_DOOR_STEP, HOSPITAL_DOOR_OUT_Y, HOSPITAL_DOOR_OUT_Z);
        SetPlayerFacingAngle(playerid, 270.0);
    }
    SetPlayerInterior(playerid, HOSPITAL_INTERIOR);
    SetPlayerVirtualWorld(playerid, HOSPITAL_WORLD);
    SetCameraBehindPlayer(playerid);
    return true;
}

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
    SetPlayerVirtualWorld(playerid, HOSPITAL_WORLD);
    SetPlayerHealth(playerid, 100.0);

    SendClientMessage(playerid, COLOR_WHITE, "Събуждаш се в болница {FFD54A}All Saints{FFFFFF}. Лекарите те излекуваха.");
    SendClientMessage(playerid, COLOR_GREY, "Изходът е на партера, при стъклените врати.");
    return true;
}
