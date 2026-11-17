// Гориво: разход при каране, бензиностанции, зареждане, пътна помощ.
//
// Разходът е според модела (каталога) и товара: празен -> база, пълен товар -> +50%.
// Празен резервоар -> двигателят спира и не пали.
// Зареждане: Y с превозното средство в зоната на бензиностанция (двигателят трябва да е изгасен).
// Пътна помощ: Меню > Гориво > Пътна помощ (+30 л за $400), когато резервоарът е почти празен.
//
// Бензиностанциите са в базата; админ: /addfuel (тук), /delfuel (най-близката).

#define FUEL_PRICE          3           // $ за литър
#define FUEL_RANGE          9.0
#define FUEL_IDLE_PER_SEC   0.01        // литри в секунда на място с работещ двигател
#define ROADSIDE_LITERS     30
#define ROADSIDE_PRICE      400
#define MAX_FUEL_STATIONS   40

static Float:FuelStation[MAX_FUEL_STATIONS][3];
static FuelStationID[MAX_FUEL_STATIONS];
static Text3D:FuelLabel[MAX_FUEL_STATIONS] = {Text3D:-1, ...};
static FuelIcon[MAX_FUEL_STATIONS] = {-1, ...};
static FuelStationCount;

static Float:FuelLastPos[MAX_PLAYERS][3];
static bool:FuelHasLastPos[MAX_PLAYERS];
static bool:FuelWarned[MAX_PLAYERS];

// Известни бензиностанции в San Andreas (влизат само ако таблицата е празна)
static const Float:FuelDefaults[][3] = {
    {1941.6, -1772.9, 13.6},    // LS Idlewood
    {1004.0, -940.0, 42.2},     // LS Temple
    {-91.0, -1169.0, 2.4},      // Flint County
    {655.0, -565.0, 16.3},      // Dillimore
    {1381.0, 459.0, 19.9},      // Montgomery
    {-2410.8, 975.0, 45.4},     // SF Juniper Hollow
    {-1675.9, 413.0, 7.2},      // SF Easter Basin
    {-2244.0, -2560.0, 32.0},   // Angel Pine
    {-1328.0, 2677.0, 50.0},    // Tierra Robada
    {70.0, 1218.0, 18.8},       // Fort Carson
    {611.0, 1694.0, 7.0},       // Bone County
    {2115.0, 920.0, 10.8},      // LV Come-a-Lot
    {2640.0, 1106.0, 10.8},     // LV Strip
    {2202.0, 2474.0, 10.8}      // LV Spinybed
};

// ---------- Бензиностанции ----------

stock Fuel_Init()
{
    Database_Exec("CREATE TABLE IF NOT EXISTS fuel_stations (id INTEGER PRIMARY KEY AUTOINCREMENT, x REAL NOT NULL, y REAL NOT NULL, z REAL NOT NULL)");
    if (!Database_GetMeta("fuel_defaults")) {
        new q[160];
        for (new i = 0; i < sizeof (FuelDefaults); i++) {
            format(q, sizeof (q), "INSERT INTO fuel_stations (x, y, z) VALUES (%f, %f, %f)", FuelDefaults[i][0], FuelDefaults[i][1], FuelDefaults[i][2]);
            Database_Exec(q);
        }
        Database_SetMeta("fuel_defaults", 1);
    }
    Fuel_LoadStations();
}

static stock Fuel_LoadStations()
{
    for (new i = 0; i < FuelStationCount; i++) {
        if (FuelLabel[i] != Text3D:-1) DestroyDynamic3DTextLabel(FuelLabel[i]);
        if (FuelIcon[i] != -1) DestroyDynamicMapIcon(FuelIcon[i]);
        FuelLabel[i] = Text3D:-1;
        FuelIcon[i] = -1;
    }
    FuelStationCount = 0;

    new DBResult:r = Database_Query("SELECT id, x, y, z FROM fuel_stations ORDER BY id");
    if (r == DBResult:0) return;
    new rows = DB_GetRowCount(r);
    for (new i = 0; i < rows && FuelStationCount < MAX_FUEL_STATIONS; i++) {
        new k = FuelStationCount++;
        FuelStationID[k] = DB_GetFieldIntByName(r, "id");
        FuelStation[k][0] = DB_GetFieldFloatByName(r, "x");
        FuelStation[k][1] = DB_GetFieldFloatByName(r, "y");
        FuelStation[k][2] = DB_GetFieldFloatByName(r, "z");
        new text[160];
        format(text, sizeof (text), C_BRAND "БЕНЗИНОСТАНЦИЯ\n" C_WHITE "$%d за литър\n" C_GREY "Y в превозното средство - зареждане", FUEL_PRICE);
        FuelLabel[k] = CreateDynamic3DTextLabel(text,
            COLOR_WHITE, FuelStation[k][0], FuelStation[k][1], FuelStation[k][2] + 1.0, 30.0, INVALID_PLAYER_ID, INVALID_VEHICLE_ID, 0, 0, 0);
        // Червено квадратче на радара – само наблизо, за да не претрупва картата
        FuelIcon[k] = CreateDynamicMapIcon(FuelStation[k][0], FuelStation[k][1], FuelStation[k][2], 0, 0xE04040FF, 0, 0, -1, MAP_ICON_RANGE, MAPICON_LOCAL);
        DB_SelectNextRow(r);
    }
    DB_FreeResultSet(r);
}

// Достъп за други модули (GPS)
stock Fuel_Total()
{
    return FuelStationCount;
}

stock Fuel_GetPos(i, &Float:x, &Float:y, &Float:z)
{
    x = FuelStation[i][0];
    y = FuelStation[i][1];
    z = FuelStation[i][2];
}

static stock Fuel_GetNearestStation(playerid, &Float:dist)
{
    new best = -1;
    dist = 999999.0;
    for (new i = 0; i < FuelStationCount; i++) {
        new Float:d = GetPlayerDistanceFromPoint(playerid, FuelStation[i][0], FuelStation[i][1], FuelStation[i][2]);
        if (d < dist) {
            dist = d;
            best = i;
        }
    }
    return best;
}

// ---------- Разход ----------

// Вика се всяка секунда от Server_Tick
stock Fuel_Tick(playerid)
{
    new vehicleid = GetPlayerVehicleID(playerid);
    if (vehicleid == 0 || GetPlayerState(playerid) != PLAYER_STATE_DRIVER) {
        FuelHasLastPos[playerid] = false;
        return;
    }
    new Float:x, Float:y, Float:z;
    GetVehiclePos(vehicleid, x, y, z);

    new Float:fuel = Veh_GetFuel(vehicleid);
    if (fuel < 0.0 || !Engine_IsOn(vehicleid)) {
        FuelLastPos[playerid][0] = x;
        FuelLastPos[playerid][1] = y;
        FuelLastPos[playerid][2] = z;
        FuelHasLastPos[playerid] = true;
        return;
    }

    new Float:moved = 0.0;
    if (FuelHasLastPos[playerid]) {
        moved = VectorSize(x - FuelLastPos[playerid][0], y - FuelLastPos[playerid][1], z - FuelLastPos[playerid][2]);
        if (moved > 80.0) moved = 0.0;      // телепорт (гараж, админ) – не се брои
    }
    FuelLastPos[playerid][0] = x;
    FuelLastPos[playerid][1] = y;
    FuelLastPos[playerid][2] = z;
    FuelHasLastPos[playerid] = true;

    new model = GetVehicleModel(vehicleid);
    new Float:perKm = float(Catalog_GetFuelUse(model)) / 10.0;
    // Товар: до +50% при пълна товароносимост
    new payload = Catalog_GetPayload(model), loadKg = Delivery_GetLoadKg(playerid);
    if (payload > 0 && loadKg > 0) perKm *= 1.0 + 0.5 * float(loadKg) / float(payload);

    new Float:used = perKm * moved / 1000.0 + FUEL_IDLE_PER_SEC;
    fuel -= used;
    if (fuel <= 0.0) {
        fuel = 0.0;
        Veh_SetFuel(vehicleid, 0.0);
        Engine_Set(vehicleid, false);
        GameTextForPlayer(playerid, "~r~OUT OF FUEL", 3000, 4);
        SendClientMessage(playerid, COLOR_RED, "[ГОРИВО] " C_WHITE "Резервоарът е празен! Пътна помощ: " C_BRAND "/roadside" C_WHITE " (+30 л за $400).");
        return;
    }
    Veh_SetFuel(vehicleid, fuel);

    new tank = Catalog_GetTank(model);
    if (tank > 0 && fuel < float(tank) * FUEL_LOW_PERCENT / 100.0) {
        if (!FuelWarned[playerid]) {
            FuelWarned[playerid] = true;
            SendClientMessage(playerid, COLOR_BRAND, "[ГОРИВО] " C_WHITE "Горивото свършва! Най-близката бензиностанция: " C_BRAND "/gps" C_WHITE " > Бензиностанции.");
        }
    } else {
        FuelWarned[playerid] = false;
    }
}

// ---------- Зареждане ----------

static stock Fuel_ShowRefuel(playerid, vehicleid)
{
    new model = GetVehicleModel(vehicleid), tank = Catalog_GetTank(model);
    new Float:fuel = Veh_GetFuel(vehicleid);
    new need = floatround(float(tank) - fuel, floatround_floor);
    if (need <= 0) {
        SendInfo(playerid, "Резервоарът е пълен.");
        return;
    }
    new price[16];
    FormatMoney(need * FUEL_PRICE, price);
    ShowPlayerDialog(playerid, DIALOG_FUEL_REFUEL, DIALOG_STYLE_MSGBOX, C_BRAND "Бензиностанция",
        C_WHITE "В резервоара: " C_BRAND "%d / %d л\n\n" \
        C_WHITE "Зареди догоре: " C_BRAND "%d л" C_WHITE " за " C_GREEN "$%s\n" \
        C_GREY "Ако нямаш достатъчно пари, ще се зареди колкото стигат.",
        "Зареди", "Отказ", floatround(fuel, floatround_floor), tank, need, price);
}

static stock Fuel_Refuel(playerid)
{
    new vehicleid = GetPlayerVehicleID(playerid);
    if (vehicleid == 0 || Veh_GetFuel(vehicleid) < 0.0) return 0;
    new Float:dist;
    new st = Fuel_GetNearestStation(playerid, dist);
    if (st == -1 || dist > FUEL_RANGE) { SendError(playerid, "Не си на бензиностанция."); return 0; }

    new tank = Catalog_GetTank(GetVehicleModel(vehicleid));
    new Float:fuel = Veh_GetFuel(vehicleid);
    new liters = floatround(float(tank) - fuel, floatround_floor);
    new affordable = Money_Get(playerid) / FUEL_PRICE;
    if (liters > affordable) liters = affordable;
    if (liters <= 0) { SendError(playerid, "Нямаш пари за гориво."); return 0; }

    Money_Take(playerid, liters * FUEL_PRICE);
    Veh_SetFuel(vehicleid, fuel + float(liters));
    FuelWarned[playerid] = false;
    PlayerPlaySound(playerid, 1133, 0.0, 0.0, 0.0);
    SendClientMessage(playerid, COLOR_BRAND, "[ГОРИВО] " C_WHITE "Заредени " C_BRAND "%d л" C_WHITE " за " C_GREEN "$%d" C_WHITE ".", liters, liters * FUEL_PRICE);
    return 1;
}

// Y в превозно средство на бензиностанция
stock Fuel_OnKeyStateChange(playerid, KEY:newkeys, KEY:oldkeys)
{
    if (!((newkeys & KEY_YES) && !(oldkeys & KEY_YES))) return 0;
    if (GetPlayerState(playerid) != PLAYER_STATE_DRIVER) return 0;
    new vehicleid = GetPlayerVehicleID(playerid);
    if (Veh_GetFuel(vehicleid) < 0.0) return 0;
    new Float:dist;
    new st = Fuel_GetNearestStation(playerid, dist);
    if (st == -1 || dist > FUEL_RANGE) return 0;

    if (Engine_IsOn(vehicleid)) {
        SendError(playerid, "Изгаси двигателя (CTRL), за да заредиш.");
        return 1;
    }
    Fuel_ShowRefuel(playerid, vehicleid);
    return 1;
}

// ---------- Меню "Гориво" ----------

stock Fuel_ShowMenu(playerid)
{
    new vehicleid = GetPlayerVehicleID(playerid), line[96];
    if (vehicleid != 0 && Veh_GetFuel(vehicleid) >= 0.0) {
        format(line, sizeof (line), "В резервоара: %d / %d л", floatround(Veh_GetFuel(vehicleid), floatround_floor), Catalog_GetTank(GetVehicleModel(vehicleid)));
    } else {
        format(line, sizeof (line), "Седни в своето превозно средство");
    }
    ShowPlayerDialog(playerid, DIALOG_FUEL_MENU, DIALOG_STYLE_LIST, C_BRAND "Гориво",
        C_GREY "%s\n" C_WHITE "GPS до най-близката бензиностанция\n" C_WHITE "Пътна помощ: +%d л за $%d",
        "Избери", "Назад", line, ROADSIDE_LITERS, ROADSIDE_PRICE);
}

static stock Fuel_Roadside(playerid)
{
    new vehicleid = GetPlayerVehicleID(playerid);
    if (vehicleid == 0 || GetPlayerState(playerid) != PLAYER_STATE_DRIVER || Veh_GetOwner(vehicleid) != playerid) {
        { SendError(playerid, "Трябва да си шофьор на своето превозно средство."); return 0; }
    }
    new tank = Catalog_GetTank(GetVehicleModel(vehicleid));
    if (Veh_GetFuel(vehicleid) > float(tank) * FUEL_LOW_PERCENT / 100.0) {
        { SendError(playerid, "Пътна помощ се вика само при почти празен резервоар."); return 0; }
    }
    if (!Money_Take(playerid, ROADSIDE_PRICE)) { SendError(playerid, "Нямаш достатъчно пари за пътна помощ."); return 0; }
    Veh_SetFuel(vehicleid, Veh_GetFuel(vehicleid) + float(ROADSIDE_LITERS));
    FuelWarned[playerid] = false;
    SendClientMessage(playerid, COLOR_BRAND, "[ГОРИВО] " C_WHITE "Пътна помощ: +%d л. Карай до бензиностанция!", ROADSIDE_LITERS);
    return 1;
}

stock Fuel_OnDialogResponse(playerid, dialogid, response, listitem)
{
    switch (dialogid) {
        case DIALOG_FUEL_REFUEL: {
            if (response) Fuel_Refuel(playerid);
            return 1;
        }
        case DIALOG_FUEL_MENU: {
            if (!response) {
                Menu_Show(playerid);
                return 1;
            }
            if (listitem == 1) {
                new Float:dist;
                new st = Fuel_GetNearestStation(playerid, dist);
                if (st != -1) Gps_SetPoint(playerid, "бензиностанция", FuelStation[st][0], FuelStation[st][1], FuelStation[st][2]);
            } else if (listitem == 2) {
                Fuel_Roadside(playerid);
            } else {
                Fuel_ShowMenu(playerid);
            }
            return 1;
        }
    }
    return 0;
}

stock Fuel_OnPlayerConnect(playerid)
{
    FuelHasLastPos[playerid] = false;
    FuelWarned[playerid] = false;
}

// ---------- Админ ----------

CMD:roadside(playerid, params[])
{
    #pragma unused params
    Fuel_Roadside(playerid);
    return 1;
}

CMD:fuel(playerid, params[])
{
    #pragma unused params
    Fuel_ShowMenu(playerid);
    return 1;
}

CMD:addfuel(playerid, params[])
{
    #pragma unused params
    if (!Admin_Check(playerid, 4)) return 1;
    if (FuelStationCount >= MAX_FUEL_STATIONS) return SendError(playerid, "Достигнат е максимумът бензиностанции.");
    new Float:x, Float:y, Float:z, q[160];
    GetPlayerPos(playerid, x, y, z);
    format(q, sizeof (q), "INSERT INTO fuel_stations (x, y, z) VALUES (%f, %f, %f)", x, y, z);
    Database_Exec(q);
    Fuel_LoadStations();
    SendClientMessage(playerid, COLOR_ADMIN, "[A] Добавена бензиностанция тук (общо %d).", FuelStationCount);
    return 1;
}

CMD:delfuel(playerid, params[])
{
    #pragma unused params
    if (!Admin_Check(playerid, 4)) return 1;
    new Float:dist;
    new st = Fuel_GetNearestStation(playerid, dist);
    if (st == -1 || dist > 30.0) return SendError(playerid, "Няма бензиностанция в радиус 30 м.");
    new q[96];
    format(q, sizeof (q), "DELETE FROM fuel_stations WHERE id = %d", FuelStationID[st]);
    Database_Exec(q);
    Fuel_LoadStations();
    SendClientMessage(playerid, COLOR_ADMIN, "[A] Бензиностанцията е изтрита (остават %d).", FuelStationCount);
    return 1;
}

CMD:fixveh(playerid, params[])
{
    #pragma unused params
    if (!Admin_Check(playerid, 3)) return 1;
    new vehicleid = GetPlayerVehicleID(playerid);
    if (vehicleid == 0) return SendError(playerid, "Трябва да си в превозно средство.");
    RepairVehicle(vehicleid);
    SetVehicleHealth(vehicleid, 1000.0);
    if (Veh_GetFuel(vehicleid) >= 0.0) Veh_SetFuel(vehicleid, float(Catalog_GetTank(GetVehicleModel(vehicleid))));
    SendClientMessage(playerid, COLOR_ADMIN, "[A] Превозното средство е поправено и заредено.");
    return 1;
}
