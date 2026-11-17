// Нужди: глад, жажда, тоалетна. Места: магазини, закусвални, кафенета, тоалетни.
//
// - Глад и жажда падат с времето (от 100 до 0 за ~2 ч / ~1.5 ч), тоалетната расте (~2 ч, напитките я пълнят).
// - На 0 глад или жажда здравето бавно пада. Пълна тоалетна -> "инцидент" (глоба и нулиране).
// - Пазаруване и тоалетна: ALT пеша до място, на бензиностанция или в гаража (хладилник + тоалетна).
//   С превозно средство на бензиностанция: ALT -> Магазин.
// - Места се добавят от админ панела (Места на картата) – пазят се в базата.

#define NEEDS_HUNGER_SECONDS    72      // -1 глад на толкова секунди
#define NEEDS_THIRST_SECONDS    54      // -1 жажда
#define NEEDS_BLADDER_SECONDS   72      // +1 тоалетна
#define NEEDS_WARN              20
#define NEEDS_DAMAGE_SECONDS    20      // на 0 глад/жажда: -2 здраве на толкова секунди
#define NEEDS_ACCIDENT_FINE     50
#define TOILET_PRICE            2
#define PLACE_RANGE             4.0
#define MAX_PLACES              120

// Умора
#define FATIGUE_DRIVE_SECONDS   30      // +1 умора на толкова секунди каране
#define FATIGUE_FOOT_SECONDS    20      // -1 умора на толкова секунди пеша
#define FATIGUE_WARN            70
#define FATIGUE_SWAY            85      // от тук картината се люшка
#define FATIGUE_SEVERE          95
#define REST_SECONDS            15

enum {
    PLACE_SHOP,             // магазин: всичко
    PLACE_DINER,            // закусвалня: храна + безалкохолни
    PLACE_CAFE,             // кафене: напитки + сандвич
    PLACE_TOILET,           // обществена тоалетна
    PLACE_REST,             // паркинг за почивка (умора)
    PLACE_WEIGH,            // кантар (виж delivery.inc) – не се ползва с ALT
    MAX_PLACE_TYPES
}

stock const PlaceTypeNames[MAX_PLACE_TYPES][] = {"Магазин", "Закусвалня", "Кафене", "Обществена тоалетна", "Паркинг за почивка", "Кантар"};
static const PlaceIcons[MAX_PLACE_TYPES] = {17, 10, 17, 0, 0, 0};

// Артикули: име, цена, +глад, +жажда, +тоалетна, в кои места (битове PLACE_*)
enum E_ITEM {
    itName[32],
    itPrice,
    itFood,
    itDrink,
    itBladder,
    itWhere,
    itRest              // намалява умората с толкова
}
static const ShopItems[][E_ITEM] = {
    {"Сандвич",          15, 30,  0,  0, (1 << PLACE_SHOP) | (1 << PLACE_DINER) | (1 << PLACE_CAFE), 0},
    {"Бургер",           25, 45,  0,  0, (1 << PLACE_SHOP) | (1 << PLACE_DINER), 0},
    {"Пица",             40, 70,  0,  0, (1 << PLACE_DINER), 0},
    {"Вода",              5,  0, 35, 10, (1 << PLACE_SHOP) | (1 << PLACE_DINER) | (1 << PLACE_CAFE), 0},
    {"Кола",              6,  0, 30, 12, (1 << PLACE_SHOP) | (1 << PLACE_DINER), 5},
    {"Кафе",              8,  0, 20, 10, (1 << PLACE_SHOP) | (1 << PLACE_CAFE), 15},
    {"Енергийна напитка", 12, 5, 30, 15, (1 << PLACE_SHOP) | (1 << PLACE_CAFE), 25}
};

// Специалните "места" при показване на менюто
#define SHOP_KIND_STATION   100     // бензиностанция = магазин + тоалетна
#define SHOP_KIND_GARAGE    101     // гараж: хладилник (безплатна вода и сандвич) + тоалетна

static Float:PlacePos[MAX_PLACES][3];
static PlaceType[MAX_PLACES];
static PlaceID[MAX_PLACES];
static Text3D:PlaceLabel[MAX_PLACES] = {Text3D:-1, ...};
static PlaceIcon[MAX_PLACES] = {-1, ...};

// Обекти към местата (тоалетна кабина, сглобен кантар)
#define PLACE_MAX_OBJECTS   16
#define OBJ_PORTALOO        2984
#define PORTALOO_Z_OFFSET   1.2         // моделът е центриран ниско, иначе потъва в земята

// Кантар (координати спрямо мястото: напред = по посоката на движение)
#define OBJ_WALL_LONG       19447       // стена 10.5 x 3.5 м (легнала = плоча)
#define OBJ_WALL_SMALL      19353       // стена 3.5 x 3.5 м
#define OBJ_GATE_BOX        966         // кутия на бариерата
#define OBJ_GATE_BAR        968         // рамо на бариерата
#define WEIGH_HEIGHT        0.30        // височина на платформата над пътя
#define WEIGH_SEG_Y         4.75        // центровете на двете плочи (плочата е ~9.6 м, леко се застъпват)
#define WEIGH_HALF_LEN      9.55        // краят на платформата (~19 м дълга, 3.5 м широка)
#define WEIGH_BOOTH_DX      4.4         // будката е вдясно от платформата
#define WEIGH_GATE_DX       2.9         // кутията на бариерата вдясно; рамото стига до центъра и малко отвъд
#define WEIGH_GATE_Z        0.85
#define WEIGH_GATE_OPEN_SEC 12
#define WEIGH_BAR_SLOT      15          // индексът на рамото в PlaceObj
static PlaceObj[MAX_PLACES][PLACE_MAX_OBJECTS];
static Float:PlaceAngle[MAX_PLACES];
static PlaceCount;
static PlaceGateClose[MAX_PLACES];         // gettime() за затваряне на бариерата (0 = затворена)

static NeedHunger[MAX_PLAYERS], NeedThirst[MAX_PLAYERS], NeedBladder[MAX_PLAYERS];
static NeedFatigue[MAX_PLAYERS];
static FatigueDrive[MAX_PLAYERS], FatigueRestTick[MAX_PLAYERS];
static bool:Resting[MAX_PLAYERS];
static NeedTick[MAX_PLAYERS];
static bool:NeedWarned[MAX_PLAYERS][3];
static ShopKind[MAX_PLAYERS];
static ShopMap[MAX_PLAYERS][16];


// ---------- Места ----------

stock Places_Init()
{
    Database_Exec("CREATE TABLE IF NOT EXISTS places (id INTEGER PRIMARY KEY AUTOINCREMENT, type INTEGER NOT NULL, x REAL NOT NULL, y REAL NOT NULL, z REAL NOT NULL)");
    Database_AddColumn("places", "a", "REAL NOT NULL DEFAULT 0");
    Database_AddColumn("accounts", "hunger", "INTEGER NOT NULL DEFAULT 100");
    Database_AddColumn("accounts", "thirst", "INTEGER NOT NULL DEFAULT 100");
    Database_AddColumn("accounts", "bladder", "INTEGER NOT NULL DEFAULT 0");
    Database_AddColumn("accounts", "fatigue", "INTEGER NOT NULL DEFAULT 0");
    Places_Load();
}

static stock Places_Load()
{
    for (new i = 0; i < PlaceCount; i++) {
        if (PlaceLabel[i] != Text3D:-1) DestroyDynamic3DTextLabel(PlaceLabel[i]);
        if (PlaceIcon[i] != -1) DestroyDynamicMapIcon(PlaceIcon[i]);
        PlaceLabel[i] = Text3D:-1;
        PlaceIcon[i] = -1;
        for (new o = 0; o < PLACE_MAX_OBJECTS; o++) {
            if (PlaceObj[i][o] != 0) DestroyDynamicObject(PlaceObj[i][o]);
            PlaceObj[i][o] = 0;
        }
    }
    PlaceCount = 0;
    new DBResult:r = Database_Query("SELECT id, type, x, y, z, a FROM places ORDER BY id");
    if (r == DBResult:0) return;
    new rows = DB_GetRowCount(r), text[160];
    for (new i = 0; i < rows && PlaceCount < MAX_PLACES; i++) {
        new k = PlaceCount++;
        PlaceID[k] = DB_GetFieldIntByName(r, "id");
        PlaceType[k] = DB_GetFieldIntByName(r, "type");
        if (PlaceType[k] < 0 || PlaceType[k] >= MAX_PLACE_TYPES) PlaceType[k] = PLACE_SHOP;
        PlacePos[k][0] = DB_GetFieldFloatByName(r, "x");
        PlacePos[k][1] = DB_GetFieldFloatByName(r, "y");
        PlacePos[k][2] = DB_GetFieldFloatByName(r, "z");
        PlaceAngle[k] = DB_GetFieldFloatByName(r, "a");
        Places_CreateObjects(k);
        if (PlaceType[k] == PLACE_WEIGH) format(text, sizeof (text), C_BRAND "КАНТАР\n" C_WHITE "С товар - спри на платформата за %d сек", 10);
        else format(text, sizeof (text), C_BRAND "%s\n" C_GREY "ALT", PlaceTypeNames[PlaceType[k]]);
        PlaceLabel[k] = CreateDynamic3DTextLabel(text, COLOR_WHITE, PlacePos[k][0], PlacePos[k][1], PlacePos[k][2] + 0.6, 20.0,
            INVALID_PLAYER_ID, INVALID_VEHICLE_ID, 1, 0, 0);
        if (PlaceIcons[PlaceType[k]] != 0) {
            PlaceIcon[k] = CreateDynamicMapIcon(PlacePos[k][0], PlacePos[k][1], PlacePos[k][2], PlaceIcons[PlaceType[k]], 0, 0, 0, -1, MAP_ICON_RANGE, MAPICON_LOCAL);
        }
        DB_SelectNextRow(r);
    }
    DB_FreeResultSet(r);
}

// Точка на dx (надясно) и dy (напред) спрямо мястото и посоката му
static stock Places_Offset(k, Float:dx, Float:dy, &Float:x, &Float:y)
{
    new Float:a = PlaceAngle[k];
    x = PlacePos[k][0] + dy * floatsin(-a, degrees) + dx * floatcos(-a, degrees);
    y = PlacePos[k][1] + dy * floatcos(-a, degrees) - dx * floatsin(-a, degrees);
}

static stock Places_CreateObjects(k)
{
    new Float:x, Float:y, Float:z = PlacePos[k][2] - 1.0, Float:a = PlaceAngle[k];
    switch (PlaceType[k]) {
        case PLACE_TOILET: {
            // Кабина на 1.8 м пред мястото, с вратата към него
            Places_Offset(k, 0.0, 1.8, x, y);
            PlaceObj[k][0] = CreateDynamicObject(OBJ_PORTALOO, x, y, z + PORTALOO_Z_OFFSET, 0.0, 0.0, a + 270.0, 0, 0, -1, 200.0, 200.0);
        }
        case PLACE_WEIGH: Places_BuildWeigh(k);
    }
}

// Обект на кантара на dx/dy от центъра, на височина dz над пътя
static stock Places_WeighObj(k, slot, model, Float:dx, Float:dy, Float:dz, Float:rx, Float:ry, Float:rzAdd, colour = 0)
{
    new Float:x, Float:y, Float:z = PlacePos[k][2] - 1.0;
    Places_Offset(k, dx, dy, x, y);
    new obj = CreateDynamicObject(model, x, y, z + dz, rx, ry, PlaceAngle[k] + rzAdd, 0, 0, -1, 250.0, 250.0);
    if (colour != 0) SetDynamicObjectMaterial(obj, 0, 18646, "matcolours", "white", colour);   // плътен цвят
    PlaceObj[k][slot] = obj;
    return obj;
}

// Сглобен кантар: стоманена платформа с рампи, жълти бордюри, будка и бариера на изхода
static stock Places_BuildWeigh(k)
{
    new slot = 0, obj;
    new Float:ramp = 4.9;                  // наклон на рампата: 0.30 м на 3.5 м

    // Платформа: 2 легнали дълги стени
    for (new i = -1; i <= 1; i += 2) {
        obj = Places_WeighObj(k, slot++, OBJ_WALL_LONG, 0.0, i * WEIGH_SEG_Y, WEIGH_HEIGHT - 0.15 + ((i > 0) ? 0.01 : 0.0), 0.0, 90.0, 0.0);
        SetDynamicObjectMaterial(obj, 0, 16322, "a51_stores", "metalic128", 0xFFC8CCD0);
    }
    // Рампи на двата края (качват от пътя до платформата)
    for (new i = -1; i <= 1; i += 2) {
        obj = Places_WeighObj(k, slot++, OBJ_WALL_SMALL, 0.0, i * (WEIGH_HALF_LEN + 1.6), WEIGH_HEIGHT / 2.0 - 0.15, -i * ramp, 90.0, 0.0);
        SetDynamicObjectMaterial(obj, 0, 16322, "a51_stores", "metalic128", 0xFF9AA0A6);
    }
    // Жълти бордюри от двете страни (стърчат 25 см над платформата)
    for (new side = -1; side <= 1; side += 2) {
        for (new i = -1; i <= 1; i += 2) {
            Places_WeighObj(k, slot++, OBJ_WALL_LONG, side * (1.9 + ((i > 0) ? 0.01 : 0.0)), i * WEIGH_SEG_Y, WEIGH_HEIGHT + 0.25 - 1.75, 0.0, 0.0, 0.0, 0xFFF2C200);
        }
    }
    // Будка 3.5 x 3.5 вдясно: 4 стени и покрив
    Places_WeighObj(k, slot++, OBJ_WALL_SMALL, WEIGH_BOOTH_DX - 1.75, 0.0, 1.75, 0.0, 0.0, 0.0, 0xFFE8E8E8);
    Places_WeighObj(k, slot++, OBJ_WALL_SMALL, WEIGH_BOOTH_DX + 1.75, 0.0, 1.75, 0.0, 0.0, 0.0, 0xFFE8E8E8);
    Places_WeighObj(k, slot++, OBJ_WALL_SMALL, WEIGH_BOOTH_DX, -1.75, 1.75, 0.0, 0.0, 90.0, 0xFFE8E8E8);
    Places_WeighObj(k, slot++, OBJ_WALL_SMALL, WEIGH_BOOTH_DX, 1.75, 1.75, 0.0, 0.0, 90.0, 0xFF2E6DB4);
    Places_WeighObj(k, slot++, OBJ_WALL_SMALL, WEIGH_BOOTH_DX, 0.0, 3.6, 0.0, 90.0, 0.0, 0xFF3A3A3A);
    // Бариера на изхода: кутия вдясно и рамо през платното (затворена)
    Places_WeighObj(k, slot, OBJ_GATE_BOX, WEIGH_GATE_DX, WEIGH_HALF_LEN + 0.5, 0.0, 0.0, 0.0, 0.0);
    Places_WeighObj(k, WEIGH_BAR_SLOT, OBJ_GATE_BAR, WEIGH_GATE_DX, WEIGH_HALF_LEN + 0.5, WEIGH_GATE_Z, 0.0, -90.0, 0.0);
    PlaceGateClose[k] = 0;
}

// Отваря бариерата на кантар k (затваря се сама след WEIGH_GATE_OPEN_SEC)
stock Places_OpenWeighGate(k)
{
    if (k < 0 || k >= PlaceCount || PlaceType[k] != PLACE_WEIGH) return;
    new bar = PlaceObj[k][WEIGH_BAR_SLOT];
    if (bar == 0) return;
    if (PlaceGateClose[k] == 0) {
        new Float:x, Float:y, Float:z;
        GetDynamicObjectPos(bar, x, y, z);
        MoveDynamicObject(bar, x, y, z + 0.01, 0.02, 0.0, 0.0, PlaceAngle[k]);
    }
    PlaceGateClose[k] = gettime() + WEIGH_GATE_OPEN_SEC;
}

// Всяка секунда: затваря бариерите, чието време е изтекло
stock Places_GateTick()
{
    new now = gettime();
    for (new k = 0; k < PlaceCount; k++) {
        if (PlaceGateClose[k] == 0 || now < PlaceGateClose[k]) continue;
        PlaceGateClose[k] = 0;
        new bar = PlaceObj[k][WEIGH_BAR_SLOT];
        if (bar == 0) continue;
        new Float:x, Float:y, Float:z;
        GetDynamicObjectPos(bar, x, y, z);
        MoveDynamicObject(bar, x, y, z - 0.01, 0.02, 0.0, -90.0, PlaceAngle[k]);
    }
}

// Без товар за претегляне: бариерата се вдига, щом спре на платформата
static stock Places_WeighFreePass(playerid)
{
    if (GetPlayerState(playerid) != PLAYER_STATE_DRIVER) return;
    new w = Places_GetNearWeigh(playerid, 9.0);
    if (w == -1 || Delivery_MustWeigh(playerid, w)) return;
    new vehicleid = GetPlayerVehicleID(playerid), Float:vx, Float:vy, Float:vz;
    GetVehicleVelocity(vehicleid, vx, vy, vz);
    if (VectorSize(vx, vy, vz) * 180.0 > 8.0) return;
    if (PlaceGateClose[w] == 0) GameTextForPlayer(playerid, "~g~NO CARGO~n~~w~GO AHEAD", 2000, 3);
    Places_OpenWeighGate(w);
}

stock Places_AddHere(playerid, type)
{
    if (type < 0 || type >= MAX_PLACE_TYPES) return 0;
    if (PlaceCount >= MAX_PLACES) { SendError(playerid, "Достигнат е максимумът места."); return 0; }
    new Float:x, Float:y, Float:z, Float:a, q[200];
    GetPlayerPos(playerid, x, y, z);
    if (IsPlayerInAnyVehicle(playerid)) GetVehicleZAngle(GetPlayerVehicleID(playerid), a);
    else GetPlayerFacingAngle(playerid, a);
    format(q, sizeof (q), "INSERT INTO places (type, x, y, z, a) VALUES (%d, %f, %f, %f, %f)", type, x, y, z, a);
    Database_Exec(q);
    Places_Load();
    SendClientMessage(playerid, COLOR_ADMIN, "[A] Добавено: %s (общо %d места).", PlaceTypeNames[type], PlaceCount);
    return 1;
}

stock Places_DeleteNearest(playerid)
{
    new best = -1, Float:bestDist = 30.0;
    for (new i = 0; i < PlaceCount; i++) {
        new Float:d = GetPlayerDistanceFromPoint(playerid, PlacePos[i][0], PlacePos[i][1], PlacePos[i][2]);
        if (d < bestDist) {
            bestDist = d;
            best = i;
        }
    }
    if (best == -1) { SendError(playerid, "Няма място (магазин, тоалетна...) в радиус 30 м."); return 0; }
    new q[96], type = PlaceType[best];
    format(q, sizeof (q), "DELETE FROM places WHERE id = %d", PlaceID[best]);
    Database_Exec(q);
    Places_Load();
    SendClientMessage(playerid, COLOR_ADMIN, "[A] Изтрито: %s.", PlaceTypeNames[type]);
    return 1;
}

// Най-близкото място от даден вид в радиус (или -1)
stock Places_GetNearOfType(playerid, type, Float:range)
{
    if (GetPlayerVirtualWorld(playerid) != 0) return -1;
    for (new i = 0; i < PlaceCount; i++) {
        if (PlaceType[i] == type && IsPlayerInRangeOfPoint(playerid, range, PlacePos[i][0], PlacePos[i][1], PlacePos[i][2])) return i;
    }
    return -1;
}

// Най-близкият кантар (за delivery.inc, който се зарежда преди този файл)
stock Places_GetNearWeigh(playerid, Float:range)
{
    return Places_GetNearOfType(playerid, PLACE_WEIGH, range);
}

static stock Places_GetNear(playerid)
{
    if (GetPlayerVirtualWorld(playerid) != 0) return -1;
    for (new i = 0; i < PlaceCount; i++) {
        if (PlaceType[i] == PLACE_WEIGH) continue;
        if (IsPlayerInRangeOfPoint(playerid, PLACE_RANGE, PlacePos[i][0], PlacePos[i][1], PlacePos[i][2])) return i;
    }
    return -1;
}

// ---------- Нужди ----------

stock Needs_OnLogin(playerid)
{
    new q[96];
    format(q, sizeof (q), "SELECT hunger, thirst, bladder, fatigue FROM accounts WHERE id = %d", PlayerData[playerid][pID]);
    new DBResult:r = Database_Query(q);
    NeedHunger[playerid] = 100;
    NeedThirst[playerid] = 100;
    NeedBladder[playerid] = 0;
    if (r != DBResult:0) {
        if (DB_GetRowCount(r) > 0) {
            NeedHunger[playerid] = DB_GetFieldIntByName(r, "hunger");
            NeedThirst[playerid] = DB_GetFieldIntByName(r, "thirst");
            NeedBladder[playerid] = DB_GetFieldIntByName(r, "bladder");
            NeedFatigue[playerid] = DB_GetFieldIntByName(r, "fatigue");
        }
        DB_FreeResultSet(r);
    }
    NeedTick[playerid] = 0;
    for (new k = 0; k < 3; k++) NeedWarned[playerid][k] = false;
}

stock Needs_Save(playerid)
{
    if (!PlayerData[playerid][pLogged] || PlayerData[playerid][pID] <= 0) return;
    new q[160];
    format(q, sizeof (q), "UPDATE accounts SET hunger = %d, thirst = %d, bladder = %d, fatigue = %d WHERE id = %d",
        NeedHunger[playerid], NeedThirst[playerid], NeedBladder[playerid], NeedFatigue[playerid], PlayerData[playerid][pID]);
    Database_Exec(q);
}

static stock Needs_Clamp(playerid)
{
    if (NeedHunger[playerid] < 0) NeedHunger[playerid] = 0;
    if (NeedHunger[playerid] > 100) NeedHunger[playerid] = 100;
    if (NeedThirst[playerid] < 0) NeedThirst[playerid] = 0;
    if (NeedThirst[playerid] > 100) NeedThirst[playerid] = 100;
    if (NeedBladder[playerid] < 0) NeedBladder[playerid] = 0;
    if (NeedBladder[playerid] > 100) NeedBladder[playerid] = 100;
    if (NeedFatigue[playerid] < 0) NeedFatigue[playerid] = 0;
    if (NeedFatigue[playerid] > 100) NeedFatigue[playerid] = 100;
}

// Всяка секунда (за спаунати играчи)
stock Needs_Tick(playerid)
{
    new t = ++NeedTick[playerid];
    if (t % NEEDS_HUNGER_SECONDS == 0) NeedHunger[playerid]--;
    if (t % NEEDS_THIRST_SECONDS == 0) NeedThirst[playerid]--;
    if (t % NEEDS_BLADDER_SECONDS == 0) NeedBladder[playerid]++;
    Needs_Clamp(playerid);
    Places_WeighFreePass(playerid);

    // Предупреждения
    if (NeedHunger[playerid] <= NEEDS_WARN && !NeedWarned[playerid][0]) {
        NeedWarned[playerid][0] = true;
        SendClientMessage(playerid, COLOR_BRAND, "[НУЖДИ] " C_WHITE "Гладен си. Купи храна от магазин, закусвалня или бензиностанция (ALT).");
    } else if (NeedHunger[playerid] > NEEDS_WARN) NeedWarned[playerid][0] = false;
    if (NeedThirst[playerid] <= NEEDS_WARN && !NeedWarned[playerid][1]) {
        NeedWarned[playerid][1] = true;
        SendClientMessage(playerid, COLOR_BRAND, "[НУЖДИ] " C_WHITE "Жаден си. Купи нещо за пиене (ALT в магазин или на бензиностанция).");
    } else if (NeedThirst[playerid] > NEEDS_WARN) NeedWarned[playerid][1] = false;
    if (NeedBladder[playerid] >= 100 - NEEDS_WARN && !NeedWarned[playerid][2]) {
        NeedWarned[playerid][2] = true;
        SendClientMessage(playerid, COLOR_BRAND, "[НУЖДИ] " C_WHITE "Трябва ти тоалетна! Бензиностанция, обществена тоалетна или гаража (ALT).");
    } else if (NeedBladder[playerid] < 100 - NEEDS_WARN) NeedWarned[playerid][2] = false;

    // Последствия
    if ((NeedHunger[playerid] == 0 || NeedThirst[playerid] == 0) && t % NEEDS_DAMAGE_SECONDS == 0) {
        new Float:hp;
        GetPlayerHealth(playerid, hp);
        if (hp > 10.0) SetPlayerHealth(playerid, hp - 2.0);
    }
    if (NeedBladder[playerid] >= 100) {
        NeedBladder[playerid] = 0;
        Money_Take(playerid, NEEDS_ACCIDENT_FINE);
        GameTextForPlayer(playerid, "~y~OOPS...", 3000, 1);
        SendClientMessage(playerid, COLOR_RED, "[НУЖДИ] " C_WHITE "Не издържа до тоалетна... Глоба за замърсяване: $%d.", NEEDS_ACCIDENT_FINE);
    }
    Fatigue_Tick(playerid, t);
    Needs_UpdateHud(playerid);
}

// ---------- HUD: иконки под парите (вдясно горе) ----------
// Храна, вода, тоалетна, умора: иконка + стойност. Червено, когато е зле.

#define NEEDS_HUD_X     492.0
#define NEEDS_HUD_Y     98.0
#define NEEDS_HUD_STEP  22.0

static PlayerText:NeedsIcon[MAX_PLAYERS][4];
static PlayerText:NeedsValue[MAX_PLAYERS][4];
static bool:NeedsHudOn[MAX_PLAYERS];

// Иконите са от картата на играта (hud.txd); тоалетната и умората са надписи
static const NeedsSprites[4][] = {"hud:radar_burgerShot", "hud:radar_dateDrink", "", ""};
static const NeedsLabels[4][] = {"", "", "WC", "Zz"};
static const NeedsLabelColours[4] = {0, 0, 0x8FC8FFFF, 0xC8A8FFFF};

stock Needs_CreateHud(playerid)
{
    Needs_DestroyHud(playerid);
    for (new k = 0; k < 4; k++) {
        new Float:x = NEEDS_HUD_X + k * NEEDS_HUD_STEP;
        new PlayerText:icon;
        if (NeedsSprites[k][0] != EOS) {
            icon = CreatePlayerTextDraw(playerid, x, NEEDS_HUD_Y, NeedsSprites[k]);
            PlayerTextDrawFont(playerid, icon, TEXT_DRAW_FONT_SPRITE_DRAW);
            PlayerTextDrawTextSize(playerid, icon, 9.0, 9.0);
            PlayerTextDrawColour(playerid, icon, 0xFFFFFFFF);
        } else {
            icon = CreatePlayerTextDraw(playerid, x, NEEDS_HUD_Y, NeedsLabels[k]);
            PlayerTextDrawFont(playerid, icon, TEXT_DRAW_FONT_2);
            PlayerTextDrawLetterSize(playerid, icon, 0.16, 0.9);
            PlayerTextDrawColour(playerid, icon, NeedsLabelColours[k]);
            PlayerTextDrawSetOutline(playerid, icon, 1);
            PlayerTextDrawBackgroundColour(playerid, icon, 0x000000AA);
            PlayerTextDrawSetProportional(playerid, icon, true);
        }
        NeedsIcon[playerid][k] = icon;

        new PlayerText:t = CreatePlayerTextDraw(playerid, x + 4.5, NEEDS_HUD_Y + 9.5, "_");
        PlayerTextDrawFont(playerid, t, TEXT_DRAW_FONT_2);
        PlayerTextDrawLetterSize(playerid, t, 0.15, 0.75);
        PlayerTextDrawAlignment(playerid, t, TEXT_DRAW_ALIGN_CENTRE);
        PlayerTextDrawColour(playerid, t, COLOR_WHITE);
        PlayerTextDrawSetOutline(playerid, t, 1);
        PlayerTextDrawSetShadow(playerid, t, 0);
        PlayerTextDrawBackgroundColour(playerid, t, 0x000000AA);
        PlayerTextDrawSetProportional(playerid, t, true);
        NeedsValue[playerid][k] = t;
    }
    NeedsHudOn[playerid] = true;
    Needs_UpdateHud(playerid);
    for (new k = 0; k < 4; k++) {
        PlayerTextDrawShow(playerid, NeedsIcon[playerid][k]);
        PlayerTextDrawShow(playerid, NeedsValue[playerid][k]);
    }
}

stock Needs_DestroyHud(playerid)
{
    if (!NeedsHudOn[playerid]) return;
    for (new k = 0; k < 4; k++) {
        PlayerTextDrawDestroy(playerid, NeedsIcon[playerid][k]);
        PlayerTextDrawDestroy(playerid, NeedsValue[playerid][k]);
    }
    NeedsHudOn[playerid] = false;
}

static stock Needs_UpdateHud(playerid)
{
    if (!NeedsHudOn[playerid]) return;
    new values[4], bool:bad[4], text[16];
    values[0] = NeedHunger[playerid];  bad[0] = (values[0] <= NEEDS_WARN);
    values[1] = NeedThirst[playerid];  bad[1] = (values[1] <= NEEDS_WARN);
    values[2] = NeedBladder[playerid]; bad[2] = (values[2] >= 100 - NEEDS_WARN);
    values[3] = NeedFatigue[playerid]; bad[3] = (values[3] >= FATIGUE_WARN);
    for (new k = 0; k < 4; k++) {
        format(text, sizeof (text), "%s%d", bad[k] ? ("~r~") : ("~w~"), values[k]);
        PlayerTextDrawSetString(playerid, NeedsValue[playerid][k], text);
    }
}

// ---------- Умора ----------

// Вика се всяка секунда от Needs_Tick
static stock Fatigue_Tick(playerid, t)
{
    if (Resting[playerid]) return;
    new vehicleid = GetPlayerVehicleID(playerid);
    if (vehicleid != 0 && GetPlayerState(playerid) == PLAYER_STATE_DRIVER && Engine_IsOn(vehicleid)) {
        new Float:vx, Float:vy, Float:vz;
        GetVehicleVelocity(vehicleid, vx, vy, vz);
        if (VectorSize(vx, vy, vz) * 180.0 > 10.0 && ++FatigueDrive[playerid] >= FATIGUE_DRIVE_SECONDS) {
            FatigueDrive[playerid] = 0;
            NeedFatigue[playerid]++;
        }
    } else if (vehicleid == 0 && ++FatigueRestTick[playerid] >= FATIGUE_FOOT_SECONDS) {
        FatigueRestTick[playerid] = 0;
        NeedFatigue[playerid]--;
    }
    Needs_Clamp(playerid);

    new f = NeedFatigue[playerid];
    if (f >= FATIGUE_WARN && t % 120 == 0) {
        SendClientMessage(playerid, COLOR_BRAND, "[УМОРА] " C_WHITE "Уморен си (%d%%). Почини в гаража, на бензиностанция или паркинг (ALT), или изпий кафе.", f);
    }
    if (vehicleid != 0 && f >= FATIGUE_SWAY && t % 5 == 0) {
        SetPlayerDrunkLevel(playerid, (f >= FATIGUE_SEVERE) ? 6000 : 2600);
        if (f >= FATIGUE_SEVERE && t % 30 == 0) GameTextForPlayer(playerid, "~r~WAKE UP!", 1500, 3);
    }
}

stock Fatigue_StartRest(playerid)
{
    if (Resting[playerid]) return;
    new vehicleid = GetPlayerVehicleID(playerid);
    if (vehicleid != 0 && Engine_IsOn(vehicleid)) {
        SendError(playerid, "Изгаси двигателя (CTRL), за да почиваш.");
        return;
    }
    if (NeedFatigue[playerid] < 5) {
        SendInfo(playerid, "Не си уморен.");
        return;
    }
    Resting[playerid] = true;
    TogglePlayerControllable(playerid, false);
    GameTextForPlayer(playerid, "~b~RESTING...", REST_SECONDS * 1000, 3);
    SetTimerEx("Fatigue_RestDone", REST_SECONDS * 1000, false, "ii", playerid, PlayerData[playerid][pConnSeq]);
}

forward Fatigue_RestDone(playerid, seq);
public Fatigue_RestDone(playerid, seq)
{
    if (!IsPlayerConnected(playerid) || PlayerData[playerid][pConnSeq] != seq) return 0;
    Resting[playerid] = false;
    TogglePlayerControllable(playerid, true);
    NeedFatigue[playerid] = 0;
    FatigueDrive[playerid] = 0;
    SetPlayerDrunkLevel(playerid, 0);
    Needs_UpdateHud(playerid);
    SendClientMessage(playerid, COLOR_BRAND, "[УМОРА] " C_WHITE "Отпочинал си. Приятен път!");
    return 1;
}

stock Fatigue_Get(playerid)
{
    return NeedFatigue[playerid];
}

// ---------- Пазаруване и тоалетна ----------

// kind: PLACE_* или SHOP_KIND_STATION / SHOP_KIND_GARAGE
stock Needs_ShowShop(playerid, kind)
{
    ShopKind[playerid] = kind;
    new body[900], line[128], n = 0;
    body[0] = EOS;
    if (kind == PLACE_TOILET) {
        Needs_UseToilet(playerid, TOILET_PRICE);
        return;
    }
    new where = (kind == SHOP_KIND_STATION || kind == SHOP_KIND_GARAGE) ? (1 << PLACE_SHOP) : (1 << kind);
    for (new i = 0; i < sizeof (ShopItems) && n < sizeof (ShopMap[]) - 1; i++) {
        if (!(ShopItems[i][itWhere] & where)) continue;
        // В гаража (хладилника) са само вода и сандвич, безплатно
        if (kind == SHOP_KIND_GARAGE && i != 0 && i != 3) continue;
        new price = (kind == SHOP_KIND_GARAGE) ? 0 : ShopItems[i][itPrice];
        new effect[48];
        if (ShopItems[i][itFood] > 0 && ShopItems[i][itDrink] > 0) format(effect, sizeof (effect), "храна +%d, жажда +%d", ShopItems[i][itFood], ShopItems[i][itDrink]);
        else if (ShopItems[i][itFood] > 0) format(effect, sizeof (effect), "храна +%d", ShopItems[i][itFood]);
        else if (ShopItems[i][itRest] > 0) format(effect, sizeof (effect), "жажда +%d, умора -%d", ShopItems[i][itDrink], ShopItems[i][itRest]);
        else format(effect, sizeof (effect), "жажда +%d", ShopItems[i][itDrink]);
        if (price == 0) format(line, sizeof (line), "%s\t" C_GREY "%s\t" C_GREEN "безплатно\n", ShopItems[i][itName], effect);
        else format(line, sizeof (line), "%s\t" C_GREY "%s\t" C_GREEN "$%d\n", ShopItems[i][itName], effect, price);
        strcat(body, line);
        ShopMap[playerid][n++] = i;
    }
    if (kind == SHOP_KIND_STATION || kind == SHOP_KIND_GARAGE) {
        if (kind == SHOP_KIND_GARAGE) format(line, sizeof (line), C_BRAND "Тоалетна\t" C_GREY "тоалетна 0\t" C_GREEN "безплатно\n");
        else format(line, sizeof (line), C_BRAND "Тоалетна\t" C_GREY "тоалетна 0\t" C_GREEN "$%d\n", TOILET_PRICE);
        strcat(body, line);
        ShopMap[playerid][n++] = -1;
        format(line, sizeof (line), C_BRAND "Почивка\t" C_GREY "умора 0 (%d сек)\t" C_GREEN "безплатно\n", REST_SECONDS);
        strcat(body, line);
        ShopMap[playerid][n] = -2;
    }
    new title[64];
    switch (kind) {
        case SHOP_KIND_STATION: format(title, sizeof (title), C_BRAND "Магазин на бензиностанцията");
        case SHOP_KIND_GARAGE:  format(title, sizeof (title), C_BRAND "Гараж: хладилник и тоалетна");
        default:                format(title, sizeof (title), C_BRAND "%s", PlaceTypeNames[kind]);
    }
    ShowPlayerDialog(playerid, DIALOG_SHOP, DIALOG_STYLE_TABLIST, title, "%s", "Купи", "Затвори", body);
}

static stock Needs_UseToilet(playerid, price)
{
    if (NeedBladder[playerid] < 5) { SendInfo(playerid, "Не ти трябва тоалетна в момента."); return 0; }
    if (price > 0 && !Money_Take(playerid, price)) { SendError(playerid, "Нямаш пари за тоалетна."); return 0; }
    NeedBladder[playerid] = 0;
    NeedWarned[playerid][2] = false;
    Needs_UpdateHud(playerid);
    SendClientMessage(playerid, COLOR_BRAND, "[НУЖДИ] " C_WHITE "Облекчение!");
    return 1;
}

static stock Needs_Buy(playerid, item)
{
    new price = (ShopKind[playerid] == SHOP_KIND_GARAGE) ? 0 : ShopItems[item][itPrice];
    if (price > 0 && !Money_Take(playerid, price)) { SendError(playerid, "Нямаш достатъчно пари."); return 0; }
    NeedHunger[playerid] += ShopItems[item][itFood];
    NeedThirst[playerid] += ShopItems[item][itDrink];
    NeedBladder[playerid] += ShopItems[item][itBladder];
    NeedFatigue[playerid] -= ShopItems[item][itRest];
    Needs_Clamp(playerid);
    Needs_UpdateHud(playerid);
    if (ShopItems[item][itFood] > 0) PlayerPlaySound(playerid, 32200, 0.0, 0.0, 0.0);
    else PlayerPlaySound(playerid, 42600, 0.0, 0.0, 0.0);
    SendClientMessage(playerid, COLOR_BRAND, "[НУЖДИ] " C_WHITE "%s. Храна %d%%, жажда %d%%.", ShopItems[item][itName], NeedHunger[playerid], NeedThirst[playerid]);
    return 1;
}

// ALT пеша: място, бензиностанция, гараж
stock Needs_OnKeyStateChange(playerid, KEY:newkeys, KEY:oldkeys)
{
    if (!((newkeys & KEY_WALK) && !(oldkeys & KEY_WALK))) return 0;
    if (!PlayerData[playerid][pLogged] || IsPlayerInAnyVehicle(playerid)) return 0;

    if (PlayerData[playerid][pInGarage]) {
        Needs_ShowShop(playerid, SHOP_KIND_GARAGE);
        return 1;
    }
    new p = Places_GetNear(playerid);
    if (p != -1) {
        if (PlaceType[p] == PLACE_REST) Fatigue_StartRest(playerid);
        else Needs_ShowShop(playerid, PlaceType[p]);
        return 1;
    }
    if (Fuel_IsNearStation(playerid)) {
        Needs_ShowShop(playerid, SHOP_KIND_STATION);
        return 1;
    }
    return 0;
}

stock Needs_OnDialogResponse(playerid, dialogid, response, listitem)
{
    if (dialogid != DIALOG_SHOP) return 0;
    if (!response || listitem < 0 || listitem >= sizeof (ShopMap[])) return 1;
    new item = ShopMap[playerid][listitem];
    if (item == -1) {
        Needs_UseToilet(playerid, (ShopKind[playerid] == SHOP_KIND_GARAGE) ? 0 : TOILET_PRICE);
        return 1;
    }
    if (item == -2) {
        Fatigue_StartRest(playerid);
        return 1;
    }
    Needs_Buy(playerid, item);
    Needs_ShowShop(playerid, ShopKind[playerid]);
    return 1;
}

stock Needs_OnPlayerConnect(playerid)
{
    NeedsHudOn[playerid] = false;
    NeedFatigue[playerid] = 0;
    FatigueDrive[playerid] = 0;
    FatigueRestTick[playerid] = 0;
    Resting[playerid] = false;
    NeedHunger[playerid] = 100;
    NeedThirst[playerid] = 100;
    NeedBladder[playerid] = 0;
}

stock Needs_Get(playerid, &hunger, &thirst, &bladder)
{
    hunger = NeedHunger[playerid];
    thirst = NeedThirst[playerid];
    bladder = NeedBladder[playerid];
}

stock Needs_Set(playerid, hunger, thirst, bladder)
{
    NeedHunger[playerid] = hunger;
    NeedThirst[playerid] = thirst;
    NeedBladder[playerid] = bladder;
    Needs_Clamp(playerid);
    Needs_UpdateHud(playerid);
}
