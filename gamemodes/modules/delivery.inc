// Курсове А -> Б: пул от поръчки, табло, товарене, доставка, плащане и панел (TextDraw).
//
// Поръчка: товар + компания А (дава) + компания Б (приема) + тегло.
// Плащане = $150 + км x тонове x ставка на товара (виж modules/goods.inc).
// Таблото показва само поръчки, които превозното средство кара и побира.

#define MAX_CONTRACTS           50
#define TRAILER_MIN_KG          10000       // поръчки за влекачи (с ремарке): 10-38 т
#define TRAILER_MAX_KG          38000
#define TRAILER_CHANCE          35          // % от поръчките за кутии/насипни са тежки (с ремарке)
#define TRAILER_REATTACH_RANGE  15.0
#define SPILL_DAMAGE_PERCENT    25          // гориво: над толкова % щети -> разлив
#define SPILL_FINE              300
#define CONTRACT_SHOW_MAX       15
#define DELIVERY_CP_SIZE        6.0
#define DELIVERY_LOAD_MS        10000
#define DELIVERY_UNLOAD_MS      8000
#define DELIVERY_FREE_CANCEL    120         // секунди без глоба
#define DELIVERY_CANCEL_FINE    100
#define DELIVERY_SPEED_MS       12.0        // за срока: м/с средно
#define PERISHABLE_SPEED_MS     15.0        // хладилни товари: по-кратък срок
#define PERISHABLE_LOSS_PER_MIN 15          // хладилни: -15% от плащането за всяка минута закъснение
#define FRAGILE_BREAK_PERCENT   40          // чупливи: над толкова % щети товарът се чупи
#define OVERLOAD_MAX_PERCENT    130         // поръчки до толкова % от товароносимостта (претоварване)
#define WEIGH_RANGE             25.0        // кантар: в този радиус започва проверката
#define WEIGH_PLATFORM_RANGE    9.0         // колко близо до центъра = на платформата
#define WEIGH_STOP_SECONDS      10          // толкова секунди трябва да стои на платформата
#define WEIGH_STILL_KMH         3           // под тази скорост = стои
#define WEIGH_SKIP_FINE         300
#define WEIGH_FINE_PER_TON      250
#define WEIGH_FINE_BASE         200
#define DELIVERY_EXTRA_TIME     120         // + секунди към срока

enum E_CONTRACT {
    bool:ctActive,
    ctGood,
    ctFrom,
    ctTo,
    ctKg
}

static Contracts[MAX_CONTRACTS][E_CONTRACT];

enum {
    JOB_NONE,
    JOB_TO_PICKUP,
    JOB_LOADING,
    JOB_TO_DROP,
    JOB_UNLOADING
}

enum E_JOB {
    jbStage,
    jbGood,
    jbFrom,
    jbTo,
    jbKg,
    jbBasePay,
    jbCompanyBonus,         // бонус от компанията А (вече взет от касата й)
    jbVehicle,
    jbTrailer,              // ремаркето (INVALID_VEHICLE_ID = без)
    jbLastWeigh,            // последният кантар, който е проверил този курс
    jbWeighPending,         // кантарът, в чийто район е влязъл (-1 = никой)
    jbWeighWait,            // секунди престой на платформата
    bool:jbTrailerLost,     // откачено по пътя
    jbAcceptedAt,
    jbDeadline,             // gettime(), когато трябва да е доставено (след товарене)
    Float:jbStartHealth,
    jbTimerEnd              // GetTickCount() за края на товарене/разтоварване
}

static Job[MAX_PLAYERS][E_JOB];
static BoardMap[MAX_PLAYERS][CONTRACT_SHOW_MAX];
static BoardCount[MAX_PLAYERS];

// ---------- Пул с поръчки ----------

static stock Contracts_Generate(slot)
{
    // Опитваме няколко пъти да намерим двойка дава/приема
    for (new attempt = 0; attempt < 40; attempt++) {
        new good = random(MAX_GOODS);

        // Само складове, които имат товара / имат място за него
        new producers[MAX_COMPANIES], np = 0, acceptors[MAX_COMPANIES], na = 0;
        for (new c = 0; c < MAX_COMPANIES; c++) {
            if (Company_Produces(c, good) && CompanyStock[c][good] >= Goods[good][gdMinKg]) producers[np++] = c;
            if (Economy_CanAccept(c, good)) acceptors[na++] = c;
        }
        if (np == 0 || na == 0) continue;

        // Получателят: от два случайни – този с по-голямо търсене (по-празен склад)
        new from = producers[random(np)], to = acceptors[random(na)];
        new other = acceptors[random(na)];
        if (Economy_FillPercent(other, good) < Economy_FillPercent(to, good)) to = other;
        if (from == to) continue;
        if (Company_Distance(from, to) < 600.0) continue;       // твърде близо

        new minKg = Goods[good][gdMinKg], maxKg = Goods[good][gdMaxKg];
        new cargo = Goods[good][gdCargo];
        // Горивото е само с ремарке; кутии и насипни – понякога тежки (с ремарке)
        if (cargo == CARGO_LIQUID || ((cargo == CARGO_BOX || cargo == CARGO_BULK) && random(100) < TRAILER_CHANCE)) {
            minKg = TRAILER_MIN_KG;
            maxKg = TRAILER_MAX_KG;
        }
        new kg = minKg + random(maxKg - minKg + 1);
        kg = (kg / 100) * 100;
        if (kg < 100) kg = 100;
        if (CompanyStock[from][good] < kg) continue;           // складът няма толкова

        Contracts[slot][ctActive] = true;
        Contracts[slot][ctGood] = good;
        Contracts[slot][ctFrom] = from;
        Contracts[slot][ctTo] = to;
        Contracts[slot][ctKg] = kg;
        return 1;
    }
    Contracts[slot][ctActive] = false;
    return 0;
}

stock Contracts_Init()
{
    for (new i = 0; i < MAX_CONTRACTS; i++) Contracts_Generate(i);
}

// Заета поръчка се заменя с нова след малко
forward Contracts_Refill(slot);
public Contracts_Refill(slot)
{
    Contracts_Generate(slot);
    return 1;
}

stock Contracts_OnCompanyMoved(c)
{
    #pragma unused c
    // Разстоянията се смятат в момента, така че нищо не трябва да се прави.
}

// Плащане = база по разстояние, тегло и ценност x търсенето при получателя
static stock Contracts_GetPay(slot)
{
    new base = Goods_CalcPay(Contracts[slot][ctGood], Contracts[slot][ctKg],
        Company_Distance(Contracts[slot][ctFrom], Contracts[slot][ctTo]));
    return base * Economy_DemandPercent(Contracts[slot][ctTo], Contracts[slot][ctGood]) / 100;
}

// ---------- Табло ----------

static stock bool:Delivery_CanCarry(model, slot)
{
    new good = Contracts[slot][ctGood];
    if (!(Catalog_GetCargo(model) & Goods[good][gdCargo])) return false;
    // Влекачите взимат само тежки товари (с ремарке)
    if (Catalog_HasTrailer(model) && Contracts[slot][ctKg] < TRAILER_MIN_KG) return false;
    // До 130% от товароносимостта (претоварване – риск от глоба на кантар)
    return (Contracts[slot][ctKg] <= Catalog_GetPayload(model) * OVERLOAD_MAX_PERCENT / 100);
}

// onlyFrom = -1: всички (подредени по близост до базата А); иначе само от тази компания
stock Delivery_ShowBoard(playerid, onlyFrom = -1)
{
    if (Job[playerid][jbStage] != JOB_NONE) {
        Delivery_ShowCurrent(playerid);
        return 1;
    }

    new vehicleid = GetPlayerVehicleID(playerid);
    if (vehicleid == 0 || GetPlayerState(playerid) != PLAYER_STATE_DRIVER || Veh_GetOwner(vehicleid) != playerid) {
        ShowPlayerDialog(playerid, DIALOG_MENU_INFO, DIALOG_STYLE_MSGBOX, C_BRAND "Работа",
            C_WHITE "Седни зад волана на своето превозно средство,\nза да видиш поръчките, които то може да кара.", "Добре", "");
        return 1;
    }
    if (PlayerData[playerid][pInGarage]) {
        SendError(playerid, "Първо излез от гаража.");
        return 1;
    }
    new model = GetVehicleModel(vehicleid);

    // Подходящи поръчки с разстояние до А
    new list[MAX_CONTRACTS], Float:dist[MAX_CONTRACTS], n = 0;
    for (new i = 0; i < MAX_CONTRACTS; i++) {
        if (!Contracts[i][ctActive] || !Delivery_CanCarry(model, i)) continue;
        if (onlyFrom != -1 && Contracts[i][ctFrom] != onlyFrom) continue;
        new from = Contracts[i][ctFrom];
        list[n] = i;
        dist[n] = GetPlayerDistanceFromPoint(playerid, Company[from][cpX], Company[from][cpY], Company[from][cpZ]);
        n++;
    }
    // Сортиране по разстояние (малко елементи - прост метод)
    for (new a = 0; a < n - 1; a++) {
        for (new b = a + 1; b < n; b++) {
            if (dist[b] < dist[a]) {
                new t = list[a]; list[a] = list[b]; list[b] = t;
                new Float:tf = dist[a]; dist[a] = dist[b]; dist[b] = tf;
            }
        }
    }

    if (n == 0) {
        new name[24], payload[16], extra[24];
        if (onlyFrom != -1) format(extra, sizeof (extra), " от тази компания");
        Veh_GetModelName(model, name);
        Catalog_FormatPayload(model, payload);
        ShowPlayerDialog(playerid, DIALOG_MENU_INFO, DIALOG_STYLE_MSGBOX, C_BRAND "Работа",
            C_WHITE "В момента няма поръчки за " C_BRAND "%s" C_WHITE " (%s)%s.\nОпитай пак след малко.", "Добре", "",
            name, payload, extra);
        return 1;
    }

    new body[3200], line[260], tons[48], payStr[48], money[16];
    format(body, sizeof (body), "Товар\tОт > До\tТегло, разстояние\tПлащане\n");
    BoardCount[playerid] = 0;
    for (new k = 0; k < n && k < CONTRACT_SHOW_MAX; k++) {
        new i = list[k], good = Contracts[i][ctGood];
        FormatTons(Contracts[i][ctKg], tons);
        if (Contracts[i][ctKg] > Catalog_GetPayload(model)) strcat(tons, " " C_RED "претов." C_WHITE);
        new pay = Contracts_GetPay(i), demand = Economy_DemandPercent(Contracts[i][ctTo], Contracts[i][ctGood]) - 100;
        FormatMoney(pay + Company_GetBonusAmount(Contracts[i][ctFrom], pay), money);
        if (demand > 0) format(payStr, sizeof (payStr), "%s " C_GREEN "+%d%%", money, demand);
        else if (demand < 0) format(payStr, sizeof (payStr), "%s " C_RED "%d%%", money, demand);
        else format(payStr, sizeof (payStr), "%s", money);
        if (Firm_OrderBonus(playerid, Contracts[i][ctFrom]) > 0) strcat(payStr, " " C_BRAND "ФИРМА");
        new gname[64];
        Delivery_GoodLabel(good, gname);
        format(line, sizeof (line), "%s\t%s > %s\t%s, %.1f км\t" C_GREEN "$%s\n",
            gname, CompanyDefs[Contracts[i][ctFrom]][cdName], CompanyDefs[Contracts[i][ctTo]][cdName],
            tons, Company_Distance(Contracts[i][ctFrom], Contracts[i][ctTo]) / 1000.0, payStr);
        strcat(body, line);
        BoardMap[playerid][BoardCount[playerid]++] = i;
    }

    new title[96];
    if (onlyFrom != -1) format(title, sizeof (title), C_BRAND "Поръчки" C_WHITE " | %s", CompanyDefs[onlyFrom][cdName]);
    else format(title, sizeof (title), C_BRAND "Поръчки" C_WHITE " | най-близките до теб");
    ShowPlayerDialog(playerid, DIALOG_JOB_BOARD, DIALOG_STYLE_TABLIST_HEADERS, title, "%s", "Детайли", "Назад", body);
    return 1;
}

static BoardPick[MAX_PLAYERS];

static stock Delivery_ShowDetails(playerid, slot)
{
    new good = Contracts[slot][ctGood], from = Contracts[slot][ctFrom], to = Contracts[slot][ctTo];
    new tons[16], payStr[16], stars[48], bonusLine[96];
    FormatTons(Contracts[slot][ctKg], tons);
    new pay = Contracts_GetPay(slot), compBonus = Company_GetBonusAmount(from, pay);
    FormatMoney(pay + compBonus, payStr);
    if (compBonus > 0) format(bonusLine, sizeof (bonusLine), C_GREEN "Включен бонус от компанията: +%d%%\n", Company[from][cpBonus]);
    Goods_Stars(good, stars);
    new Float:toA = GetPlayerDistanceFromPoint(playerid, Company[from][cpX], Company[from][cpY], Company[from][cpZ]);
    new Float:ab = Company_Distance(from, to);
    new limit = Delivery_TimeLimit(good, ab);
    new rules[160];
    if (Goods[good][gdFlags] & GOODFLAG_PERISHABLE) {
        format(rules, sizeof (rules), "\n" C_CB "Хладилен товар: кратък срок; за всяка минута закъснение -%d%% (разваля се).", PERISHABLE_LOSS_PER_MIN);
    } else if (Goods[good][gdFlags] & GOODFLAG_FRAGILE) {
        format(rules, sizeof (rules), "\n" C_RED "Чуплив товар: щетите тежат двойно, над %d%% щети се чупи.", FRAGILE_BREAK_PERCENT);
    }

    ShowPlayerDialog(playerid, DIALOG_JOB_DETAILS, DIALOG_STYLE_MSGBOX, C_BRAND "Поръчка",
        C_WHITE "Товар: " C_BRAND "%s" C_WHITE ", %s, ценност %s\n\n" \
        C_WHITE "Взимаш от: " C_BRAND "%s" C_GREY " (%s) - на %.1f км от теб\n" \
        C_WHITE "Караш до: " C_BRAND "%s" C_GREY " (%s) - %.1f км\n\n" \
        C_WHITE "Плащане: " C_GREEN "$%s" C_WHITE ", срок след товарене: %d:%02d мин\n%s" \
        C_GREY "Навреме +10%%, закъснял -20%%, щети до -50%%.%s",
        "Приеми", "Назад",
        Goods[good][gdName], tons, stars,
        CompanyDefs[from][cdName], CompanyDefs[from][cdPlace], toA / 1000.0,
        CompanyDefs[to][cdName], CompanyDefs[to][cdPlace], ab / 1000.0,
        payStr, limit / 60, limit % 60, bonusLine, rules);
}

// ---------- Курс ----------

static stock Delivery_SetCheckpoint(playerid, company, bool:finish)
{
    DisablePlayerRaceCheckpoint(playerid);
    SetPlayerRaceCheckpoint(playerid, finish ? CP_TYPE_GROUND_FINISH : CP_TYPE_GROUND_NORMAL,
        Company[company][cpX], Company[company][cpY], Company[company][cpZ],
        Company[company][cpX], Company[company][cpY], Company[company][cpZ], DELIVERY_CP_SIZE);
    Route_Set(playerid, Company[company][cpX], Company[company][cpY], Company[company][cpZ], ROUTE_SRC_JOB);
}

static stock Delivery_Accept(playerid, slot)
{
    if (!Contracts[slot][ctActive]) {
        SendError(playerid, "Тази поръчка вече е взета от друг шофьор.");
        Delivery_ShowBoard(playerid);
        return;
    }
    new vehicleid = GetPlayerVehicleID(playerid);
    if (vehicleid == 0 || Veh_GetOwner(vehicleid) != playerid || !Delivery_CanCarry(GetVehicleModel(vehicleid), slot)) {
        SendError(playerid, "Трябва да си в своето превозно средство, което може да кара този товар.");
        return;
    }
    // Гориво и други течни товари - нужна е категория ADR
    if (Goods[Contracts[slot][ctGood]][gdCargo] == CARGO_LIQUID && !License_Has(playerid, LIC_ADR)) {
        SendError(playerid, "За гориво ти трябва категория ADR (опасни товари). Вземи я в автошкола.");
        return;
    }
    // Складът може да се е изпразнил междувременно
    if (!Economy_HasStock(Contracts[slot][ctFrom], Contracts[slot][ctGood], Contracts[slot][ctKg])
        || !Economy_CanAccept(Contracts[slot][ctTo], Contracts[slot][ctGood])) {
        SendError(playerid, "Поръчката вече не е актуална (складът се промени). Избери друга.");
        Contracts_Generate(slot);
        Delivery_ShowBoard(playerid);
        return;
    }
    // Товарът се запазва от склада на А веднага
    Economy_Take(Contracts[slot][ctFrom], Contracts[slot][ctGood], Contracts[slot][ctKg]);

    Job[playerid][jbStage] = JOB_TO_PICKUP;
    Job[playerid][jbGood] = Contracts[slot][ctGood];
    Job[playerid][jbFrom] = Contracts[slot][ctFrom];
    Job[playerid][jbTo] = Contracts[slot][ctTo];
    Job[playerid][jbKg] = Contracts[slot][ctKg];
    Job[playerid][jbBasePay] = Contracts_GetPay(slot);
    Job[playerid][jbCompanyBonus] = Company_GetBonusAmount(Contracts[slot][ctFrom], Job[playerid][jbBasePay]);
    if (Job[playerid][jbCompanyBonus] > 0) {
        // Бонусът се запазва от касата веднага (връща се при отказ)
        Company[Contracts[slot][ctFrom]][cpCash] -= Job[playerid][jbCompanyBonus];
        Company_Save(Contracts[slot][ctFrom]);
    }
    Job[playerid][jbVehicle] = vehicleid;
    Job[playerid][jbTrailer] = INVALID_VEHICLE_ID;
    Job[playerid][jbTrailerLost] = false;
    Job[playerid][jbLastWeigh] = -1;
    Job[playerid][jbWeighPending] = -1;
    Job[playerid][jbWeighWait] = 0;
    Job[playerid][jbAcceptedAt] = gettime();
    Job[playerid][jbDeadline] = 0;

    Contracts[slot][ctActive] = false;
    SetTimerEx("Contracts_Refill", 30000, false, "i", slot);

    Gps_Cancel(playerid);
    Delivery_SetCheckpoint(playerid, Job[playerid][jbFrom], false);
    Delivery_ShowPanel(playerid);

    SendClientMessage(playerid, COLOR_BRAND, "[КУРС] " C_WHITE "Приет! Карай до " C_BRAND "%s" C_WHITE " и влез в червения маркер, за да товариш.",
        CompanyDefs[Job[playerid][jbFrom]][cdName]);

    // Ако вече е в базата - товарим веднага
    if (IsPlayerInRaceCheckpoint(playerid)) Delivery_OnEnterRaceCP(playerid);
}

static stock Delivery_End(playerid)
{
    Job[playerid][jbStage] = JOB_NONE;
    Trailer_Destroy(playerid);
    Route_Clear(playerid, ROUTE_SRC_JOB);
    DisablePlayerRaceCheckpoint(playerid);
    Delivery_HidePanel(playerid);
}

stock Delivery_Cancel(playerid, const reason[], bool:fine)
{
    if (Job[playerid][jbStage] == JOB_NONE) return;
    if (Job[playerid][jbStage] == JOB_LOADING || Job[playerid][jbStage] == JOB_UNLOADING) {
        TogglePlayerControllable(playerid, true);
    }
    Delivery_RefundBonus(playerid);
    Economy_Put(Job[playerid][jbFrom], Job[playerid][jbGood], Job[playerid][jbKg]);   // товарът се връща в склада
    Delivery_End(playerid);
    if (fine && Money_Take(playerid, DELIVERY_CANCEL_FINE)) {
        SendClientMessage(playerid, COLOR_RED, "[КУРС] " C_WHITE "%s Глоба: $%d.", reason, DELIVERY_CANCEL_FINE);
    } else {
        SendClientMessage(playerid, COLOR_RED, "[КУРС] " C_WHITE "%s", reason);
    }
}

static stock Delivery_StartTimer(playerid, stage, ms)
{
    Job[playerid][jbStage] = stage;
    Job[playerid][jbTimerEnd] = GetTickCount() + ms;
    TogglePlayerControllable(playerid, false);
    SetTimerEx("Delivery_TimerDone", ms, false, "iii", playerid, PlayerData[playerid][pConnSeq], stage);
}

forward Delivery_TimerDone(playerid, seq, stage);
public Delivery_TimerDone(playerid, seq, stage)
{
    if (!IsPlayerConnected(playerid) || PlayerData[playerid][pConnSeq] != seq) return 0;
    if (Job[playerid][jbStage] != stage) return 0;
    TogglePlayerControllable(playerid, true);

    if (stage == JOB_LOADING) {
        if (Catalog_HasTrailer(GetVehicleModel(Job[playerid][jbVehicle]))) Trailer_Spawn(playerid);
        new Float:ab = Company_Distance(Job[playerid][jbFrom], Job[playerid][jbTo]);
        Job[playerid][jbDeadline] = gettime() + Delivery_TimeLimit(Job[playerid][jbGood], ab);
        GetVehicleHealth(Job[playerid][jbVehicle], Job[playerid][jbStartHealth]);
        Job[playerid][jbStage] = JOB_TO_DROP;
        Delivery_SetCheckpoint(playerid, Job[playerid][jbTo], true);
        PlayerPlaySound(playerid, 1056, 0.0, 0.0, 0.0);
        SendClientMessage(playerid, COLOR_BRAND, "[КУРС] " C_WHITE "Натоварено! Карай до " C_BRAND "%s" C_WHITE " (%s).",
            CompanyDefs[Job[playerid][jbTo]][cdName], CompanyDefs[Job[playerid][jbTo]][cdPlace]);
    } else if (stage == JOB_UNLOADING) {
        Delivery_Complete(playerid);
    }
    return 1;
}

// Щети в проценти от началото на превоза (0-100)
static stock Delivery_DamagePercent(playerid)
{
    if (Job[playerid][jbStage] != JOB_TO_DROP && Job[playerid][jbStage] != JOB_UNLOADING) return 0;
    new Float:hp;
    GetVehicleHealth(Job[playerid][jbVehicle], hp);
    new lost = floatround((Job[playerid][jbStartHealth] - hp) / 7.5);
    if (lost < 0) lost = 0;
    if (lost > 100) lost = 100;
    return lost;
}

// Връща неизползвания бонус в касата на компанията
static stock Delivery_RefundBonus(playerid)
{
    new amount = Job[playerid][jbCompanyBonus], from = Job[playerid][jbFrom];
    if (amount <= 0) return;
    Job[playerid][jbCompanyBonus] = 0;
    if (Company[from][cpOwner] != 0) {
        Company[from][cpCash] += amount;
        Company_Save(from);
    }
}

// Очаквано плащане в момента
static stock Delivery_CurrentPay(playerid, &bonus = 0, &late = 0, &damage = 0)
{
    new base = Job[playerid][jbBasePay];
    bonus = 0;
    late = 0;
    new flags = Goods[Job[playerid][jbGood]][gdFlags];
    if (Job[playerid][jbDeadline] != 0) {
        new over = gettime() - Job[playerid][jbDeadline];
        if (over <= 0) bonus = base / 10;
        else if (flags & GOODFLAG_PERISHABLE) {
            // Разваля се: -15% за всяка започната минута закъснение
            new pct = ((over + 59) / 60) * PERISHABLE_LOSS_PER_MIN;
            if (pct > 100) pct = 100;
            late = base * pct / 100;
        }
        else late = base / 5;
    }
    new dmg = Delivery_DamagePercent(playerid);
    if (flags & GOODFLAG_FRAGILE) dmg *= 2;
    if (dmg > 50 && !(flags & GOODFLAG_FRAGILE)) dmg = 50;
    if (dmg > 100) dmg = 100;
    damage = base * dmg / 100;
    new total = base + bonus - late - damage;
    if (total < 0) total = 0;
    return total + Job[playerid][jbCompanyBonus];
}

static stock Delivery_Complete(playerid)
{
    new bonus, late, damage;
    new total = Delivery_CurrentPay(playerid, bonus, late, damage);
    new from = Job[playerid][jbFrom], to = Job[playerid][jbTo], good = Job[playerid][jbGood];
    new Float:ab = Company_Distance(from, to);
    new exp = floatround(ab / 100.0) + ((bonus > 0) ? 10 : 0);
    if (exp < 1) exp = 1;
    new base = Job[playerid][jbBasePay];

    Delivery_End(playerid);
    new firmLine[192];
    total += Firm_OnDelivery(playerid, from, base, total, firmLine, sizeof (firmLine));
    if (total < 0) total = 0;
    Money_Give(playerid, total);
    Level_GiveExp(playerid, exp);
    Company_OnDelivery(from, to, base);
    Economy_Put(to, good, Job[playerid][jbKg]);
    Tutorial_OnDelivery(playerid);

    new sBase[16], sBonus[16], sLate[16], sDmg[16], sTotal[16], tons[16], sComp[16];
    FormatMoney(Job[playerid][jbCompanyBonus], sComp);
    FormatMoney(base, sBase);
    FormatMoney(bonus, sBonus);
    FormatMoney(late, sLate);
    FormatMoney(damage, sDmg);
    FormatMoney(total, sTotal);
    FormatTons(Job[playerid][jbKg], tons);

    PlayerPlaySound(playerid, 1058, 0.0, 0.0, 0.0);
    GameTextForPlayer(playerid, "~g~DELIVERED!~n~~w~+$%s", 4000, 1, sTotal);
    ShowPlayerDialog(playerid, DIALOG_JOB_DONE, DIALOG_STYLE_MSGBOX, C_GREEN "Доставено!",
        C_WHITE "%s (%s): %s > %s\n\n" \
        C_WHITE "Основно плащане:\t" C_GREEN "$%s\n" \
        C_WHITE "Бонус навреме:\t" C_GREEN "+$%s\n" \
        C_WHITE "Закъснение:\t" C_RED "-$%s\n" \
        C_WHITE "Щети:\t\t" C_RED "-$%s\n" \
        C_WHITE "Бонус от компанията:\t" C_GREEN "+$%s\n" \
        "%s\n" \
        C_WHITE "Общо: " C_GREEN "$%s" C_WHITE ", опит: " C_BRAND "+%d",
        "Нов курс", "Затвори",
        Goods[good][gdName], tons, CompanyDefs[from][cdName], CompanyDefs[to][cdName],
        sBase, sBonus, sLate, sDmg, sComp, firmLine, sTotal, exp);
}

// Влизане в маркера (товарене/разтоварване стартира само - удобно и на таблет)
stock Delivery_OnEnterRaceCP(playerid)
{
    new stage = Job[playerid][jbStage];
    if (stage != JOB_TO_PICKUP && stage != JOB_TO_DROP) return 0;

    if (GetPlayerVehicleID(playerid) != Job[playerid][jbVehicle] || GetPlayerState(playerid) != PLAYER_STATE_DRIVER) {
        SendError(playerid, "Влез в маркера с превозното средство, с което си приел курса.");
        return 1;
    }
    if (stage == JOB_TO_DROP && Job[playerid][jbTrailer] != INVALID_VEHICLE_ID
        && GetVehicleTrailer(Job[playerid][jbVehicle]) != Job[playerid][jbTrailer]) {
        SendError(playerid, "Ремаркето не е закачено. Иди до него и натисни H.");
        return 1;
    }
    if (stage == JOB_TO_PICKUP) {
        Delivery_StartTimer(playerid, JOB_LOADING, DELIVERY_LOAD_MS);
        SendClientMessage(playerid, COLOR_BRAND, "[КУРС] " C_WHITE "Товарене...");
    } else {
        Delivery_StartTimer(playerid, JOB_UNLOADING, DELIVERY_UNLOAD_MS);
        SendClientMessage(playerid, COLOR_BRAND, "[КУРС] " C_WHITE "Разтоварване...");
    }
    return 1;
}

stock Delivery_ShowCurrent(playerid)
{
    new good = Job[playerid][jbGood], tons[16];
    FormatTons(Job[playerid][jbKg], tons);
    new target = (Job[playerid][jbStage] <= JOB_LOADING) ? Job[playerid][jbFrom] : Job[playerid][jbTo];
    ShowPlayerDialog(playerid, DIALOG_JOB_CURRENT, DIALOG_STYLE_LIST, C_BRAND "Текущ курс",
        C_WHITE "%s (%s) > %s\n" C_BRAND "Покажи маркера отново\n" C_RED "Откажи курса",
        "Избери", "Назад", Goods[good][gdName], tons, CompanyDefs[target][cdName]);
}

// ---------- Бутон N - всичко за работата ----------
// Текущ курс -> детайли/отказ. Без курс: в база на компания -> нейните поръчки, иначе най-близките.

stock Delivery_OpenWork(playerid)
{
    if (Job[playerid][jbStage] != JOB_NONE) {
        Delivery_ShowCurrent(playerid);
        return 1;
    }
    new c = (GetPlayerState(playerid) == PLAYER_STATE_DRIVER) ? Company_GetNear(playerid) : -1;
    Delivery_ShowBoard(playerid, c);
    return 1;
}

stock Delivery_OnKeyStateChange(playerid, KEY:newkeys, KEY:oldkeys)
{
    // H (клаксон) – закачане на откачено ремарке
    if ((newkeys & KEY_CROUCH) && !(oldkeys & KEY_CROUCH) && Job[playerid][jbStage] == JOB_TO_DROP) {
        if (Trailer_TryReattach(playerid)) return 1;
    }
    if (!((newkeys & KEY_NO) && !(oldkeys & KEY_NO))) return 0;
    if (!PlayerData[playerid][pLogged] || !PlayerData[playerid][pSpawned]) return 0;
    return Delivery_OpenWork(playerid);
}

CMD:work(playerid, params[])
{
    #pragma unused params
    return Delivery_OpenWork(playerid);
}

// ---------- Събития ----------

stock Delivery_OnPlayerConnect(playerid)
{
    Job[playerid][jbStage] = JOB_NONE;
    Job[playerid][jbTrailer] = INVALID_VEHICLE_ID;
    Delivery_ResetPanel(playerid);
}

stock Delivery_OnPlayerDisconnect(playerid)
{
    if (Job[playerid][jbStage] != JOB_NONE) {
        Delivery_RefundBonus(playerid);
        Economy_Put(Job[playerid][jbFrom], Job[playerid][jbGood], Job[playerid][jbKg]);
    }
    Job[playerid][jbStage] = JOB_NONE;
    Trailer_Destroy(playerid);
    Delivery_DestroyPanel(playerid);
}

stock Delivery_OnVehicleDeath(vehicleid)
{
    for (new i = 0; i < MAX_PLAYERS; i++) {
        if (IsPlayerConnected(i) && Job[i][jbStage] != JOB_NONE
            && (Job[i][jbVehicle] == vehicleid || Job[i][jbTrailer] == vehicleid)) {
            Delivery_Cancel(i, "Превозното средство е унищожено - курсът е провален.", false);
        }
    }
}

stock Delivery_OnDialogResponse(playerid, dialogid, response, listitem)
{
    switch (dialogid) {
        case DIALOG_JOB_BOARD: {
            if (!response) return 1;
            if (listitem < 0 || listitem >= BoardCount[playerid]) return 1;
            BoardPick[playerid] = BoardMap[playerid][listitem];
            Delivery_ShowDetails(playerid, BoardPick[playerid]);
            return 1;
        }
        case DIALOG_JOB_DETAILS: {
            if (!response) {
                Delivery_ShowBoard(playerid);
                return 1;
            }
            Delivery_Accept(playerid, BoardPick[playerid]);
            return 1;
        }
        case DIALOG_JOB_DONE: {
            if (response) Delivery_ShowBoard(playerid);
            return 1;
        }
        case DIALOG_JOB_CURRENT: {
            if (!response || Job[playerid][jbStage] == JOB_NONE) return 1;
            if (listitem == 1) {
                new stage = Job[playerid][jbStage];
                if (stage == JOB_TO_PICKUP) Delivery_SetCheckpoint(playerid, Job[playerid][jbFrom], false);
                else if (stage == JOB_TO_DROP) Delivery_SetCheckpoint(playerid, Job[playerid][jbTo], true);
            } else if (listitem == 2) {
                new bool:fine = (gettime() - Job[playerid][jbAcceptedAt] > DELIVERY_FREE_CANCEL);
                Delivery_Cancel(playerid, "Курсът е отказан.", fine);
            }
            return 1;
        }
    }
    return 0;
}

// ======================= Панел (TextDraw) =======================
// Вдясно по средата на екрана (вляво от бутоните на мобилния клиент). На латиница.

#define DP_X            488.0
#define DP_Y            160.0
#define DP_W            176.0
#define DP_LINES        6

static PlayerText:DpBox[MAX_PLAYERS] = {INVALID_PLAYER_TEXT_DRAW, ...};
static PlayerText:DpHeader[MAX_PLAYERS] = {INVALID_PLAYER_TEXT_DRAW, ...};
static PlayerText:DpTitle[MAX_PLAYERS] = {INVALID_PLAYER_TEXT_DRAW, ...};
static PlayerText:DpLine[MAX_PLAYERS][DP_LINES];
static bool:DpShown[MAX_PLAYERS];

stock Delivery_ResetPanel(playerid)
{
    DpShown[playerid] = false;
    DpBox[playerid] = INVALID_PLAYER_TEXT_DRAW;
    DpHeader[playerid] = INVALID_PLAYER_TEXT_DRAW;
    DpTitle[playerid] = INVALID_PLAYER_TEXT_DRAW;
    for (new i = 0; i < DP_LINES; i++) DpLine[playerid][i] = INVALID_PLAYER_TEXT_DRAW;
}

static stock PlayerText:Dp_Box(playerid, Float:y, Float:lines, color)
{
    new PlayerText:t = CreatePlayerTextDraw(playerid, DP_X, y, "_");
    PlayerTextDrawUseBox(playerid, t, true);
    PlayerTextDrawBoxColour(playerid, t, color);
    PlayerTextDrawAlignment(playerid, t, TEXT_DRAW_ALIGN_CENTRE);
    PlayerTextDrawLetterSize(playerid, t, 0.0, lines);
    PlayerTextDrawTextSize(playerid, t, 0.0, DP_W);
    return t;
}

static stock PlayerText:Dp_Text(playerid, Float:x, Float:y, Float:size, TEXT_DRAW_ALIGN:align, TEXT_DRAW_FONT:font)
{
    new PlayerText:t = CreatePlayerTextDraw(playerid, x, y, "_");
    PlayerTextDrawFont(playerid, t, font);
    PlayerTextDrawLetterSize(playerid, t, size, size * 4.2);
    PlayerTextDrawAlignment(playerid, t, align);
    PlayerTextDrawColour(playerid, t, COLOR_WHITE);
    PlayerTextDrawSetOutline(playerid, t, 1);
    PlayerTextDrawSetShadow(playerid, t, 0);
    PlayerTextDrawBackgroundColour(playerid, t, 0x00000088);
    PlayerTextDrawSetProportional(playerid, t, true);
    return t;
}

static stock Delivery_CreatePanel(playerid)
{
    Delivery_DestroyPanel(playerid);
    new Float:left = DP_X - DP_W / 2.0 + 6.0;

    DpBox[playerid]    = Dp_Box(playerid, DP_Y, 9.6, 0x101418CC);
    DpHeader[playerid] = Dp_Box(playerid, DP_Y, 1.7, 0xF5A623FF);
    DpTitle[playerid]  = Dp_Text(playerid, DP_X, DP_Y + 1.0, 0.26, TEXT_DRAW_ALIGN_CENTRE, TEXT_DRAW_FONT_2);
    PlayerTextDrawColour(playerid, DpTitle[playerid], 0x101418FF);
    PlayerTextDrawSetOutline(playerid, DpTitle[playerid], 0);

    for (new i = 0; i < DP_LINES; i++) {
        DpLine[playerid][i] = Dp_Text(playerid, left, DP_Y + 19.0 + i * 11.5, 0.2, TEXT_DRAW_ALIGN_LEFT, TEXT_DRAW_FONT_1);
    }
}

stock Delivery_DestroyPanel(playerid)
{
    if (DpBox[playerid] != INVALID_PLAYER_TEXT_DRAW) PlayerTextDrawDestroy(playerid, DpBox[playerid]);
    if (DpHeader[playerid] != INVALID_PLAYER_TEXT_DRAW) PlayerTextDrawDestroy(playerid, DpHeader[playerid]);
    if (DpTitle[playerid] != INVALID_PLAYER_TEXT_DRAW) PlayerTextDrawDestroy(playerid, DpTitle[playerid]);
    for (new i = 0; i < DP_LINES; i++) {
        if (DpLine[playerid][i] != INVALID_PLAYER_TEXT_DRAW) PlayerTextDrawDestroy(playerid, DpLine[playerid][i]);
    }
    Delivery_ResetPanel(playerid);
}

stock Delivery_ShowPanel(playerid)
{
    if (DpBox[playerid] == INVALID_PLAYER_TEXT_DRAW) Delivery_CreatePanel(playerid);
    Delivery_UpdatePanel(playerid);
    PlayerTextDrawShow(playerid, DpBox[playerid]);
    PlayerTextDrawShow(playerid, DpHeader[playerid]);
    PlayerTextDrawShow(playerid, DpTitle[playerid]);
    for (new i = 0; i < DP_LINES; i++) PlayerTextDrawShow(playerid, DpLine[playerid][i]);
    DpShown[playerid] = true;
}

stock Delivery_HidePanel(playerid)
{
    Delivery_DestroyPanel(playerid);
}

// Вика се от таймера на HUD-а
stock Delivery_UpdatePanel(playerid)
{
    if (DpBox[playerid] == INVALID_PLAYER_TEXT_DRAW || Job[playerid][jbStage] == JOB_NONE) return;

    new stage = Job[playerid][jbStage], good = Job[playerid][jbGood];
    new from = Job[playerid][jbFrom], to = Job[playerid][jbTo];
    new text[128], stars[32], money[16];

    // Заглавие
    switch (stage) {
        case JOB_TO_PICKUP: format(text, sizeof (text), "DRIVE TO PICKUP");
        case JOB_LOADING:   format(text, sizeof (text), "LOADING...");
        case JOB_TO_DROP: {
            if (Job[playerid][jbTrailerLost]) format(text, sizeof (text), "TRAILER DETACHED! (H)");
            else format(text, sizeof (text), "DELIVER THE CARGO");
        }
        case JOB_UNLOADING: format(text, sizeof (text), "UNLOADING...");
    }
    PlayerTextDrawSetString(playerid, DpTitle[playerid], text);

    // 1: товар, тегло, ценност
    format(stars, sizeof (stars), "%d/5", Goods[good][gdStars]);
    new mark[40];
    if (Goods[good][gdFlags] & GOODFLAG_PERISHABLE) format(mark, sizeof (mark), "  ~b~COLD");
    else if (Goods[good][gdFlags] & GOODFLAG_FRAGILE) format(mark, sizeof (mark), "  ~r~FRAGILE");
    if (Job[playerid][jbKg] > Catalog_GetPayload(GetVehicleModel(Job[playerid][jbVehicle]))) strcat(mark, "  ~r~OVER");
    format(text, sizeof (text), "~w~CARGO  ~y~%s  ~w~%d.%d T  ~y~%s%s", Goods[good][gdNameEN], Job[playerid][jbKg] / 1000, (Job[playerid][jbKg] % 1000) / 100, stars, mark);
    PlayerTextDrawSetString(playerid, DpLine[playerid][0], text);

    // 2-3: откъде, докъде
    format(text, sizeof (text), "%sFROM  ~w~%s", (stage <= JOB_LOADING) ? ("~y~") : ("~w~"), CompanyDefs[from][cdName]);
    PlayerTextDrawSetString(playerid, DpLine[playerid][1], text);
    format(text, sizeof (text), "%sTO      ~w~%s", (stage >= JOB_TO_DROP) ? ("~y~") : ("~w~"), CompanyDefs[to][cdName]);
    PlayerTextDrawSetString(playerid, DpLine[playerid][2], text);

    // 4: разстояние до текущата цел
    new target = (stage <= JOB_LOADING) ? from : to;
    new Float:dist = GetPlayerDistanceFromPoint(playerid, Company[target][cpX], Company[target][cpY], Company[target][cpZ]);
    format(text, sizeof (text), "~w~DISTANCE  ~y~%.1f KM", dist / 1000.0);
    PlayerTextDrawSetString(playerid, DpLine[playerid][3], text);

    // 5: време / прогрес
    if (stage == JOB_LOADING || stage == JOB_UNLOADING) {
        new total = (stage == JOB_LOADING) ? DELIVERY_LOAD_MS : DELIVERY_UNLOAD_MS;
        new left = Job[playerid][jbTimerEnd] - GetTickCount();
        if (left < 0) left = 0;
        new filled = (total - left) * 20 / total, bar[64];
        format(bar, sizeof (bar), "~g~");
        for (new i = 0; i < 20; i++) {
            if (i == filled) strcat(bar, "~w~");
            strcat(bar, "I");
        }
        format(text, sizeof (text), "~w~PROGRESS  %s", bar);
    } else if (Job[playerid][jbDeadline] == 0) {
        format(text, sizeof (text), "~w~TIME  ~y~starts after loading");
    } else {
        new left = Job[playerid][jbDeadline] - gettime();
        if (left >= 0) format(text, sizeof (text), "~w~TIME LEFT  ~g~%d:%02d", left / 60, left % 60);
        else format(text, sizeof (text), "~w~LATE  ~r~%d:%02d", (-left) / 60, (-left) % 60);
    }
    PlayerTextDrawSetString(playerid, DpLine[playerid][4], text);

    // 6: плащане и щети
    new dmg = Delivery_DamagePercent(playerid);
    FormatMoney(Delivery_CurrentPay(playerid), money);
    format(text, sizeof (text), "~w~PAY  ~g~$%s   ~w~DAMAGE  %s%d%%", money, (dmg >= 20) ? ("~r~") : ("~w~"), dmg);
    PlayerTextDrawSetString(playerid, DpLine[playerid][5], text);
}

stock Delivery_Tick()
{
    for (new i = 0; i < MAX_PLAYERS; i++) {
        if (Job[i][jbStage] == JOB_NONE || !IsPlayerConnected(i)) continue;
        Trailer_Check(i);
        Delivery_SpeedLimit(i);
        Delivery_CheckSpill(i);
        if (Job[i][jbStage] != JOB_NONE) Delivery_CheckFragile(i);
        if (Job[i][jbStage] != JOB_NONE) Delivery_CheckWeigh(i);
        if (Job[i][jbStage] != JOB_NONE) Delivery_UpdatePanel(i);
    }
}

// ======================= Ремаркета =======================

static stock Trailer_ModelFor(good)
{
    switch (Goods[good][gdCargo]) {
        case CARGO_BULK:   return 450;                          // самосвал
        case CARGO_LIQUID: return 584;                          // цистерна
    }
    return (random(2) == 0) ? 435 : 591;                        // затворено ремарке
}

// Създава ремаркето зад влекача и го закача
static stock Trailer_Spawn(playerid)
{
    Trailer_Destroy(playerid);
    new truck = Job[playerid][jbVehicle];
    new Float:x, Float:y, Float:z, Float:a;
    GetVehiclePos(truck, x, y, z);
    GetVehicleZAngle(truck, a);
    // 10 м зад камиона
    x -= 10.0 * floatsin(-a, degrees);
    y -= 10.0 * floatcos(-a, degrees);
    new trailer = CreateVehicle(Trailer_ModelFor(Job[playerid][jbGood]), x, y, z + 0.5, a, 1, 1, -1);
    if (trailer == INVALID_VEHICLE_ID) return;
    SetVehicleVirtualWorld(trailer, GetVehicleVirtualWorld(truck));
    Job[playerid][jbTrailer] = trailer;
    Job[playerid][jbTrailerLost] = false;
    // Малко изчакване, за да се появи при клиента преди закачането
    SetTimerEx("Trailer_Attach", 400, false, "iii", playerid, PlayerData[playerid][pConnSeq], trailer);
}

forward Trailer_Attach(playerid, seq, trailer);
public Trailer_Attach(playerid, seq, trailer)
{
    if (!IsPlayerConnected(playerid) || PlayerData[playerid][pConnSeq] != seq) return 0;
    if (Job[playerid][jbTrailer] != trailer) return 0;
    AttachTrailerToVehicle(trailer, Job[playerid][jbVehicle]);
    return 1;
}

static stock Trailer_Destroy(playerid)
{
    new trailer = Job[playerid][jbTrailer];
    if (trailer != INVALID_VEHICLE_ID && trailer != 0) {
        DestroyVehicle(trailer);
    }
    Job[playerid][jbTrailer] = INVALID_VEHICLE_ID;
    Job[playerid][jbTrailerLost] = false;
}

// Следи дали ремаркето е откачено (вика се от таймера)
static stock Trailer_Check(playerid)
{
    new trailer = Job[playerid][jbTrailer];
    if (trailer == INVALID_VEHICLE_ID || Job[playerid][jbStage] != JOB_TO_DROP) return;
    new bool:attached = (GetVehicleTrailer(Job[playerid][jbVehicle]) == trailer);
    if (!attached && !Job[playerid][jbTrailerLost]) {
        Job[playerid][jbTrailerLost] = true;
        SetVehicleParamsForPlayer(trailer, playerid, 1, -1);     // стрелка над ремаркето
        SendClientMessage(playerid, COLOR_RED, "[КУРС] " C_WHITE "Ремаркето се откачи! Иди до него с камиона и натисни " C_BRAND "H" C_WHITE ".");
    } else if (attached && Job[playerid][jbTrailerLost]) {
        Job[playerid][jbTrailerLost] = false;
        SetVehicleParamsForPlayer(trailer, playerid, 0, -1);
    }
}

// H (клаксонът) до откаченото ремарке -> закача го отново
static stock Trailer_TryReattach(playerid)
{
    new trailer = Job[playerid][jbTrailer];
    if (trailer == INVALID_VEHICLE_ID || !Job[playerid][jbTrailerLost]) return 0;
    if (GetPlayerVehicleID(playerid) != Job[playerid][jbVehicle] || GetPlayerState(playerid) != PLAYER_STATE_DRIVER) return 0;
    new Float:tx, Float:ty, Float:tz;
    GetVehiclePos(trailer, tx, ty, tz);
    if (!IsPlayerInRangeOfPoint(playerid, TRAILER_REATTACH_RANGE, tx, ty, tz)) {
        SendError(playerid, "Приближи се до ремаркето (до 15 м).");
        return 1;
    }
    // Слагаме ремаркето точно зад камиона и го закачаме
    new Float:x, Float:y, Float:z, Float:a;
    GetVehiclePos(Job[playerid][jbVehicle], x, y, z);
    GetVehicleZAngle(Job[playerid][jbVehicle], a);
    SetVehiclePos(trailer, x - 10.0 * floatsin(-a, degrees), y - 10.0 * floatcos(-a, degrees), z + 0.5);
    SetVehicleZAngle(trailer, a);
    SetTimerEx("Trailer_Attach", 400, false, "iii", playerid, PlayerData[playerid][pConnSeq], trailer);
    SendClientMessage(playerid, COLOR_BRAND, "[КУРС] " C_WHITE "Ремаркето е закачено отново.");
    return 1;
}

// Тежкият товар ограничава скоростта: над 10 т - 110 км/ч, над 20 т - 95 км/ч
static stock Delivery_SpeedLimit(playerid)
{
    if (Job[playerid][jbStage] != JOB_TO_DROP) return;
    new kg = Job[playerid][jbKg], Float:limit;
    if (kg > 20000) limit = 95.0;
    else if (kg > 10000) limit = 110.0;
    else return;

    new vehicleid = Job[playerid][jbVehicle];
    if (GetPlayerVehicleID(playerid) != vehicleid) return;
    new Float:vx, Float:vy, Float:vz;
    GetVehicleVelocity(vehicleid, vx, vy, vz);
    new Float:kmh = VectorSize(vx, vy, vz) * 180.0;
    if (kmh > limit) {
        new Float:f = limit / kmh;
        SetVehicleVelocity(vehicleid, vx * f, vy * f, vz);
    }
}

// Гориво: при силен удар -> разлив, курсът пропада
// Срок за доставка (секунди) според разстоянието и товара
static stock Delivery_TimeLimit(good, Float:meters)
{
    if (Goods[good][gdFlags] & GOODFLAG_PERISHABLE) return floatround(meters / PERISHABLE_SPEED_MS) + DELIVERY_EXTRA_TIME / 2;
    return floatround(meters / DELIVERY_SPEED_MS) + DELIVERY_EXTRA_TIME;
}

// Име на товара с отметка (хладилен) / (чуплив) – за таблото
static stock Delivery_GoodLabel(good, dest[], size = sizeof (dest))
{
    if (Goods[good][gdFlags] & GOODFLAG_PERISHABLE) format(dest, size, "%s " C_CB "(хладилен)" C_WHITE, Goods[good][gdName]);
    else if (Goods[good][gdFlags] & GOODFLAG_FRAGILE) format(dest, size, "%s " C_RED "(чуплив)" C_WHITE, Goods[good][gdName]);
    else format(dest, size, "%s", Goods[good][gdName]);
}

// Трябва ли играчът да се претегли на кантар w (натоварен, в своя камион, още непретеглен там)
stock bool:Delivery_MustWeigh(playerid, w)
{
    return (Job[playerid][jbStage] == JOB_TO_DROP && GetPlayerVehicleID(playerid) == Job[playerid][jbVehicle] && Job[playerid][jbLastWeigh] != w);
}

// Кантар: с натоварен товар в района на кантар трябва да спре на платформата за 10 сек.
// Ако напусне района без претегляне - глоба. Всеки кантар проверява веднъж на курс.
static stock Delivery_CheckWeigh(playerid)
{
    if (Job[playerid][jbStage] != JOB_TO_DROP) return;
    new vehicleid = Job[playerid][jbVehicle];
    if (GetPlayerVehicleID(playerid) != vehicleid) return;

    new w = Places_GetNearWeigh(playerid, WEIGH_RANGE);
    new pending = Job[playerid][jbWeighPending];

    // Напусна района на кантар, без да се претегли
    if (pending != -1 && w != pending) {
        Job[playerid][jbLastWeigh] = pending;
        Job[playerid][jbWeighPending] = -1;
        Job[playerid][jbWeighWait] = 0;
        Money_Take(playerid, WEIGH_SKIP_FINE);
        GameTextForPlayer(playerid, "~r~WEIGH STATION~n~~w~FINE $%d", 3000, 1, WEIGH_SKIP_FINE);
        SendClientMessage(playerid, COLOR_RED, "[КАНТАР] " C_WHITE "Не спря за претегляне! Глоба $%d.", WEIGH_SKIP_FINE);
        return;
    }
    if (w == -1 || w == Job[playerid][jbLastWeigh]) return;

    // Влиза в района
    if (pending == -1) {
        Job[playerid][jbWeighPending] = w;
        Job[playerid][jbWeighWait] = 0;
        GameTextForPlayer(playerid, "~y~WEIGH STATION~n~~w~STOP ON THE PLATFORM", 3000, 3);
        SendClientMessage(playerid, COLOR_BRAND, "[КАНТАР] " C_WHITE "Кантар! Спри на платформата и изчакай %d секунди.", WEIGH_STOP_SECONDS);
        return;
    }

    new Float:vx, Float:vy, Float:vz;
    GetVehicleVelocity(vehicleid, vx, vy, vz);
    new kmh = floatround(VectorSize(vx, vy, vz) * 180.0);
    if (Places_GetNearWeigh(playerid, WEIGH_PLATFORM_RANGE) != w || kmh > WEIGH_STILL_KMH) {
        if (Job[playerid][jbWeighWait] > 0) {
            Job[playerid][jbWeighWait] = 0;
            GameTextForPlayer(playerid, "~r~WEIGHING STOPPED~n~~w~STAND STILL", 1500, 3);
        }
        return;
    }

    Job[playerid][jbWeighWait]++;
    if (Job[playerid][jbWeighWait] < WEIGH_STOP_SECONDS) {
        GameTextForPlayer(playerid, "~y~WEIGHING... ~w~%d", 1100, 3, WEIGH_STOP_SECONDS - Job[playerid][jbWeighWait]);
        return;
    }

    // Претеглен: бариерата се вдига
    Job[playerid][jbLastWeigh] = w;
    Places_OpenWeighGate(w);
    Job[playerid][jbWeighPending] = -1;
    Job[playerid][jbWeighWait] = 0;
    new payload = Catalog_GetPayload(GetVehicleModel(vehicleid)), kg = Job[playerid][jbKg];
    if (kg > payload) {
        new overTons = (kg - payload + 999) / 1000;
        new fine = WEIGH_FINE_BASE + overTons * WEIGH_FINE_PER_TON;
        Money_Take(playerid, fine);
        GameTextForPlayer(playerid, "~r~OVERLOADED!", 3000, 1);
        SendClientMessage(playerid, COLOR_RED, "[КАНТАР] " C_WHITE "Претоварен си с %d т! Глоба $%d.", overTons, fine);
    } else {
        GameTextForPlayer(playerid, "~g~WEIGH STATION~n~~w~OK", 2000, 1);
        SendClientMessage(playerid, COLOR_GREEN, "[КАНТАР] " C_WHITE "Теглото е %d кг, в норма. Приятен път!", kg);
    }
}

// Чуплив товар: при силен удар се чупи
static stock Delivery_CheckFragile(playerid)
{
    if (Job[playerid][jbStage] != JOB_TO_DROP) return;
    if (!(Goods[Job[playerid][jbGood]][gdFlags] & GOODFLAG_FRAGILE)) return;
    if (Delivery_DamagePercent(playerid) <= FRAGILE_BREAK_PERCENT) return;

    Delivery_RefundBonus(playerid);
    Delivery_End(playerid);
    GameTextForPlayer(playerid, "~r~CARGO BROKEN!", 4000, 1);
    SendClientMessage(playerid, COLOR_RED, "[КУРС] " C_WHITE "Чупливият товар се счупи! Курсът е провален (без глоба, но без плащане).");
}

static stock Delivery_CheckSpill(playerid)
{
    if (Job[playerid][jbStage] != JOB_TO_DROP) return;
    if (Goods[Job[playerid][jbGood]][gdCargo] != CARGO_LIQUID) return;
    if (Delivery_DamagePercent(playerid) <= SPILL_DAMAGE_PERCENT) return;

    Delivery_RefundBonus(playerid);
    Delivery_End(playerid);
    Money_Take(playerid, SPILL_FINE);
    GameTextForPlayer(playerid, "~r~FUEL SPILL!", 4000, 1);
    SendClientMessage(playerid, COLOR_RED, "[КУРС] " C_WHITE "Разлив на гориво! Курсът е провален. Глоба: $%d.", SPILL_FINE);
}

// Тегло на товара в превозното средство (0 ако няма натоварен товар)
stock Delivery_GetLoadKg(playerid)
{
    new stage = Job[playerid][jbStage];
    if (stage == JOB_TO_DROP || stage == JOB_UNLOADING) return Job[playerid][jbKg];
    return 0;
}

// Компанията е изтрита: поръчките от/до нея изчезват, текущите курсове се прекратяват
stock Delivery_OnCompanyRemoved(c)
{
    for (new i = 0; i < MAX_CONTRACTS; i++) {
        if (Contracts[i][ctActive] && (Contracts[i][ctFrom] == c || Contracts[i][ctTo] == c)) Contracts_Generate(i);
    }
    for (new p = 0; p < MAX_PLAYERS; p++) {
        if (IsPlayerConnected(p) && Job[p][jbStage] != JOB_NONE && (Job[p][jbFrom] == c || Job[p][jbTo] == c)) {
            Delivery_Cancel(p, "Компанията беше закрита - курсът е прекратен.", false);
        }
    }
}

stock Delivery_GetJobVehicle(playerid)
{
    return Job[playerid][jbVehicle];
}

stock bool:Delivery_HasJob(playerid)
{
    return (Job[playerid][jbStage] != JOB_NONE);
}
