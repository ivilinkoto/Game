// Фирми, етап 3: собствени поръчки, автопарк и офис на всяка компания.
//
// Собствени поръчки: собственикът поръчва товар, който компанията приема (напр. 20 т руда по $300/т).
//   Цялата сума се заделя от касата. Поръчките излизат на борсата ("ПОРЪЧКА"), всеки може да ги кара,
//   а служителите получават и фирмения бонус. Поръчката важи 24 ч; остатъкът се връща в касата.
// Автопарк: компанията купува камиони от каталога (от касата). Карат ги само нейните хора (с нужната
//   категория), без гориво. Стоят "в гаража" на компанията; изкарват се от панела - появяват се на
//   базата (там, където се товари). Оставен 5 мин без шофьор - прибира се сам.
// Офис: наш интериор (maps/office.inc), отделен свят за всяка компания, с името й на стената.
//   Входът е до базата (собственикът го мести). Вътре секретарката (ALT) отваря панела на компанията.

#define FIRM_ORDER_HOURS        24
#define FIRM_ORDER_MAX_TONS     200
#define FIRM_ORDER_MIN_PPT      50
#define FIRM_ORDER_MAX_PPT      5000
#define FLEET_MAX               8
#define FLEET_SELL_PERCENT      60
#define FLEET_RESPAWN_SEC       300

#define OFFICE_WORLD_BASE       7000
#define OFFICE_SECRETARY_SKIN   141

static const FleetSlots[] = {3, 5, 8};
static Firm3Company[MAX_PLAYERS];

stock bool:Firm_IsOwnerOf(playerid, c)
{
    return (c >= 0 && c < MAX_COMPANIES && Company[c][cpOwner] != 0 && Company[c][cpOwner] == PlayerData[playerid][pID]);
}

// ---------- Собствени поръчки ----------

enum E_FIRM_ORDER {
    foGood,
    foTotalKg,
    foLeftKg,
    foPPT,                  // $ на тон
    foReserved,             // заделени пари от касата
    foExpires               // gettime(); 0 = няма поръчка
}
static FirmOrder[MAX_COMPANIES][E_FIRM_ORDER];
static FirmOrderGoods[MAX_PLAYERS][MAX_GOODS];
static FirmOrderGood[MAX_PLAYERS];
static FirmOrderTons[MAX_PLAYERS];

// ---------- Автопарк ----------

static FleetDBID[MAX_COMPANIES][FLEET_MAX];
static FleetModel[MAX_COMPANIES][FLEET_MAX];
static FleetVeh[MAX_COMPANIES][FLEET_MAX];
static FleetCompanyOf[MAX_VEHICLES] = {-1, ...};
static FleetIdle[MAX_COMPANIES][FLEET_MAX];
static FleetPick[MAX_PLAYERS];
static FleetBuyMap[MAX_PLAYERS][16];

// ---------- Офис ----------

static Float:OfficeDoor[MAX_COMPANIES][4];
static OfficePickup[MAX_COMPANIES] = {-1, ...};
static Text3D:OfficeLabel[MAX_COMPANIES] = {Text3D:-1, ...};
static OfficeActor[MAX_COMPANIES] = {INVALID_ACTOR_ID, ...};
static Text3D:OfficeInLabel[MAX_COMPANIES] = {Text3D:-1, ...};
static OfficeSign[MAX_COMPANIES];

// =====================================================================
// Начало
// =====================================================================

stock Firm3_Init()
{
    Database_Exec("CREATE TABLE IF NOT EXISTS company_orders (company INTEGER PRIMARY KEY, good INTEGER NOT NULL, total_kg INTEGER NOT NULL, left_kg INTEGER NOT NULL, ppt INTEGER NOT NULL, reserved INTEGER NOT NULL, expires INTEGER NOT NULL)");
    Database_Exec("CREATE TABLE IF NOT EXISTS company_fleet (id INTEGER PRIMARY KEY AUTOINCREMENT, company INTEGER NOT NULL, model INTEGER NOT NULL)");
    Database_Exec("CREATE TABLE IF NOT EXISTS company_points (company INTEGER NOT NULL, kind INTEGER NOT NULL, x REAL NOT NULL, y REAL NOT NULL, z REAL NOT NULL, a REAL NOT NULL, PRIMARY KEY (company, kind))");

    // Точки по подразбиране: офисът и паркингът до базата
    for (new c = 0; c < MAX_COMPANIES; c++) {
        OfficeDoor[c][0] = Company[c][cpX] + 6.0;
        OfficeDoor[c][1] = Company[c][cpY] + 6.0;
        OfficeDoor[c][2] = Company[c][cpZ];
        OfficeDoor[c][3] = 0.0;
        for (new i = 0; i < FLEET_MAX; i++) {
            FleetDBID[c][i] = 0;
            FleetVeh[c][i] = INVALID_VEHICLE_ID;
        }
    }
    new DBResult:r = Database_Query("SELECT * FROM company_points");
    if (r != DBResult:0) {
        new rows = DB_GetRowCount(r);
        for (new i = 0; i < rows; i++) {
            new c = DB_GetFieldIntByName(r, "company"), kind = DB_GetFieldIntByName(r, "kind");
            if (c >= 0 && c < MAX_COMPANIES && kind == 0) {
                OfficeDoor[c][0] = DB_GetFieldFloatByName(r, "x");
                OfficeDoor[c][1] = DB_GetFieldFloatByName(r, "y");
                OfficeDoor[c][2] = DB_GetFieldFloatByName(r, "z");
                OfficeDoor[c][3] = DB_GetFieldFloatByName(r, "a");
            }
            DB_SelectNextRow(r);
        }
        DB_FreeResultSet(r);
    }

    r = Database_Query("SELECT * FROM company_orders");
    if (r != DBResult:0) {
        new rows = DB_GetRowCount(r);
        for (new i = 0; i < rows; i++) {
            new c = DB_GetFieldIntByName(r, "company");
            if (c >= 0 && c < MAX_COMPANIES) {
                FirmOrder[c][foGood] = DB_GetFieldIntByName(r, "good");
                FirmOrder[c][foTotalKg] = DB_GetFieldIntByName(r, "total_kg");
                FirmOrder[c][foLeftKg] = DB_GetFieldIntByName(r, "left_kg");
                FirmOrder[c][foPPT] = DB_GetFieldIntByName(r, "ppt");
                FirmOrder[c][foReserved] = DB_GetFieldIntByName(r, "reserved");
                FirmOrder[c][foExpires] = DB_GetFieldIntByName(r, "expires");
            }
            DB_SelectNextRow(r);
        }
        DB_FreeResultSet(r);
    }

    r = Database_Query("SELECT id, company, model FROM company_fleet ORDER BY id");
    if (r != DBResult:0) {
        new rows = DB_GetRowCount(r);
        for (new i = 0; i < rows; i++) {
            new c = DB_GetFieldIntByName(r, "company");
            if (c >= 0 && c < MAX_COMPANIES) {
                for (new s = 0; s < FLEET_MAX; s++) {
                    if (FleetDBID[c][s] != 0) continue;
                    FleetDBID[c][s] = DB_GetFieldIntByName(r, "id");
                    FleetModel[c][s] = DB_GetFieldIntByName(r, "model");
                    break;
                }
            }
            DB_SelectNextRow(r);
        }
        DB_FreeResultSet(r);
    }

    Office_CreateMap();
    for (new c = 0; c < MAX_COMPANIES; c++) {
        if (!Company_IsValid(c)) continue;
        Office_Build(c);
    }
}

static stock Firm3_SavePoint(c, kind, const Float:p[4])
{
    new q[200];
    format(q, sizeof (q), "INSERT OR REPLACE INTO company_points (company, kind, x, y, z, a) VALUES (%d, %d, %f, %f, %f, %f)", c, kind, p[0], p[1], p[2], p[3]);
    Database_Exec(q);
}

// =====================================================================
// Собствени поръчки
// =====================================================================

static stock FirmOrder_Save(c)
{
    new q[256];
    if (FirmOrder[c][foExpires] == 0) {
        format(q, sizeof (q), "DELETE FROM company_orders WHERE company = %d", c);
    } else {
        format(q, sizeof (q), "INSERT OR REPLACE INTO company_orders (company, good, total_kg, left_kg, ppt, reserved, expires) VALUES (%d, %d, %d, %d, %d, %d, %d)",
            c, FirmOrder[c][foGood], FirmOrder[c][foTotalKg], FirmOrder[c][foLeftKg], FirmOrder[c][foPPT], FirmOrder[c][foReserved], FirmOrder[c][foExpires]);
    }
    Database_Exec(q);
}

// Приключва поръчката и връща заделеното в касата
static stock FirmOrder_Close(c, const reason[])
{
    if (FirmOrder[c][foExpires] == 0) return;
    new back = FirmOrder[c][foReserved];
    FirmOrder[c][foExpires] = 0;
    FirmOrder[c][foReserved] = 0;
    FirmOrder_Save(c);
    if (back > 0) {
        Company[c][cpCash] += back;
        Company_Save(c);
        Firm_Log(c, back, "Върнато от поръчка");
    }
    new msg[128];
    format(msg, sizeof (msg), "Поръчката за %s приключи (%s).", Goods[FirmOrder[c][foGood]][gdName], reason);
    Firm_Broadcast(c, msg);
}

stock bool:FirmOrder_Active(c)
{
    if (c < 0 || c >= MAX_COMPANIES || FirmOrder[c][foExpires] == 0) return false;
    if (gettime() > FirmOrder[c][foExpires]) {
        FirmOrder_Close(c, "изтече времето");
        return false;
    }
    return FirmOrder[c][foLeftKg] > 0;
}

// За борсата: случайна активна поръчка (1 = има). maxKg = колко остава.
stock Firm_PickOrder(&c, &good, &maxKg)
{
    new list[MAX_COMPANIES], n = 0;
    for (new i = 0; i < MAX_COMPANIES; i++) {
        if (FirmOrder_Active(i)) list[n++] = i;
    }
    if (n == 0) return 0;
    c = list[random(n)];
    good = FirmOrder[c][foGood];
    maxKg = FirmOrder[c][foLeftKg];
    return 1;
}

stock Firm_OrderPPT(c)
{
    return FirmOrder[c][foPPT];
}

// Доставка по поръчка: плащането идва от заделените пари
stock Firm_OrderDelivered(c, kg, pay)
{
    if (c < 0 || c >= MAX_COMPANIES || FirmOrder[c][foExpires] == 0) return;
    FirmOrder[c][foLeftKg] -= kg;
    if (pay > FirmOrder[c][foReserved]) pay = FirmOrder[c][foReserved];
    FirmOrder[c][foReserved] -= pay;
    new log[64];
    format(log, sizeof (log), "Поръчка: %d кг %s", kg, Goods[FirmOrder[c][foGood]][gdName]);
    Firm_Log(c, -pay, log);
    if (FirmOrder[c][foLeftKg] <= 0) {
        FirmOrder[c][foLeftKg] = 0;
        FirmOrder_Close(c, "изпълнена");
        return;
    }
    FirmOrder_Save(c);
}

stock Firm_OrderShow(playerid, c)
{
    Firm3Company[playerid] = c;
    if (FirmOrder_Active(c)) {
        new left = (FirmOrder[c][foExpires] - gettime()) / 60;
        if (left < 0) left = 0;
        ShowPlayerDialog(playerid, DIALOG_FIRM_ORDER_INFO, DIALOG_STYLE_MSGBOX, C_BRAND "Собствена поръчка",
            C_WHITE "Товар: " C_BRAND "%s\n" \
            C_WHITE "Доставено: %d от %d т\n" \
            C_WHITE "Плащане: " C_GREEN "$%d на тон\n" \
            C_WHITE "Заделени в поръчката: " C_GREEN "$%d\n" \
            C_WHITE "Остава: %d ч %d мин\n\n" \
            C_GREY "Поръчките са на борсата с надпис ПОРЪЧКА.",
            "Откажи я", "Назад", Goods[FirmOrder[c][foGood]][gdName],
            (FirmOrder[c][foTotalKg] - FirmOrder[c][foLeftKg]) / 1000, FirmOrder[c][foTotalKg] / 1000,
            FirmOrder[c][foPPT], FirmOrder[c][foReserved], left / 60, left % 60);
        return;
    }
    // Нова: какъв товар (от приеманите)
    new body[1024], line[96], n = 0;
    body[0] = EOS;
    for (new g = 0; g < MAX_GOODS; g++) {
        if (!Company_Accepts(c, g)) continue;
        format(line, sizeof (line), "%s\t" C_GREY "склад %d%%\n", Goods[g][gdName], Economy_FillPercent(c, g));
        strcat(body, line);
        FirmOrderGoods[playerid][n++] = g;
    }
    if (n == 0) {
        ShowPlayerDialog(playerid, DIALOG_FIRM_INFO, DIALOG_STYLE_MSGBOX, C_BRAND "Собствена поръчка",
            C_WHITE "Компанията не приема никакви товари, затова не може да поръчва.", "Назад", "");
        return;
    }
    for (new i = n; i < MAX_GOODS; i++) FirmOrderGoods[playerid][i] = -1;
    ShowPlayerDialog(playerid, DIALOG_FIRM_ORDER_GOOD, DIALOG_STYLE_TABLIST, C_BRAND "Нова поръчка (1/3): товар", "%s", "Напред", "Назад", body);
}

// =====================================================================
// Автопарк
// =====================================================================

stock Fleet_CompanyOf(vehicleid)
{
    if (vehicleid <= 0 || vehicleid >= MAX_VEHICLES) return -1;
    return FleetCompanyOf[vehicleid];
}

// Изкарва служебния камион на базата (там, където се товари)
static stock Fleet_Spawn(c, s)
{
    if (FleetDBID[c][s] == 0 || FleetVeh[c][s] != INVALID_VEHICLE_ID) return INVALID_VEHICLE_ID;
    // Ако на базата вече има изкаран камион - малко встрани
    new out = 0;
    for (new k = 0; k < FLEET_MAX; k++) if (FleetVeh[c][k] != INVALID_VEHICLE_ID) out++;
    new col = (Company[c][cpColor] != 0) ? 1 : random(126);
    new v = CreateVehicle(FleetModel[c][s], Company[c][cpX] + out * 4.0, Company[c][cpY], Company[c][cpZ] + 1.0, 0.0, col, col, -1);
    if (v == INVALID_VEHICLE_ID) return INVALID_VEHICLE_ID;
    FleetVeh[c][s] = v;
    FleetCompanyOf[v] = c;
    FleetIdle[c][s] = 0;
    new plate[16];
    if (Company[c][cpTag][0] != EOS) format(plate, sizeof (plate), "%s %d", Company[c][cpTag], FleetDBID[c][s]);
    else format(plate, sizeof (plate), "FIRM %d", FleetDBID[c][s]);
    SetVehicleNumberPlate(v, plate);
    return v;
}

// Всеки 10 сек: служебен камион без шофьор 5 мин се прибира
stock Fleet_Tick()
{
    for (new c = 0; c < MAX_COMPANIES; c++) {
        for (new s = 0; s < FLEET_MAX; s++) {
            new v = FleetVeh[c][s];
            if (v == INVALID_VEHICLE_ID) continue;
            new busy = 0;
            for (new i = 0; i < MAX_PLAYERS; i++) {
                if (IsPlayerConnected(i) && GetPlayerVehicleID(i) == v) { busy = 1; break; }
            }
            if (busy) { FleetIdle[c][s] = 0; continue; }
            FleetIdle[c][s] += 10;
            if (FleetIdle[c][s] >= FLEET_RESPAWN_SEC) Fleet_Despawn(c, s);
        }
    }
}

// Разбит служебен камион: прибира се (вместо да се появи наново)
stock Fleet_OnVehicleSpawn(vehicleid)
{
    new c = Fleet_CompanyOf(vehicleid);
    if (c == -1) return 0;
    for (new s = 0; s < FLEET_MAX; s++) {
        if (FleetVeh[c][s] == vehicleid) Fleet_Despawn(c, s);
    }
    return 1;
}

// Изкарай служебния камион s: появява се на базата и шофьорът е в него
static stock Fleet_TakeOut(playerid, c, s)
{
    if (Firm_GetCompany(playerid) != c) return SendError(playerid, "Не работиш в тази компания.");
    if (FleetDBID[c][s] == 0) return 1;
    if (FleetVeh[c][s] != INVALID_VEHICLE_ID) return SendError(playerid, "Този камион вече е навън.");
    if (!License_CanDrive(playerid, FleetModel[c][s])) return SendError(playerid, "Нямаш нужната категория за този камион.");
    if (IsPlayerInAnyVehicle(playerid)) return SendError(playerid, "Първо слез от превозното средство.");
    new v = Fleet_Spawn(c, s);
    if (v == INVALID_VEHICLE_ID) return SendError(playerid, "Не успях да изкарам камиона.");
    SetPlayerInterior(playerid, 0);
    SetPlayerVirtualWorld(playerid, 0);
    PutPlayerInVehicle(playerid, v, 0);
    return 1;
}

// Служител: списък със свободните камиони
stock Fleet_ShowTake(playerid, c)
{
    Firm3Company[playerid] = c;
    new body[1024], line[128], name[24], payload[16], n = 0;
    format(body, sizeof (body), "Камион\tТовароносимост\tСъстояние\n");
    for (new s = 0; s < FLEET_MAX; s++) {
        if (FleetDBID[c][s] == 0) continue;
        Veh_GetModelName(FleetModel[c][s], name);
        Catalog_FormatPayload(FleetModel[c][s], payload);
        if (FleetVeh[c][s] == INVALID_VEHICLE_ID) format(line, sizeof (line), "%s\t%s\t" C_GREEN "в гаража\n", name, payload);
        else format(line, sizeof (line), "%s\t%s\t" C_GREY "навън\n", name, payload);
        strcat(body, line);
        FleetBuyMap[playerid][n++] = s;
    }
    if (n == 0) {
        ShowPlayerDialog(playerid, DIALOG_FIRM_INFO, DIALOG_STYLE_MSGBOX, C_BRAND "Служебни камиони",
            C_WHITE "Компанията още няма служебни камиони.", "Назад", "");
        return;
    }
    ShowPlayerDialog(playerid, DIALOG_FLEET_TAKE, DIALOG_STYLE_TABLIST_HEADERS, C_BRAND "Изкарай служебен камион", "%s", "Изкарай", "Назад", body);
}

static stock Fleet_Despawn(c, s)
{
    new v = FleetVeh[c][s];
    if (v == INVALID_VEHICLE_ID) return;
    FleetCompanyOf[v] = -1;
    DestroyVehicle(v);
    FleetVeh[c][s] = INVALID_VEHICLE_ID;
}

stock Fleet_Count(c)
{
    new n = 0;
    for (new s = 0; s < FLEET_MAX; s++) if (FleetDBID[c][s] != 0) n++;
    return n;
}

stock Fleet_Slots(c)
{
    return FleetSlots[CompanyDefs[c][cdSize]];
}

// Може ли да кара курсове с това превозно средство (свое или от автопарка на компанията си)
stock Fleet_CanWork(playerid, vehicleid)
{
    if (Veh_GetOwner(vehicleid) == playerid) return 1;
    new c = Fleet_CompanyOf(vehicleid);
    return (c != -1 && Firm_GetCompany(playerid) == c) ? 1 : 0;
}

// Сяда зад волана на служебен камион: само хората на компанията
stock Fleet_OnDriverEnter(playerid)
{
    new v = GetPlayerVehicleID(playerid), c = Fleet_CompanyOf(v);
    if (c == -1) return;
    if (PlayerData[playerid][pAdmin] >= 1 && PlayerData[playerid][pAdminAuthed]) return;
    if (Firm_GetCompany(playerid) != c) {
        RemovePlayerFromVehicle(playerid);
        SendClientMessage(playerid, COLOR_RED, "[!] " C_WHITE "Това е служебен камион на " C_BRAND "%s" C_WHITE ".", CompanyDefs[c][cdName]);
        return;
    }
    Engine_Set(v, true);
    SendClientMessage(playerid, COLOR_BRAND, "[АВТОПАРК] " C_WHITE "Служебен камион на %s. Натисни " C_BRAND "N" C_WHITE " за поръчки.", CompanyDefs[c][cdName]);
}

stock Fleet_ShowMenu(playerid, c)
{
    Firm3Company[playerid] = c;
    new body[1400], line[128], name[24], payload[16];
    format(body, sizeof (body), "Превозно средство\tТоварoносимост\tСъстояние\n");
    for (new s = 0; s < FLEET_MAX; s++) {
        if (FleetDBID[c][s] == 0) continue;
        Veh_GetModelName(FleetModel[c][s], name);
        Catalog_FormatPayload(FleetModel[c][s], payload);
        new status[48], v = FleetVeh[c][s], driver = INVALID_PLAYER_ID;
        for (new i = 0; i < MAX_PLAYERS && v != INVALID_VEHICLE_ID; i++) {
            if (IsPlayerConnected(i) && GetPlayerVehicleID(i) == v && GetPlayerState(i) == PLAYER_STATE_DRIVER) { driver = i; break; }
        }
        if (driver != INVALID_PLAYER_ID) format(status, sizeof (status), C_BRAND "кара го %s", PlayerData[driver][pName]);
        else if (v != INVALID_VEHICLE_ID) format(status, sizeof (status), C_GREY "навън, без шофьор");
        else format(status, sizeof (status), C_GREEN "в гаража");
        format(line, sizeof (line), "%s\t%s\t%s\n", name, payload, status);
        strcat(body, line);
    }
    format(line, sizeof (line), C_GREEN "+ Купи камион " C_GREY "(%d / %d)\t\t\n", Fleet_Count(c), Fleet_Slots(c));
    strcat(body, line);
    strcat(body, C_BRAND "> Входът на офиса е тук\t\t");
    ShowPlayerDialog(playerid, DIALOG_FLEET, DIALOG_STYLE_TABLIST_HEADERS, C_BRAND "Автопарк", "%s", "Избери", "Назад", body);
}

static stock Fleet_ShowBuy(playerid, c)
{
    new body[1400], line[128], price[16], payload[16], n = 0;
    format(body, sizeof (body), "Модел\tТоварoносимост\tЦена (от касата)\n");
    for (new i = 0; i < sizeof (VehCatalog) && n < 16; i++) {
        FormatMoney(VehCatalog[i][vcPrice], price);
        Catalog_FormatPayload(VehCatalog[i][vcModel], payload);
        format(line, sizeof (line), "%s\t%s\t" C_GREEN "$%s\n", VehCatalog[i][vcName], payload, price);
        strcat(body, line);
        FleetBuyMap[playerid][n++] = i;
    }
    ShowPlayerDialog(playerid, DIALOG_FLEET_BUY, DIALOG_STYLE_TABLIST_HEADERS, C_BRAND "Купи камион за автопарка", "%s", "Купи", "Назад", body);
}

static stock Fleet_Buy(playerid, c, i)
{
    if (!Firm_IsOwnerOf(playerid, c)) return SendError(playerid, "Само собственикът купува камиони.");
    if (Fleet_Count(c) >= Fleet_Slots(c)) return SendError(playerid, "Автопаркът е пълен.");
    new price = VehCatalog[i][vcPrice];
    if (price <= 0) price = 5000;
    if (Company[c][cpCash] < price) return SendError(playerid, "В касата няма достатъчно пари.");
    new s = -1;
    for (new k = 0; k < FLEET_MAX; k++) if (FleetDBID[c][k] == 0) { s = k; break; }
    if (s == -1) return SendError(playerid, "Автопаркът е пълен.");
    new q[160];
    format(q, sizeof (q), "INSERT INTO company_fleet (company, model) VALUES (%d, %d)", c, VehCatalog[i][vcModel]);
    Database_Exec(q);
    new DBResult:r = Database_Query("SELECT last_insert_rowid() AS id");
    new id = 0;
    if (r != DBResult:0) {
        if (DB_GetRowCount(r) > 0) id = DB_GetFieldIntByName(r, "id");
        DB_FreeResultSet(r);
    }
    if (id <= 0) return SendError(playerid, "Грешка при покупката.");
    Company[c][cpCash] -= price;
    Company_Save(c);
    FleetDBID[c][s] = id;
    FleetModel[c][s] = VehCatalog[i][vcModel];
    Fleet_Spawn(c, s);
    new log[64], msg[96];
    format(log, sizeof (log), "Автопарк: %s", VehCatalog[i][vcName]);
    Firm_Log(c, -price, log);
    format(msg, sizeof (msg), "Нов служебен камион: %s! Чака на паркинга на компанията.", VehCatalog[i][vcName]);
    Firm_Broadcast(c, msg);
    PlayerPlaySound(playerid, 1058, 0.0, 0.0, 0.0);
    return 1;
}

static stock Fleet_Sell(playerid, c, s)
{
    if (!Firm_IsOwnerOf(playerid, c) || FleetDBID[c][s] == 0) return 0;
    new cat = Catalog_Find(FleetModel[c][s]);
    new back = (cat == -1) ? 0 : VehCatalog[cat][vcPrice] * FLEET_SELL_PERCENT / 100;
    new v = FleetVeh[c][s];
    for (new i = 0; i < MAX_PLAYERS && v != INVALID_VEHICLE_ID; i++) {
        if (IsPlayerConnected(i) && GetPlayerVehicleID(i) == v) {
            if (Delivery_GetJobVehicle(i) == v) Delivery_Cancel(i, "Служебният камион е продаден.", false);
            RemovePlayerFromVehicle(i);
        }
    }
    Fleet_Despawn(c, s);
    new q[96];
    format(q, sizeof (q), "DELETE FROM company_fleet WHERE id = %d", FleetDBID[c][s]);
    Database_Exec(q);
    FleetDBID[c][s] = 0;
    Company[c][cpCash] += back;
    Company_Save(c);
    Firm_Log(c, back, "Продаден служебен камион");
    SendClientMessage(playerid, COLOR_BRAND, "[АВТОПАРК] " C_WHITE "Продаден за " C_GREEN "$%d" C_WHITE " (в касата).", back);
    return 1;
}

// =====================================================================
// Офис
// =====================================================================

static stock Office_Build(c)
{
    if (OfficePickup[c] != -1) DestroyDynamicPickup(OfficePickup[c]);
    if (OfficeLabel[c] != Text3D:-1) DestroyDynamic3DTextLabel(OfficeLabel[c]);
    if (OfficeActor[c] != INVALID_ACTOR_ID) DestroyActor(OfficeActor[c]);
    if (OfficeInLabel[c] != Text3D:-1) DestroyDynamic3DTextLabel(OfficeInLabel[c]);
    if (OfficeSign[c] != 0) DestroyDynamicObject(OfficeSign[c]);
    OfficePickup[c] = -1;
    OfficeLabel[c] = Text3D:-1;
    OfficeActor[c] = INVALID_ACTOR_ID;
    OfficeInLabel[c] = Text3D:-1;
    OfficeSign[c] = 0;
    if (!Company_IsValid(c)) return;

    new rgb = (Company[c][cpColor] != 0) ? Company[c][cpColor] : 0xF5A623;
    new col[12], text[160];
    format(col, sizeof (col), "{%06x}", rgb);
    format(text, sizeof (text), "%s%s\n" C_WHITE "ОФИС\n" C_GREY "ALT - влез", col, CompanyDefs[c][cdName]);
    OfficePickup[c] = CreateDynamicPickup(1318, 1, OfficeDoor[c][0], OfficeDoor[c][1], OfficeDoor[c][2], 0, 0);
    OfficeLabel[c] = CreateDynamic3DTextLabel(text, COLOR_WHITE, OfficeDoor[c][0], OfficeDoor[c][1], OfficeDoor[c][2] + 0.8, 20.0,
        INVALID_PLAYER_ID, INVALID_VEHICLE_ID, 1, 0, 0);

    // Вътре: табелата с името, секретарката и надпис над рецепцията
    new world = OFFICE_WORLD_BASE + c;
    OfficeSign[c] = Office_CreateSign(world, CompanyDefs[c][cdName], rgb);
    OfficeActor[c] = CreateActor(OFFICE_SECRETARY_SKIN, OFFICE_X, OFFICE_Y + OFFICE_SECRETARY_Y, OFFICE_Z + 1.0, 180.0);
    if (OfficeActor[c] != INVALID_ACTOR_ID) {
        SetActorVirtualWorld(OfficeActor[c], world);
        SetActorInvulnerable(OfficeActor[c], true);
    }
    format(text, sizeof (text), C_BRAND "Рецепция\n" C_WHITE "ALT");
    OfficeInLabel[c] = CreateDynamic3DTextLabel(text, COLOR_WHITE, OFFICE_X, OFFICE_Y + OFFICE_DESK_Y + 0.5, OFFICE_Z + 1.5, 8.0,
        INVALID_PLAYER_ID, INVALID_VEHICLE_ID, 0, world, -1);
}

// Изходът в офиса (веднъж за всички светове)
stock Office_CreateExits()
{
    for (new c = 0; c < MAX_COMPANIES; c++) {
        new world = OFFICE_WORLD_BASE + c;
        CreateDynamicPickup(1318, 1, OFFICE_X + OFFICE_ENTRY_X, OFFICE_Y + OFFICE_ENTRY_Y, OFFICE_Z + 1.0, world, -1);
        CreateDynamic3DTextLabel(C_WHITE "ИЗХОД\n" C_BRAND "ALT", COLOR_WHITE, OFFICE_X + OFFICE_ENTRY_X, OFFICE_Y + OFFICE_ENTRY_Y, OFFICE_Z + 1.8, 10.0,
            INVALID_PLAYER_ID, INVALID_VEHICLE_ID, 0, world, -1);
    }
}

// Етикетите на офиса след смяна на име или цвят, нова или закрита компания
stock Office_Refresh(c)
{
    Office_Build(c);
}

// Закрита компания: автопаркът изчезва, поръчката се маха
stock Firm3_OnCompanyDeleted(c)
{
    for (new s = 0; s < FLEET_MAX; s++) {
        if (FleetDBID[c][s] == 0) continue;
        Fleet_Despawn(c, s);
        FleetDBID[c][s] = 0;
    }
    new q[96];
    format(q, sizeof (q), "DELETE FROM company_fleet WHERE company = %d", c);
    Database_Exec(q);
    FirmOrder[c][foExpires] = 0;
    FirmOrder_Save(c);
    Office_Build(c);
}

// Нова компания: офис до базата
stock Firm3_OnCompanyCreated(c)
{
    OfficeDoor[c][0] = Company[c][cpX] + 6.0;
    OfficeDoor[c][1] = Company[c][cpY] + 6.0;
    OfficeDoor[c][2] = Company[c][cpZ];
    Office_Build(c);
}

static stock Office_Enter(playerid, c)
{
    new Float:x = OFFICE_X + OFFICE_ENTRY_X, Float:y = OFFICE_Y + OFFICE_ENTRY_Y + 0.6, Float:z = OFFICE_Z + 1.0;
    SetPlayerInterior(playerid, OFFICE_INTERIOR);
    SetPlayerVirtualWorld(playerid, OFFICE_WORLD_BASE + c);
    Streamer_UpdateEx(playerid, x, y, z, OFFICE_WORLD_BASE + c, OFFICE_INTERIOR);
    SetPlayerPos(playerid, x, y, z);
    SetPlayerFacingAngle(playerid, 0.0);
    SetCameraBehindPlayer(playerid);
    GameTextForPlayer(playerid, "~y~OFFICE", 2000, 1);
}

static stock Office_Exit(playerid, c)
{
    new Float:a = OfficeDoor[c][3];
    SetPlayerInterior(playerid, 0);
    SetPlayerVirtualWorld(playerid, 0);
    SetPlayerPos(playerid, OfficeDoor[c][0] + 1.5 * floatsin(-a, degrees), OfficeDoor[c][1] + 1.5 * floatcos(-a, degrees), OfficeDoor[c][2]);
    SetPlayerFacingAngle(playerid, a);
    SetCameraBehindPlayer(playerid);
}

stock Office_Inside(playerid)
{
    new c = GetPlayerVirtualWorld(playerid) - OFFICE_WORLD_BASE;
    if (c < 0 || c >= MAX_COMPANIES) return -1;
    return c;
}

static stock Office_SetDoor(playerid, c)
{
    new Float:x, Float:y, Float:z, Float:a;
    GetPlayerPos(playerid, x, y, z);
    GetPlayerFacingAngle(playerid, a);
    if (VectorSize(x - Company[c][cpX], y - Company[c][cpY], 0.0) > 120.0) return SendError(playerid, "Входът трябва да е до 120 м от базата.");
    OfficeDoor[c][0] = x;
    OfficeDoor[c][1] = y;
    OfficeDoor[c][2] = z;
    OfficeDoor[c][3] = a;
    Firm3_SavePoint(c, 0, OfficeDoor[c]);
    Office_Build(c);
    SendClientMessage(playerid, COLOR_BRAND, "[ОФИС] " C_WHITE "Входът на офиса е тук. Гледай навън - натам излизат хората.");
    return 1;
}

stock Office_OnKeyStateChange(playerid, KEY:newkeys, KEY:oldkeys)
{
    if (!((newkeys & KEY_WALK) && !(oldkeys & KEY_WALK))) return 0;
    if (!PlayerData[playerid][pLogged] || IsPlayerInAnyVehicle(playerid)) return 0;

    new inside = Office_Inside(playerid);
    if (inside != -1) {
        if (IsPlayerInRangeOfPoint(playerid, 2.5, OFFICE_X, OFFICE_Y + OFFICE_DESK_Y, OFFICE_Z + 1.0)) {
            if (OfficeActor[inside] != INVALID_ACTOR_ID) ApplyActorAnimation(OfficeActor[inside], "PED", "IDLE_CHAT", 4.1, false, false, false, false, 0);
            Business_Show(playerid, inside);
            return 1;
        }
        if (IsPlayerInRangeOfPoint(playerid, 2.0, OFFICE_X + OFFICE_ENTRY_X, OFFICE_Y + OFFICE_ENTRY_Y, OFFICE_Z + 1.0)) {
            Office_Exit(playerid, inside);
            return 1;
        }
        return 0;
    }
    if (GetPlayerVirtualWorld(playerid) != 0 || GetPlayerInterior(playerid) != 0) return 0;
    for (new c = 0; c < MAX_COMPANIES; c++) {
        if (!Company_IsValid(c)) continue;
        if (IsPlayerInRangeOfPoint(playerid, 1.8, OfficeDoor[c][0], OfficeDoor[c][1], OfficeDoor[c][2])) {
            Office_Enter(playerid, c);
            return 1;
        }
    }
    return 0;
}

// =====================================================================
// Диалози
// =====================================================================

stock Firm3_OnDialogResponse(playerid, dialogid, response, listitem, const inputtext[])
{
    new c = Firm3Company[playerid];
    switch (dialogid) {
        case DIALOG_FIRM_ORDER_INFO: {
            if (!response) { Business_OpenOwn(playerid); return 1; }
            if (Firm_IsOwnerOf(playerid, c)) FirmOrder_Close(c, "отказана от собственика");
            Business_OpenOwn(playerid);
            return 1;
        }
        case DIALOG_FIRM_ORDER_GOOD: {
            if (!response || listitem < 0 || listitem >= MAX_GOODS || FirmOrderGoods[playerid][listitem] == -1) { Business_OpenOwn(playerid); return 1; }
            FirmOrderGood[playerid] = FirmOrderGoods[playerid][listitem];
            ShowPlayerDialog(playerid, DIALOG_FIRM_ORDER_TONS, DIALOG_STYLE_INPUT, C_BRAND "Нова поръчка (2/3): количество",
                C_WHITE "Колко тона %s поръчваш? (1-200)", "Напред", "Назад", Goods[FirmOrderGood[playerid]][gdName]);
            return 1;
        }
        case DIALOG_FIRM_ORDER_TONS: {
            if (!response) { Firm_OrderShow(playerid, c); return 1; }
            new t = strval(inputtext);
            if (!IsNumeric(inputtext) || t < 1 || t > FIRM_ORDER_MAX_TONS) { SendError(playerid, "От 1 до 200 тона."); Firm_OrderShow(playerid, c); return 1; }
            FirmOrderTons[playerid] = t;
            // Ориентир: колко плаща борсата за тон на 3 км
            new hint = Goods_CalcPay(FirmOrderGood[playerid], 1000, 3000.0);
            ShowPlayerDialog(playerid, DIALOG_FIRM_ORDER_PPT, DIALOG_STYLE_INPUT, C_BRAND "Нова поръчка (3/3): цена",
                C_WHITE "Колко плащаш на тон? ($50-5000)\n" C_GREY "Ориентир: борсата плаща около $%d на тон за 3 км.\n" \
                C_GREY "Колкото повече даваш, толкова по-бързо ще я изпълнят.", "Поръчай", "Назад", hint);
            return 1;
        }
        case DIALOG_FIRM_ORDER_PPT: {
            if (!response) { Firm_OrderShow(playerid, c); return 1; }
            if (!Firm_IsOwnerOf(playerid, c)) return 1;
            new ppt = strval(inputtext);
            if (!IsNumeric(inputtext) || ppt < FIRM_ORDER_MIN_PPT || ppt > FIRM_ORDER_MAX_PPT) { SendError(playerid, "Цената е от $50 до $5000 на тон."); Firm_OrderShow(playerid, c); return 1; }
            new total = ppt * FirmOrderTons[playerid];
            if (Company[c][cpCash] < total) {
                SendClientMessage(playerid, COLOR_RED, "[!] " C_WHITE "Трябват $%d в касата (цялата поръчка се заделя предварително).", total);
                return 1;
            }
            Company[c][cpCash] -= total;
            Company_Save(c);
            FirmOrder[c][foGood] = FirmOrderGood[playerid];
            FirmOrder[c][foTotalKg] = FirmOrderTons[playerid] * 1000;
            FirmOrder[c][foLeftKg] = FirmOrder[c][foTotalKg];
            FirmOrder[c][foPPT] = ppt;
            FirmOrder[c][foReserved] = total;
            FirmOrder[c][foExpires] = gettime() + FIRM_ORDER_HOURS * 3600;
            FirmOrder_Save(c);
            new log[64], msg[128];
            format(log, sizeof (log), "Заделени за поръчка (%s)", Goods[FirmOrder[c][foGood]][gdName]);
            Firm_Log(c, -total, log);
            Contracts_AddOrderOffers();
            format(msg, sizeof (msg), "Нова поръчка: %d т %s по $%d/т - на борсата е!", FirmOrderTons[playerid], Goods[FirmOrder[c][foGood]][gdName], ppt);
            Firm_Broadcast(c, msg);
            SendClientMessageToAll(COLOR_BRAND, "[БОРСА] " C_WHITE "%s поръчва " C_BRAND "%d т %s" C_WHITE " по " C_GREEN "$%d/т" C_WHITE ". Натисни N в камиона.",
                CompanyDefs[c][cdName], FirmOrderTons[playerid], Goods[FirmOrder[c][foGood]][gdName], ppt);
            return 1;
        }
        case DIALOG_FLEET: {
            if (!response) { Business_OpenOwn(playerid); return 1; }
            if (!Firm_IsOwnerOf(playerid, c)) return SendError(playerid, "Само собственикът управлява автопарка.");
            new n = Fleet_Count(c);
            if (listitem < n) {
                // n-тият зает слот
                new k = 0;
                for (new s = 0; s < FLEET_MAX; s++) {
                    if (FleetDBID[c][s] == 0) continue;
                    if (k++ == listitem) { FleetPick[playerid] = s; break; }
                }
                new name[24], title[48];
                Veh_GetModelName(FleetModel[c][FleetPick[playerid]], name);
                format(title, sizeof (title), C_BRAND "%s", name);
                ShowPlayerDialog(playerid, DIALOG_FLEET_ACT, DIALOG_STYLE_LIST, title, "Изкарай го на базата\n" C_RED "Продай го", "Избери", "Назад");
            } else if (listitem == n) Fleet_ShowBuy(playerid, c);
            else {
                Office_SetDoor(playerid, c);
                Fleet_ShowMenu(playerid, c);
            }
            return 1;
        }
        case DIALOG_FLEET_BUY: {
            if (response && listitem >= 0 && listitem < 16) Fleet_Buy(playerid, c, FleetBuyMap[playerid][listitem]);
            Fleet_ShowMenu(playerid, c);
            return 1;
        }
        case DIALOG_FLEET_ACT: {
            if (!response) { Fleet_ShowMenu(playerid, c); return 1; }
            if (listitem == 0) Fleet_TakeOut(playerid, c, FleetPick[playerid]);
            else {
                new name[24];
                Veh_GetModelName(FleetModel[c][FleetPick[playerid]], name);
                ShowPlayerDialog(playerid, DIALOG_FLEET_SELL, DIALOG_STYLE_MSGBOX, C_BRAND "Служебен камион",
                    C_WHITE "%s\n\nДа го продам ли? Касата получава %d%% от цената.", "Продай", "Назад", name, FLEET_SELL_PERCENT);
            }
            return 1;
        }
        case DIALOG_FLEET_TAKE: {
            if (!response) { Business_OpenOwn(playerid); return 1; }
            if (listitem >= 0 && listitem < FLEET_MAX) Fleet_TakeOut(playerid, c, FleetBuyMap[playerid][listitem]);
            return 1;
        }
        case DIALOG_FLEET_SELL: {
            if (response) Fleet_Sell(playerid, c, FleetPick[playerid]);
            Fleet_ShowMenu(playerid, c);
            return 1;
        }
    }
    return 0;
}
