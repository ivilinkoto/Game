// Работа: ФЕРМЕР (начална – без ниво, без кметство)
//
// NPC "Иван - Фермер" (ALT) дава задачи една след друга. Всяка е различна и кратка:
//
//   Скил 1:  Щайги домати    – носиш щайги от полето до камиона (на ръце)
//            Ограда          – поправяш счупени колове (мини-игра с чук)
//            Оранжерия       – поливаш разсада (мини-игра)
//   Скил 2:  Кошери          – опушваш кошерите и вадиш пити мед, носиш меда в склада
//            Сено            – с пикап зареждаш сено и го караш до обора
//            Оран            – трактор по маршрут из полето
//   Скил 3:  Жътва           – комбайн по маршрут
//            Зърно           – с пикап караш зърното до мелницата
//
// Репутация при Иван: всяка задача я вдига, отказът я сваля. По-висок ранг = бонус към заплащането.
// Продукцията (домати, мед, пшеница) отива на пазара – суровина за бъдещите бизнеси.
//
// Всички места са отместване (в метри) от точката на Иван ("job").
// /farmedit center – мести цялата ферма (Иван, местата и картата "ferma") там, където стоиш.

// ------------------------------------------------------------
//  Начално място – полето западно от Blueberry (Red County)
// ------------------------------------------------------------

#define FARM_DEFAULT_X      -123.02
#define FARM_DEFAULT_Y      -32.47
#define FARM_DEFAULT_Z      3.12

// ------------------------------------------------------------
//  Места във фермата (отместване от Иван: dx, dy)
// ------------------------------------------------------------

enum {
    FSPOT_VEHICLE,      // тук се появява техниката
    FSPOT_FIELD,        // полето с доматите (щайги)
    FSPOT_TRUCK,        // камионът за щайгите
    FSPOT_FENCE1,
    FSPOT_FENCE2,
    FSPOT_FENCE3,
    FSPOT_GREEN1,       // оранжерия
    FSPOT_GREEN2,
    FSPOT_GREEN3,
    FSPOT_HIVE1,        // кошери
    FSPOT_HIVE2,
    FSPOT_HIVE3,
    FSPOT_HONEY,        // склад за мед
    FSPOT_HAY,          // купа сено
    FSPOT_BARN,         // обор
    FSPOT_MILL,         // мелница
    FSPOT_COUNT
}

static const Float:FarmSpotOffset[FSPOT_COUNT][2] = {
    {-8.0,   0.0},
    {-25.0,  18.0},
    {8.0,   -8.0},
    {-45.0, -6.0},
    {-45.0,  14.0},
    {-45.0,  34.0},
    {14.0,   20.0},
    {17.0,   20.0},
    {20.0,   20.0},
    {24.0,  -16.0},
    {27.0,  -16.0},
    {30.0,  -16.0},
    {12.0,  -14.0},
    {-12.0, -22.0},
    {30.0,   40.0},
    {45.0,  -35.0}
};

static const FarmSpotName[FSPOT_COUNT][] = {
    "Техника", "Поле с домати", "Камион", "Ограда", "Ограда", "Ограда",
    "Оранжерия", "Оранжерия", "Оранжерия", "Кошер", "Кошер", "Кошер",
    "Склад за мед", "Купа сено", "Обор", "Мелница"
};

// Маршрут за оран и жътва (змийка из полето)
static const Float:FarmRouteOffset[][2] = {
    {-20.0, 10.0}, {-60.0, 10.0}, {-60.0, 18.0}, {-20.0, 18.0},
    {-20.0, 26.0}, {-60.0, 26.0}, {-60.0, 34.0}, {-20.0, 34.0}
};

// ------------------------------------------------------------
//  Задачи
// ------------------------------------------------------------

enum {
    FTASK_NONE,
    FTASK_CRATES,
    FTASK_FENCE,
    FTASK_GREENHOUSE,
    FTASK_HIVES,
    FTASK_HAY,
    FTASK_PLOW,
    FTASK_HARVEST,
    FTASK_GRAIN,
    FTASK_COUNT
}

static const FarmTaskName[FTASK_COUNT][] = {
    "", "Щайги домати", "Поправка на оградата", "Поливане в оранжерията", "Мед от кошерите",
    "Сено за обора", "Оран с трактор", "Жътва с комбайн", "Зърно до мелницата"
};

static const FarmTaskDesc[FTASK_COUNT][] = {
    "",
    "Пренеси 4 щайги домати от полето до камиона. Носиш ги на ръце.",
    "Вятърът събори няколко кола. Поправи 3 места по оградата.",
    "Разсадът в оранжерията е жаден. Полей 3 лехи, без да прелееш.",
    "Опуши 3 кошера и извади питите мед. Внимавай – пчелите жилят! После занеси меда в склада.",
    "Вземи пикапа, натовари сено от купата и го закарай до обора. Два курса.",
    "Изори полето с трактора по маркирания маршрут.",
    "Ожъни пшеницата с комбайна по маркирания маршрут.",
    "Натовари зърното от полето и го закарай с пикапа до мелницата."
};

static const FarmTaskSkill[FTASK_COUNT] = {0, 1, 1, 1, 2, 2, 2, 3, 3};
static const FarmTaskPay[FTASK_COUNT]   = {0, 180, 160, 170, 220, 200, 260, 300, 280};
static const FarmTaskTime[FTASK_COUNT]  = {0, 150, 120, 120, 180, 210, 180, 200, 200}; // секунди за бонус "бързо"

// Действие на всяка стъпка
#define FSTEP_REACH         0   // само стигни
#define FSTEP_PICK_CRATE    1   // вземи щайга (на ръце)
#define FSTEP_DROP_CRATE    2   // остави щайгата
#define FSTEP_MG_HAMMER     3   // мини-игра: чук
#define FSTEP_MG_WATER      4   // мини-игра: поливане
#define FSTEP_MG_HONEY      5   // мини-игра: мед (при провал – ужилване)
#define FSTEP_LOAD          6   // товарене с превозно средство (3 сек.)
#define FSTEP_UNLOAD        7   // разтоварване (3 сек.)

#define FARM_MAX_STEPS      12
#define FARM_CARRY_SLOT     9

// Мини-игри на фермата (purpose)
#define FARM_MG_HAMMER      1
#define FARM_MG_WATER       2
#define FARM_MG_HONEY       3

// Репутация при Иван
static const FarmRankName[][] = {"Новак", "Помощник", "Работник", "Доверен човек", "Дясната ръка на Иван"};
static const FarmRankRep[]    = {0, 20, 60, 150, 300};
static const FarmRankBonus[]  = {0, 5, 10, 15, 20};   // % към заплащането

// Модели
#define FARM_TRACTOR_MODEL  531
#define FARM_COMBINE_MODEL  532
#define FARM_PICKUP_MODEL   478
#define FARM_CRATE_MODEL    2912
#define FARM_HONEY_MODEL    1220

// ------------------------------------------------------------
//  Данни
// ------------------------------------------------------------

static Float:gFarmX, Float:gFarmY, Float:gFarmZ;      // точката на Иван
static STREAMER_TAG_3D_TEXT_LABEL:gFarmSpotLabel[FSPOT_COUNT];
static bool:gFarmLabelsCreated;

static FarmTask[MAX_PLAYERS],
    FarmStep[MAX_PLAYERS],
    FarmStepCount[MAX_PLAYERS],
    FarmStepAction[MAX_PLAYERS][FARM_MAX_STEPS],
    bool:FarmStepVehicle[MAX_PLAYERS][FARM_MAX_STEPS],
    Float:FarmStepX[MAX_PLAYERS][FARM_MAX_STEPS],
    Float:FarmStepY[MAX_PLAYERS][FARM_MAX_STEPS],
    FarmVehicle[MAX_PLAYERS],
    FarmStartTime[MAX_PLAYERS],
    FarmLastTask[MAX_PLAYERS],
    FarmOfferTask[MAX_PLAYERS],
    FarmRep[MAX_PLAYERS],
    bool:FarmRepLoaded[MAX_PLAYERS],
    bool:FarmBusy[MAX_PLAYERS],         // чака мини-игра или товарене
    bool:FarmCarrying[MAX_PLAYERS],
    FarmOutOfVehicle[MAX_PLAYERS],      // секунди извън превозното средство
    FarmMenuAction[MAX_PLAYERS][6];

// ============================================================
//  Инициализация
// ============================================================

stock Farm_Init()
{
    Database_Exec("CREATE TABLE IF NOT EXISTS farm_rep (account_id INTEGER PRIMARY KEY, rep INTEGER NOT NULL DEFAULT 0)");

    new Float:a;
    if (!Job_GetPoint(JOB_FARM, "job", gFarmX, gFarmY, gFarmZ, a)) {
        gFarmX = FARM_DEFAULT_X;
        gFarmY = FARM_DEFAULT_Y;
        gFarmZ = FARM_DEFAULT_Z;
        Job_SetPoint(JOB_FARM, "job", gFarmX, gFarmY, gFarmZ, 0.0);
    }
    Farm_CreateLabels();
}

stock Farm_GetSpot(spot, &Float:x, &Float:y, &Float:z)
{
    x = gFarmX + FarmSpotOffset[spot][0];
    y = gFarmY + FarmSpotOffset[spot][1];
    z = gFarmZ;
}

stock Farm_CreateLabels()
{
    if (gFarmLabelsCreated) {
        for (new s = 0; s < FSPOT_COUNT; s++) DestroyDynamic3DTextLabel(gFarmSpotLabel[s]);
    }
    new Float:x, Float:y, Float:z, text[64];
    for (new s = 0; s < FSPOT_COUNT; s++) {
        Farm_GetSpot(s, x, y, z);
        format(text, sizeof (text), "{FF6A00}%s", FarmSpotName[s]);
        gFarmSpotLabel[s] = CreateDynamic3DTextLabel(text, COLOR_WHITE, x, y, z + 0.8, 20.0);
    }
    gFarmLabelsCreated = true;
}

// Mapping на фермата – създава се веднъж в картата "ferma" (редактира се с /maps, /msel, /medit).
stock Farm_SeedMap()
{
    if (Map_GetID("ferma") != 0) return;
    new mapid = Map_CreateMap("ferma", "HOTFIX");
    if (mapid == 0) return;

    new Float:x, Float:y, Float:z, obj;
    new Float:gz = gFarmZ - 1.0; // земята

    // Табела до Иван
    obj = Map_CreateObject(mapid, 19353, gFarmX + 2.5, gFarmY + 1.5, gz + 1.8, 0.0, 0.0, 90.0, 0, 0);
    if (obj != -1) Map_SetText(obj, 0, "{FF6A00}ФЕРМА\n{FFFFFF}HOTFIX", 40);

    // Щайги на полето и при камиона
    Farm_GetSpot(FSPOT_FIELD, x, y, z);
    Map_CreateObject(mapid, FARM_CRATE_MODEL, x + 1.5, y, gz + 0.5, 0.0, 0.0, 0.0, 0, 0);
    Map_CreateObject(mapid, FARM_CRATE_MODEL, x + 1.5, y + 1.2, gz + 0.5, 0.0, 0.0, 15.0, 0, 0);
    Map_CreateObject(mapid, FARM_CRATE_MODEL, x + 1.5, y + 0.6, gz + 1.5, 0.0, 0.0, 5.0, 0, 0);
    Farm_GetSpot(FSPOT_TRUCK, x, y, z);
    Map_CreateObject(mapid, FARM_CRATE_MODEL, x + 2.0, y, gz + 0.5, 0.0, 0.0, 0.0, 0, 0);

    // Сено
    Farm_GetSpot(FSPOT_HAY, x, y, z);
    Map_CreateObject(mapid, 3374, x + 3.0, y, gz + 1.0, 0.0, 0.0, 0.0, 0, 0);
    Map_CreateObject(mapid, 3374, x + 3.0, y + 4.0, gz + 1.0, 0.0, 0.0, 0.0, 0, 0);

    // Кошери (засега от щайги – mapper може да ги смени)
    for (new s = FSPOT_HIVE1; s <= FSPOT_HIVE3; s++) {
        Farm_GetSpot(s, x, y, z);
        Map_CreateObject(mapid, FARM_CRATE_MODEL, x, y + 1.2, gz + 0.5, 0.0, 0.0, 0.0, 0, 0);
    }

    // Оранжерия – саксии с разсад
    for (new s = FSPOT_GREEN1; s <= FSPOT_GREEN3; s++) {
        Farm_GetSpot(s, x, y, z);
        Map_CreateObject(mapid, 2245, x, y + 1.0, gz + 0.1, 0.0, 0.0, 0.0, 0, 0);
        Map_CreateObject(mapid, 2245, x, y + 2.0, gz + 0.1, 0.0, 0.0, 0.0, 0, 0);
    }
    print("[FARM] Farm map \"ferma\" created. Adjust with /maps, /msel, /medit.");
}

stock Farm_LoadRep(playerid)
{
    FarmRep[playerid] = 0;
    FarmRepLoaded[playerid] = true;
    new q[96];
    format(q, sizeof (q), "SELECT rep FROM farm_rep WHERE account_id = %d", PlayerData[playerid][pID]);
    new DBResult:result = Database_Query(q);
    if (result != DBResult:0) {
        if (DB_GetRowCount(result) > 0) FarmRep[playerid] = DB_GetFieldIntByName(result, "rep");
        DB_FreeResultSet(result);
    }
}

stock Farm_GetRank(playerid)
{
    new rank = 0;
    for (new r = 0; r < sizeof (FarmRankRep); r++) {
        if (FarmRep[playerid] >= FarmRankRep[r]) rank = r;
    }
    return rank;
}

stock Farm_AddRep(playerid, amount)
{
    new before = Farm_GetRank(playerid);
    FarmRep[playerid] += amount;
    if (FarmRep[playerid] < 0) FarmRep[playerid] = 0;
    new q[128];
    format(q, sizeof (q), "INSERT OR REPLACE INTO farm_rep (account_id, rep) VALUES (%d, %d)", PlayerData[playerid][pID], FarmRep[playerid]);
    Database_Exec(q);

    new after = Farm_GetRank(playerid);
    if (after > before) {
        SendClientMessage(playerid, COLOR_GOLD, "[Ферма] Иван вече те смята за \"%s\" (+%d%% към заплащането).",
            FarmRankName[after], FarmRankBonus[after]);
    }
}

stock Farm_ResetPlayer(playerid)
{
    FarmTask[playerid] = FTASK_NONE;
    FarmStep[playerid] = 0;
    FarmStepCount[playerid] = 0;
    FarmVehicle[playerid] = INVALID_VEHICLE_ID;
    FarmLastTask[playerid] = FTASK_NONE;
    FarmOfferTask[playerid] = FTASK_NONE;
    FarmRep[playerid] = 0;
    FarmRepLoaded[playerid] = false;
    FarmBusy[playerid] = false;
    FarmCarrying[playerid] = false;
    FarmOutOfVehicle[playerid] = 0;
}

// ============================================================
//  Разговор с Иван
// ============================================================

stock Farm_OnNpcInteract(playerid)
{
    if (!FarmRepLoaded[playerid]) Farm_LoadRep(playerid);

    new body[512], count = 0;
    body[0] = EOS;
    if (FarmTask[playerid] == FTASK_NONE) {
        strcat(body, C_GREEN "» " C_WHITE "Дай ми работа\n");
        FarmMenuAction[playerid][count++] = 1;
    } else {
        strcat(body, C_RED "» " C_WHITE "Откажи се от задачата " C_GREY "(-2 репутация)\n");
        FarmMenuAction[playerid][count++] = 2;
    }
    strcat(body, C_BLUE "» " C_WHITE "Моят напредък\n");
    FarmMenuAction[playerid][count++] = 3;
    strcat(body, C_BLUE "» " C_WHITE "Как работи фермата");
    FarmMenuAction[playerid][count] = 4;

    ShowPlayerDialog(playerid, DIALOG_FARM_NPC, DIALOG_STYLE_LIST, C_BRAND "Иван - Фермер", "%s", "Избери", "Затвори", body);
}

stock Farm_PickTask(playerid)
{
    new skill = Job_GetSkill(playerid, JOB_FARM), options[FTASK_COUNT], count = 0;
    for (new t = 1; t < FTASK_COUNT; t++) {
        if (FarmTaskSkill[t] > skill || t == FarmLastTask[playerid]) continue;
        options[count++] = t;
    }
    if (count == 0) return FTASK_CRATES;
    return options[random(count)];
}

stock Farm_GetTaskPay(playerid, task)
{
    new pay = FarmTaskPay[task] * Job_GetPayPercent(playerid, JOB_FARM) / 100;
    pay += pay * FarmRankBonus[Farm_GetRank(playerid)] / 100;
    return pay;
}

stock Farm_OfferTask(playerid)
{
    new task = Farm_PickTask(playerid);
    FarmOfferTask[playerid] = task;

    new body[768];
    format(body, sizeof (body),
        C_GREY "Иван: " C_WHITE "\"Имам нещо за теб...\"\n\n" \
        C_BRAND "%s\n" \
        C_WHITE "%s\n\n" \
        C_WHITE "Заплащане: " C_GREEN "$%d" C_GREY " (+бонус, ако се справиш бързо)",
        FarmTaskName[task], FarmTaskDesc[task], Farm_GetTaskPay(playerid, task));
    ShowPlayerDialog(playerid, DIALOG_FARM_OFFER, DIALOG_STYLE_MSGBOX, C_BRAND "Иван - Фермер", "%s", "Приемам", "Друга", body);
}

stock Farm_ShowProgress(playerid)
{
    new body[1024], line[160];
    new skill = Job_GetSkill(playerid, JOB_FARM), maxSkill = Job_GetMaxSkill(JOB_FARM), rank = Farm_GetRank(playerid);

    format(body, sizeof (body), C_WHITE "Скил: " C_GOLD "%d/%d" C_WHITE " (%d EXP)", skill, maxSkill, PlayerJobExp[playerid][JOB_FARM]);
    if (skill < maxSkill) {
        format(line, sizeof (line), C_GREY " - следващ при %d EXP", Job_GetSkillExp(JOB_FARM, skill + 1));
        strcat(body, line);
    }
    format(line, sizeof (line), "\n" C_WHITE "Репутация при Иван: " C_GOLD "%s" C_WHITE " (%d т.)", FarmRankName[rank], FarmRep[playerid]);
    strcat(body, line);
    if (rank < sizeof (FarmRankRep) - 1) {
        format(line, sizeof (line), C_GREY " - следващ ранг при %d т.", FarmRankRep[rank + 1]);
        strcat(body, line);
    }
    format(line, sizeof (line), "\n" C_WHITE "Бонус към заплащането: " C_GREEN "скил %d%%, ранг +%d%%\n\n",
        Job_GetPayPercent(playerid, JOB_FARM), FarmRankBonus[rank]);
    strcat(body, line);

    strcat(body, C_BRAND "Задачи\n");
    for (new t = 1; t < FTASK_COUNT; t++) {
        if (FarmTaskSkill[t] <= skill) {
            format(line, sizeof (line), C_GREEN "+ " C_WHITE "%s " C_GREY "($%d)\n", FarmTaskName[t], Farm_GetTaskPay(playerid, t));
        } else {
            format(line, sizeof (line), C_GREY "- %s (скил %d)\n", FarmTaskName[t], FarmTaskSkill[t]);
        }
        strcat(body, line);
    }
    ShowPlayerDialog(playerid, DIALOG_FARM_INFO, DIALOG_STYLE_MSGBOX, C_BRAND "Ферма " C_WHITE "| Напредък", "%s", "Назад", "", body);
}

stock Farm_ShowHelp(playerid)
{
    new body[1500];
    body[0] = EOS;
    strcat(body, C_WHITE "Говори с " C_BRAND "Иван" C_WHITE " (ALT) и той ще ти даде задача.\n");
    strcat(body, C_WHITE "Задачите са кратки и всяка следваща е различна.\n\n");
    strcat(body, C_BRAND "Как се изпълняват\n");
    strcat(body, C_WHITE "Следвай червената точка на картата. На всяко място действието започва само:\n");
    strcat(body, C_WHITE "взимаш щайга, пускаш мини-игра, товариш и т.н.\n\n");
    strcat(body, C_BRAND "Скил и репутация\n");
    strcat(body, C_WHITE "Скил 1: щайги, ограда, оранжерия\n");
    strcat(body, C_WHITE "Скил 2: + кошери, сено, оран с трактор\n");
    strcat(body, C_WHITE "Скил 3: + жътва с комбайн, зърно до мелницата\n");
    strcat(body, C_WHITE "Репутацията при Иван дава до " C_GREEN "+20%" C_WHITE " към заплащането.\n\n");
    strcat(body, C_GREY "Всяка задача: +1 EXP (+1 ако си бърз). Продукцията отива на пазара.");
    ShowPlayerDialog(playerid, DIALOG_FARM_INFO, DIALOG_STYLE_MSGBOX, C_BRAND "Ферма " C_WHITE "| Как работи", "%s", "Назад", "", body);
}

// ============================================================
//  Задачата – стъпки
// ============================================================

static stock Farm_AddStep(playerid, Float:x, Float:y, action, bool:vehicle)
{
    new s = FarmStepCount[playerid];
    if (s >= FARM_MAX_STEPS) return;
    FarmStepX[playerid][s] = x;
    FarmStepY[playerid][s] = y;
    FarmStepAction[playerid][s] = action;
    FarmStepVehicle[playerid][s] = vehicle;
    FarmStepCount[playerid]++;
}

static stock Farm_AddSpotStep(playerid, spot, action, bool:vehicle)
{
    new Float:x, Float:y, Float:z;
    Farm_GetSpot(spot, x, y, z);
    Farm_AddStep(playerid, x, y, action, vehicle);
}

stock Farm_StartTask(playerid, task)
{
    FarmTask[playerid] = task;
    FarmStep[playerid] = 0;
    FarmStepCount[playerid] = 0;
    FarmStartTime[playerid] = gettime();
    FarmBusy[playerid] = false;
    FarmCarrying[playerid] = false;
    FarmOutOfVehicle[playerid] = 0;
    Gps_Stop(playerid);

    new vehicleModel = 0;
    switch (task) {
        case FTASK_CRATES: {
            new Float:x, Float:y, Float:z;
            Farm_GetSpot(FSPOT_FIELD, x, y, z);
            for (new c = 0; c < 4; c++) {
                Farm_AddStep(playerid, x - float(random(8)), y + float(random(8)), FSTEP_PICK_CRATE, false);
                Farm_AddSpotStep(playerid, FSPOT_TRUCK, FSTEP_DROP_CRATE, false);
            }
        }
        case FTASK_FENCE: {
            Farm_AddSpotStep(playerid, FSPOT_FENCE1, FSTEP_MG_HAMMER, false);
            Farm_AddSpotStep(playerid, FSPOT_FENCE2, FSTEP_MG_HAMMER, false);
            Farm_AddSpotStep(playerid, FSPOT_FENCE3, FSTEP_MG_HAMMER, false);
        }
        case FTASK_GREENHOUSE: {
            Farm_AddSpotStep(playerid, FSPOT_GREEN1, FSTEP_MG_WATER, false);
            Farm_AddSpotStep(playerid, FSPOT_GREEN2, FSTEP_MG_WATER, false);
            Farm_AddSpotStep(playerid, FSPOT_GREEN3, FSTEP_MG_WATER, false);
        }
        case FTASK_HIVES: {
            Farm_AddSpotStep(playerid, FSPOT_HIVE1, FSTEP_MG_HONEY, false);
            Farm_AddSpotStep(playerid, FSPOT_HIVE2, FSTEP_MG_HONEY, false);
            Farm_AddSpotStep(playerid, FSPOT_HIVE3, FSTEP_MG_HONEY, false);
            Farm_AddSpotStep(playerid, FSPOT_HONEY, FSTEP_DROP_CRATE, false);
        }
        case FTASK_HAY: {
            vehicleModel = FARM_PICKUP_MODEL;
            for (new r = 0; r < 2; r++) {
                Farm_AddSpotStep(playerid, FSPOT_HAY, FSTEP_LOAD, true);
                Farm_AddSpotStep(playerid, FSPOT_BARN, FSTEP_UNLOAD, true);
            }
        }
        case FTASK_PLOW, FTASK_HARVEST: {
            vehicleModel = (task == FTASK_PLOW) ? FARM_TRACTOR_MODEL : FARM_COMBINE_MODEL;
            for (new r = 0; r < sizeof (FarmRouteOffset); r++) {
                Farm_AddStep(playerid, gFarmX + FarmRouteOffset[r][0], gFarmY + FarmRouteOffset[r][1], FSTEP_REACH, true);
            }
        }
        case FTASK_GRAIN: {
            vehicleModel = FARM_PICKUP_MODEL;
            Farm_AddSpotStep(playerid, FSPOT_FIELD, FSTEP_LOAD, true);
            Farm_AddSpotStep(playerid, FSPOT_MILL, FSTEP_UNLOAD, true);
        }
    }

    if (vehicleModel != 0) {
        new Float:x, Float:y, Float:z;
        Farm_GetSpot(FSPOT_VEHICLE, x, y, z);
        new vehicleid = CreateVehicle(vehicleModel, x, y, z + 0.5, 90.0, 1, 1, -1);
        if (vehicleid == INVALID_VEHICLE_ID || vehicleid == 0) {
            SendError(playerid, "Техниката не може да бъде създадена. Опитай пак.");
            Farm_EndTask(playerid);
            return;
        }
        FarmVehicle[playerid] = vehicleid;
        SetVehicleNumberPlate(vehicleid, "FARM");
        PutPlayerInVehicle(playerid, vehicleid, 0);
    }

    SendClientMessage(playerid, COLOR_GREEN, "[Ферма] {FFFFFF}%s: %s", FarmTaskName[task], FarmTaskDesc[task]);
    SendClientMessage(playerid, COLOR_GREY, "Следвай червената точка на картата.");
    Farm_ShowStep(playerid);
}

stock Farm_ShowStep(playerid)
{
    new s = FarmStep[playerid];
    new Float:x = FarmStepX[playerid][s], Float:y = FarmStepY[playerid][s], Float:z = gFarmZ;

    DisablePlayerCheckpoint(playerid);
    DisablePlayerRaceCheckpoint(playerid);

    if (FarmStepVehicle[playerid][s]) {
        new Float:nx = x, Float:ny = y;
        new bool:last = (s + 1 >= FarmStepCount[playerid]);
        if (!last) {
            nx = FarmStepX[playerid][s + 1];
            ny = FarmStepY[playerid][s + 1];
        }
        SetPlayerRaceCheckpoint(playerid, last ? CP_TYPE_GROUND_FINISH : CP_TYPE_GROUND_NORMAL, x, y, z, nx, ny, z, 5.0);
    } else {
        SetPlayerCheckpoint(playerid, x, y, z, 1.5);
    }

    switch (FarmStepAction[playerid][s]) {
        case FSTEP_PICK_CRATE: GameTextForPlayer(playerid, "~w~Take a ~y~crate", 2500, 3);
        case FSTEP_DROP_CRATE: GameTextForPlayer(playerid, "~w~Carry it to the ~y~marker", 2500, 3);
        case FSTEP_LOAD:       GameTextForPlayer(playerid, "~w~Drive to ~y~load", 2500, 3);
        case FSTEP_UNLOAD:     GameTextForPlayer(playerid, "~w~Drive to ~y~unload", 2500, 3);
    }
}

// От OnPlayerEnterCheckpoint / OnPlayerEnterRaceCheckpoint. Връща true, ако е на фермата.
stock bool:Farm_OnEnterCheckpoint(playerid, bool:race)
{
    if (FarmTask[playerid] == FTASK_NONE || FarmBusy[playerid]) return false;
    new s = FarmStep[playerid];
    if (FarmStepVehicle[playerid][s] != race) return false;

    if (race) {
        if (GetPlayerVehicleID(playerid) != FarmVehicle[playerid]) {
            SendError(playerid, "Трябва да си в превозното средство на фермата.");
            return true;
        }
    } else if (IsPlayerInAnyVehicle(playerid)) {
        SendError(playerid, "Слез от превозното средство.");
        return true;
    }

    switch (FarmStepAction[playerid][s]) {
        case FSTEP_REACH: Farm_NextStep(playerid);
        case FSTEP_PICK_CRATE: {
            Farm_Carry(playerid, FARM_CRATE_MODEL);
            Farm_NextStep(playerid);
        }
        case FSTEP_DROP_CRATE: {
            Farm_StopCarry(playerid);
            ApplyAnimation(playerid, "CARRY", "putdwn", 4.1, false, false, false, false, 0);
            Farm_NextStep(playerid);
        }
        case FSTEP_MG_HAMMER: {
            FarmBusy[playerid] = true;
            ApplyAnimation(playerid, "BASEBALL", "Bat_4", 4.1, false, false, false, false, 0);
            Minigame_Start(playerid, MG_TYPE_BOUNCE, "FIX THE FENCE", MG_OWNER_FARM, FARM_MG_HAMMER, s);
        }
        case FSTEP_MG_WATER: {
            FarmBusy[playerid] = true;
            Minigame_Start(playerid, MG_TYPE_FILL, "WATER", MG_OWNER_FARM, FARM_MG_WATER, s);
        }
        case FSTEP_MG_HONEY: {
            FarmBusy[playerid] = true;
            ApplyAnimation(playerid, "BOMBER", "BOM_Plant", 4.1, false, false, false, false, 0);
            Minigame_Start(playerid, MG_TYPE_BOUNCE, "HONEY", MG_OWNER_FARM, FARM_MG_HONEY, s);
        }
        case FSTEP_LOAD, FSTEP_UNLOAD: {
            FarmBusy[playerid] = true;
            TogglePlayerControllable(playerid, false);
            if (FarmStepAction[playerid][s] == FSTEP_LOAD) GameTextForPlayer(playerid, "~y~Loading...", 3000, 3);
            else GameTextForPlayer(playerid, "~y~Unloading...", 3000, 3);
            SetTimerEx("Farm_LoadDone", 3000, false, "i", playerid);
        }
    }
    return true;
}

forward Farm_LoadDone(playerid);
public Farm_LoadDone(playerid)
{
    if (!IsPlayerConnected(playerid) || FarmTask[playerid] == FTASK_NONE) return 0;
    TogglePlayerControllable(playerid, true);
    FarmBusy[playerid] = false;
    Farm_NextStep(playerid);
    return 1;
}

stock Farm_OnMinigameResult(playerid, purpose, step, result)
{
    ClearAnimations(playerid);
    if (FarmTask[playerid] == FTASK_NONE || step != FarmStep[playerid]) return;
    FarmBusy[playerid] = false;

    if (result == MG_RESULT_FAIL) {
        switch (purpose) {
            case FARM_MG_HAMMER: SendClientMessage(playerid, COLOR_GREY, "Удари си пръста... Опитай пак.");
            case FARM_MG_WATER:  SendClientMessage(playerid, COLOR_GREY, "Преля! Опитай пак по-внимателно.");
            case FARM_MG_HONEY: {
                new Float:hp;
                GetPlayerHealth(playerid, hp);
                SetPlayerHealth(playerid, (hp > 8.0) ? (hp - 5.0) : 3.0);
                SendClientMessage(playerid, COLOR_RED, "Ох! Пчелите те ужилиха (-5 HP). Опитай пак.");
            }
        }
        // Пак същата стъпка – мини-играта тръгва отново след малко
        SetTimerEx("Farm_RetryStep", 1200, false, "ii", playerid, step);
        return;
    }

    // След последния кошер носиш меда до склада
    if (purpose == FARM_MG_HONEY && step + 1 < FarmStepCount[playerid] && FarmStepAction[playerid][step + 1] == FSTEP_DROP_CRATE) {
        Farm_Carry(playerid, FARM_HONEY_MODEL);
    }
    Farm_NextStep(playerid);
}

forward Farm_RetryStep(playerid, step);
public Farm_RetryStep(playerid, step)
{
    if (!IsPlayerConnected(playerid) || FarmTask[playerid] == FTASK_NONE || FarmStep[playerid] != step) return 0;
    if (GetPlayerDistanceFromPoint(playerid, FarmStepX[playerid][step], FarmStepY[playerid][step], gFarmZ) > 4.0) {
        Farm_ShowStep(playerid); // отдалечил се е – пак маркираме точката
        return 0;
    }
    Farm_OnEnterCheckpoint(playerid, false);
    return 1;
}

stock Farm_NextStep(playerid)
{
    FarmStep[playerid]++;
    if (FarmStep[playerid] >= FarmStepCount[playerid]) {
        Farm_CompleteTask(playerid);
        return;
    }
    Farm_ShowStep(playerid);
}

stock Farm_Carry(playerid, model)
{
    FarmCarrying[playerid] = true;
    SetPlayerSpecialAction(playerid, SPECIAL_ACTION_CARRY);
    SetPlayerAttachedObject(playerid, FARM_CARRY_SLOT, model, 1, 0.25, 0.45, 0.0, 0.0, 90.0, 0.0, 0.6, 0.6, 0.6);
}

stock Farm_StopCarry(playerid)
{
    if (!FarmCarrying[playerid]) return;
    FarmCarrying[playerid] = false;
    if (IsPlayerConnected(playerid)) {
        RemovePlayerAttachedObject(playerid, FARM_CARRY_SLOT);
        SetPlayerSpecialAction(playerid, SPECIAL_ACTION_NONE);
    }
}

stock Farm_CompleteTask(playerid)
{
    new task = FarmTask[playerid];
    new pay = Farm_GetTaskPay(playerid, task), exp = 1;
    new seconds = gettime() - FarmStartTime[playerid];
    new bool:fast = (seconds <= FarmTaskTime[task]);
    if (fast) {
        pay += pay / 5;
        exp++;
    }

    // Продукцията отива на пазара
    switch (task) {
        case FTASK_CRATES, FTASK_GREENHOUSE: Market_Add(GOODS_TOMATO, 20);
        case FTASK_HIVES:                     Market_Add(GOODS_HONEY, 8);
        case FTASK_HARVEST, FTASK_GRAIN:      Market_Add(GOODS_WHEAT, 60);
    }

    Farm_EndTask(playerid);
    Money_Give(playerid, pay);
    Auth_SaveAccount(playerid);

    if (fast) {
        SendClientMessage(playerid, COLOR_GREEN, "[Ферма] {FFFFFF}Готово за %d:%02d – бързо! Получи {33CC66}$%d{FFFFFF} (+20%% бонус).",
            seconds / 60, seconds % 60, pay);
    } else {
        SendClientMessage(playerid, COLOR_GREEN, "[Ферма] {FFFFFF}Задачата е готова. Получи {33CC66}$%d{FFFFFF}.", pay);
    }
    SendClientMessage(playerid, COLOR_GREY, "Върни се при Иван за следваща задача.");
    Job_AddExp(playerid, JOB_FARM, exp);
    Farm_AddRep(playerid, 1);

    new details[64];
    format(details, sizeof (details), "%s: $%d", FarmTaskName[task], pay);
    Log_Add("farm_task", PlayerData[playerid][pName], "", details);
}

stock Farm_EndTask(playerid)
{
    if (FarmTask[playerid] == FTASK_NONE) return;
    FarmLastTask[playerid] = FarmTask[playerid];
    FarmTask[playerid] = FTASK_NONE;
    FarmBusy[playerid] = false;

    if (IsPlayerConnected(playerid)) {
        DisablePlayerCheckpoint(playerid);
        DisablePlayerRaceCheckpoint(playerid);
        TogglePlayerControllable(playerid, true);
        if (Minigame_IsActive(playerid)) Minigame_Stop(playerid);
    }
    Farm_StopCarry(playerid);
    if (FarmVehicle[playerid] != INVALID_VEHICLE_ID) {
        DestroyVehicle(FarmVehicle[playerid]);
        FarmVehicle[playerid] = INVALID_VEHICLE_ID;
    }
}

stock Farm_FailTask(playerid, const reason[])
{
    if (FarmTask[playerid] == FTASK_NONE) return;
    Farm_EndTask(playerid);
    Farm_AddRep(playerid, -2);
    SendClientMessage(playerid, COLOR_RED, "[Ферма] Задачата е провалена: %s (-2 репутация)", reason);
}

stock Farm_OnPlayerDisconnect(playerid)
{
    Farm_EndTask(playerid);
    Farm_ResetPlayer(playerid);
}

stock Farm_OnPlayerDeath(playerid)
{
    if (FarmTask[playerid] != FTASK_NONE) Farm_FailTask(playerid, "пострада по време на работа");
}

// Всяка секунда: проверка дали е в техниката
stock Farm_OnTick()
{
    for (new p = 0; p < MAX_PLAYERS; p++) {
        if (FarmTask[p] == FTASK_NONE || FarmVehicle[p] == INVALID_VEHICLE_ID) continue;
        if (!IsPlayerConnected(p)) continue;

        if (GetVehicleModel(FarmVehicle[p]) == 0) {
            FarmVehicle[p] = INVALID_VEHICLE_ID;
            Farm_FailTask(p, "техниката е унищожена");
            continue;
        }
        if (GetPlayerVehicleID(p) == FarmVehicle[p]) {
            FarmOutOfVehicle[p] = 0;
            continue;
        }
        FarmOutOfVehicle[p]++;
        if (FarmOutOfVehicle[p] == 5) {
            SendClientMessage(p, COLOR_GOLD, "[Ферма] Върни се в превозното средство до 60 секунди, иначе задачата се проваля.");
        } else if (FarmOutOfVehicle[p] >= 60) {
            Farm_FailTask(p, "изостави техниката");
        }
    }
}

// ============================================================
//  Диалози
// ============================================================

stock Farm_OnDialogResponse(playerid, dialogid, response, listitem)
{
    switch (dialogid) {
        case DIALOG_FARM_NPC: {
            if (!response || listitem < 0 || listitem > 3) return 1;
            switch (FarmMenuAction[playerid][listitem]) {
                case 1: Farm_OfferTask(playerid);
                case 2: Farm_FailTask(playerid, "отказа се");
                case 3: Farm_ShowProgress(playerid);
                case 4: Farm_ShowHelp(playerid);
            }
            return 1;
        }
        case DIALOG_FARM_OFFER: {
            new task = FarmOfferTask[playerid];
            FarmOfferTask[playerid] = FTASK_NONE;
            if (task == FTASK_NONE || FarmTask[playerid] != FTASK_NONE) return 1;
            if (!response) {
                // "Друга" – друга задача (без наказание)
                FarmLastTask[playerid] = task;
                Farm_OfferTask(playerid);
                return 1;
            }
            Farm_StartTask(playerid, task);
            return 1;
        }
        case DIALOG_FARM_INFO: {
            Farm_OnNpcInteract(playerid);
            return 1;
        }
    }
    return 0;
}

// ============================================================
//  Админ
// ============================================================

CMD:farmedit(playerid, params[])
{
    if (!Admin_Check(playerid, ADMIN_ADMIN)) return 1;
    new sub[16];
    if (!GetParam(params, 0, sub) || strcmp(sub, "center", true) != 0) {
        SendUsage(playerid, "/farmedit center - мести цялата ферма (Иван, местата и картата \"ferma\") тук");
        return 1;
    }

    new Float:x, Float:y, Float:z, Float:a;
    GetPlayerPos(playerid, x, y, z);
    GetPlayerFacingAngle(playerid, a);
    new Float:dx = x - gFarmX, Float:dy = y - gFarmY, Float:dz = z - gFarmZ;

    gFarmX = x;
    gFarmY = y;
    gFarmZ = z;
    Job_SetPoint(JOB_FARM, "job", x, y, z, a);
    Farm_CreateLabels();
    Npc_MoveRole(NPC_ROLE_FARM, x, y, z, a);
    new moved = Map_ShiftMap("ferma", dx, dy, dz);

    SendClientMessage(playerid, COLOR_ADMIN, "[Ферма] Фермата е преместена тук (%d обекта от картата \"ferma\" също).", moved);
    Log_Add("farmedit", PlayerData[playerid][pName], "center", "");
    return 1;
}

CMD:farm(playerid, params[])
{
    #pragma unused params
    Farm_ShowHelp(playerid);
    return 1;
}
