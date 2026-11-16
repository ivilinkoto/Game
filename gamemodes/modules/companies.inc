// Компании: бази, какво дават и какво приемат. По-късно – собственост (бизнеси).
//
// Базите имат приблизителни позиции; админ ги мести на място: /compos [номер].
// Списък: /complist

#define MAX_COMPANIES       14
#define COMPANY_RANGE       12.0        // колко близо до базата се брои "в базата"
#define COMPANY_ICON_TYPE   51          // икона "камион" на картата
#define COMPANY_ICON_NEAREST 5          // колко от най-близките компании да се виждат
#define COMPANY_ICON_ID_BASE 91         // ID-та на иконите при играча: 91..95

enum {
    COMPANY_SMALL,
    COMPANY_MEDIUM,
    COMPANY_LARGE
}

enum E_COMPANY_DEF {
    cdName[32],
    cdPlace[24],
    cdSize,
    cdProduces,         // GOOD_BIT(...)
    cdAccepts,
    Float:cdDefX,
    Float:cdDefY,
    Float:cdDefZ
}

stock const CompanyDefs[MAX_COMPANIES][E_COMPANY_DEF] = {
    {"Ocean Docks Shipping", "Los Santos",   COMPANY_LARGE,
        GOOD_BIT(GOOD_ELECTRONICS) | GOOD_BIT(GOOD_FURNITURE) | GOOD_BIT(GOOD_PARTS) | GOOD_BIT(GOOD_DRINKS) | GOOD_BIT(GOOD_PARCELS),
        GOOD_BIT(GOOD_METAL) | GOOD_BIT(GOOD_FOOD),
        2770.0, -2420.0, 13.6},
    {"LS Parcel Express",    "Los Santos",   COMPANY_SMALL,
        GOOD_BIT(GOOD_PARCELS) | GOOD_BIT(GOOD_MEDS),
        GOOD_BIT(GOOD_PARCELS) | GOOD_BIT(GOOD_MEDS),
        1965.0, -2185.0, 13.5},
    {"Santos Mart",          "Los Santos",   COMPANY_SMALL,
        0,
        GOOD_BIT(GOOD_FOOD) | GOOD_BIT(GOOD_DRINKS) | GOOD_BIT(GOOD_ELECTRONICS) | GOOD_BIT(GOOD_PARCELS),
        1130.0, -1440.0, 15.8},
    {"Bay Lumber Co.",       "Angel Pine",   COMPANY_MEDIUM,
        GOOD_BIT(GOOD_WOOD) | GOOD_BIT(GOOD_FURNITURE),
        GOOD_BIT(GOOD_PARTS),
        -1966.0, -2436.0, 30.6},
    {"Doherty Construction", "San Fierro",   COMPANY_MEDIUM,
        0,
        GOOD_BIT(GOOD_WOOD) | GOOD_BIT(GOOD_SAND) | GOOD_BIT(GOOD_GRAVEL) | GOOD_BIT(GOOD_METAL),
        -2080.0, 230.0, 35.3},
    {"SF Post Center",       "San Fierro",   COMPANY_SMALL,
        GOOD_BIT(GOOD_PARCELS) | GOOD_BIT(GOOD_MEDS),
        GOOD_BIT(GOOD_PARCELS) | GOOD_BIT(GOOD_MEDS),
        -1700.0, 30.0, 3.6},
    {"Hunter Quarry",        "Bone County",  COMPANY_LARGE,
        GOOD_BIT(GOOD_SAND) | GOOD_BIT(GOOD_GRAVEL) | GOOD_BIT(GOOD_ORE),
        GOOD_BIT(GOOD_PARTS),
        588.0, 870.0, -42.5},
    {"Desert Steel Foundry", "Las Venturas", COMPANY_LARGE,
        GOOD_BIT(GOOD_METAL) | GOOD_BIT(GOOD_PARTS),
        GOOD_BIT(GOOD_ORE),
        1050.0, 2100.0, 10.8},
    {"Green Palms Refinery", "Bone County",  COMPANY_LARGE,
        GOOD_BIT(GOOD_FUEL),
        GOOD_BIT(GOOD_PARTS),
        265.0, 1405.0, 10.6},
    {"Venturas Logistics",   "Las Venturas", COMPANY_MEDIUM,
        GOOD_BIT(GOOD_FOOD) | GOOD_BIT(GOOD_DRINKS) | GOOD_BIT(GOOD_ELECTRONICS) | GOOD_BIT(GOOD_PARCELS),
        GOOD_BIT(GOOD_FURNITURE) | GOOD_BIT(GOOD_ELECTRONICS) | GOOD_BIT(GOOD_PARCELS) | GOOD_BIT(GOOD_METAL),
        1640.0, 1030.0, 10.8},
    {"Blueberry Farms",      "Red County",   COMPANY_MEDIUM,
        GOOD_BIT(GOOD_FOOD),
        GOOD_BIT(GOOD_PARCELS) | GOOD_BIT(GOOD_PARTS),
        -60.0, 70.0, 3.1},
    {"Fuel Station LS",      "Los Santos",   COMPANY_SMALL,
        0,
        GOOD_BIT(GOOD_FUEL) | GOOD_BIT(GOOD_FOOD) | GOOD_BIT(GOOD_DRINKS),
        1940.0, -1773.0, 13.4},
    {"Fuel Station SF",      "San Fierro",   COMPANY_SMALL,
        0,
        GOOD_BIT(GOOD_FUEL) | GOOD_BIT(GOOD_FOOD) | GOOD_BIT(GOOD_DRINKS),
        -2410.0, 970.0, 45.3},
    {"Fuel Station LV",      "Las Venturas", COMPANY_SMALL,
        0,
        GOOD_BIT(GOOD_FUEL) | GOOD_BIT(GOOD_FOOD) | GOOD_BIT(GOOD_DRINKS),
        2115.0, 920.0, 10.8}
};

stock const CompanyPrices[] = {50000, 120000, 250000};

// Данни, които се менят (позиция, собственост, статистика)
enum E_COMPANY {
    Float:cpX,
    Float:cpY,
    Float:cpZ,
    cpOwner,            // ID на акаунт (0 = държавна)
    cpCash,
    cpDeliveries,
    cpEarned
}

new Company[MAX_COMPANIES][E_COMPANY];
static Text3D:CompanyLabel[MAX_COMPANIES] = {Text3D:-1, ...};

stock Company_Init()
{
    new q[512];
    q[0] = EOS;
    strcat(q, "CREATE TABLE IF NOT EXISTS companies (");
    strcat(q, "id INTEGER PRIMARY KEY,");
    strcat(q, "x REAL NOT NULL, y REAL NOT NULL, z REAL NOT NULL,");
    strcat(q, "owner_id INTEGER NOT NULL DEFAULT 0,");
    strcat(q, "cash INTEGER NOT NULL DEFAULT 0,");
    strcat(q, "deliveries INTEGER NOT NULL DEFAULT 0,");
    strcat(q, "earned INTEGER NOT NULL DEFAULT 0,");
    strcat(q, "owner_since INTEGER NOT NULL DEFAULT 0)");
    Database_Exec(q);

    for (new c = 0; c < MAX_COMPANIES; c++) {
        format(q, sizeof (q), "INSERT OR IGNORE INTO companies (id, x, y, z) VALUES (%d, %f, %f, %f)",
            c, CompanyDefs[c][cdDefX], CompanyDefs[c][cdDefY], CompanyDefs[c][cdDefZ]);
        Database_Exec(q);
    }

    new DBResult:r = Database_Query("SELECT * FROM companies");
    if (r != DBResult:0) {
        new rows = DB_GetRowCount(r);
        for (new i = 0; i < rows; i++) {
            new c = DB_GetFieldIntByName(r, "id");
            if (c >= 0 && c < MAX_COMPANIES) {
                Company[c][cpX] = DB_GetFieldFloatByName(r, "x");
                Company[c][cpY] = DB_GetFieldFloatByName(r, "y");
                Company[c][cpZ] = DB_GetFieldFloatByName(r, "z");
                Company[c][cpOwner] = DB_GetFieldIntByName(r, "owner_id");
                Company[c][cpCash] = DB_GetFieldIntByName(r, "cash");
                Company[c][cpDeliveries] = DB_GetFieldIntByName(r, "deliveries");
                Company[c][cpEarned] = DB_GetFieldIntByName(r, "earned");
            }
            DB_SelectNextRow(r);
        }
        DB_FreeResultSet(r);
    }

    for (new c = 0; c < MAX_COMPANIES; c++) Company_CreateMarkers(c);
}

// Списък с товари: "пратки, медикаменти"
stock Company_GoodsList(mask, dest[], size = sizeof (dest), bool:latin = false)
{
    dest[0] = EOS;
    for (new g = 0; g < MAX_GOODS; g++) {
        if (!(mask & GOOD_BIT(g))) continue;
        if (dest[0] != EOS) strcat(dest, ", ", size);
        if (latin) strcat(dest, Goods[g][gdNameEN], size);
        else strcat(dest, Goods[g][gdName], size);
    }
    if (dest[0] == EOS) format(dest, size, "-");
}

static stock Company_CreateMarkers(c)
{
    if (CompanyLabel[c] != Text3D:-1) DestroyDynamic3DTextLabel(CompanyLabel[c]);

    new produces[160], accepts[160], text[512];
    Company_GoodsList(CompanyDefs[c][cdProduces], produces);
    Company_GoodsList(CompanyDefs[c][cdAccepts], accepts);
    format(text, sizeof (text),
        C_BRAND "%s\n" C_GREY "%s\n" C_WHITE "Дава: " C_GREEN "%s\n" C_WHITE "Приема: " C_BRAND "%s\n" C_GREY "N с превозно средство – поръчки",
        CompanyDefs[c][cdName], CompanyDefs[c][cdPlace], produces, accepts);
    CompanyLabel[c] = CreateDynamic3DTextLabel(text, COLOR_WHITE, Company[c][cpX], Company[c][cpY], Company[c][cpZ] + 1.5, 30.0,
        INVALID_PLAYER_ID, INVALID_VEHICLE_ID, 0, 0, 0);
}

// Показва на картата само най-близките компании (вика се на няколко секунди).
stock Company_UpdateIcons(playerid)
{
    new Float:px, Float:py, Float:pz;
    if (PlayerData[playerid][pInGarage]) {
        // В гаража – спрямо входа в града
        new Float:a;
        City_GetEntrance(PlayerData[playerid][pGarageCity], px, py, pz, a);
    } else {
        GetPlayerPos(playerid, px, py, pz);
    }

    new picked[COMPANY_ICON_NEAREST], n = 0;
    for (new k = 0; k < COMPANY_ICON_NEAREST; k++) {
        new best = -1, Float:bestDist = 999999.0;
        for (new c = 0; c < MAX_COMPANIES; c++) {
            new bool:used = false;
            for (new j = 0; j < n; j++) if (picked[j] == c) used = true;
            if (used) continue;
            new Float:d = VectorSize(Company[c][cpX] - px, Company[c][cpY] - py, 0.0);
            if (d < bestDist) {
                bestDist = d;
                best = c;
            }
        }
        if (best == -1) break;
        picked[n++] = best;
    }
    for (new k = 0; k < COMPANY_ICON_NEAREST; k++) {
        if (k < n) {
            new c = picked[k];
            SetPlayerMapIcon(playerid, COMPANY_ICON_ID_BASE + k, Company[c][cpX], Company[c][cpY], Company[c][cpZ], COMPANY_ICON_TYPE, 0, MAPICON_GLOBAL);
        } else {
            RemovePlayerMapIcon(playerid, COMPANY_ICON_ID_BASE + k);
        }
    }
}

stock bool:Company_Produces(c, good)
{
    return (CompanyDefs[c][cdProduces] & GOOD_BIT(good)) != 0;
}

stock bool:Company_Accepts(c, good)
{
    return (CompanyDefs[c][cdAccepts] & GOOD_BIT(good)) != 0;
}

stock Float:Company_Distance(a, b)
{
    return VectorSize(Company[a][cpX] - Company[b][cpX], Company[a][cpY] - Company[b][cpY], Company[a][cpZ] - Company[b][cpZ]);
}

// Компанията, в чиято база е играчът (или -1)
stock Company_GetNear(playerid, Float:range = COMPANY_RANGE)
{
    for (new c = 0; c < MAX_COMPANIES; c++) {
        if (IsPlayerInRangeOfPoint(playerid, range, Company[c][cpX], Company[c][cpY], Company[c][cpZ])) return c;
    }
    return -1;
}

// Статистика след курс (по-късно и процентът за собственика)
stock Company_OnDelivery(from, to, value)
{
    new q[160];
    Company[from][cpDeliveries]++;
    Company[to][cpDeliveries]++;
    #pragma unused value
    format(q, sizeof (q), "UPDATE companies SET deliveries = deliveries + 1 WHERE id IN (%d, %d)", from, to);
    Database_Exec(q);
}

// ---------- Админ ----------

CMD:compos(playerid, params[])
{
    if (!Admin_Check(playerid, 5)) return 1;
    if (IsNull(params) || !IsNumeric(params)) return SendUsage(playerid, "/compos [номер от /complist] – премества базата тук");
    new c = strval(params);
    if (c < 0 || c >= MAX_COMPANIES) return SendError(playerid, "Няма такава компания.");
    if (GetPlayerVirtualWorld(playerid) != 0) return SendError(playerid, "Трябва да си навън (свят 0).");

    new Float:x, Float:y, Float:z, q[160];
    GetPlayerPos(playerid, x, y, z);
    Company[c][cpX] = x;
    Company[c][cpY] = y;
    Company[c][cpZ] = z;
    format(q, sizeof (q), "UPDATE companies SET x = %f, y = %f, z = %f WHERE id = %d", x, y, z, c);
    Database_Exec(q);
    Company_CreateMarkers(c);
    Contracts_OnCompanyMoved(c);
    SendClientMessage(playerid, COLOR_ADMIN, "[A] Базата на %s е преместена тук.", CompanyDefs[c][cdName]);
    return 1;
}

CMD:complist(playerid, params[])
{
    #pragma unused params
    if (!Admin_Check(playerid, 3)) return 1;
    for (new c = 0; c < MAX_COMPANIES; c++) {
        SendClientMessage(playerid, COLOR_GREY, "#%d · %s (%s) · %.1f км от теб", c, CompanyDefs[c][cdName], CompanyDefs[c][cdPlace],
            GetPlayerDistanceFromPoint(playerid, Company[c][cpX], Company[c][cpY], Company[c][cpZ]) / 1000.0);
    }
    return 1;
}

CMD:gotocomp(playerid, params[])
{
    if (!Admin_Check(playerid, 3)) return 1;
    if (IsNull(params) || !IsNumeric(params)) return SendUsage(playerid, "/gotocomp [номер от /complist]");
    new c = strval(params);
    if (c < 0 || c >= MAX_COMPANIES) return SendError(playerid, "Няма такава компания.");
    SetPlayerVirtualWorld(playerid, 0);
    SetPlayerInterior(playerid, 0);
    PlayerData[playerid][pInGarage] = false;
    if (IsPlayerInAnyVehicle(playerid) && GetPlayerState(playerid) == PLAYER_STATE_DRIVER) {
        new v = GetPlayerVehicleID(playerid);
        if (Veh_GetOwner(v) == playerid) Veh_SetOut(playerid, Veh_GetSlot(v), true);
        SetVehicleVirtualWorld(v, 0);
        SetVehiclePos(v, Company[c][cpX], Company[c][cpY], Company[c][cpZ] + 1.0);
        PutPlayerInVehicle(playerid, v, 0);
    } else {
        SetPlayerPos(playerid, Company[c][cpX], Company[c][cpY], Company[c][cpZ] + 1.0);
    }
    return 1;
}
