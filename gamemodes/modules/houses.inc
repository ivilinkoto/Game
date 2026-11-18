// Жилища: къщи, които играчите купуват.
//
// - Админ ги поставя (Места на картата -> "Нова къща тук"): вратата, цена и интериор.
// - Отвън: зелена къщичка = за продан, синя = има собственик. ALT пеша на вратата:
//   влизаш (ако е твоя или е отключена) или виждаш офертата и купуваш.
// - Вътре: ALT до изхода - навън; другаде - хладилник, тоалетна и легло (безплатно).
// - Меню > Моят дом: GPS, заключване, спаун в дома, данък, продажба (60% от цената).
// - PayDay: собствениците не получават социална помощ, а плащат данък (0.1% от цената).
//   След 24 неплатени данъка къщата се връща на държавата.
// - Един играч - едно жилище.

#define MAX_HOUSES              300
#define HOUSE_WORLD_BASE        20000           // виртуален свят = база + ID на къщата
#define HOUSE_DOOR_RANGE        1.6
#define HOUSE_EXIT_RANGE        1.8
#define HOUSE_SELL_PERCENT      60
#define HOUSE_TAX_PERMILLE      1               // данък на PayDay: 0.1% от цената
#define HOUSE_TAX_MIN           10
#define HOUSE_MAX_UNPAID        24
#define HOUSE_PICKUP_SALE       1273            // зелена къщичка
#define HOUSE_PICKUP_OWNED      1272            // синя къщичка

// Интериори от играта: име, интериор, x, y, z, ъгъл (входът отвътре)
enum E_HOUSE_INT {
    hiName[24],
    hiInterior,
    Float:hiX,
    Float:hiY,
    Float:hiZ,
    Float:hiA
}
stock const HouseInteriors[][E_HOUSE_INT] = {
    {"Малка къща",   1,  223.20, 1287.08, 1082.14, 0.0},
    {"Апартамент",   5, 2233.80, -1115.36, 1050.88, 0.0},
    {"Средна къща",  8, 2365.31, -1135.60, 1050.88, 0.0},
    {"Уютна къща",  12, 2324.53, -1149.54, 1050.71, 0.0},
    {"Голяма къща",  3,  235.34, 1186.68, 1080.26, 0.0},
    {"Имение",       7,  225.68, 1021.45, 1084.02, 0.0}
};

enum E_HOUSE {
    hID,
    Float:hX,
    Float:hY,
    Float:hZ,
    Float:hA,
    hPrice,
    hInt,                       // индекс в HouseInteriors
    hOwner,                     // ID на акаунта (0 = за продан)
    hOwnerName[MAX_PLAYER_NAME + 1],
    bool:hLocked,
    bool:hSpawnHome,
    hUnpaid
}
static House[MAX_HOUSES][E_HOUSE];
static HouseCount;
static HousePickup[MAX_HOUSES] = {-1, ...};
static Text3D:HouseLabel[MAX_HOUSES] = {Text3D:-1, ...};
static HouseIcon[MAX_HOUSES] = {-1, ...};
static HouseOffer[MAX_PLAYERS];           // коя къща разглежда (диалог за покупка)

// ---------- Зареждане ----------

stock Houses_Init()
{
    Database_Exec("CREATE TABLE IF NOT EXISTS houses (id INTEGER PRIMARY KEY AUTOINCREMENT, x REAL NOT NULL, y REAL NOT NULL, z REAL NOT NULL, a REAL NOT NULL, price INTEGER NOT NULL, interior INTEGER NOT NULL, owner INTEGER NOT NULL DEFAULT 0, owner_name TEXT NOT NULL DEFAULT '', locked INTEGER NOT NULL DEFAULT 1, spawn_home INTEGER NOT NULL DEFAULT 0, unpaid INTEGER NOT NULL DEFAULT 0)");
    Houses_Load();
}

static stock Houses_Load()
{
    for (new k = 0; k < HouseCount; k++) House_DestroyMarkers(k);
    HouseCount = 0;
    new DBResult:r = Database_Query("SELECT * FROM houses ORDER BY id");
    if (r == DBResult:0) return;
    new rows = DB_GetRowCount(r);
    for (new i = 0; i < rows && HouseCount < MAX_HOUSES; i++) {
        new k = HouseCount++;
        House[k][hID] = DB_GetFieldIntByName(r, "id");
        House[k][hX] = DB_GetFieldFloatByName(r, "x");
        House[k][hY] = DB_GetFieldFloatByName(r, "y");
        House[k][hZ] = DB_GetFieldFloatByName(r, "z");
        House[k][hA] = DB_GetFieldFloatByName(r, "a");
        House[k][hPrice] = DB_GetFieldIntByName(r, "price");
        House[k][hInt] = DB_GetFieldIntByName(r, "interior");
        if (House[k][hInt] < 0 || House[k][hInt] >= sizeof (HouseInteriors)) House[k][hInt] = 0;
        House[k][hOwner] = DB_GetFieldIntByName(r, "owner");
        DB_GetFieldStringByName(r, "owner_name", House[k][hOwnerName], MAX_PLAYER_NAME + 1);
        House[k][hLocked] = (DB_GetFieldIntByName(r, "locked") != 0);
        House[k][hSpawnHome] = (DB_GetFieldIntByName(r, "spawn_home") != 0);
        House[k][hUnpaid] = DB_GetFieldIntByName(r, "unpaid");
        House_CreateMarkers(k);
        DB_SelectNextRow(r);
    }
    DB_FreeResultSet(r);
}

static stock House_Save(k)
{
    new q[256], safe[64];
    SQL_Escape(House[k][hOwnerName], safe);
    format(q, sizeof (q), "UPDATE houses SET owner = %d, owner_name = '%s', locked = %d, spawn_home = %d, unpaid = %d, price = %d WHERE id = %d",
        House[k][hOwner], safe, House[k][hLocked] ? 1 : 0, House[k][hSpawnHome] ? 1 : 0, House[k][hUnpaid], House[k][hPrice], House[k][hID]);
    Database_Exec(q);
}

static stock House_DestroyMarkers(k)
{
    if (HousePickup[k] != -1) DestroyDynamicPickup(HousePickup[k]);
    if (HouseLabel[k] != Text3D:-1) DestroyDynamic3DTextLabel(HouseLabel[k]);
    if (HouseIcon[k] != -1) DestroyDynamicMapIcon(HouseIcon[k]);
    HousePickup[k] = -1;
    HouseLabel[k] = Text3D:-1;
    HouseIcon[k] = -1;
}

static stock House_CreateMarkers(k)
{
    House_DestroyMarkers(k);
    new text[192], money[16];
    FormatMoney(House[k][hPrice], money);
    if (House[k][hOwner] == 0) {
        format(text, sizeof (text), C_GREEN "ЗА ПРОДАН\n" C_WHITE "%s №%d\n" C_GREEN "$%s\n" C_GREY "ALT - разгледай",
            HouseInteriors[House[k][hInt]][hiName], House[k][hID], money);
    } else {
        format(text, sizeof (text), C_BRAND "%s №%d\n" C_WHITE "Собственик: %s\n" C_GREY "ALT - влез",
            HouseInteriors[House[k][hInt]][hiName], House[k][hID], House[k][hOwnerName]);
    }
    HousePickup[k] = CreateDynamicPickup((House[k][hOwner] == 0) ? HOUSE_PICKUP_SALE : HOUSE_PICKUP_OWNED, 1, House[k][hX], House[k][hY], House[k][hZ], 0, 0);
    HouseLabel[k] = CreateDynamic3DTextLabel(text, COLOR_WHITE, House[k][hX], House[k][hY], House[k][hZ] + 0.7, 15.0,
        INVALID_PLAYER_ID, INVALID_VEHICLE_ID, 1, 0, 0);
    HouseIcon[k] = CreateDynamicMapIcon(House[k][hX], House[k][hY], House[k][hZ], (House[k][hOwner] == 0) ? 31 : 32, 0, 0, 0, -1, MAP_ICON_RANGE, MAPICON_LOCAL);
}

// ---------- Помощни ----------

static stock House_World(k)
{
    return HOUSE_WORLD_BASE + House[k][hID];
}

// Къщата на играча (-1 = няма)
stock House_GetOwned(playerid)
{
    new acc = PlayerData[playerid][pID];
    if (acc <= 0) return -1;
    for (new k = 0; k < HouseCount; k++) {
        if (House[k][hOwner] == acc) return k;
    }
    return -1;
}

// В коя къща е играчът (-1 = в никоя)
stock House_GetInside(playerid)
{
    new w = GetPlayerVirtualWorld(playerid) - HOUSE_WORLD_BASE;
    if (w <= 0) return -1;
    for (new k = 0; k < HouseCount; k++) {
        if (House[k][hID] == w) return k;
    }
    return -1;
}

static stock House_GetNearDoor(playerid)
{
    if (GetPlayerVirtualWorld(playerid) != 0 || GetPlayerInterior(playerid) != 0) return -1;
    for (new k = 0; k < HouseCount; k++) {
        if (IsPlayerInRangeOfPoint(playerid, HOUSE_DOOR_RANGE, House[k][hX], House[k][hY], House[k][hZ])) return k;
    }
    return -1;
}

static stock House_FindOnlineOwner(k)
{
    if (House[k][hOwner] == 0) return INVALID_PLAYER_ID;
    for (new i = 0; i < MAX_PLAYERS; i++) {
        if (IsPlayerLogged(i) && PlayerData[i][pID] == House[k][hOwner]) return i;
    }
    return INVALID_PLAYER_ID;
}

// Пари на собственика (онлайн или офлайн)
static stock House_PayOwner(k, amount)
{
    new owner = House_FindOnlineOwner(k);
    if (owner != INVALID_PLAYER_ID) {
        Money_Give(owner, amount);
        return;
    }
    new q[128];
    format(q, sizeof (q), "UPDATE accounts SET money = money + %d WHERE id = %d", amount, House[k][hOwner]);
    Database_Exec(q);
}

static stock House_Tax(k)
{
    new tax = House[k][hPrice] * HOUSE_TAX_PERMILLE / 1000;
    return (tax < HOUSE_TAX_MIN) ? HOUSE_TAX_MIN : tax;
}

static stock House_Release(k)
{
    House[k][hOwner] = 0;
    House[k][hOwnerName][0] = EOS;
    House[k][hLocked] = true;
    House[k][hSpawnHome] = false;
    House[k][hUnpaid] = 0;
    House_Save(k);
    House_CreateMarkers(k);
}

// ---------- Вход и изход ----------

static stock House_Enter(playerid, k)
{
    new i = House[k][hInt];
    Gps_Cancel(playerid);
    SetPlayerInterior(playerid, HouseInteriors[i][hiInterior]);
    SetPlayerVirtualWorld(playerid, House_World(k));
    SetPlayerPos(playerid, HouseInteriors[i][hiX], HouseInteriors[i][hiY], HouseInteriors[i][hiZ]);
    SetPlayerFacingAngle(playerid, HouseInteriors[i][hiA]);
    SetCameraBehindPlayer(playerid);
    if (House[k][hOwner] == PlayerData[playerid][pID]) GameTextForPlayer(playerid, "~g~HOME SWEET HOME", 2500, 1);
}

static stock House_Exit(playerid, k)
{
    new Float:a = House[k][hA];
    SetPlayerInterior(playerid, 0);
    SetPlayerVirtualWorld(playerid, 0);
    SetPlayerPos(playerid, House[k][hX] + 1.2 * floatsin(-a, degrees), House[k][hY] + 1.2 * floatcos(-a, degrees), House[k][hZ]);
    SetPlayerFacingAngle(playerid, a);
    SetCameraBehindPlayer(playerid);
}

// Спаун в дома (вместо в гаража). 1 = спаунат в дома
stock House_SpawnAtHome(playerid)
{
    new k = House_GetOwned(playerid);
    if (k == -1 || !House[k][hSpawnHome]) return 0;
    House_Enter(playerid, k);
    return 1;
}

// ---------- Оферта и покупка ----------

static stock House_ShowOffer(playerid, k)
{
    new money[16], tax[16];
    FormatMoney(House[k][hPrice], money);
    FormatMoney(House_Tax(k), tax);
    HouseOffer[playerid] = k;
    if (House[k][hOwner] != 0) {
        ShowPlayerDialog(playerid, DIALOG_MENU_INFO, DIALOG_STYLE_MSGBOX, C_BRAND "Жилище",
            C_WHITE "%s №%d\n\nСобственик: " C_BRAND "%s\n" C_WHITE "Вратата е " C_RED "заключена" C_WHITE ".",
            "Добре", "", HouseInteriors[House[k][hInt]][hiName], House[k][hID], House[k][hOwnerName]);
        return;
    }
    ShowPlayerDialog(playerid, DIALOG_HOUSE_INFO, DIALOG_STYLE_MSGBOX, C_GREEN "Жилище за продан",
        C_WHITE "%s №%d\n\n" \
        C_WHITE "Цена: " C_GREEN "$%s\n" \
        C_WHITE "Данък: " C_BRAND "$%s" C_WHITE " на всеки PayDay\n\n" \
        C_GREY "Вътре: безплатен хладилник, тоалетна и легло.\n" \
        C_GREY "Със собствен дом не получаваш социална помощ на PayDay.",
        "Купи", "Затвори", HouseInteriors[House[k][hInt]][hiName], House[k][hID], money, tax);
}

static stock House_Buy(playerid, k)
{
    if (k < 0 || k >= HouseCount) return 0;
    if (House[k][hOwner] != 0) return SendError(playerid, "Някой вече я купи.");
    if (House_GetOwned(playerid) != -1) return SendError(playerid, "Вече имаш жилище. Продай го от Меню > Моят дом.");
    if (!IsPlayerInRangeOfPoint(playerid, 5.0, House[k][hX], House[k][hY], House[k][hZ])) return SendError(playerid, "Трябва да си до вратата.");
    if (!Money_Take(playerid, House[k][hPrice])) return SendError(playerid, "Нямаш достатъчно пари.");
    House[k][hOwner] = PlayerData[playerid][pID];
    format(House[k][hOwnerName], MAX_PLAYER_NAME + 1, "%s", PlayerData[playerid][pName]);
    House[k][hLocked] = true;
    House[k][hSpawnHome] = true;
    House[k][hUnpaid] = 0;
    House_Save(k);
    House_CreateMarkers(k);
    PlayerPlaySound(playerid, 1058, 0.0, 0.0, 0.0);
    GameTextForPlayer(playerid, "~g~NEW HOME!", 4000, 1);
    SendClientMessage(playerid, COLOR_GREEN, "[ДОМ] " C_WHITE "Поздравления! Купи %s №%d. Управлявай го от " C_BRAND "Меню > Моят дом" C_WHITE ".",
        HouseInteriors[House[k][hInt]][hiName], House[k][hID]);
    House_Enter(playerid, k);
    return 1;
}

// ---------- Меню "Моят дом" ----------

stock House_ShowMenu(playerid)
{
    new k = House_GetOwned(playerid);
    if (k == -1) {
        ShowPlayerDialog(playerid, DIALOG_MENU_INFO, DIALOG_STYLE_MSGBOX, C_BRAND "Моят дом",
            C_WHITE "Нямаш жилище.\n\nКъщите за продан са на картата (зелена къщичка) и в " C_BRAND "GPS > Жилища за продан" C_WHITE ".",
            "Назад", "");
        return;
    }
    new body[512], tax[16], sell[16], title[64];
    FormatMoney(House_Tax(k), tax);
    FormatMoney(House[k][hPrice] * HOUSE_SELL_PERCENT / 100, sell);
    format(body, sizeof (body),
        "Покажи на GPS\n" \
        "Вратата:\t%s\n" \
        "Спаун в дома:\t%s\n" \
        "Данък на PayDay:\t" C_BRAND "$%s" C_WHITE "%s\n" \
        C_RED "Продай на държавата\t" C_GREEN "$%s",
        House[k][hLocked] ? (C_RED "заключена") : (C_GREEN "отключена"),
        House[k][hSpawnHome] ? (C_GREEN "вкл") : (C_GREY "изкл"),
        tax, (House[k][hUnpaid] > 0) ? (C_RED " (има неплатени)") : (""),
        sell);
    format(title, sizeof (title), C_BRAND "Моят дом" C_WHITE " | %s №%d", HouseInteriors[House[k][hInt]][hiName], House[k][hID]);
    ShowPlayerDialog(playerid, DIALOG_HOUSE_MENU, DIALOG_STYLE_TABLIST, title, "%s", "Избери", "Назад", body);
}

// ---------- PayDay ----------

stock bool:House_HasHome(playerid)
{
    return (House_GetOwned(playerid) != -1);
}

// Данъкът на PayDay (вика се само за собственици)
stock House_ChargeTax(playerid)
{
    new k = House_GetOwned(playerid);
    if (k == -1) return;
    new tax = House_Tax(k);
    if (Money_Take(playerid, tax)) {
        if (House[k][hUnpaid] > 0) House[k][hUnpaid] = 0;
        SendClientMessage(playerid, COLOR_BRAND, "[PAYDAY] " C_WHITE "Данък за жилището: " C_RED "-$%d", tax);
    } else {
        House[k][hUnpaid]++;
        SendClientMessage(playerid, COLOR_RED, "[PAYDAY] " C_WHITE "Нямаш пари за данъка ($%d)! Неплатени: %d от %d - после къщата се отнема.",
            tax, House[k][hUnpaid], HOUSE_MAX_UNPAID);
        if (House[k][hUnpaid] >= HOUSE_MAX_UNPAID) {
            SendClientMessage(playerid, COLOR_RED, "[ДОМ] " C_WHITE "Жилището ти е отнето заради неплатени данъци.");
            House_Release(k);
            return;
        }
    }
    House_Save(k);
}

// ---------- ALT пеша ----------

stock House_OnKeyStateChange(playerid, KEY:newkeys, KEY:oldkeys)
{
    if (!((newkeys & KEY_WALK) && !(oldkeys & KEY_WALK))) return 0;
    if (!PlayerData[playerid][pLogged] || IsPlayerInAnyVehicle(playerid)) return 0;

    new inside = House_GetInside(playerid);
    if (inside != -1) {
        new i = House[inside][hInt];
        if (IsPlayerInRangeOfPoint(playerid, HOUSE_EXIT_RANGE, HouseInteriors[i][hiX], HouseInteriors[i][hiY], HouseInteriors[i][hiZ])) {
            House_Exit(playerid, inside);
        } else {
            Needs_ShowShop(playerid, SHOP_KIND_HOME);
        }
        return 1;
    }

    new k = House_GetNearDoor(playerid);
    if (k == -1) return 0;
    if (House[k][hOwner] == PlayerData[playerid][pID] || (House[k][hOwner] != 0 && !House[k][hLocked])) {
        House_Enter(playerid, k);
    } else {
        House_ShowOffer(playerid, k);
    }
    return 1;
}

// ---------- Диалози ----------

stock House_OnDialogResponse(playerid, dialogid, response, listitem)
{
    switch (dialogid) {
        case DIALOG_HOUSE_INFO: {
            if (response) House_Buy(playerid, HouseOffer[playerid]);
            return 1;
        }
        case DIALOG_HOUSE_MENU: {
            if (!response) { Menu_Show(playerid); return 1; }
            new k = House_GetOwned(playerid);
            if (k == -1) return 1;
            switch (listitem) {
                case 0: {
                    new name[32];
                    format(name, sizeof (name), "моят дом");
                    Gps_SetPoint(playerid, name, House[k][hX], House[k][hY], House[k][hZ]);
                }
                case 1: {
                    House[k][hLocked] = !House[k][hLocked];
                    House_Save(k);
                    PlayerPlaySound(playerid, 1145, 0.0, 0.0, 0.0);
                    House_ShowMenu(playerid);
                }
                case 2: {
                    House[k][hSpawnHome] = !House[k][hSpawnHome];
                    House_Save(k);
                    House_ShowMenu(playerid);
                }
                case 3: House_ShowMenu(playerid);
                case 4: {
                    new sell[16];
                    FormatMoney(House[k][hPrice] * HOUSE_SELL_PERCENT / 100, sell);
                    ShowPlayerDialog(playerid, DIALOG_HOUSE_SELL, DIALOG_STYLE_MSGBOX, C_RED "Продажба на жилище",
                        C_WHITE "Сигурен ли си, че продаваш дома си на държавата за " C_GREEN "$%s" C_WHITE "?\n\nТова не може да се върне.",
                        "Продай", "Отказ", sell);
                }
            }
            return 1;
        }
        case DIALOG_HOUSE_SELL: {
            new k = House_GetOwned(playerid);
            if (!response || k == -1) { House_ShowMenu(playerid); return 1; }
            new amount = House[k][hPrice] * HOUSE_SELL_PERCENT / 100;
            if (House_GetInside(playerid) == k) House_Exit(playerid, k);
            House_Release(k);
            Money_Give(playerid, amount);
            SendClientMessage(playerid, COLOR_BRAND, "[ДОМ] " C_WHITE "Продаде жилището си за " C_GREEN "$%d" C_WHITE ".", amount);
            return 1;
        }
    }
    return 0;
}

// ---------- GPS ----------

stock House_CountForSale()
{
    new n = 0;
    for (new k = 0; k < HouseCount; k++) {
        if (House[k][hOwner] == 0) n++;
    }
    return n;
}

stock House_GetNthForSale(n, &Float:x, &Float:y, &Float:z, name[], size)
{
    for (new k = 0; k < HouseCount; k++) {
        if (House[k][hOwner] != 0) continue;
        if (n-- == 0) {
            x = House[k][hX];
            y = House[k][hY];
            z = House[k][hZ];
            new money[16];
            FormatMoney(House[k][hPrice], money);
            format(name, size, "%s №%d - $%s", HouseInteriors[House[k][hInt]][hiName], House[k][hID], money);
            return 1;
        }
    }
    return 0;
}

// ---------- Админ ----------

stock House_Create(playerid, price, interior)
{
    if (HouseCount >= MAX_HOUSES) return SendError(playerid, "Достигнат е максимумът къщи.");
    if (interior < 0 || interior >= sizeof (HouseInteriors)) return 0;
    new Float:x, Float:y, Float:z, Float:a, q[256];
    GetPlayerPos(playerid, x, y, z);
    GetPlayerFacingAngle(playerid, a);
    format(q, sizeof (q), "INSERT INTO houses (x, y, z, a, price, interior) VALUES (%f, %f, %f, %f, %d, %d)", x, y, z, a, price, interior);
    Database_Exec(q);
    Houses_Load();
    SendClientMessage(playerid, COLOR_ADMIN, "[A] Нова къща: %s за $%d (общо %d). Вратата е тук, излизането - натам, накъдето гледаш.",
        HouseInteriors[interior][hiName], price, HouseCount);
    return 1;
}

stock House_DeleteNearest(playerid)
{
    new best = -1, Float:bestDist = 10.0;
    for (new k = 0; k < HouseCount; k++) {
        new Float:d = GetPlayerDistanceFromPoint(playerid, House[k][hX], House[k][hY], House[k][hZ]);
        if (d < bestDist) {
            bestDist = d;
            best = k;
        }
    }
    if (best == -1) return SendError(playerid, "Няма къща в 10 м около теб.");
    // Собственикът получава цялата цена обратно
    if (House[best][hOwner] != 0) House_PayOwner(best, House[best][hPrice]);
    new q[96], id = House[best][hID];
    format(q, sizeof (q), "DELETE FROM houses WHERE id = %d", id);
    Database_Exec(q);
    Houses_Load();
    SendClientMessage(playerid, COLOR_ADMIN, "[A] Къща №%d е изтрита (собственикът, ако има, получи цената обратно).", id);
    return 1;
}
