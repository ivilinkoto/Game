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
#define MAX_PLACES              80

enum {
    PLACE_SHOP,             // магазин: всичко
    PLACE_DINER,            // закусвалня: храна + безалкохолни
    PLACE_CAFE,             // кафене: напитки + сандвич
    PLACE_TOILET,           // обществена тоалетна
    MAX_PLACE_TYPES
}

stock const PlaceTypeNames[MAX_PLACE_TYPES][] = {"Магазин", "Закусвалня", "Кафене", "Обществена тоалетна"};
static const PlaceIcons[MAX_PLACE_TYPES] = {17, 10, 17, 0};

// Артикули: име, цена, +глад, +жажда, +тоалетна, в кои места (битове PLACE_*)
enum E_ITEM {
    itName[32],
    itPrice,
    itFood,
    itDrink,
    itBladder,
    itWhere
}
static const ShopItems[][E_ITEM] = {
    {"Сандвич",          15, 30,  0,  0, (1 << PLACE_SHOP) | (1 << PLACE_DINER) | (1 << PLACE_CAFE)},
    {"Бургер",           25, 45,  0,  0, (1 << PLACE_SHOP) | (1 << PLACE_DINER)},
    {"Пица",             40, 70,  0,  0, (1 << PLACE_DINER)},
    {"Вода",              5,  0, 35, 10, (1 << PLACE_SHOP) | (1 << PLACE_DINER) | (1 << PLACE_CAFE)},
    {"Кола",              6,  0, 30, 12, (1 << PLACE_SHOP) | (1 << PLACE_DINER)},
    {"Кафе",              8,  0, 20, 10, (1 << PLACE_SHOP) | (1 << PLACE_CAFE)},
    {"Енергийна напитка", 12, 5, 30, 15, (1 << PLACE_SHOP) | (1 << PLACE_CAFE)}
};

// Специалните "места" при показване на менюто
#define SHOP_KIND_STATION   100     // бензиностанция = магазин + тоалетна
#define SHOP_KIND_GARAGE    101     // гараж: хладилник (безплатна вода и сандвич) + тоалетна

static Float:PlacePos[MAX_PLACES][3];
static PlaceType[MAX_PLACES];
static PlaceID[MAX_PLACES];
static Text3D:PlaceLabel[MAX_PLACES] = {Text3D:-1, ...};
static PlaceIcon[MAX_PLACES] = {-1, ...};
static PlaceCount;

static NeedHunger[MAX_PLAYERS], NeedThirst[MAX_PLAYERS], NeedBladder[MAX_PLAYERS];
static NeedTick[MAX_PLAYERS];
static bool:NeedWarned[MAX_PLAYERS][3];
static ShopKind[MAX_PLAYERS];
static ShopMap[MAX_PLAYERS][16];

static PlayerText:NeedsTD[MAX_PLAYERS] = {INVALID_PLAYER_TEXT_DRAW, ...};

// ---------- Места ----------

stock Places_Init()
{
    Database_Exec("CREATE TABLE IF NOT EXISTS places (id INTEGER PRIMARY KEY AUTOINCREMENT, type INTEGER NOT NULL, x REAL NOT NULL, y REAL NOT NULL, z REAL NOT NULL)");
    Database_AddColumn("accounts", "hunger", "INTEGER NOT NULL DEFAULT 100");
    Database_AddColumn("accounts", "thirst", "INTEGER NOT NULL DEFAULT 100");
    Database_AddColumn("accounts", "bladder", "INTEGER NOT NULL DEFAULT 0");
    Places_Load();
}

static stock Places_Load()
{
    for (new i = 0; i < PlaceCount; i++) {
        if (PlaceLabel[i] != Text3D:-1) DestroyDynamic3DTextLabel(PlaceLabel[i]);
        if (PlaceIcon[i] != -1) DestroyDynamicMapIcon(PlaceIcon[i]);
        PlaceLabel[i] = Text3D:-1;
        PlaceIcon[i] = -1;
    }
    PlaceCount = 0;
    new DBResult:r = Database_Query("SELECT id, type, x, y, z FROM places ORDER BY id");
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
        format(text, sizeof (text), C_BRAND "%s\n" C_GREY "ALT", PlaceTypeNames[PlaceType[k]]);
        PlaceLabel[k] = CreateDynamic3DTextLabel(text, COLOR_WHITE, PlacePos[k][0], PlacePos[k][1], PlacePos[k][2] + 0.6, 20.0,
            INVALID_PLAYER_ID, INVALID_VEHICLE_ID, 1, 0, 0);
        if (PlaceIcons[PlaceType[k]] != 0) {
            PlaceIcon[k] = CreateDynamicMapIcon(PlacePos[k][0], PlacePos[k][1], PlacePos[k][2], PlaceIcons[PlaceType[k]], 0, 0, 0, -1, MAP_ICON_RANGE, MAPICON_LOCAL);
        }
        DB_SelectNextRow(r);
    }
    DB_FreeResultSet(r);
}

stock Places_AddHere(playerid, type)
{
    if (type < 0 || type >= MAX_PLACE_TYPES) return 0;
    if (PlaceCount >= MAX_PLACES) { SendError(playerid, "Достигнат е максимумът места."); return 0; }
    new Float:x, Float:y, Float:z, q[160];
    GetPlayerPos(playerid, x, y, z);
    format(q, sizeof (q), "INSERT INTO places (type, x, y, z) VALUES (%d, %f, %f, %f)", type, x, y, z);
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

static stock Places_GetNear(playerid)
{
    if (GetPlayerVirtualWorld(playerid) != 0) return -1;
    for (new i = 0; i < PlaceCount; i++) {
        if (IsPlayerInRangeOfPoint(playerid, PLACE_RANGE, PlacePos[i][0], PlacePos[i][1], PlacePos[i][2])) return i;
    }
    return -1;
}

// ---------- Нужди ----------

stock Needs_OnLogin(playerid)
{
    new q[96];
    format(q, sizeof (q), "SELECT hunger, thirst, bladder FROM accounts WHERE id = %d", PlayerData[playerid][pID]);
    new DBResult:r = Database_Query(q);
    NeedHunger[playerid] = 100;
    NeedThirst[playerid] = 100;
    NeedBladder[playerid] = 0;
    if (r != DBResult:0) {
        if (DB_GetRowCount(r) > 0) {
            NeedHunger[playerid] = DB_GetFieldIntByName(r, "hunger");
            NeedThirst[playerid] = DB_GetFieldIntByName(r, "thirst");
            NeedBladder[playerid] = DB_GetFieldIntByName(r, "bladder");
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
    format(q, sizeof (q), "UPDATE accounts SET hunger = %d, thirst = %d, bladder = %d WHERE id = %d",
        NeedHunger[playerid], NeedThirst[playerid], NeedBladder[playerid], PlayerData[playerid][pID]);
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
}

// Всяка секунда (за спаунати играчи)
stock Needs_Tick(playerid)
{
    new t = ++NeedTick[playerid];
    if (t % NEEDS_HUNGER_SECONDS == 0) NeedHunger[playerid]--;
    if (t % NEEDS_THIRST_SECONDS == 0) NeedThirst[playerid]--;
    if (t % NEEDS_BLADDER_SECONDS == 0) NeedBladder[playerid]++;
    Needs_Clamp(playerid);

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
    Needs_UpdateHud(playerid);
}

// ---------- HUD ----------

stock Needs_CreateHud(playerid)
{
    if (NeedsTD[playerid] != INVALID_PLAYER_TEXT_DRAW) PlayerTextDrawDestroy(playerid, NeedsTD[playerid]);
    new PlayerText:t = CreatePlayerTextDraw(playerid, 320.0, 384.0, "_");
    PlayerTextDrawFont(playerid, t, TEXT_DRAW_FONT_2);
    PlayerTextDrawLetterSize(playerid, t, 0.2, 0.9);
    PlayerTextDrawAlignment(playerid, t, TEXT_DRAW_ALIGN_CENTRE);
    PlayerTextDrawColour(playerid, t, COLOR_WHITE);
    PlayerTextDrawSetOutline(playerid, t, 1);
    PlayerTextDrawSetShadow(playerid, t, 0);
    PlayerTextDrawBackgroundColour(playerid, t, 0x000000AA);
    PlayerTextDrawSetProportional(playerid, t, true);
    NeedsTD[playerid] = t;
    Needs_UpdateHud(playerid);
    PlayerTextDrawShow(playerid, t);
}

stock Needs_DestroyHud(playerid)
{
    if (NeedsTD[playerid] != INVALID_PLAYER_TEXT_DRAW) PlayerTextDrawDestroy(playerid, NeedsTD[playerid]);
    NeedsTD[playerid] = INVALID_PLAYER_TEXT_DRAW;
}

static stock Needs_UpdateHud(playerid)
{
    if (NeedsTD[playerid] == INVALID_PLAYER_TEXT_DRAW) return;
    new text[96];
    format(text, sizeof (text), "%sFOOD %d%%   %sWATER %d%%   %sWC %d%%",
        (NeedHunger[playerid] <= NEEDS_WARN) ? ("~r~") : ("~w~"), NeedHunger[playerid],
        (NeedThirst[playerid] <= NEEDS_WARN) ? ("~r~") : ("~w~"), NeedThirst[playerid],
        (NeedBladder[playerid] >= 100 - NEEDS_WARN) ? ("~r~") : ("~w~"), NeedBladder[playerid]);
    PlayerTextDrawSetString(playerid, NeedsTD[playerid], text);
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
        Needs_ShowShop(playerid, PlaceType[p]);
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
    Needs_Buy(playerid, item);
    Needs_ShowShop(playerid, ShopKind[playerid]);
    return 1;
}

stock Needs_OnPlayerConnect(playerid)
{
    NeedsTD[playerid] = INVALID_PLAYER_TEXT_DRAW;
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
