// Камионите на играчите.
//
// Всеки играч има GARAGE_SLOTS места. Докато е онлайн, камионите му съществуват:
// паркираните - в неговия гараж (неговия виртуален свят), а този, с който кара - навън.
// При изход от сървъра всички се записват и махат; при следващ вход са отново в гаража.

#define GARAGE_SLOTS        2
#define STARTER_MODEL       440         // Rumpo - бус
#define GARAGE_VW_BASE      10000       // свят на гаража = база + ID на акаунта

enum E_PLAYER_VEH {
    pvID,                   // ID в базата (0 = празно място)
    pvModel,
    pvColor1,
    pvColor2,
    Float:pvHealth,
    Float:pvFuel,           // литри
    pvVehicle,              // ID в играта (INVALID_VEHICLE_ID = не е създаден)
    bool:pvOut,             // навън ли е (извън гаража)

    // Тунинг (виж modules/tuning.inc)
    pvUpEngine,             // икономичен двигател 0-3
    pvUpTank,               // голям резервоар 0-3
    pvUpArmor,              // здрава кабина 0-3
    pvNeon,                 // неон: 0 = без, иначе модел на обекта
    pvNeonL,                // обектите на неона (динамични)
    pvNeonR
}

enum {
    UPGRADE_ENGINE,
    UPGRADE_TANK,
    UPGRADE_ARMOR,
    MAX_UPGRADES
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
    new i = Catalog_Find(model);
    if (i != -1) format(dest, size, "%s", VehCatalog[i][vcName]);
    else format(dest, size, "Превозно средство (%d)", model);
}

// Еднократни промени по базата
stock Veh_Init()
{
    // Стартовото превозно средство стана бус (Rumpo) вместо Yankee.
    if (!Database_GetMeta("starter_van")) {
        new q[128];
        format(q, sizeof (q), "UPDATE player_vehicles SET model = %d WHERE model = 456", STARTER_MODEL);
        Database_Exec(q);
        Database_SetMeta("starter_van", 1);
        print("[VEH] Starter Yankees replaced with Rumpo vans.");
    }
}

// ---------- Тунинг: данни ----------

// Резервоар с подобрението "голям резервоар" (+20% на ниво)
stock Veh_TankSize(playerid, slot)
{
    return Catalog_GetTank(PlayerVeh[playerid][slot][pvModel]) * (100 + 20 * PlayerVeh[playerid][slot][pvUpTank]) / 100;
}

// Резервоар по ID на превозното средство (каталога, ако не е на играч)
stock Veh_GetTank(vehicleid)
{
    new owner = Veh_GetOwner(vehicleid);
    if (owner == INVALID_PLAYER_ID) return Catalog_GetTank(GetVehicleModel(vehicleid));
    return Veh_TankSize(owner, VehSlot[vehicleid]);
}

stock Veh_GetUpgrade(vehicleid, type)
{
    new owner = Veh_GetOwner(vehicleid);
    if (owner == INVALID_PLAYER_ID) return 0;
    new slot = VehSlot[vehicleid];
    switch (type) {
        case UPGRADE_ENGINE: return PlayerVeh[owner][slot][pvUpEngine];
        case UPGRADE_TANK:   return PlayerVeh[owner][slot][pvUpTank];
        case UPGRADE_ARMOR:  return PlayerVeh[owner][slot][pvUpArmor];
    }
    return 0;
}

stock Veh_SetUpgrade(vehicleid, type, level)
{
    new owner = Veh_GetOwner(vehicleid);
    if (owner == INVALID_PLAYER_ID) return;
    new slot = VehSlot[vehicleid];
    switch (type) {
        case UPGRADE_ENGINE: PlayerVeh[owner][slot][pvUpEngine] = level;
        case UPGRADE_TANK:   PlayerVeh[owner][slot][pvUpTank] = level;
        case UPGRADE_ARMOR:  PlayerVeh[owner][slot][pvUpArmor] = level;
    }
    Veh_Save(owner, slot);
}

stock Veh_GetColor(vehicleid, which)
{
    new owner = Veh_GetOwner(vehicleid);
    if (owner == INVALID_PLAYER_ID) return 0;
    return (which == 0) ? PlayerVeh[owner][VehSlot[vehicleid]][pvColor1] : PlayerVeh[owner][VehSlot[vehicleid]][pvColor2];
}

stock Veh_SetColor(vehicleid, which, color)
{
    new owner = Veh_GetOwner(vehicleid);
    if (owner == INVALID_PLAYER_ID) return;
    new slot = VehSlot[vehicleid];
    if (which == 0) PlayerVeh[owner][slot][pvColor1] = color;
    else PlayerVeh[owner][slot][pvColor2] = color;
    ChangeVehicleColours(vehicleid, PlayerVeh[owner][slot][pvColor1], PlayerVeh[owner][slot][pvColor2]);
    Veh_Save(owner, slot);
}

stock Veh_GetNeon(vehicleid)
{
    new owner = Veh_GetOwner(vehicleid);
    if (owner == INVALID_PLAYER_ID) return 0;
    return PlayerVeh[owner][VehSlot[vehicleid]][pvNeon];
}

stock Veh_SetNeon(vehicleid, neonModel)
{
    new owner = Veh_GetOwner(vehicleid);
    if (owner == INVALID_PLAYER_ID) return;
    new slot = VehSlot[vehicleid];
    Veh_RemoveNeon(owner, slot);
    PlayerVeh[owner][slot][pvNeon] = neonModel;
    Veh_ApplyNeon(owner, slot);
    Veh_Save(owner, slot);
}

static stock Veh_RemoveNeon(playerid, slot)
{
    if (PlayerVeh[playerid][slot][pvNeonL] != -1) DestroyDynamicObject(PlayerVeh[playerid][slot][pvNeonL]);
    if (PlayerVeh[playerid][slot][pvNeonR] != -1) DestroyDynamicObject(PlayerVeh[playerid][slot][pvNeonR]);
    PlayerVeh[playerid][slot][pvNeonL] = -1;
    PlayerVeh[playerid][slot][pvNeonR] = -1;
}

// Неонът – две тръби под шасито, отляво и отдясно (според размера на модела)
static stock Veh_ApplyNeon(playerid, slot)
{
    new vehicleid = PlayerVeh[playerid][slot][pvVehicle], model = PlayerVeh[playerid][slot][pvNeon];
    if (vehicleid == INVALID_VEHICLE_ID || model == 0) return;
    new Float:sx, Float:sy, Float:sz;
    GetVehicleModelInfo(PlayerVeh[playerid][slot][pvModel], VEHICLE_MODEL_INFO_SIZE, sx, sy, sz);
    new Float:ox = sx / 2.0 - 0.25, Float:oz = -sz / 2.0 + 0.15;
    new l = CreateDynamicObject(model, 0.0, 0.0, -1000.0, 0.0, 0.0, 0.0, -1, -1, -1, 200.0, 200.0);
    new r = CreateDynamicObject(model, 0.0, 0.0, -1000.0, 0.0, 0.0, 0.0, -1, -1, -1, 200.0, 200.0);
    AttachDynamicObjectToVehicle(l, vehicleid, -ox, 0.0, oz, 0.0, 0.0, 0.0);
    AttachDynamicObjectToVehicle(r, vehicleid,  ox, 0.0, oz, 0.0, 0.0, 0.0);
    PlayerVeh[playerid][slot][pvNeonL] = l;
    PlayerVeh[playerid][slot][pvNeonR] = r;
}

// Гориво по ID на превозното средство (-1.0 = не е на играч, няма ограничение)
stock Float:Veh_GetFuel(vehicleid)
{
    new owner = Veh_GetOwner(vehicleid);
    if (owner == INVALID_PLAYER_ID) return -1.0;
    return PlayerVeh[owner][VehSlot[vehicleid]][pvFuel];
}

stock Veh_SetFuel(vehicleid, Float:fuel)
{
    new owner = Veh_GetOwner(vehicleid);
    if (owner == INVALID_PLAYER_ID) return;
    new slot = VehSlot[vehicleid];
    new Float:tank = float(Veh_TankSize(owner, slot));
    if (fuel < 0.0) fuel = 0.0;
    if (fuel > tank) fuel = tank;
    PlayerVeh[owner][slot][pvFuel] = fuel;
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
        PlayerVeh[playerid][s][pvUpEngine] = 0;
        PlayerVeh[playerid][s][pvUpTank] = 0;
        PlayerVeh[playerid][s][pvUpArmor] = 0;
        PlayerVeh[playerid][s][pvNeon] = 0;
        PlayerVeh[playerid][s][pvNeonL] = -1;
        PlayerVeh[playerid][s][pvNeonR] = -1;
        PlayerVeh[playerid][s][pvID] = 0;
        PlayerVeh[playerid][s][pvModel] = 0;
        PlayerVeh[playerid][s][pvVehicle] = INVALID_VEHICLE_ID;
        PlayerVeh[playerid][s][pvOut] = false;
        PlayerVeh[playerid][s][pvHealth] = 1000.0;
        PlayerVeh[playerid][s][pvFuel] = 0.0;
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
            PlayerVeh[playerid][slot][pvUpEngine] = DB_GetFieldIntByName(r, "up_engine");
            PlayerVeh[playerid][slot][pvUpTank]   = DB_GetFieldIntByName(r, "up_tank");
            PlayerVeh[playerid][slot][pvUpArmor]  = DB_GetFieldIntByName(r, "up_armor");
            PlayerVeh[playerid][slot][pvNeon]     = DB_GetFieldIntByName(r, "neon");
            new Float:fuel = DB_GetFieldFloatByName(r, "fuel");
            new tank = Veh_TankSize(playerid, slot);
            if (fuel < 0.0 || fuel > float(tank)) fuel = float(tank);   // -1 = пълен (стари записи)
            PlayerVeh[playerid][slot][pvFuel] = fuel;
        }
        DB_SelectNextRow(r);
    }
    DB_FreeResultSet(r);
}

// Дава безплатния бус (веднъж на акаунт).
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
    SendClientMessage(playerid, COLOR_BRAND, "[ГАРАЖ] " C_WHITE "Получаваш безплатен бус " C_BRAND "Rumpo" C_WHITE " (1.5 т, пратки) - чака те в гаража.");
}

stock Veh_FindFreeSlot(playerid)
{
    for (new s = 0; s < GARAGE_SLOTS; s++) {
        if (PlayerVeh[playerid][s][pvID] == 0) return s;
    }
    return -1;
}

stock Veh_Count(playerid)
{
    new n = 0;
    for (new s = 0; s < GARAGE_SLOTS; s++) {
        if (PlayerVeh[playerid][s][pvID] != 0) n++;
    }
    return n;
}

// Ново превозно средство в първото свободно място (покупка). Връща мястото или -1.
stock Veh_Add(playerid, model, color1, color2)
{
    new slot = Veh_FindFreeSlot(playerid);
    if (slot == -1) return -1;

    new q[256];
    format(q, sizeof (q), "INSERT INTO player_vehicles (owner_id, slot, model, color1, color2, health, created_at) VALUES (%d, %d, %d, %d, %d, 1000, %d)",
        PlayerData[playerid][pID], slot, model, color1, color2, gettime());
    Database_Exec(q);

    new id = 0;
    new DBResult:r = Database_Query("SELECT last_insert_rowid() AS id");
    if (r != DBResult:0) {
        if (DB_GetRowCount(r) > 0) id = DB_GetFieldIntByName(r, "id");
        DB_FreeResultSet(r);
    }
    if (id <= 0) return -1;

    PlayerVeh[playerid][slot][pvID] = id;
    PlayerVeh[playerid][slot][pvModel] = model;
    PlayerVeh[playerid][slot][pvColor1] = color1;
    PlayerVeh[playerid][slot][pvColor2] = color2;
    PlayerVeh[playerid][slot][pvHealth] = 1000.0;
    PlayerVeh[playerid][slot][pvUpEngine] = 0;
    PlayerVeh[playerid][slot][pvUpTank] = 0;
    PlayerVeh[playerid][slot][pvUpArmor] = 0;
    PlayerVeh[playerid][slot][pvNeon] = 0;
    PlayerVeh[playerid][slot][pvNeonL] = -1;
    PlayerVeh[playerid][slot][pvNeonR] = -1;
    PlayerVeh[playerid][slot][pvFuel] = float(Catalog_GetTank(model));
    PlayerVeh[playerid][slot][pvVehicle] = INVALID_VEHICLE_ID;
    PlayerVeh[playerid][slot][pvOut] = false;
    Veh_ParkInGarage(playerid, slot);
    return slot;
}

// Маха превозното средство (продажба). Не става, ако някой е вътре.
stock bool:Veh_Remove(playerid, slot)
{
    if (slot < 0 || slot >= GARAGE_SLOTS || PlayerVeh[playerid][slot][pvID] == 0) return false;
    new vehicleid = PlayerVeh[playerid][slot][pvVehicle];
    if (vehicleid != INVALID_VEHICLE_ID) {
        for (new i = 0; i < MAX_PLAYERS; i++) {
            if (IsPlayerConnected(i) && GetPlayerVehicleID(i) == vehicleid) return false;
        }
        VehOwner[vehicleid] = INVALID_PLAYER_ID;
        Veh_RemoveNeon(playerid, slot);
        DestroyVehicle(vehicleid);
    }
    new q[96];
    format(q, sizeof (q), "DELETE FROM player_vehicles WHERE id = %d", PlayerVeh[playerid][slot][pvID]);
    Database_Exec(q);
    PlayerVeh[playerid][slot][pvID] = 0;
    PlayerVeh[playerid][slot][pvModel] = 0;
    PlayerVeh[playerid][slot][pvVehicle] = INVALID_VEHICLE_ID;
    PlayerVeh[playerid][slot][pvOut] = false;
    return true;
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
    new q[320];
    format(q, sizeof (q), "UPDATE player_vehicles SET health = %f, fuel = %f, color1 = %d, color2 = %d, " \
        "up_engine = %d, up_tank = %d, up_armor = %d, neon = %d WHERE id = %d",
        PlayerVeh[playerid][slot][pvHealth], PlayerVeh[playerid][slot][pvFuel],
        PlayerVeh[playerid][slot][pvColor1], PlayerVeh[playerid][slot][pvColor2],
        PlayerVeh[playerid][slot][pvUpEngine], PlayerVeh[playerid][slot][pvUpTank],
        PlayerVeh[playerid][slot][pvUpArmor], PlayerVeh[playerid][slot][pvNeon],
        PlayerVeh[playerid][slot][pvID]);
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
    Veh_RemoveNeon(playerid, slot);
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
        Tune_ResetArmor(vehicleid);
        Veh_ApplyNeon(playerid, slot);
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
