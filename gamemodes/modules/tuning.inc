// Тунинг и ремонт.
//
// В гаража: седиш в своето превозно средство и натискаш ALT -> ремонт, цвят, неон, подобрения.
// На бензиностанция: ALT с изгасен двигател -> гориво или ремонт.
//
// Подобрения (по 3 нива): икономичен двигател (-10% разход на ниво), голям резервоар (+20% на ниво),
// здрава кабина (-15% щети на ниво). Всичко се пази към превозното средство.

#define REPAIR_PRICE_PER_HP     2       // $ за всяка точка здраве до 1000
#define REPAIR_MIN_PRICE        50
#define COLOR_PRICE             500
#define NEON_PRICE              3000
#define ARMOR_PERCENT_PER_LEVEL 15

static const UpgradePrices[] = {6000, 14000, 28000};    // ниво 1, 2, 3
static const UpgradeNames[MAX_UPGRADES][] = {"Икономичен двигател", "Голям резервоар", "Здрава кабина"};

// Цветове на SA (ID на цвета, име, приблизителен цвят за прозореца)
enum E_TUNE_COLOR {
    tcID,
    tcName[24],
    tcHex[10]
}
static const TuneColors[][E_TUNE_COLOR] = {
    {0,  "Черен",          "{333333}"},
    {1,  "Бял",            "{F5F5F5}"},
    {3,  "Червен",         "{C01020}"},
    {6,  "Оранжев",        "{D78E10}"},
    {2,  "Син",            "{2A77A1}"},
    {7,  "Светлосин",      "{4C75B7}"},
    {16, "Зелен",          "{335F3F}"},
    {5,  "Лилав",          "{86446E}"},
    {8,  "Сребрист",       "{BDBEC6}"},
    {13, "Тъмносив",       "{58595A}"},
    {17, "Тъмночервен",    "{730E1A}"},
    {4,  "Графит",         "{263739}"}
};

// Неон (обекти) – тръби под шасито
enum E_TUNE_NEON {
    tnModel,
    tnName[24],
    tnHex[10]
}
static const TuneNeons[][E_TUNE_NEON] = {
    {18647, "Червен",   "{FF3030}"},
    {18648, "Син",      "{3070FF}"},
    {18649, "Зелен",    "{30FF60}"},
    {18650, "Жълт",     "{FFE030}"},
    {18651, "Розов",    "{FF60D0}"},
    {18652, "Бял",      "{FFFFFF}"}
};

enum {
    TUNE_ITEM_REPAIR,
    TUNE_ITEM_COLOR1,
    TUNE_ITEM_COLOR2,
    TUNE_ITEM_NEON,
    TUNE_ITEM_ENGINE,
    TUNE_ITEM_TANK,
    TUNE_ITEM_ARMOR
}

static TuneVehicle[MAX_PLAYERS];
static TuneColorWhich[MAX_PLAYERS];
static TuneUpgrade[MAX_PLAYERS];
static Float:ArmorLastHealth[MAX_VEHICLES];

// ---------- Ремонт ----------

stock Repair_GetCost(vehicleid)
{
    new Float:hp;
    GetVehicleHealth(vehicleid, hp);
    if (hp >= 999.0) return 0;
    new cost = floatround((1000.0 - hp) * REPAIR_PRICE_PER_HP);
    return (cost < REPAIR_MIN_PRICE) ? REPAIR_MIN_PRICE : cost;
}

static stock Repair_Do(playerid, vehicleid)
{
    new cost = Repair_GetCost(vehicleid);
    if (cost == 0) return SendInfo(playerid, "Превозното средство е здраво.");
    if (!Money_Take(playerid, cost)) return SendError(playerid, "Нямаш достатъчно пари за ремонт.");
    RepairVehicle(vehicleid);
    SetVehicleHealth(vehicleid, 1000.0);
    ArmorLastHealth[vehicleid] = 1000.0;
    PlayerPlaySound(playerid, 1133, 0.0, 0.0, 0.0);
    SendClientMessage(playerid, COLOR_BRAND, "[СЕРВИЗ] " C_WHITE "Ремонтът струва " C_GREEN "$%d" C_WHITE ". Като нов!", cost);
    return 1;
}

// ---------- Бензиностанция: гориво или ремонт ----------

stock Tune_ShowStation(playerid, vehicleid)
{
    TuneVehicle[playerid] = vehicleid;
    new cost = Repair_GetCost(vehicleid), repair[64];
    if (cost == 0) format(repair, sizeof (repair), C_GREY "Ремонт (не е нужен)");
    else format(repair, sizeof (repair), C_WHITE "Ремонт - " C_GREEN "$%d", cost);
    ShowPlayerDialog(playerid, DIALOG_STATION, DIALOG_STYLE_LIST, C_BRAND "Бензиностанция",
        C_WHITE "Зареди гориво\n%s\n" C_WHITE "Магазин и тоалетна\n" C_WHITE "Почивка (умора)", "Избери", "Затвори", repair);
}

// ---------- Тунинг в гаража ----------

stock Tune_ShowMain(playerid, vehicleid)
{
    TuneVehicle[playerid] = vehicleid;
    new body[900], line[160], name[24], cost = Repair_GetCost(vehicleid);
    Veh_GetModelName(GetVehicleModel(vehicleid), name);

    if (cost == 0) format(body, sizeof (body), "Ремонт\t" C_GREY "не е нужен\n");
    else format(body, sizeof (body), "Ремонт\t" C_GREEN "$%d\n", cost);

    format(line, sizeof (line), "Основен цвят\t" C_GREEN "$%d\n", COLOR_PRICE);
    strcat(body, line);
    format(line, sizeof (line), "Втори цвят\t" C_GREEN "$%d\n", COLOR_PRICE);
    strcat(body, line);
    if (Veh_GetNeon(vehicleid) == 0) format(line, sizeof (line), "Неон\t" C_GREEN "$%d\n", NEON_PRICE);
    else format(line, sizeof (line), "Неон\t" C_BRAND "има (смени/махни)\n");
    strcat(body, line);

    for (new u = 0; u < MAX_UPGRADES; u++) {
        new level = Veh_GetUpgrade(vehicleid, u);
        if (level >= 3) format(line, sizeof (line), "%s\t" C_BRAND "ниво 3/3 (макс.)\n", UpgradeNames[u]);
        else format(line, sizeof (line), "%s\tниво %d/3, следващо " C_GREEN "$%d\n", UpgradeNames[u], level, UpgradePrices[level]);
        strcat(body, line);
    }

    new title[64];
    format(title, sizeof (title), C_BRAND "Тунинг" C_WHITE " | %s", name);
    ShowPlayerDialog(playerid, DIALOG_TUNE_MAIN, DIALOG_STYLE_TABLIST, title, "%s", "Избери", "Затвори", body);
}

static stock Tune_ShowColors(playerid, which)
{
    TuneColorWhich[playerid] = which;
    new body[800], line[64];
    body[0] = EOS;
    for (new i = 0; i < sizeof (TuneColors); i++) {
        format(line, sizeof (line), "%s%s\n", TuneColors[i][tcHex], TuneColors[i][tcName]);
        strcat(body, line);
    }
    new title[48];
    if (which == 0) format(title, sizeof (title), C_BRAND "Основен цвят");
    else format(title, sizeof (title), C_BRAND "Втори цвят");
    ShowPlayerDialog(playerid, DIALOG_TUNE_COLOR, DIALOG_STYLE_LIST, title, "%s", "Боядисай", "Назад", body);
}

static stock Tune_ShowNeon(playerid)
{
    new body[600], line[64];
    body[0] = EOS;
    for (new i = 0; i < sizeof (TuneNeons); i++) {
        format(line, sizeof (line), "%s%s\n", TuneNeons[i][tnHex], TuneNeons[i][tnName]);
        strcat(body, line);
    }
    strcat(body, C_RED "Махни неона");
    ShowPlayerDialog(playerid, DIALOG_TUNE_NEON, DIALOG_STYLE_LIST, C_BRAND "Неон", "%s", "Избери", "Назад", body);
}

static stock Tune_AskUpgrade(playerid, type)
{
    new vehicleid = TuneVehicle[playerid], level = Veh_GetUpgrade(vehicleid, type);
    if (level >= 3) {
        SendInfo(playerid, "Това подобрение е на максимално ниво.");
        Tune_ShowMain(playerid, vehicleid);
        return;
    }
    TuneUpgrade[playerid] = type;
    new effect[96];
    switch (type) {
        case UPGRADE_ENGINE: format(effect, sizeof (effect), "Разход на гориво: -%d%% (сега -%d%%)", (level + 1) * 10, level * 10);
        case UPGRADE_TANK:   format(effect, sizeof (effect), "Резервоар: +%d%% (сега +%d%%)", (level + 1) * 20, level * 20);
        default:             format(effect, sizeof (effect), "Щети: -%d%% (сега -%d%%)", (level + 1) * ARMOR_PERCENT_PER_LEVEL, level * ARMOR_PERCENT_PER_LEVEL);
    }
    new price[16];
    FormatMoney(UpgradePrices[level], price);
    ShowPlayerDialog(playerid, DIALOG_TUNE_UPGRADE, DIALOG_STYLE_MSGBOX, C_BRAND "Подобрение",
        C_BRAND "%s" C_WHITE " - ниво %d\n\n%s\n\nЦена: " C_GREEN "$%s",
        "Купи", "Назад", UpgradeNames[type], level + 1, effect, price);
}

// ---------- Здрава кабина: намалява щетите ----------

// Вика се всяка секунда за шофьорите
stock Tune_ArmorTick(playerid)
{
    new vehicleid = GetPlayerVehicleID(playerid);
    if (vehicleid == 0 || GetPlayerState(playerid) != PLAYER_STATE_DRIVER) return;
    new Float:hp;
    GetVehicleHealth(vehicleid, hp);
    new level = Veh_GetUpgrade(vehicleid, UPGRADE_ARMOR);
    if (level > 0 && hp < ArmorLastHealth[vehicleid] && ArmorLastHealth[vehicleid] - hp < 1000.0) {
        new Float:refund = (ArmorLastHealth[vehicleid] - hp) * float(level * ARMOR_PERCENT_PER_LEVEL) / 100.0;
        hp += refund;
        SetVehicleHealth(vehicleid, hp);
    }
    ArmorLastHealth[vehicleid] = hp;
}

// Нов шофьор / ново превозно средство – започваме отначало (без "стари" стойности)
stock Tune_ResetArmor(vehicleid)
{
    if (vehicleid <= 0 || vehicleid >= MAX_VEHICLES) return;
    new Float:hp;
    GetVehicleHealth(vehicleid, hp);
    ArmorLastHealth[vehicleid] = hp;
}

// ---------- ALT ----------

stock Tune_OnKeyStateChange(playerid, KEY:newkeys, KEY:oldkeys)
{
    new KEY:keys = KEY_FIRE | KEY_WALK;
    if (!((newkeys & keys) && !(oldkeys & keys))) return 0;
    if (!PlayerData[playerid][pInGarage] || GetPlayerState(playerid) != PLAYER_STATE_DRIVER) return 0;
    new vehicleid = GetPlayerVehicleID(playerid);
    if (Veh_GetOwner(vehicleid) != playerid) return 0;
    // До вратата ALT не прави нищо специално с превозното средство (изходът е на H)
    Tune_ShowMain(playerid, vehicleid);
    return 1;
}

stock Tune_OnDialogResponse(playerid, dialogid, response, listitem)
{
    new vehicleid = TuneVehicle[playerid];
    switch (dialogid) {
        case DIALOG_STATION: {
            if (!response) return 1;
            if (GetPlayerVehicleID(playerid) != vehicleid) return 1;
            if (listitem == 0) Fuel_ShowRefuel(playerid, vehicleid);
            else if (listitem == 1) Repair_Do(playerid, vehicleid);
            else if (listitem == 2) Needs_ShowShop(playerid, SHOP_KIND_STATION);
            else Fatigue_StartRest(playerid);
            return 1;
        }
        case DIALOG_TUNE_MAIN: {
            if (!response || GetPlayerVehicleID(playerid) != vehicleid || !PlayerData[playerid][pInGarage]) return 1;
            switch (listitem) {
                case TUNE_ITEM_REPAIR: {
                    Repair_Do(playerid, vehicleid);
                    Tune_ShowMain(playerid, vehicleid);
                }
                case TUNE_ITEM_COLOR1: Tune_ShowColors(playerid, 0);
                case TUNE_ITEM_COLOR2: Tune_ShowColors(playerid, 1);
                case TUNE_ITEM_NEON:   Tune_ShowNeon(playerid);
                case TUNE_ITEM_ENGINE: Tune_AskUpgrade(playerid, UPGRADE_ENGINE);
                case TUNE_ITEM_TANK:   Tune_AskUpgrade(playerid, UPGRADE_TANK);
                case TUNE_ITEM_ARMOR:  Tune_AskUpgrade(playerid, UPGRADE_ARMOR);
            }
            return 1;
        }
        case DIALOG_TUNE_COLOR: {
            if (GetPlayerVehicleID(playerid) != vehicleid) return 1;
            if (response && listitem >= 0 && listitem < sizeof (TuneColors)) {
                if (Veh_GetColor(vehicleid, TuneColorWhich[playerid]) == TuneColors[listitem][tcID]) {
                    SendInfo(playerid, "Превозното средство вече е в този цвят.");
                } else if (Money_Take(playerid, COLOR_PRICE)) {
                    Veh_SetColor(vehicleid, TuneColorWhich[playerid], TuneColors[listitem][tcID]);
                    PlayerPlaySound(playerid, 1134, 0.0, 0.0, 0.0);
                    SendClientMessage(playerid, COLOR_BRAND, "[ТУНИНГ] " C_WHITE "Боядисано в %s%s" C_WHITE " за " C_GREEN "$%d" C_WHITE ".",
                        TuneColors[listitem][tcHex], TuneColors[listitem][tcName], COLOR_PRICE);
                } else {
                    SendError(playerid, "Нямаш достатъчно пари.");
                }
            }
            Tune_ShowMain(playerid, vehicleid);
            return 1;
        }
        case DIALOG_TUNE_NEON: {
            if (GetPlayerVehicleID(playerid) != vehicleid) return 1;
            if (response) {
                if (listitem == sizeof (TuneNeons)) {
                    Veh_SetNeon(vehicleid, 0);
                    SendInfo(playerid, "Неонът е махнат.");
                } else if (listitem >= 0 && listitem < sizeof (TuneNeons)) {
                    if (Money_Take(playerid, NEON_PRICE)) {
                        Veh_SetNeon(vehicleid, TuneNeons[listitem][tnModel]);
                        SendClientMessage(playerid, COLOR_BRAND, "[ТУНИНГ] " C_WHITE "Сложен е %s%s неон" C_WHITE " за " C_GREEN "$%d" C_WHITE ".",
                            TuneNeons[listitem][tnHex], TuneNeons[listitem][tnName], NEON_PRICE);
                    } else {
                        SendError(playerid, "Нямаш достатъчно пари.");
                    }
                }
            }
            Tune_ShowMain(playerid, vehicleid);
            return 1;
        }
        case DIALOG_TUNE_UPGRADE: {
            if (GetPlayerVehicleID(playerid) != vehicleid) return 1;
            if (response) {
                new type = TuneUpgrade[playerid], level = Veh_GetUpgrade(vehicleid, type);
                if (level < 3) {
                    if (Money_Take(playerid, UpgradePrices[level])) {
                        Veh_SetUpgrade(vehicleid, type, level + 1);
                        PlayerPlaySound(playerid, 1133, 0.0, 0.0, 0.0);
                        SendClientMessage(playerid, COLOR_BRAND, "[ТУНИНГ] " C_BRAND "%s" C_WHITE " - ниво %d!", UpgradeNames[type], level + 1);
                    } else {
                        SendError(playerid, "Нямаш достатъчно пари.");
                    }
                }
            }
            Tune_ShowMain(playerid, vehicleid);
            return 1;
        }
    }
    return 0;
}
