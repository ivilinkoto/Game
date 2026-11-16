// Градове и входове на гаражите.
//
// Всеки играч има гараж в своя град. Във всеки град има вход (пикап + надпис + икона на картата).
// Входовете се местят от админ на място: /garagepos [0 LS | 1 SF | 2 LV]
// Админът застава пред вратата С ГРЪБ КЪМ НЕЯ (с лице към улицата) – натам излизат играчите.

enum {
    CITY_LS,
    CITY_SF,
    CITY_LV,
    MAX_CITIES
}

#define GARAGE_ENTER_RANGE_FOOT     3.0
#define GARAGE_ENTER_RANGE_VEH      9.0
#define GARAGE_MAP_ICON_ID          90

stock const CityNames[MAX_CITIES][] = {"Los Santos", "San Fierro", "Las Venturas"};
stock const CityShort[MAX_CITIES][] = {"LS", "SF", "LV"};

// Позиции по подразбиране (пред гаровете) – сменят се с /garagepos.
static const Float:CityDefaultEntrance[MAX_CITIES][4] = {
    {1743.0, -1862.0, 13.58, 0.0},      // LS – Unity Station
    {-1971.0, 137.0, 27.69, 90.0},      // SF – Cranberry Station
    {2847.9, 1291.2, 11.39, 90.0}       // LV – Linden Station
};

static Float:CityEntrance[MAX_CITIES][4];
static CityPickup[MAX_CITIES] = {-1, ...};
static Text3D:CityLabel[MAX_CITIES] = {Text3D:-1, ...};

stock City_Init()
{
    for (new c = 0; c < MAX_CITIES; c++) {
        for (new k = 0; k < 4; k++) CityEntrance[c][k] = CityDefaultEntrance[c][k];
    }
    new DBResult:r = Database_Query("SELECT city, x, y, z, a FROM garage_entrances");
    if (r != DBResult:0) {
        new rows = DB_GetRowCount(r);
        for (new i = 0; i < rows; i++) {
            new c = DB_GetFieldIntByName(r, "city");
            if (c >= 0 && c < MAX_CITIES) {
                CityEntrance[c][0] = DB_GetFieldFloatByName(r, "x");
                CityEntrance[c][1] = DB_GetFieldFloatByName(r, "y");
                CityEntrance[c][2] = DB_GetFieldFloatByName(r, "z");
                CityEntrance[c][3] = DB_GetFieldFloatByName(r, "a");
            }
            DB_SelectNextRow(r);
        }
        DB_FreeResultSet(r);
    }
    for (new c = 0; c < MAX_CITIES; c++) City_CreateEntrance(c);
}

static stock City_CreateEntrance(city)
{
    if (CityPickup[city] != -1) DestroyDynamicPickup(CityPickup[city]);
    if (CityLabel[city] != Text3D:-1) DestroyDynamic3DTextLabel(CityLabel[city]);

    new Float:x = CityEntrance[city][0], Float:y = CityEntrance[city][1], Float:z = CityEntrance[city][2];
    CityPickup[city] = CreateDynamicPickup(1318, 1, x, y, z, 0, 0);

    new text[160];
    format(text, sizeof (text), C_BRAND "ГАРАЖИ %s\n" C_WHITE "Натисни " C_BRAND "Y" C_WHITE " за вход\n" C_GREY "(пеша или с твоя камион)", CityShort[city]);
    CityLabel[city] = CreateDynamic3DTextLabel(text, COLOR_WHITE, x, y, z + 0.8, 25.0, INVALID_PLAYER_ID, INVALID_VEHICLE_ID, 1, 0, 0);
}

stock City_GetEntrance(city, &Float:x, &Float:y, &Float:z, &Float:a)
{
    if (city < 0 || city >= MAX_CITIES) city = CITY_LS;
    x = CityEntrance[city][0];
    y = CityEntrance[city][1];
    z = CityEntrance[city][2];
    a = CityEntrance[city][3];
}

// Точка на излизане: на dist метра пред вратата (по посоката на входа).
stock City_GetExitPoint(city, Float:dist, &Float:x, &Float:y, &Float:z, &Float:a)
{
    City_GetEntrance(city, x, y, z, a);
    x += dist * floatsin(-a, degrees);
    y += dist * floatcos(-a, degrees);
}

// Град, чийто вход е до играча (или -1).
stock City_GetEntranceNear(playerid, Float:range)
{
    for (new c = 0; c < MAX_CITIES; c++) {
        if (IsPlayerInRangeOfPoint(playerid, range, CityEntrance[c][0], CityEntrance[c][1], CityEntrance[c][2])) return c;
    }
    return -1;
}

// Икона на камион на картата – само за гаража на играча.
stock City_ShowGarageIcon(playerid)
{
    new city = PlayerData[playerid][pGarageCity];
    SetPlayerMapIcon(playerid, GARAGE_MAP_ICON_ID, CityEntrance[city][0], CityEntrance[city][1], CityEntrance[city][2], 51, 0, MAPICON_GLOBAL);
}

CMD:garagepos(playerid, params[])
{
    if (!Admin_Check(playerid, 5)) return 1;
    if (IsNull(params) || !IsNumeric(params)) return SendUsage(playerid, "/garagepos [0 LS | 1 SF | 2 LV] – застани пред вратата с лице към улицата");
    new city = strval(params);
    if (city < 0 || city >= MAX_CITIES) return SendError(playerid, "Градът е 0, 1 или 2.");
    if (GetPlayerVirtualWorld(playerid) != 0 || GetPlayerInterior(playerid) != 0) return SendError(playerid, "Трябва да си навън (свят 0).");

    new Float:x, Float:y, Float:z, Float:a;
    GetPlayerPos(playerid, x, y, z);
    if (IsPlayerInAnyVehicle(playerid)) GetVehicleZAngle(GetPlayerVehicleID(playerid), a);
    else GetPlayerFacingAngle(playerid, a);

    CityEntrance[city][0] = x;
    CityEntrance[city][1] = y;
    CityEntrance[city][2] = z;
    CityEntrance[city][3] = a;
    new q[200];
    format(q, sizeof (q), "INSERT OR REPLACE INTO garage_entrances (city, x, y, z, a) VALUES (%d, %f, %f, %f, %f)", city, x, y, z, a);
    Database_Exec(q);
    City_CreateEntrance(city);

    for (new i = 0; i < MAX_PLAYERS; i++) {
        if (IsPlayerLogged(i) && PlayerData[i][pGarageCity] == city) City_ShowGarageIcon(i);
    }
    SendClientMessage(playerid, COLOR_ADMIN, "[A] Входът на гаражите в %s е преместен тук.", CityNames[city]);
    return 1;
}
