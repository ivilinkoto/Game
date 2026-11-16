// Курсове А -> Б: пул от поръчки, табло, товарене, доставка, плащане и панел (TextDraw).
//
// Поръчка: товар + компания А (дава) + компания Б (приема) + тегло.
// Плащане = $150 + км x тонове x ставка на товара (виж modules/goods.inc).
// Таблото показва само поръчки, които превозното средство кара и побира.

#define MAX_CONTRACTS           40
#define CONTRACT_SHOW_MAX       15
#define DELIVERY_CP_SIZE        6.0
#define DELIVERY_LOAD_MS        10000
#define DELIVERY_UNLOAD_MS      8000
#define DELIVERY_FREE_CANCEL    120         // секунди без глоба
#define DELIVERY_CANCEL_FINE    100
#define DELIVERY_SPEED_MS       12.0        // за срока: м/с средно
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
    jbCompanyBonus,         // бонус от компанията А (вече взет от касата ѝ)
    jbVehicle,
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
        if (Goods[good][gdCargo] == CARGO_LIQUID) continue;     // гориво - с ремаркета, по-късно

        new producers[MAX_COMPANIES], np = 0, acceptors[MAX_COMPANIES], na = 0;
        for (new c = 0; c < MAX_COMPANIES; c++) {
            if (Company_Produces(c, good)) producers[np++] = c;
            if (Company_Accepts(c, good)) acceptors[na++] = c;
        }
        if (np == 0 || na == 0) continue;

        new from = producers[random(np)], to = acceptors[random(na)];
        if (from == to) continue;
        if (Company_Distance(from, to) < 600.0) continue;       // твърде близо

        new minKg = Goods[good][gdMinKg], maxKg = Goods[good][gdMaxKg];
        new kg = minKg + random(maxKg - minKg + 1);
        kg = (kg / 100) * 100;
        if (kg < 100) kg = 100;

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

static stock Contracts_GetPay(slot)
{
    return Goods_CalcPay(Contracts[slot][ctGood], Contracts[slot][ctKg],
        Company_Distance(Contracts[slot][ctFrom], Contracts[slot][ctTo]));
}

// ---------- Табло ----------

static stock bool:Delivery_CanCarry(model, slot)
{
    new good = Contracts[slot][ctGood];
    if (!(Catalog_GetCargo(model) & Goods[good][gdCargo])) return false;
    return (Contracts[slot][ctKg] <= Catalog_GetPayload(model));
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

    new body[2600], line[200], tons[16], payStr[16];
    format(body, sizeof (body), "Товар\tОт > До\tТегло, разстояние\tПлащане\n");
    BoardCount[playerid] = 0;
    for (new k = 0; k < n && k < CONTRACT_SHOW_MAX; k++) {
        new i = list[k], good = Contracts[i][ctGood];
        FormatTons(Contracts[i][ctKg], tons);
        new pay = Contracts_GetPay(i);
        FormatMoney(pay + Company_GetBonusAmount(Contracts[i][ctFrom], pay), payStr);
        format(line, sizeof (line), "%s\t%s > %s\t%s, %.1f км\t" C_GREEN "$%s\n",
            Goods[good][gdName], CompanyDefs[Contracts[i][ctFrom]][cdName], CompanyDefs[Contracts[i][ctTo]][cdName],
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
    new limit = floatround(ab / DELIVERY_SPEED_MS) + DELIVERY_EXTRA_TIME;

    ShowPlayerDialog(playerid, DIALOG_JOB_DETAILS, DIALOG_STYLE_MSGBOX, C_BRAND "Поръчка",
        C_WHITE "Товар: " C_BRAND "%s" C_WHITE ", %s, ценност %s\n\n" \
        C_WHITE "Взимаш от: " C_BRAND "%s" C_GREY " (%s) - на %.1f км от теб\n" \
        C_WHITE "Караш до: " C_BRAND "%s" C_GREY " (%s) - %.1f км\n\n" \
        C_WHITE "Плащане: " C_GREEN "$%s" C_WHITE ", срок след товарене: %d:%02d мин\n%s" \
        C_GREY "Навреме +10%%, закъснял -20%%, щети до -50%%.",
        "Приеми", "Назад",
        Goods[good][gdName], tons, stars,
        CompanyDefs[from][cdName], CompanyDefs[from][cdPlace], toA / 1000.0,
        CompanyDefs[to][cdName], CompanyDefs[to][cdPlace], ab / 1000.0,
        payStr, limit / 60, limit % 60, bonusLine);
}

// ---------- Курс ----------

static stock Delivery_SetCheckpoint(playerid, company, bool:finish)
{
    DisablePlayerRaceCheckpoint(playerid);
    SetPlayerRaceCheckpoint(playerid, finish ? CP_TYPE_GROUND_FINISH : CP_TYPE_GROUND_NORMAL,
        Company[company][cpX], Company[company][cpY], Company[company][cpZ],
        Company[company][cpX], Company[company][cpY], Company[company][cpZ], DELIVERY_CP_SIZE);
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
        new Float:ab = Company_Distance(Job[playerid][jbFrom], Job[playerid][jbTo]);
        Job[playerid][jbDeadline] = gettime() + floatround(ab / DELIVERY_SPEED_MS) + DELIVERY_EXTRA_TIME;
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
    if (Job[playerid][jbDeadline] != 0) {
        if (gettime() <= Job[playerid][jbDeadline]) bonus = base / 10;
        else late = base / 5;
    }
    new dmg = Delivery_DamagePercent(playerid);
    if (dmg > 50) dmg = 50;
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
    Money_Give(playerid, total);
    Level_GiveExp(playerid, exp);
    Company_OnDelivery(from, to, base);

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
        C_WHITE "Бонус от компанията:\t" C_GREEN "+$%s\n\n" \
        C_WHITE "Общо: " C_GREEN "$%s" C_WHITE ", опит: " C_BRAND "+%d",
        "Нов курс", "Затвори",
        Goods[good][gdName], tons, CompanyDefs[from][cdName], CompanyDefs[to][cdName],
        sBase, sBonus, sLate, sDmg, sComp, sTotal, exp);
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
    Delivery_ResetPanel(playerid);
}

stock Delivery_OnPlayerDisconnect(playerid)
{
    if (Job[playerid][jbStage] != JOB_NONE) Delivery_RefundBonus(playerid);
    Job[playerid][jbStage] = JOB_NONE;
    Delivery_DestroyPanel(playerid);
}

stock Delivery_OnVehicleDeath(vehicleid)
{
    for (new i = 0; i < MAX_PLAYERS; i++) {
        if (IsPlayerConnected(i) && Job[i][jbStage] != JOB_NONE && Job[i][jbVehicle] == vehicleid) {
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
        case JOB_TO_DROP:   format(text, sizeof (text), "DELIVER THE CARGO");
        case JOB_UNLOADING: format(text, sizeof (text), "UNLOADING...");
    }
    PlayerTextDrawSetString(playerid, DpTitle[playerid], text);

    // 1: товар, тегло, ценност
    format(stars, sizeof (stars), "%d/5", Goods[good][gdStars]);
    format(text, sizeof (text), "~w~CARGO  ~y~%s  ~w~%d.%d T  ~y~%s", Goods[good][gdNameEN], Job[playerid][jbKg] / 1000, (Job[playerid][jbKg] % 1000) / 100, stars);
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
        if (Job[i][jbStage] != JOB_NONE && IsPlayerConnected(i)) Delivery_UpdatePanel(i);
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
