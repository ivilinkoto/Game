// Камионите на играчите.
//
// Всеки играч има GARAGE_SLOTS места. Докато е онлайн, камионите му съществуват:
// паркираните – в неговия гараж (неговия виртуален свят), а този, с който кара – навън.
// При изход от сървъра всички се записват и махат; при следващ вход са отново в гаража.

#define GARAGE_SLOTS        2
#define STARTER_MODEL       456         // Yankee
#define GARAGE_VW_BASE      10000       // свят на гаража = база + ID на акаунта

enum E_PLAYER_VEH {
    pvID,                   // ID в базата (0 = празно място)
    pvModel,
    pvColor1,
    pvColor2,
    Float:pvHealth,
    pvVehicle,              // ID в играта (INVALID_VEHICLE_ID = не е създаден)
    bool:pvOut              // навън ли е (извън гаража)
}

static PlayerVeh[MAX_PLAYERS][GARAGE_SLOTS][E_PLAYER_VEH];

// Обратна връзка: превозно средство -> собственик и място
static VehOwner[MAX_VEHICLES] = {INVALID_PLAYER_ID, ...};
static VehSlot[MAX_VEHICLES];

stock Garage_GetWorld(playerid)
{
    return GARAGE_VW_BASE + PlayerData[playerid][pID];
}

stock Veh_GetModelName(model, dest[], size = sizeof (dest))
{
    switch (model) {
        case 456: format(dest, size, "Yankee");
        case 499: format(dest, size, "Benson");
        case 414: format(dest, size, "Mule");
        case 403: format(dest, size, "Linerunner");
        case 514: format(dest, size, "Tanker");
        case 515: format(dest, size, "Roadtrain");
        case 455: format(dest, size, "Flatbed");
        case 578: format(dest, size, "DFT-30");
        default:  format(dest, size, "Камион (%d)", model);
    }
}

stock Veh_GetOwner(vehicleid)
{
    if (vehicleid <= 0 || vehicleid >= MAX_VEHICLES) return INVALID_PLAYER_ID;
    return VehOwner[vehicleid];
}

stock Veh_GetSlot(vehicleid)
{
    if (Veh_GetOwner(vehicleid) == INVALID_PLAYER_ID) return -1;
    return VehSlot[vehicleid];
}

stock Veh_HasInSlot(playerid, slot)
{
    return (slot >= 0 && slot < GARAGE_SLOTS && PlayerVeh[playerid][slot][pvID] != 0);
}

stock Veh_GetVehicle(playerid, slot)
{
    return PlayerVeh[playerid][slot][pvVehicle];
}

stock bool:Veh_IsOut(playerid, slot)
{
    return PlayerVeh[playerid][slot][pvOut];
}

stock Veh_GetModel(playerid, slot)
{
    return PlayerVeh[playerid][slot][pvModel];
}

stock Veh_Reset(playerid)
{
    for (new s = 0; s < GARAGE_SLOTS; s++) {
        PlayerVeh[playerid][s][pvID] = 0;
        PlayerVeh[playerid][s][pvModel] = 0;
        PlayerVeh[playerid][s][pvVehicle] = INVALID_VEHICLE_ID;
        PlayerVeh[playerid][s][pvOut] = false;
        PlayerVeh[playerid][s][pvHealth] = 1000.0;
    }
}

// Зарежда камионите от базата (при вход).
stock Veh_Load(playerid)
{
    Veh_Reset(playerid);
    new q[128];
    format(q, sizeof (q), "SELECT * FROM player_vehicles WHERE owner_id = %d ORDER BY slot", PlayerData[playerid][pID]);
    new DBResult:r = Database_Query(q);
    if (r == DBResult:0) return;
    new rows = DB_GetRowCount(r);
    for (new i = 0; i < rows; i++) {
        new slot = DB_GetFieldIntByName(r, "slot");
        if (slot >= 0 && slot < GARAGE_SLOTS && PlayerVeh[playerid][slot][pvID] == 0) {
            PlayerVeh[playerid][slot][pvID]     = DB_GetFieldIntByName(r, "id");
            PlayerVeh[playerid][slot][pvModel]  = DB_GetFieldIntByName(r, "model");
            PlayerVeh[playerid][slot][pvColor1] = DB_GetFieldIntByName(r, "color1");
            PlayerVeh[playerid][slot][pvColor2] = DB_GetFieldIntByName(r, "color2");
            new Float:hp = DB_GetFieldFloatByName(r, "health");
            if (hp < 300.0) hp = 300.0;     // изгорял камион се връща поправен до минимум
            PlayerVeh[playerid][slot][pvHealth] = hp;
        }
        DB_SelectNextRow(r);
    }
    DB_FreeResultSet(r);
}

// Дава безплатния Yankee (веднъж на акаунт).
stock Veh_GiveStarter(playerid)
{
    if (PlayerData[playerid][pStarterGiven]) return;

    new slot = -1;
    for (new s = 0; s < GARAGE_SLOTS; s++) {
        if (PlayerVeh[playerid][s][pvID] == 0) {
            slot = s;
            break;
        }
    }
    if (slot == -1) return;

    new color1 = random(126), color2 = random(126);
    new q[256];
    format(q, sizeof (q), "INSERT INTO player_vehicles (owner_id, slot, model, color1, color2, health, created_at) VALUES (%d, %d, %d, %d, %d, 1000, %d)",
        PlayerData[playerid][pID], slot, STARTER_MODEL, color1, color2, gettime());
    Database_Exec(q);
    format(q, sizeof (q), "UPDATE accounts SET starter_given = 1 WHERE id = %d", PlayerData[playerid][pID]);
    Database_Exec(q);
    PlayerData[playerid][pStarterGiven] = true;

    Veh_Load(playerid);
    SendClientMessage(playerid, COLOR_BRAND, "[ГАРАЖ] " C_WHITE "Получаваш безплатен " C_BRAND "Yankee" C_WHITE " – чака те в гаража.");
}

static stock Veh_Save(playerid, slot)
{
    if (PlayerVeh[playerid][slot][pvID] == 0) return;
    new vehicleid = PlayerVeh[playerid][slot][pvVehicle];
    if (vehicleid != INVALID_VEHICLE_ID) {
        new Float:hp;
        GetVehicleHealth(vehicleid, hp);
        PlayerVeh[playerid][slot][pvHealth] = hp;
    }
    new q[128];
    format(q, sizeof (q), "UPDATE player_vehicles SET health = %f WHERE id = %d", PlayerVeh[playerid][slot][pvHealth], PlayerVeh[playerid][slot][pvID]);
    Database_Exec(q);
}

stock Veh_SaveAll(playerid)
{
    for (new s = 0; s < GARAGE_SLOTS; s++) Veh_Save(playerid, s);
}

static stock Veh_Destroy(playerid, slot)
{
    new vehicleid = PlayerVeh[playerid][slot][pvVehicle];
    if (vehicleid == INVALID_VEHICLE_ID) return;
    VehOwner[vehicleid] = INVALID_PLAYER_ID;
    DestroyVehicle(vehicleid);
    PlayerVeh[playerid][slot][pvVehicle] = INVALID_VEHICLE_ID;
}

stock Veh_DestroyAll(playerid)
{
    for (new s = 0; s < GARAGE_SLOTS; s++) Veh_Destroy(playerid, s);
}

// Създава/премества камиона на мястото му в гаража.
stock Veh_ParkInGarage(playerid, slot)
{
    if (PlayerVeh[playerid][slot][pvID] == 0) return INVALID_VEHICLE_ID;

    new Float:x = GARAGE_X + GarageSlotPos[slot][0];
    new Float:y = GARAGE_Y + GarageSlotPos[slot][1];
    new Float:z = GARAGE_Z + 1.5;
    new Float:a = GarageSlotPos[slot][2];
    new world = Garage_GetWorld(playerid);

    new vehicleid = PlayerVeh[playerid][slot][pvVehicle];
    if (vehicleid == INVALID_VEHICLE_ID) {
        vehicleid = CreateVehicle(PlayerVeh[playerid][slot][pvModel], x, y, z, a,
            PlayerVeh[playerid][slot][pvColor1], PlayerVeh[playerid][slot][pvColor2], -1);
        if (vehicleid == INVALID_VEHICLE_ID) return INVALID_VEHICLE_ID;
        PlayerVeh[playerid][slot][pvVehicle] = vehicleid;
        VehOwner[vehicleid] = playerid;
        VehSlot[vehicleid] = slot;
        SetVehicleHealth(vehicleid, PlayerVeh[playerid][slot][pvHealth]);
    } else {
        SetVehiclePos(vehicleid, x, y, z);
        SetVehicleZAngle(vehicleid, a);
    }
    SetVehicleVirtualWorld(vehicleid, world);
    LinkVehicleToInterior(vehicleid, 0);
    PlayerVeh[playerid][slot][pvOut] = false;
    return vehicleid;
}

// Всички камиони, които не са навън, отиват на местата си в гаража.
stock Veh_ParkAllInGarage(playerid)
{
    for (new s = 0; s < GARAGE_SLOTS; s++) {
        if (PlayerVeh[playerid][s][pvID] == 0 || PlayerVeh[playerid][s][pvOut]) continue;
        Veh_ParkInGarage(playerid, s);
    }
}

stock Veh_SetOut(playerid, slot, bool:out)
{
    PlayerVeh[playerid][slot][pvOut] = out;
}

// Някой в камиона (освен собственика)?
static stock bool:Veh_IsOccupiedByOthers(vehicleid, ownerid)
{
    for (new i = 0; i < MAX_PLAYERS; i++) {
        if (i != ownerid && IsPlayerConnected(i) && GetPlayerVehicleID(i) == vehicleid) return true;
    }
    return false;
}

// Връща изоставен камион обратно в гаража (Меню > Моят гараж).
stock bool:Veh_ReturnToGarage(playerid, slot)
{
    if (PlayerVeh[playerid][slot][pvID] == 0) return false;
    new vehicleid = PlayerVeh[playerid][slot][pvVehicle];
    if (vehicleid != INVALID_VEHICLE_ID) {
        if (GetPlayerVehicleID(playerid) == vehicleid) return false;
        if (Veh_IsOccupiedByOthers(vehicleid, playerid)) return false;
    }
    Veh_ParkInGarage(playerid, slot);
    return true;
}

// ---------- Събития ----------

stock Veh_OnPlayerConnect(playerid)
{
    Veh_Reset(playerid);
}

stock Veh_OnPlayerDisconnect(playerid)
{
    if (!PlayerData[playerid][pLogged]) return;
    Veh_SaveAll(playerid);
    Veh_DestroyAll(playerid);
}

// Чужд камион не може да се кара.
stock Veh_OnPlayerStateChange(playerid, PLAYER_STATE:newstate)
{
    if (newstate != PLAYER_STATE_DRIVER) return;
    new vehicleid = GetPlayerVehicleID(playerid);
    new owner = Veh_GetOwner(vehicleid);
    if (owner != INVALID_PLAYER_ID && owner != playerid) {
        RemovePlayerFromVehicle(playerid);
        SendClientMessage(playerid, COLOR_RED, "[!] " C_WHITE "Този камион е на %s.", PlayerData[owner][pName]);
    }
}

// Изгорял камион: при повторното му появяване го връщаме в гаража на собственика.
stock Veh_OnVehicleSpawn(vehicleid)
{
    new owner = Veh_GetOwner(vehicleid);
    if (owner == INVALID_PLAYER_ID) return 0;
    new slot = VehSlot[vehicleid];
    PlayerVeh[owner][slot][pvHealth] = 650.0;
    SetVehicleHealth(vehicleid, 650.0);
    Veh_ParkInGarage(owner, slot);
    Veh_Save(owner, slot);
    SendClientMessage(owner, COLOR_BRAND, "[ГАРАЖ] " C_WHITE "Камионът ти беше разбит и е откаран обратно в гаража.");
    return 1;
}
