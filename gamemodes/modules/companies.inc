// Компании: бази, какво дават и какво приемат. По-късно - собственост (бизнеси).
//
// Базите имат приблизителни позиции; админ ги мести на място: /compos [номер].
// Списък: /complist

#define MAX_COMPANIES       30          // капацитет (компаниите се пазят в базата)
#define DEFAULT_COMPANIES   13          // началните компании (засяват се в базата веднъж)
#define COMPANY_COMMISSION  10          // % от всеки курс в касата на собственика
#define COMPANY_RANGE       12.0        // колко близо до базата се брои "в базата"
#define COMPANY_ICON_TYPE   51          // икона "камион" на картата


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

static const CompanyDefaults[DEFAULT_COMPANIES][E_COMPANY_DEF] = {
    {"Ocean Docks Shipping", "Los Santos",   COMPANY_LARGE,
        GOOD_BIT(GOOD_ELECTRONICS) | GOOD_BIT(GOOD_FURNITURE) | GOOD_BIT(GOOD_PARTS) | GOOD_BIT(GOOD_DRINKS) | GOOD_BIT(GOOD_PARCELS) | GOOD_BIT(GOOD_FROZEN) | GOOD_BIT(GOOD_GLASS),
        GOOD_BIT(GOOD_METAL) | GOOD_BIT(GOOD_FOOD),
        2770.0, -2420.0, 13.6},
    {"LS Parcel Express",    "Los Santos",   COMPANY_SMALL,
        GOOD_BIT(GOOD_PARCELS) | GOOD_BIT(GOOD_MEDS),
        GOOD_BIT(GOOD_PARCELS) | GOOD_BIT(GOOD_MEDS),
        1965.0, -2185.0, 13.5},
    {"Santos Mart",          "Los Santos",   COMPANY_SMALL,
        0,
        GOOD_BIT(GOOD_FOOD) | GOOD_BIT(GOOD_DRINKS) | GOOD_BIT(GOOD_ELECTRONICS) | GOOD_BIT(GOOD_PARCELS) | GOOD_BIT(GOOD_FROZEN) | GOOD_BIT(GOOD_DAIRY),
        1130.0, -1440.0, 15.8},
    {"Bay Lumber Co.",       "Angel Pine",   COMPANY_MEDIUM,
        GOOD_BIT(GOOD_WOOD) | GOOD_BIT(GOOD_FURNITURE),
        GOOD_BIT(GOOD_PARTS),
        -1966.0, -2436.0, 30.6},
    {"Doherty Construction", "San Fierro",   COMPANY_MEDIUM,
        0,
        GOOD_BIT(GOOD_WOOD) | GOOD_BIT(GOOD_SAND) | GOOD_BIT(GOOD_GRAVEL) | GOOD_BIT(GOOD_METAL) | GOOD_BIT(GOOD_GLASS),
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
        GOOD_BIT(GOOD_FURNITURE) | GOOD_BIT(GOOD_ELECTRONICS) | GOOD_BIT(GOOD_PARCELS) | GOOD_BIT(GOOD_METAL) | GOOD_BIT(GOOD_FROZEN) | GOOD_BIT(GOOD_DAIRY) | GOOD_BIT(GOOD_GLASS),
        1640.0, 1030.0, 10.8},
    {"Blueberry Farms",      "Red County",   COMPANY_MEDIUM,
        GOOD_BIT(GOOD_FOOD) | GOOD_BIT(GOOD_DAIRY),
        GOOD_BIT(GOOD_PARCELS) | GOOD_BIT(GOOD_PARTS),
        -60.0, 70.0, 3.1},
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
    cpOwnerName[MAX_PLAYER_NAME + 1],
    cpCash,             // касата - 10% от курсовете от/до компанията
    cpDeliveries,
    cpEarned,
    cpBonus,            // бонус за шофьорите в %, плаща се от касата
    // Фирма (виж firm.inc)
    cpTag[5],
    cpColor,            // RGB, 0 = цветът на сървъра
    cpSlogan[48],
    cpCommission,       // % от курсовете на служителите
    cpSalary,           // заплата на PayDay
    bool:cpFuelCard,
    cpRenamedAt,
    // Етап 2 (виж firm.inc): подобрения 0-3, опит, репутация 0-100
    cpUp[6],
    cpXP,
    cpRep,
    cpLastDelivery
}

new Company[MAX_COMPANIES][E_COMPANY];
// Дефинициите по време на работа (от таблица company_defs). Празно име = свободно място.
new CompanyDefs[MAX_COMPANIES][E_COMPANY_DEF];

static Text3D:CompanyLabel[MAX_COMPANIES] = {Text3D:-1, ...};
static CompanyIcon[MAX_COMPANIES] = {-1, ...};

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
    Database_AddColumn("companies", "owner_name", "TEXT NOT NULL DEFAULT ''");
    Database_AddColumn("companies", "bonus", "INTEGER NOT NULL DEFAULT 0");
    Database_AddColumn("companies", "tag", "TEXT NOT NULL DEFAULT ''");
    Database_AddColumn("companies", "color", "INTEGER NOT NULL DEFAULT 0");
    Database_AddColumn("companies", "slogan", "TEXT NOT NULL DEFAULT ''");
    Database_AddColumn("companies", "commission", "INTEGER NOT NULL DEFAULT 10");
    Database_AddColumn("companies", "salary", "INTEGER NOT NULL DEFAULT 0");
    Database_AddColumn("companies", "fuel_card", "INTEGER NOT NULL DEFAULT 0");
    Database_AddColumn("companies", "renamed_at", "INTEGER NOT NULL DEFAULT 0");
    Database_AddColumn("companies", "upgrades", "TEXT NOT NULL DEFAULT '0,0,0,0,0,0'");
    Database_AddColumn("companies", "xp", "INTEGER NOT NULL DEFAULT 0");
    Database_AddColumn("companies", "rep", "INTEGER NOT NULL DEFAULT 50");
    Database_AddColumn("companies", "last_delivery", "INTEGER NOT NULL DEFAULT 0");

    // Еднократно: махната е компанията "Fuel Station LS" (беше номер 11).
    // Собственикът (ако има) получава 60% от цената + касата; номерата след нея се местят с 1 надолу.
    if (!Database_GetMeta("removed_fuel_ls")) {
        new DBResult:r0 = Database_Query("SELECT owner_id, cash FROM companies WHERE id = 11");
        if (r0 != DBResult:0) {
            if (DB_GetRowCount(r0) > 0) {
                new owner = DB_GetFieldIntByName(r0, "owner_id"), cash = DB_GetFieldIntByName(r0, "cash");
                if (owner > 0) {
                    format(q, sizeof (q), "UPDATE accounts SET money = money + %d WHERE id = %d", CompanyPrices[COMPANY_SMALL] * 60 / 100 + cash, owner);
                    Database_Exec(q);
                }
            }
            DB_FreeResultSet(r0);
        }
        Database_Exec("DELETE FROM companies WHERE id = 11");
        Database_Exec("UPDATE companies SET id = 11 WHERE id = 12");
        Database_Exec("UPDATE companies SET id = 12 WHERE id = 13");
        Database_SetMeta("removed_fuel_ls", 1);
        print("[COMPANY] Fuel Station LS removed.");
    }

    // Дефинициите на компаниите (име, място, размер, какво дават/приемат) – в базата
    Database_Exec("CREATE TABLE IF NOT EXISTS company_defs (id INTEGER PRIMARY KEY, name TEXT NOT NULL, place TEXT NOT NULL, size INTEGER NOT NULL, produces INTEGER NOT NULL, accepts INTEGER NOT NULL)");
    if (!Database_GetMeta("company_defs_seeded")) {
        new safeName[64], safePlace[48];
        for (new c = 0; c < DEFAULT_COMPANIES; c++) {
            SQL_Escape(CompanyDefaults[c][cdName], safeName);
            SQL_Escape(CompanyDefaults[c][cdPlace], safePlace);
            format(q, sizeof (q), "INSERT OR IGNORE INTO company_defs (id, name, place, size, produces, accepts) VALUES (%d, '%s', '%s', %d, %d, %d)",
                c, safeName, safePlace, CompanyDefaults[c][cdSize], CompanyDefaults[c][cdProduces], CompanyDefaults[c][cdAccepts]);
            Database_Exec(q);
            format(q, sizeof (q), "INSERT OR IGNORE INTO companies (id, x, y, z) VALUES (%d, %f, %f, %f)",
                c, CompanyDefaults[c][cdDefX], CompanyDefaults[c][cdDefY], CompanyDefaults[c][cdDefZ]);
            Database_Exec(q);
        }
        Database_SetMeta("company_defs_seeded", 1);
    }
    Company_LoadDefs();

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
                Company[c][cpBonus] = DB_GetFieldIntByName(r, "bonus");
                DB_GetFieldStringByName(r, "owner_name", Company[c][cpOwnerName], MAX_PLAYER_NAME + 1);
                DB_GetFieldStringByName(r, "tag", Company[c][cpTag], 5);
                Company[c][cpColor] = DB_GetFieldIntByName(r, "color");
                DB_GetFieldStringByName(r, "slogan", Company[c][cpSlogan], 48);
                Company[c][cpCommission] = DB_GetFieldIntByName(r, "commission");
                Company[c][cpSalary] = DB_GetFieldIntByName(r, "salary");
                Company[c][cpFuelCard] = (DB_GetFieldIntByName(r, "fuel_card") != 0);
                Company[c][cpRenamedAt] = DB_GetFieldIntByName(r, "renamed_at");
                new ups[32];
                DB_GetFieldStringByName(r, "upgrades", ups, sizeof (ups));
                Company_ParseUpgrades(c, ups);
                Company[c][cpXP] = DB_GetFieldIntByName(r, "xp");
                Company[c][cpRep] = DB_GetFieldIntByName(r, "rep");
                Company[c][cpLastDelivery] = DB_GetFieldIntByName(r, "last_delivery");
            }
            DB_SelectNextRow(r);
        }
        DB_FreeResultSet(r);
    }

    for (new c = 0; c < MAX_COMPANIES; c++) Company_CreateMarkers(c);
}

static stock Company_LoadDefs()
{
    for (new c = 0; c < MAX_COMPANIES; c++) {
        CompanyDefs[c][cdName][0] = EOS;
        CompanyDefs[c][cdProduces] = 0;
        CompanyDefs[c][cdAccepts] = 0;
        Company[c][cpZ] = -10000.0;             // празните места са далеч от всичко
    }
    new DBResult:r = Database_Query("SELECT * FROM company_defs");
    if (r == DBResult:0) return;
    new rows = DB_GetRowCount(r);
    for (new i = 0; i < rows; i++) {
        new c = DB_GetFieldIntByName(r, "id");
        if (c >= 0 && c < MAX_COMPANIES) {
            DB_GetFieldStringByName(r, "name", CompanyDefs[c][cdName], 32);
            DB_GetFieldStringByName(r, "place", CompanyDefs[c][cdPlace], 24);
            CompanyDefs[c][cdSize] = DB_GetFieldIntByName(r, "size");
            if (CompanyDefs[c][cdSize] < COMPANY_SMALL || CompanyDefs[c][cdSize] > COMPANY_LARGE) CompanyDefs[c][cdSize] = COMPANY_SMALL;
            CompanyDefs[c][cdProduces] = DB_GetFieldIntByName(r, "produces");
            CompanyDefs[c][cdAccepts] = DB_GetFieldIntByName(r, "accepts");
        }
        DB_SelectNextRow(r);
    }
    DB_FreeResultSet(r);
}

stock bool:Company_IsValid(c)
{
    return (c >= 0 && c < MAX_COMPANIES && CompanyDefs[c][cdName][0] != EOS);
}

// Нова компания на дадена позиция. Връща номера или -1.
stock Company_Create(const name[], const place[], size, produces, accepts, Float:x, Float:y, Float:z)
{
    new c = -1;
    for (new i = 0; i < MAX_COMPANIES; i++) {
        if (!Company_IsValid(i)) {
            c = i;
            break;
        }
    }
    if (c == -1) return -1;

    new safeName[64], safePlace[48], q[320];
    SQL_Escape(name, safeName);
    SQL_Escape(place, safePlace);
    format(q, sizeof (q), "INSERT OR REPLACE INTO company_defs (id, name, place, size, produces, accepts) VALUES (%d, '%s', '%s', %d, %d, %d)",
        c, safeName, safePlace, size, produces, accepts);
    Database_Exec(q);
    format(q, sizeof (q), "DELETE FROM companies WHERE id = %d", c);
    Database_Exec(q);
    format(q, sizeof (q), "INSERT INTO companies (id, x, y, z) VALUES (%d, %f, %f, %f)", c, x, y, z);
    Database_Exec(q);

    format(CompanyDefs[c][cdName], 32, "%s", name);
    format(CompanyDefs[c][cdPlace], 24, "%s", place);
    CompanyDefs[c][cdSize] = size;
    CompanyDefs[c][cdProduces] = produces;
    CompanyDefs[c][cdAccepts] = accepts;
    Company[c][cpX] = x;
    Company[c][cpY] = y;
    Company[c][cpZ] = z;
    Company[c][cpOwner] = 0;
    Company[c][cpOwnerName][0] = EOS;
    Company[c][cpCash] = 0;
    Company[c][cpDeliveries] = 0;
    Company[c][cpEarned] = 0;
    Company[c][cpBonus] = 0;
    Company_ResetFirm(c);
    Company[c][cpRep] = 50;
    Company[c][cpXP] = 0;
    for (new i = 0; i < 6; i++) Company[c][cpUp][i] = 0;
    Economy_OnCompanyCreated(c);
    Company_CreateMarkers(c);
    Firm3_OnCompanyCreated(c);
    return c;
}

// Изтрива компания (собственикът получава 60% от цената + касата)
stock Company_Delete(c)
{
    if (!Company_IsValid(c)) return 0;
    Delivery_OnCompanyRemoved(c);
    if (Company[c][cpOwner] != 0) {
        Firm_OnCompanyReleased(c);
        new q0[128];
        format(q0, sizeof (q0), "UPDATE accounts SET money = money + %d WHERE id = %d", Company_GetPrice(c) * 60 / 100 + Company[c][cpCash], Company[c][cpOwner]);
        Database_Exec(q0);
        for (new i = 0; i < MAX_PLAYERS; i++) {
            if (IsPlayerLogged(i) && PlayerData[i][pID] == Company[c][cpOwner]) {
                // Онлайн – и в паметта
                Money_Give(i, Company_GetPrice(c) * 60 / 100 + Company[c][cpCash]);
                SendClientMessage(i, COLOR_BRAND, "[КОМПАНИЯ] " C_WHITE "Компанията ти беше закрита от администратор. Получаваш обезщетение.");
            }
        }
    }
    new q[96];
    format(q, sizeof (q), "DELETE FROM company_defs WHERE id = %d", c);
    Database_Exec(q);
    format(q, sizeof (q), "DELETE FROM companies WHERE id = %d", c);
    Database_Exec(q);
    format(q, sizeof (q), "DELETE FROM economy_stock WHERE company = %d", c);
    Database_Exec(q);

    if (CompanyLabel[c] != Text3D:-1) DestroyDynamic3DTextLabel(CompanyLabel[c]);
    if (CompanyIcon[c] != -1) DestroyDynamicMapIcon(CompanyIcon[c]);
    CompanyLabel[c] = Text3D:-1;
    CompanyIcon[c] = -1;
    CompanyDefs[c][cdName][0] = EOS;
    CompanyDefs[c][cdProduces] = 0;
    CompanyDefs[c][cdAccepts] = 0;
    Company[c][cpOwner] = 0;
    Company[c][cpZ] = -10000.0;
    Economy_ClearCompany(c);
    Firm3_OnCompanyDeleted(c);
    return 1;
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

stock Company_CreateMarkers(c)
{
    if (!Company_IsValid(c)) return;
    if (CompanyLabel[c] != Text3D:-1) DestroyDynamic3DTextLabel(CompanyLabel[c]);
    // Икона "камион" – само в радиус около играча (създава се веднъж; при преместване – отново)
    if (CompanyIcon[c] == -1) {
        CompanyIcon[c] = CreateDynamicMapIcon(Company[c][cpX], Company[c][cpY], Company[c][cpZ], COMPANY_ICON_TYPE, 0, 0, 0, -1, MAP_ICON_RANGE, MAPICON_LOCAL);
    }

    // Наличности в склада (виж modules/economy.inc)
    new produces[320], accepts[320], owner[96], text[1000];
    Economy_GoodsLine(c, CompanyDefs[c][cdProduces], false, produces);
    Economy_GoodsLine(c, CompanyDefs[c][cdAccepts], true, accepts);
    if (Company[c][cpOwner] != 0) {
        format(owner, sizeof (owner), C_WHITE "Собственик: " C_BRAND "%s", Company[c][cpOwnerName]);
    } else {
        new price[16];
        FormatMoney(Company_GetPrice(c), price);
        format(owner, sizeof (owner), C_GREEN "Продава се: $%s", price);
    }
    new col[12], slogan[64];
    format(col, sizeof (col), "{%06x}", (Company[c][cpColor] != 0) ? Company[c][cpColor] : 0xF5A623);
    slogan[0] = EOS;
    if (Company[c][cpOwner] != 0 && Company[c][cpSlogan][0] != EOS) format(slogan, sizeof (slogan), C_WHITE "\"%s\"\n", Company[c][cpSlogan]);
    format(text, sizeof (text),
        "%s%s\n%s" C_GREY "%s, ниво %d, репутация %d%%\n%s\n" C_GREEN "Дава: " C_WHITE "%s\n" C_BRAND "Приема: " C_WHITE "%s\n" C_GREY "N с превозно средство - поръчки, ALT пеша - компанията",
        col, CompanyDefs[c][cdName], slogan, CompanyDefs[c][cdPlace], Firm_Level(c), Company[c][cpRep], owner, produces, accepts);
    CompanyLabel[c] = CreateDynamic3DTextLabel(text, COLOR_WHITE, Company[c][cpX], Company[c][cpY], Company[c][cpZ] + 1.5, 30.0,
        INVALID_PLAYER_ID, INVALID_VEHICLE_ID, 0, 0, 0);
}

// Достъп за други модули (GPS)
stock Company_Total()
{
    return MAX_COMPANIES;
}

stock Company_GetPos(c, &Float:x, &Float:y, &Float:z)
{
    x = Company[c][cpX];
    y = Company[c][cpY];
    z = Company[c][cpZ];
}

stock Company_GetName(c, dest[], size = sizeof (dest))
{
    format(dest, size, "%s", CompanyDefs[c][cdName]);
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
        if (!Company_IsValid(c)) continue;
        if (IsPlayerInRangeOfPoint(playerid, range, Company[c][cpX], Company[c][cpY], Company[c][cpZ])) return c;
    }
    return -1;
}

stock Company_GetPrice(c)
{
    return CompanyPrices[CompanyDefs[c][cdSize]];
}

stock Company_Save(c)
{
    new q[768], safeTag[16], safeSlogan[112], ups[32];
    SQL_Escape(Company[c][cpTag], safeTag);
    SQL_Escape(Company[c][cpSlogan], safeSlogan);
    format(ups, sizeof (ups), "%d,%d,%d,%d,%d,%d", Company[c][cpUp][0], Company[c][cpUp][1], Company[c][cpUp][2], Company[c][cpUp][3], Company[c][cpUp][4], Company[c][cpUp][5]);
    format(q, sizeof (q), "UPDATE companies SET owner_id = %d, owner_name = '%s', cash = %d, deliveries = %d, earned = %d, bonus = %d, " \
        "tag = '%s', color = %d, slogan = '%s', commission = %d, salary = %d, fuel_card = %d, renamed_at = %d, " \
        "upgrades = '%s', xp = %d, rep = %d, last_delivery = %d WHERE id = %d",
        Company[c][cpOwner], Company[c][cpOwnerName], Company[c][cpCash], Company[c][cpDeliveries], Company[c][cpEarned], Company[c][cpBonus],
        safeTag, Company[c][cpColor], safeSlogan, Company[c][cpCommission], Company[c][cpSalary], Company[c][cpFuelCard] ? 1 : 0,
        Company[c][cpRenamedAt], ups, Company[c][cpXP], Company[c][cpRep], Company[c][cpLastDelivery], c);
    Database_Exec(q);
}

// "1,0,2,0,0,3" -> нивата на подобренията
stock Company_ParseUpgrades(c, const text[])
{
    new idx = 0, val = 0;
    for (new i = 0; i < 6; i++) Company[c][cpUp][i] = 0;
    for (new i = 0; ; i++) {
        new ch = text[i];
        if (ch >= '0' && ch <= '9') {
            val = val * 10 + (ch - '0');
        } else {
            if (idx < 6) Company[c][cpUp][idx++] = (val > 3) ? 3 : val;
            val = 0;
            if (ch == EOS) break;
        }
    }
}

// Фирмените настройки по подразбиране (нов собственик или нова компания)
stock Company_ResetFirm(c)
{
    Company[c][cpTag][0] = EOS;
    Company[c][cpColor] = 0;
    Company[c][cpSlogan][0] = EOS;
    Company[c][cpCommission] = 10;
    Company[c][cpSalary] = 0;
    Company[c][cpFuelCard] = false;
}

// Бонус (в $), който компанията А добавя към курс с тази базова стойност. 0 ако касата не стига.
stock Company_GetBonusAmount(c, basePay)
{
    if (Company[c][cpOwner] == 0 || Company[c][cpBonus] <= 0) return 0;
    new amount = basePay * Company[c][cpBonus] / 100;
    return (Company[c][cpCash] >= amount) ? amount : 0;
}

// След курс: статистика и 10% в касата на всяка компания със собственик.
stock Company_OnDelivery(from, to, value)
{
    new cut = value * COMPANY_COMMISSION / 100;
    Company[from][cpDeliveries]++;
    Company[to][cpDeliveries]++;
    Firm_AddXP(from, 10);
    Firm_AddXP(to, 10);
    if (Company[from][cpOwner] != 0) {
        Company[from][cpCash] += cut;
        Company[from][cpEarned] += cut;
    }
    if (Company[to][cpOwner] != 0) {
        Company[to][cpCash] += cut;
        Company[to][cpEarned] += cut;
    }
    Company_Save(from);
    Company_Save(to);
}

// ---------- Админ ----------

CMD:compos(playerid, params[])
{
    if (!Admin_Check(playerid, 5)) return 1;
    if (IsNull(params) || !IsNumeric(params)) return SendUsage(playerid, "/compos [номер от /complist] - премества базата тук");
    new c = strval(params);
    if (!Company_IsValid(c)) return SendError(playerid, "Няма такава компания.");
    if (GetPlayerVirtualWorld(playerid) != 0) return SendError(playerid, "Трябва да си навън (свят 0).");

    new Float:x, Float:y, Float:z, q[160];
    GetPlayerPos(playerid, x, y, z);
    Company[c][cpX] = x;
    Company[c][cpY] = y;
    Company[c][cpZ] = z;
    format(q, sizeof (q), "UPDATE companies SET x = %f, y = %f, z = %f WHERE id = %d", x, y, z, c);
    Database_Exec(q);
    if (CompanyIcon[c] != -1) DestroyDynamicMapIcon(CompanyIcon[c]);
    CompanyIcon[c] = -1;
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
        if (!Company_IsValid(c)) continue;
        SendClientMessage(playerid, COLOR_GREY, "#%d, %s (%s), %.1f км от теб", c, CompanyDefs[c][cdName], CompanyDefs[c][cdPlace],
            GetPlayerDistanceFromPoint(playerid, Company[c][cpX], Company[c][cpY], Company[c][cpZ]) / 1000.0);
    }
    return 1;
}

CMD:gotocomp(playerid, params[])
{
    if (!Admin_Check(playerid, 3)) return 1;
    if (IsNull(params) || !IsNumeric(params)) return SendUsage(playerid, "/gotocomp [номер от /complist]");
    new c = strval(params);
    if (!Company_IsValid(c)) return SendError(playerid, "Няма такава компания.");
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
