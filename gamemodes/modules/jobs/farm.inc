// Работа: ФЕРМЕР (начална – без ниво, без кметство)
//
// Цикъл на парцела:
//   прекопаване -> засяване -> растеж (поливане, врани, плевели) -> жътва -> продажба
//
//  - Всеки фермер получава свои парцели за смяната (скил 1/2/3 -> 1/2/3 парцела).
//  - Растенията растат по реално време и се виждат на полето.
//  - Времето влияе: в жега изсъхват по-бързо, дъждът ги полива.
//  - Качеството (A/B/C) зависи от грижата и определя цената.
//  - Реколтата отива на пазара (modules/market.inc) – суровина за бъдещите бизнеси.
//  - Скил 2: трактор (прекопава, като минеш през парцела). Скил 3: комбайн (жъне сам).
//
// Действие на парцела: застани на него и натисни Y.
// Точките (вход, изкупвач, кладенец, техника) и парцелите се поставят от админите:
//   /farmedit job | buyer | well | vehicle | addplot | delplot

#define FARM_MAX_PLOTS          40
#define FARM_PLOT_RADIUS        2.5
#define FARM_TICK_SECONDS       5
#define FARM_WATER_START        60
#define FARM_WATER_PER_MIN      5       // разход на вода за минута при нормално време
#define FARM_WATER_ADD          40      // едно поливане
#define FARM_CAN_USES           5       // поливания с една пълна кана
#define FARM_RIPE_GRACE_MIN     20      // след толкова минути узряло растение започва да се разваля
#define FARM_RIPE_LOST_MIN      40      // след толкова минути изсъхва напълно
#define FARM_SKIN_MALE          158
#define FARM_SKIN_FEMALE        157
#define FARM_TRACTOR_MODEL      531
#define FARM_COMBINE_MODEL      532

// Състояния на парцела
#define PLOT_FREE       0
#define PLOT_RENTED     1   // нает, трябва да се прекопае
#define PLOT_PLOWED     2   // готов за засяване
#define PLOT_GROWING    3
#define PLOT_RIPE       4
#define PLOT_WITHERED   5

// Качество
#define QUALITY_A       0
#define QUALITY_B       1
#define QUALITY_C       2

// Мини-игри на фермата
#define FARM_MG_DIG     1
#define FARM_MG_WATER   2
#define FARM_MG_CROWS   3
#define FARM_MG_WEEDS   4
#define FARM_MG_HARVEST 5

// Действия в менюто на парцела
#define FARM_ACT_DIG        1
#define FARM_ACT_SEED       2
#define FARM_ACT_WATER      3
#define FARM_ACT_CROWS      4
#define FARM_ACT_WEEDS      5
#define FARM_ACT_STATUS     6
#define FARM_ACT_HARVEST    7
#define FARM_ACT_CLEAR      8
#define FARM_ACT_MAINMENU   9

// Действия в менюто на работата
#define FARM_JOB_START      1
#define FARM_JOB_END        2
#define FARM_JOB_TRACTOR    3
#define FARM_JOB_COMBINE    4
#define FARM_JOB_INFO       5

// ------------------------------------------------------------
//  Култури
// ------------------------------------------------------------

enum E_CROP {
    cpName[24],
    cpGoods,            // стока на пазара
    cpSeedCost,
    cpGrowMinutes,
    cpYield,            // кг при нормална жътва
    cpModels[3]         // обект за кълн / стрък / зряло (провери в играта и смени при нужда)
}

static const CropInfo[][E_CROP] = {
    {"Пшеница", GOODS_WHEAT,  20, 15, 12, {874, 3409, 3409}},
    {"Картофи", GOODS_POTATO, 40, 25,  9, {874, 19473, 19473}},
    {"Домати",  GOODS_TOMATO, 70, 40,  7, {874, 19473, 19473}}
};

static const QualityName[][] = {"A", "B", "C"};
static const QualityPercent[] = {150, 100, 50};

// ------------------------------------------------------------
//  Данни
// ------------------------------------------------------------

enum E_PLOT {
    bool:plExists,
    plDBID,
    Float:plX,
    Float:plY,
    Float:plZ,
    plOwner,
    plState,
    plCrop,
    plGrowth,           // секунди растеж
    plWater,
    plHealth,
    bool:plCrows,
    bool:plWeeds,
    plRipeTime,
    plStage,
    plSecondsAcc,
    plObject,
    STREAMER_TAG_3D_TEXT_LABEL:plLabel
}
static FarmPlot[FARM_MAX_PLOTS][E_PLOT];

static bool:FarmOnShift[MAX_PLAYERS],
    FarmCanUses[MAX_PLAYERS],
    FarmVehicle[MAX_PLAYERS],
    FarmActionPlot[MAX_PLAYERS],
    FarmMenuAction[MAX_PLAYERS][8],
    FarmPickupTick[MAX_PLAYERS];
static FarmGoods[MAX_PLAYERS][MAX_GOODS][3];   // кг по стока и качество

static gFarmPickupJob = -1, gFarmPickupBuyer = -1, gFarmPickupWell = -1;
static STREAMER_TAG_3D_TEXT_LABEL:gFarmLabelJob, STREAMER_TAG_3D_TEXT_LABEL:gFarmLabelBuyer, STREAMER_TAG_3D_TEXT_LABEL:gFarmLabelWell;
static bool:gFarmLabelsCreated;

// ============================================================
//  Инициализация
// ============================================================

stock Farm_Init()
{
    Database_Exec("CREATE TABLE IF NOT EXISTS farm_plots (id INTEGER PRIMARY KEY AUTOINCREMENT, x REAL NOT NULL, y REAL NOT NULL, z REAL NOT NULL)");

    for (new i = 0; i < FARM_MAX_PLOTS; i++) {
        FarmPlot[i][plExists] = false;
        FarmPlot[i][plOwner] = INVALID_PLAYER_ID;
    }

    new DBResult:result = Database_Query("SELECT id, x, y, z FROM farm_plots ORDER BY id");
    if (result != DBResult:0) {
        if (DB_GetRowCount(result) > 0) {
            do {
                Farm_AddPlotToMemory(DB_GetFieldIntByName(result, "id"),
                    DB_GetFieldFloatByName(result, "x"), DB_GetFieldFloatByName(result, "y"), DB_GetFieldFloatByName(result, "z"));
            } while (DB_SelectNextRow(result));
        }
        DB_FreeResultSet(result);
    }
    Farm_RefreshPoints();
}

stock Farm_AddPlotToMemory(dbid, Float:x, Float:y, Float:z)
{
    for (new i = 0; i < FARM_MAX_PLOTS; i++) {
        if (FarmPlot[i][plExists]) continue;
        FarmPlot[i][plExists] = true;
        FarmPlot[i][plDBID] = dbid;
        FarmPlot[i][plX] = x;
        FarmPlot[i][plY] = y;
        FarmPlot[i][plZ] = z;
        FarmPlot[i][plOwner] = INVALID_PLAYER_ID;
        FarmPlot[i][plState] = PLOT_FREE;
        FarmPlot[i][plObject] = 0;
        FarmPlot[i][plLabel] = CreateDynamic3DTextLabel("...", COLOR_WHITE, x, y, z + 0.6, 12.0);
        Farm_UpdatePlotLabel(i);
        return i;
    }
    return -1;
}

// Пикапи и надписи на точките (вход, изкупвач, кладенец)
stock Farm_RefreshPoints()
{
    if (gFarmPickupJob != -1) DestroyDynamicPickup(gFarmPickupJob);
    if (gFarmPickupBuyer != -1) DestroyDynamicPickup(gFarmPickupBuyer);
    if (gFarmPickupWell != -1) DestroyDynamicPickup(gFarmPickupWell);
    if (gFarmLabelsCreated) {
        DestroyDynamic3DTextLabel(gFarmLabelJob);
        DestroyDynamic3DTextLabel(gFarmLabelBuyer);
        DestroyDynamic3DTextLabel(gFarmLabelWell);
    }
    gFarmPickupJob = -1;
    gFarmPickupBuyer = -1;
    gFarmPickupWell = -1;

    new Float:x, Float:y, Float:z, Float:a;
    // Ако точка не е поставена, надписът се слага далеч под картата.
    if (Job_GetPoint(JOB_FARM, "job", x, y, z, a)) {
        gFarmPickupJob = CreateDynamicPickup(1275, 1, x, y, z, 0, 0);
    } else {
        x = 0.0; y = 0.0; z = -100.0;
    }
    gFarmLabelJob = CreateDynamic3DTextLabel("{FF6A00}Ферма\n{FFFFFF}Работа като фермер", COLOR_WHITE, x, y, z + 0.8, 15.0);

    if (Job_GetPoint(JOB_FARM, "buyer", x, y, z, a)) {
        gFarmPickupBuyer = CreateDynamicPickup(1274, 1, x, y, z, 0, 0);
    } else {
        x = 0.0; y = 0.0; z = -100.0;
    }
    gFarmLabelBuyer = CreateDynamic3DTextLabel("{FF6A00}Изкупуване на реколта\n{FFFFFF}Продай тук", COLOR_WHITE, x, y, z + 0.8, 15.0);

    if (Job_GetPoint(JOB_FARM, "well", x, y, z, a)) {
        gFarmPickupWell = CreateDynamicPickup(1239, 1, x, y, z, 0, 0);
    } else {
        x = 0.0; y = 0.0; z = -100.0;
    }
    gFarmLabelWell = CreateDynamic3DTextLabel("{4FC3F7}Кладенец\n{FFFFFF}Напълни каната", COLOR_WHITE, x, y, z + 0.8, 15.0);
    gFarmLabelsCreated = true;
}

stock Farm_ResetPlayer(playerid)
{
    FarmOnShift[playerid] = false;
    FarmCanUses[playerid] = 0;
    FarmVehicle[playerid] = INVALID_VEHICLE_ID;
    FarmActionPlot[playerid] = -1;
    FarmPickupTick[playerid] = 0;
    for (new g = 0; g < MAX_GOODS; g++) {
        for (new q = 0; q < 3; q++) FarmGoods[playerid][g][q] = 0;
    }
}

// ============================================================
//  Парцели
// ============================================================

stock Farm_GetGrowTotal(plot)
{
    return CropInfo[FarmPlot[plot][plCrop]][cpGrowMinutes] * 60;
}

stock Farm_SetPlotObject(plot, model)
{
    if (FarmPlot[plot][plObject] != 0) {
        DestroyDynamicObject(STREAMER_TAG_OBJECT:FarmPlot[plot][plObject]);
        FarmPlot[plot][plObject] = 0;
    }
    if (model > 0) {
        FarmPlot[plot][plObject] = _:CreateDynamicObject(model,
            FarmPlot[plot][plX], FarmPlot[plot][plY], FarmPlot[plot][plZ] - 1.0, 0.0, 0.0, float(random(360)), 0, 0, -1, 150.0);
    }
}

stock Farm_ResetPlot(plot)
{
    FarmPlot[plot][plOwner] = INVALID_PLAYER_ID;
    FarmPlot[plot][plState] = PLOT_FREE;
    FarmPlot[plot][plCrows] = false;
    FarmPlot[plot][plWeeds] = false;
    Farm_SetPlotObject(plot, 0);
    Farm_UpdatePlotLabel(plot);
}

stock Farm_GetGrade(health)
{
    if (health >= 80) return QUALITY_A;
    if (health >= 50) return QUALITY_B;
    return QUALITY_C;
}

stock Farm_UpdatePlotLabel(plot)
{
    new text[256], owner[MAX_PLAYER_NAME + 1];
    new pid = FarmPlot[plot][plOwner];
    if (pid != INVALID_PLAYER_ID) GetRPName(pid, owner);

    switch (FarmPlot[plot][plState]) {
        case PLOT_FREE: {
            format(text, sizeof (text), "{A0A0A0}Парцел #%d\nсвободен", plot + 1);
        }
        case PLOT_RENTED: {
            format(text, sizeof (text), "{FF6A00}Парцел #%d {FFFFFF}| %s\n{A0A0A0}Прекопай земята (Y)", plot + 1, owner);
        }
        case PLOT_PLOWED: {
            format(text, sizeof (text), "{FF6A00}Парцел #%d {FFFFFF}| %s\n{A0A0A0}Готов за засяване (Y)", plot + 1, owner);
        }
        case PLOT_GROWING: {
            new percent = FarmPlot[plot][plGrowth] * 100 / Farm_GetGrowTotal(plot);
            format(text, sizeof (text), "{FF6A00}%s {FFFFFF}%d%% | %s\n{4FC3F7}Вода: %d%% {FFFFFF}| Здраве: %d%%",
                CropInfo[FarmPlot[plot][plCrop]][cpName], percent, owner, FarmPlot[plot][plWater], FarmPlot[plot][plHealth]);
            if (FarmPlot[plot][plCrows]) strcat(text, "\n{FF4040}Врани!");
            if (FarmPlot[plot][plWeeds]) strcat(text, "\n{FF4040}Плевели!");
        }
        case PLOT_RIPE: {
            format(text, sizeof (text), "{33CC66}%s е готова за жътва! (Y)\n{FFFFFF}%s | Качество: %s",
                CropInfo[FarmPlot[plot][plCrop]][cpName], owner, QualityName[Farm_GetGrade(FarmPlot[plot][plHealth])]);
        }
        default: {
            format(text, sizeof (text), "{FF4040}Изсъхнало\n{A0A0A0}%s – почисти парцела (Y)", owner);
        }
    }
    UpdateDynamic3DTextLabelText(FarmPlot[plot][plLabel], COLOR_WHITE, text);
}

stock Farm_CountOwnedPlots(playerid)
{
    new count = 0;
    for (new i = 0; i < FARM_MAX_PLOTS; i++) {
        if (FarmPlot[i][plExists] && FarmPlot[i][plOwner] == playerid) count++;
    }
    return count;
}

// Собствен парцел, на който стои играчът (-1 ако няма)
stock Farm_GetNearOwnPlot(playerid)
{
    for (new i = 0; i < FARM_MAX_PLOTS; i++) {
        if (!FarmPlot[i][plExists] || FarmPlot[i][plOwner] != playerid) continue;
        if (IsPlayerInRangeOfPoint(playerid, FARM_PLOT_RADIUS, FarmPlot[i][plX], FarmPlot[i][plY], FarmPlot[i][plZ])) return i;
    }
    return -1;
}

stock Farm_NotifyOwner(plot, const msg[])
{
    new pid = FarmPlot[plot][plOwner];
    if (pid != INVALID_PLAYER_ID) {
        SendClientMessage(pid, COLOR_GREEN, "[Ферма] {FFFFFF}Парцел #%d: %s", plot + 1, msg);
    }
}

// ============================================================
//  Смяна
// ============================================================

stock Farm_StartShift(playerid)
{
    if (FarmOnShift[playerid]) return SendError(playerid, "Вече си на смяна.");

    new skill = Job_GetSkill(playerid, JOB_FARM), given = 0;
    for (new i = 0; i < FARM_MAX_PLOTS && given < skill; i++) {
        if (!FarmPlot[i][plExists] || FarmPlot[i][plOwner] != INVALID_PLAYER_ID) continue;
        FarmPlot[i][plOwner] = playerid;
        FarmPlot[i][plState] = PLOT_RENTED;
        Farm_UpdatePlotLabel(i);
        given++;
    }
    if (given == 0) {
        // Нищо не е дадено – връщаме нищо
        return SendError(playerid, "В момента всички парцели са заети. Опитай след малко.");
    }

    FarmOnShift[playerid] = true;
    FarmCanUses[playerid] = FARM_CAN_USES;
    SetPlayerSkin(playerid, (PlayerData[playerid][pGender] == GENDER_FEMALE) ? FARM_SKIN_FEMALE : FARM_SKIN_MALE);

    SendClientMessage(playerid, COLOR_GREEN, "[Ферма] {FFFFFF}Смяната започна! Получи %d парцел(а) – номерата им се виждат на полето.", given);
    SendClientMessage(playerid, COLOR_WHITE, "Застани на парцела си и натисни {FFD54A}Y{FFFFFF}: прекопай, засей, поливай, ожъни.");
    SendClientMessage(playerid, COLOR_GREY, "Каната ти е пълна (%d поливания). Пълни я на кладенеца. Реколтата се продава при изкупвача.", FARM_CAN_USES);
    return 1;
}

stock Farm_EndShift(playerid, bool:silent = false)
{
    if (!FarmOnShift[playerid]) return;
    FarmOnShift[playerid] = false;

    for (new i = 0; i < FARM_MAX_PLOTS; i++) {
        if (FarmPlot[i][plExists] && FarmPlot[i][plOwner] == playerid) Farm_ResetPlot(i);
    }
    if (FarmVehicle[playerid] != INVALID_VEHICLE_ID) {
        DestroyVehicle(FarmVehicle[playerid]);
        FarmVehicle[playerid] = INVALID_VEHICLE_ID;
    }
    if (Minigame_IsActive(playerid)) Minigame_Stop(playerid);
    if (IsPlayerConnected(playerid)) SetPlayerSkin(playerid, PlayerData[playerid][pSkin]);

    if (!silent) {
        SendClientMessage(playerid, COLOR_GREEN, "[Ферма] {FFFFFF}Смяната приключи. Ако носиш реколта, продай я при изкупвача.");
    }
}

stock Farm_OnPlayerDisconnect(playerid)
{
    Farm_EndShift(playerid, true);
    Farm_ResetPlayer(playerid);
}

stock Farm_OnPlayerSpawn(playerid)
{
    if (FarmOnShift[playerid]) {
        SetPlayerSkin(playerid, (PlayerData[playerid][pGender] == GENDER_FEMALE) ? FARM_SKIN_FEMALE : FARM_SKIN_MALE);
    }
}

stock Farm_SpawnVehicle(playerid, model)
{
    new Float:x, Float:y, Float:z, Float:a;
    if (!Job_GetPoint(JOB_FARM, "vehicle", x, y, z, a)) {
        SendError(playerid, "Мястото за техниката още не е поставено (админ: /farmedit vehicle).");
        return;
    }
    if (FarmVehicle[playerid] != INVALID_VEHICLE_ID) DestroyVehicle(FarmVehicle[playerid]);

    new vehicleid = CreateVehicle(model, x, y, z + 0.5, a, 1, 1, -1);
    if (vehicleid == INVALID_VEHICLE_ID || vehicleid == 0) {
        FarmVehicle[playerid] = INVALID_VEHICLE_ID;
        SendError(playerid, "Техниката не може да бъде създадена.");
        return;
    }
    FarmVehicle[playerid] = vehicleid;
    SetVehicleNumberPlate(vehicleid, "FARM");
    PutPlayerInVehicle(playerid, vehicleid, 0);

    if (model == FARM_TRACTOR_MODEL) {
        SendClientMessage(playerid, COLOR_GREEN, "[Ферма] {FFFFFF}Трактор: мини през свой парцел, който чака прекопаване – прекопава го сам.");
    } else {
        SendClientMessage(playerid, COLOR_GREEN, "[Ферма] {FFFFFF}Комбайн: мини през свой узрял парцел – ожънва го сам.");
    }
}

// ============================================================
//  Менюта
// ============================================================

stock Farm_ShowJobMenu(playerid)
{
    new body[512], count = 0, skill = Job_GetSkill(playerid, JOB_FARM);
    body[0] = EOS;

    if (!FarmOnShift[playerid]) {
        strcat(body, C_GREEN "» " C_WHITE "Започни смяна\n");
        FarmMenuAction[playerid][count++] = FARM_JOB_START;
    } else {
        strcat(body, C_RED "» " C_WHITE "Приключи смяната\n");
        FarmMenuAction[playerid][count++] = FARM_JOB_END;
        if (skill >= 2) {
            strcat(body, C_BRAND "» " C_WHITE "Вземи трактор\n");
            FarmMenuAction[playerid][count++] = FARM_JOB_TRACTOR;
        } else {
            strcat(body, C_GREY "» Трактор (нужен скил 2)\n");
            FarmMenuAction[playerid][count++] = 0;
        }
        if (skill >= 3) {
            strcat(body, C_BRAND "» " C_WHITE "Вземи комбайн\n");
            FarmMenuAction[playerid][count++] = FARM_JOB_COMBINE;
        } else {
            strcat(body, C_GREY "» Комбайн (нужен скил 3)\n");
            FarmMenuAction[playerid][count++] = 0;
        }
    }
    strcat(body, C_BLUE "» " C_WHITE "Как работи фермата");
    FarmMenuAction[playerid][count++] = FARM_JOB_INFO;

    new title[64];
    format(title, sizeof (title), C_BRAND "Ферма " C_WHITE "| скил %d/%d", skill, Job_GetMaxSkill(JOB_FARM));
    ShowPlayerDialog(playerid, DIALOG_FARM_JOB, DIALOG_STYLE_LIST, title, "%s", "Избери", "Затвори", body);
}

stock Farm_ShowInfo(playerid)
{
    new body[2048], line[192], weather[24];
    new skill = Job_GetSkill(playerid, JOB_FARM), max = Job_GetMaxSkill(JOB_FARM);
    Weather_GetName(weather);

    format(body, sizeof (body),
        C_WHITE "Твоят скил: " C_GOLD "%d/%d" C_WHITE " (%d EXP)   Заплащане: " C_GREEN "%d%%" C_WHITE "   Парцели: " C_GOLD "%d\n" \
        C_WHITE "Времето сега: " C_BLUE "%s\n\n",
        skill, max, PlayerJobExp[playerid][JOB_FARM], Job_GetPayPercent(playerid, JOB_FARM), skill, weather);

    strcat(body, C_BRAND "Как се работи\n");
    strcat(body, C_WHITE "1. Застани на своя парцел и натисни " C_GOLD "Y" C_WHITE " - прекопай земята.\n");
    strcat(body, C_WHITE "2. Засей култура (семената се плащат).\n");
    strcat(body, C_WHITE "3. Поливай навреме, гони враните, скубни плевелите.\n");
    strcat(body, C_WHITE "4. Ожъни, щом узрее - не чакай, иначе се разваля.\n");
    strcat(body, C_WHITE "5. Продай реколтата при изкупвача.\n\n");

    strcat(body, C_BRAND "Култури и изкупни цени сега\n");
    for (new c = 0; c < sizeof (CropInfo); c++) {
        format(line, sizeof (line), C_GOLD "%s" C_WHITE ": семена $%d, расте %d мин., ~%d кг, " C_GREEN "$%d/кг" C_GREY " (склад: %d кг)\n",
            CropInfo[c][cpName], CropInfo[c][cpSeedCost], CropInfo[c][cpGrowMinutes], CropInfo[c][cpYield],
            Market_GetPrice(CropInfo[c][cpGoods]), Market_GetStock(CropInfo[c][cpGoods]));
        strcat(body, line);
    }

    strcat(body, "\n" C_BRAND "Качество\n");
    strcat(body, C_WHITE "A (здраве 80+) - x1.5   B (50+) - x1.0   C - x0.5\n");
    strcat(body, C_GREY "В жега растенията пият повече вода, а дъждът ги полива.\n\n");

    strcat(body, C_BRAND "Скил\n");
    strcat(body, C_WHITE "1: 1 парцел, на ръка   2: 2 парцела + трактор   3: 3 парцела + комбайн\n");
    strcat(body, C_GREY "+1 EXP за всяка жътва, +1 за качество A.");

    ShowPlayerDialog(playerid, DIALOG_FARM_INFO, DIALOG_STYLE_MSGBOX, C_BRAND "Ферма " C_WHITE "| Информация", "%s", "Назад", "", body);
}

stock Farm_ShowPlotMenu(playerid, plot)
{
    new body[512], count = 0, title[96];
    body[0] = EOS;
    FarmActionPlot[playerid] = plot;

    switch (FarmPlot[plot][plState]) {
        case PLOT_RENTED: {
            strcat(body, C_BRAND "» " C_WHITE "Прекопай земята\n");
            FarmMenuAction[playerid][count++] = FARM_ACT_DIG;
            format(title, sizeof (title), C_BRAND "Парцел #%d " C_WHITE "| неподготвен", plot + 1);
        }
        case PLOT_PLOWED: {
            strcat(body, C_BRAND "» " C_WHITE "Засей\n");
            FarmMenuAction[playerid][count++] = FARM_ACT_SEED;
            format(title, sizeof (title), C_BRAND "Парцел #%d " C_WHITE "| прекопан", plot + 1);
        }
        case PLOT_GROWING: {
            new line[96];
            format(line, sizeof (line), C_BLUE "» " C_WHITE "Полей (в каната: %d)\n", FarmCanUses[playerid]);
            strcat(body, line);
            FarmMenuAction[playerid][count++] = FARM_ACT_WATER;
            if (FarmPlot[plot][plCrows]) {
                strcat(body, C_RED "» " C_WHITE "Прогони враните\n");
                FarmMenuAction[playerid][count++] = FARM_ACT_CROWS;
            }
            if (FarmPlot[plot][plWeeds]) {
                strcat(body, C_RED "» " C_WHITE "Изскубни плевелите\n");
                FarmMenuAction[playerid][count++] = FARM_ACT_WEEDS;
            }
            strcat(body, C_GREY "» " C_WHITE "Състояние\n");
            FarmMenuAction[playerid][count++] = FARM_ACT_STATUS;
            format(title, sizeof (title), C_BRAND "Парцел #%d " C_WHITE "| %s %d%%", plot + 1,
                CropInfo[FarmPlot[plot][plCrop]][cpName], FarmPlot[plot][plGrowth] * 100 / Farm_GetGrowTotal(plot));
        }
        case PLOT_RIPE: {
            strcat(body, C_GREEN "» " C_WHITE "Ожъни\n");
            FarmMenuAction[playerid][count++] = FARM_ACT_HARVEST;
            format(title, sizeof (title), C_BRAND "Парцел #%d " C_GREEN "| узряло", plot + 1);
        }
        default: {
            strcat(body, C_RED "» " C_WHITE "Почисти парцела\n");
            FarmMenuAction[playerid][count++] = FARM_ACT_CLEAR;
            format(title, sizeof (title), C_BRAND "Парцел #%d " C_RED "| изсъхнало", plot + 1);
        }
    }
    strcat(body, C_GREY "» Главно меню");
    FarmMenuAction[playerid][count++] = FARM_ACT_MAINMENU;
    ShowPlayerDialog(playerid, DIALOG_FARM_PLOT, DIALOG_STYLE_LIST, title, "%s", "Избери", "Затвори", body);
}

stock Farm_ShowSeedMenu(playerid)
{
    new body[512], line[128];
    body[0] = EOS;
    for (new c = 0; c < sizeof (CropInfo); c++) {
        format(line, sizeof (line), C_GOLD "%s" C_WHITE " - семена " C_GREEN "$%d" C_WHITE ", расте %d мин.\n",
            CropInfo[c][cpName], CropInfo[c][cpSeedCost], CropInfo[c][cpGrowMinutes]);
        strcat(body, line);
    }
    ShowPlayerDialog(playerid, DIALOG_FARM_SEED, DIALOG_STYLE_LIST, C_BRAND "Ферма " C_WHITE "| Какво ще садиш?", "%s", "Засей", "Назад", body);
}

stock Farm_ShowStatus(playerid, plot)
{
    new body[768], weather[24];
    Weather_GetName(weather);
    new total = Farm_GetGrowTotal(plot), left = (total - FarmPlot[plot][plGrowth]) / 60 + 1;
    format(body, sizeof (body),
        C_WHITE "Култура: " C_GOLD "%s\n" \
        C_WHITE "Растеж: " C_GOLD "%d%%" C_WHITE " (още ~%d мин.)\n" \
        C_WHITE "Вода: " C_BLUE "%d%%\n" \
        C_WHITE "Здраве: " C_GREEN "%d%%" C_WHITE " -> очаквано качество " C_GOLD "%s\n" \
        C_WHITE "Времето: " C_BLUE "%s\n\n" \
        C_GREY "Без вода растенето спира и здравето пада. Враните и плевелите също вредят.",
        CropInfo[FarmPlot[plot][plCrop]][cpName], FarmPlot[plot][plGrowth] * 100 / total, left,
        FarmPlot[plot][plWater], FarmPlot[plot][plHealth], QualityName[Farm_GetGrade(FarmPlot[plot][plHealth])], weather);
    ShowPlayerDialog(playerid, DIALOG_FARM_STATUS, DIALOG_STYLE_MSGBOX, C_BRAND "Ферма " C_WHITE "| Състояние", "%s", "Назад", "", body);
}

// Изкупвач
stock Farm_GetGoodsValue(playerid, goods, quality)
{
    new kg = FarmGoods[playerid][goods][quality];
    if (kg <= 0) return 0;
    return kg * Market_GetPrice(goods) * QualityPercent[quality] / 100 * Job_GetPayPercent(playerid, JOB_FARM) / 100;
}

stock Farm_ShowSellMenu(playerid)
{
    new body[1024], line[160], total = 0, name[24];
    format(body, sizeof (body), C_WHITE "Изкупвачът предлага за реколтата ти:\n\n");
    for (new g = 0; g < MAX_GOODS; g++) {
        for (new q = 0; q < 3; q++) {
            new kg = FarmGoods[playerid][g][q];
            if (kg <= 0) continue;
            new value = Farm_GetGoodsValue(playerid, g, q);
            Market_GetName(g, name);
            format(line, sizeof (line), C_GOLD "%s" C_WHITE " (кач. %s): %d кг -> " C_GREEN "$%d\n", name, QualityName[q], kg, value);
            strcat(body, line);
            total += value;
        }
    }
    if (total == 0) {
        ShowPlayerDialog(playerid, DIALOG_INFO, DIALOG_STYLE_MSGBOX, C_BRAND "Изкупуване", C_WHITE "Нямаш реколта за продан.", "Затвори", "");
        return;
    }
    format(line, sizeof (line), "\n" C_WHITE "Общо: " C_GREEN "$%d" C_GREY " (с бонус за скил %d%%)", total, Job_GetPayPercent(playerid, JOB_FARM));
    strcat(body, line);
    ShowPlayerDialog(playerid, DIALOG_FARM_SELL, DIALOG_STYLE_MSGBOX, C_BRAND "Изкупуване на реколта", "%s", "Продай", "Отказ", body);
}

stock Farm_SellAll(playerid)
{
    new total = 0;
    for (new g = 0; g < MAX_GOODS; g++) {
        new kgTotal = 0;
        for (new q = 0; q < 3; q++) {
            total += Farm_GetGoodsValue(playerid, g, q);
            kgTotal += FarmGoods[playerid][g][q];
            FarmGoods[playerid][g][q] = 0;
        }
        if (kgTotal > 0) Market_Add(g, kgTotal);
    }
    if (total <= 0) return;
    Money_Give(playerid, total);
    Auth_SaveAccount(playerid);
    SendClientMessage(playerid, COLOR_GREEN, "[Ферма] {FFFFFF}Продаде реколтата за {33CC66}$%d{FFFFFF}.", total);

    new details[32];
    format(details, sizeof (details), "$%d", total);
    Log_Add("farm_sell", PlayerData[playerid][pName], "", details);
}

// ============================================================
//  Y и пикапи
// ============================================================

// Връща true, ако Y е използван от фермата (играчът е на свой парцел).
stock bool:Farm_OnYPressed(playerid)
{
    if (!FarmOnShift[playerid] || IsPlayerInAnyVehicle(playerid) || Minigame_IsActive(playerid)) return false;
    new plot = Farm_GetNearOwnPlot(playerid);
    if (plot == -1) return false;
    Farm_ShowPlotMenu(playerid, plot);
    return true;
}

stock bool:Farm_OnPickUp(playerid, pickupid)
{
    if (pickupid != gFarmPickupJob && pickupid != gFarmPickupBuyer && pickupid != gFarmPickupWell) return false;
    if (!PlayerData[playerid][pLogged] || IsPlayerInAnyVehicle(playerid)) return true;

    new now = GetTickCount();
    if (now - FarmPickupTick[playerid] < 4000) return true;
    FarmPickupTick[playerid] = now;

    if (pickupid == gFarmPickupJob) {
        Farm_ShowJobMenu(playerid);
    } else if (pickupid == gFarmPickupBuyer) {
        Farm_ShowSellMenu(playerid);
    } else {
        if (!FarmOnShift[playerid]) {
            SendInfo(playerid, "Каната се пълни само по време на смяна във фермата.");
        } else if (FarmCanUses[playerid] >= FARM_CAN_USES) {
            SendInfo(playerid, "Каната ти вече е пълна.");
        } else {
            FarmCanUses[playerid] = FARM_CAN_USES;
            ApplyAnimation(playerid, "BOMBER", "BOM_Plant", 4.1, false, false, false, false, 0);
            SendClientMessage(playerid, COLOR_LIGHTBLUE, "[Ферма] {FFFFFF}Напълни каната (%d поливания).", FARM_CAN_USES);
        }
    }
    return true;
}

// ============================================================
//  Диалози
// ============================================================

stock Farm_OnDialogResponse(playerid, dialogid, response, listitem)
{
    switch (dialogid) {
        case DIALOG_FARM_JOB: {
            if (!response || listitem < 0 || listitem >= 8) return 1;
            switch (FarmMenuAction[playerid][listitem]) {
                case FARM_JOB_START:   Farm_StartShift(playerid);
                case FARM_JOB_END:     Farm_EndShift(playerid);
                case FARM_JOB_TRACTOR: Farm_SpawnVehicle(playerid, FARM_TRACTOR_MODEL);
                case FARM_JOB_COMBINE: Farm_SpawnVehicle(playerid, FARM_COMBINE_MODEL);
                case FARM_JOB_INFO:    Farm_ShowInfo(playerid);
                default:               SendError(playerid, "Скилът ти още не стига за това.");
            }
            return 1;
        }
        case DIALOG_FARM_INFO: {
            Farm_ShowJobMenu(playerid);
            return 1;
        }
        case DIALOG_FARM_STATUS: {
            new plot = FarmActionPlot[playerid];
            if (plot >= 0 && FarmPlot[plot][plOwner] == playerid) Farm_ShowPlotMenu(playerid, plot);
            return 1;
        }
        case DIALOG_FARM_PLOT: {
            new plot = FarmActionPlot[playerid];
            if (!response || listitem < 0 || listitem >= 8) return 1;
            if (plot < 0 || FarmPlot[plot][plOwner] != playerid) return 1;
            if (FarmMenuAction[playerid][listitem] != FARM_ACT_MAINMENU && Farm_GetNearOwnPlot(playerid) != plot) {
                SendError(playerid, "Отиде твърде далеч от парцела.");
                return 1;
            }
            switch (FarmMenuAction[playerid][listitem]) {
                case FARM_ACT_DIG: {
                    ApplyAnimation(playerid, "BASEBALL", "Bat_4", 4.1, false, false, false, false, 0);
                    Minigame_Start(playerid, MG_TYPE_BOUNCE, "DIG", MG_OWNER_FARM, FARM_MG_DIG, plot);
                }
                case FARM_ACT_SEED: Farm_ShowSeedMenu(playerid);
                case FARM_ACT_WATER: {
                    if (FarmCanUses[playerid] <= 0) return SendError(playerid, "Каната е празна. Напълни я на кладенеца.");
                    Minigame_Start(playerid, MG_TYPE_FILL, "WATER", MG_OWNER_FARM, FARM_MG_WATER, plot);
                }
                case FARM_ACT_CROWS: Minigame_Start(playerid, MG_TYPE_BOUNCE, "SCARE THE CROWS", MG_OWNER_FARM, FARM_MG_CROWS, plot);
                case FARM_ACT_WEEDS: {
                    ApplyAnimation(playerid, "BOMBER", "BOM_Plant", 4.1, false, false, false, false, 0);
                    Minigame_Start(playerid, MG_TYPE_BOUNCE, "WEEDS", MG_OWNER_FARM, FARM_MG_WEEDS, plot);
                }
                case FARM_ACT_STATUS: Farm_ShowStatus(playerid, plot);
                case FARM_ACT_HARVEST: {
                    ApplyAnimation(playerid, "BOMBER", "BOM_Plant", 4.1, false, false, false, false, 0);
                    Minigame_Start(playerid, MG_TYPE_BOUNCE, "HARVEST", MG_OWNER_FARM, FARM_MG_HARVEST, plot);
                }
                case FARM_ACT_CLEAR: {
                    FarmPlot[plot][plState] = PLOT_RENTED;
                    Farm_SetPlotObject(plot, 0);
                    Farm_UpdatePlotLabel(plot);
                    SendClientMessage(playerid, COLOR_GREEN, "[Ферма] {FFFFFF}Парцелът е почистен. Прекопай го отново.");
                }
                case FARM_ACT_MAINMENU: Menu_Show(playerid);
            }
            return 1;
        }
        case DIALOG_FARM_SEED: {
            new plot = FarmActionPlot[playerid];
            if (plot < 0 || FarmPlot[plot][plOwner] != playerid) return 1;
            if (!response) {
                Farm_ShowPlotMenu(playerid, plot);
                return 1;
            }
            if (listitem < 0 || listitem >= sizeof (CropInfo) || FarmPlot[plot][plState] != PLOT_PLOWED) return 1;

            new cost = CropInfo[listitem][cpSeedCost];
            if (Money_Get(playerid) < cost) return SendError(playerid, "Нямаш достатъчно пари за семената.");
            Money_Give(playerid, -cost);

            FarmPlot[plot][plState] = PLOT_GROWING;
            FarmPlot[plot][plCrop] = listitem;
            FarmPlot[plot][plGrowth] = 0;
            FarmPlot[plot][plWater] = FARM_WATER_START;
            FarmPlot[plot][plHealth] = 100;
            FarmPlot[plot][plCrows] = false;
            FarmPlot[plot][plWeeds] = false;
            FarmPlot[plot][plStage] = 0;
            FarmPlot[plot][plSecondsAcc] = 0;
            Farm_SetPlotObject(plot, CropInfo[listitem][cpModels][0]);
            Farm_UpdatePlotLabel(plot);

            ApplyAnimation(playerid, "BOMBER", "BOM_Plant", 4.1, false, false, false, false, 0);
            SendClientMessage(playerid, COLOR_GREEN, "[Ферма] {FFFFFF}Засади %s ($%d). Ще узрее след около %d мин. Не забравяй да поливаш!",
                CropInfo[listitem][cpName], cost, CropInfo[listitem][cpGrowMinutes]);
            return 1;
        }
        case DIALOG_FARM_SELL: {
            if (response) Farm_SellAll(playerid);
            return 1;
        }
    }
    return 0;
}

// ============================================================
//  Резултати от мини-игрите
// ============================================================

stock Farm_OnMinigameResult(playerid, purpose, plot, result)
{
    ClearAnimations(playerid);
    if (plot < 0 || plot >= FARM_MAX_PLOTS || FarmPlot[plot][plOwner] != playerid) return;

    switch (purpose) {
        case FARM_MG_DIG: {
            if (result == MG_RESULT_FAIL) {
                SendClientMessage(playerid, COLOR_GREY, "Мотиката удари камък. Опитай пак (Y).");
                return;
            }
            FarmPlot[plot][plState] = PLOT_PLOWED;
            Farm_UpdatePlotLabel(plot);
            SendClientMessage(playerid, COLOR_GREEN, "[Ферма] {FFFFFF}Земята е прекопана. Натисни Y, за да засееш.");
        }
        case FARM_MG_WATER: {
            if (FarmPlot[plot][plState] != PLOT_GROWING) return;
            FarmCanUses[playerid]--;
            if (result == MG_RESULT_FAIL) {
                FarmPlot[plot][plWater] += 10;
                FarmPlot[plot][plHealth] -= 5;
                SendClientMessage(playerid, COLOR_GREY, "Преля! Корените се преовлажниха (-5 здраве).");
            } else if (result == MG_RESULT_PERFECT) {
                FarmPlot[plot][plWater] += FARM_WATER_ADD + 10;
                FarmPlot[plot][plHealth] += 3;
                SendClientMessage(playerid, COLOR_GREEN, "[Ферма] {FFFFFF}Перфектно поливане! (+3 здраве)");
            } else {
                FarmPlot[plot][plWater] += FARM_WATER_ADD;
                SendClientMessage(playerid, COLOR_GREEN, "[Ферма] {FFFFFF}Поля растенията.");
            }
            if (FarmPlot[plot][plWater] > 100) FarmPlot[plot][plWater] = 100;
            if (FarmPlot[plot][plHealth] > 100) FarmPlot[plot][plHealth] = 100;
            if (FarmPlot[plot][plHealth] < 0) FarmPlot[plot][plHealth] = 0;
            Farm_UpdatePlotLabel(plot);
        }
        case FARM_MG_CROWS: {
            if (result == MG_RESULT_FAIL) {
                SendClientMessage(playerid, COLOR_GREY, "Враните само отлетяха на съседния ред... Опитай пак.");
                return;
            }
            FarmPlot[plot][plCrows] = false;
            Farm_UpdatePlotLabel(plot);
            SendClientMessage(playerid, COLOR_GREEN, "[Ферма] {FFFFFF}Прогони враните!");
        }
        case FARM_MG_WEEDS: {
            if (result == MG_RESULT_FAIL) {
                SendClientMessage(playerid, COLOR_GREY, "Плевелът се скъса, но корените останаха. Опитай пак.");
                return;
            }
            FarmPlot[plot][plWeeds] = false;
            if (result == MG_RESULT_PERFECT && FarmPlot[plot][plHealth] < 100) FarmPlot[plot][plHealth] += 2;
            Farm_UpdatePlotLabel(plot);
            SendClientMessage(playerid, COLOR_GREEN, "[Ферма] {FFFFFF}Изскубна плевелите.");
        }
        case FARM_MG_HARVEST: {
            if (FarmPlot[plot][plState] != PLOT_RIPE) return;
            new percent = 100;
            if (result == MG_RESULT_FAIL) percent = 80;
            else if (result == MG_RESULT_PERFECT) percent = 110;
            Farm_Harvest(playerid, plot, percent);
        }
    }
}

stock Farm_Harvest(playerid, plot, yieldPercent)
{
    new crop = FarmPlot[plot][plCrop];
    new grade = Farm_GetGrade(FarmPlot[plot][plHealth]);
    new kg = CropInfo[crop][cpYield] * yieldPercent / 100;
    if (kg < 1) kg = 1;

    FarmGoods[playerid][CropInfo[crop][cpGoods]][grade] += kg;

    FarmPlot[plot][plState] = PLOT_RENTED;
    Farm_SetPlotObject(plot, 0);
    Farm_UpdatePlotLabel(plot);

    SendClientMessage(playerid, COLOR_GREEN, "[Ферма] {FFFFFF}Ожъна {FFD54A}%d кг %s{FFFFFF}, качество {FFD54A}%s{FFFFFF}. Продай ги при изкупвача.",
        kg, CropInfo[crop][cpName], QualityName[grade]);

    Job_AddExp(playerid, JOB_FARM, (grade == QUALITY_A) ? 2 : 1);
}

// ============================================================
//  Таймер (всяка секунда от Server_Tick)
// ============================================================

stock Farm_OnTick(tick)
{
    if (tick % FARM_TICK_SECONDS != 0) return;

    for (new i = 0; i < FARM_MAX_PLOTS; i++) {
        if (!FarmPlot[i][plExists] || FarmPlot[i][plOwner] == INVALID_PLAYER_ID) continue;
        new st = FarmPlot[i][plState];
        if (st != PLOT_GROWING && st != PLOT_RIPE) continue;

        if (st == PLOT_GROWING && FarmPlot[i][plWater] > 0) {
            FarmPlot[i][plGrowth] += FARM_TICK_SECONDS;
        }

        FarmPlot[i][plSecondsAcc] += FARM_TICK_SECONDS;
        if (FarmPlot[i][plSecondsAcc] >= 60) {
            FarmPlot[i][plSecondsAcc] = 0;
            Farm_PlotMinute(i);
        }
        if (FarmPlot[i][plState] == PLOT_GROWING) Farm_CheckGrowth(i);
        Farm_UpdatePlotLabel(i);
    }

    // Техника: трактор прекопава, комбайн жъне
    for (new p = 0; p < MAX_PLAYERS; p++) {
        if (!FarmOnShift[p] || FarmVehicle[p] == INVALID_VEHICLE_ID) continue;
        if (GetPlayerVehicleID(p) != FarmVehicle[p]) continue;
        new model = GetVehicleModel(FarmVehicle[p]);
        for (new i = 0; i < FARM_MAX_PLOTS; i++) {
            if (!FarmPlot[i][plExists] || FarmPlot[i][plOwner] != p) continue;
            if (!IsPlayerInRangeOfPoint(p, 5.0, FarmPlot[i][plX], FarmPlot[i][plY], FarmPlot[i][plZ])) continue;
            if (model == FARM_TRACTOR_MODEL && FarmPlot[i][plState] == PLOT_RENTED) {
                FarmPlot[i][plState] = PLOT_PLOWED;
                Farm_UpdatePlotLabel(i);
                SendClientMessage(p, COLOR_GREEN, "[Ферма] {FFFFFF}Тракторът прекопа парцел #%d.", i + 1);
            } else if (model == FARM_COMBINE_MODEL && FarmPlot[i][plState] == PLOT_RIPE) {
                Farm_Harvest(p, i, 105);
            }
        }
    }
}

stock Farm_PlotMinute(plot)
{
    // Вода според времето
    switch (Weather_GetKind()) {
        case WEATHER_KIND_RAIN:   FarmPlot[plot][plWater] += 15;
        case WEATHER_KIND_HOT:    FarmPlot[plot][plWater] -= FARM_WATER_PER_MIN * 2;
        case WEATHER_KIND_CLOUDY: FarmPlot[plot][plWater] -= FARM_WATER_PER_MIN * 7 / 10;
        default:                  FarmPlot[plot][plWater] -= FARM_WATER_PER_MIN;
    }
    if (FarmPlot[plot][plWater] > 100) FarmPlot[plot][plWater] = 100;
    if (FarmPlot[plot][plWater] < 0) FarmPlot[plot][plWater] = 0;

    if (FarmPlot[plot][plState] == PLOT_GROWING) {
        if (FarmPlot[plot][plWater] == 0) FarmPlot[plot][plHealth] -= 4;
        if (FarmPlot[plot][plCrows]) FarmPlot[plot][plHealth] -= 3;
        if (FarmPlot[plot][plWeeds]) FarmPlot[plot][plHealth] -= 2;

        if (FarmPlot[plot][plWater] == 20) Farm_NotifyOwner(plot, "{4FC3F7}растенията жадуват – полей ги!");
        if (!FarmPlot[plot][plCrows] && random(100) < 6) {
            FarmPlot[plot][plCrows] = true;
            Farm_NotifyOwner(plot, "{FF4040}врани кацнаха на посевите!");
        }
        if (!FarmPlot[plot][plWeeds] && random(100) < 4) {
            FarmPlot[plot][plWeeds] = true;
            Farm_NotifyOwner(plot, "{FF4040}поникнаха плевели!");
        }
    } else if (FarmPlot[plot][plState] == PLOT_RIPE) {
        new overdue = (gettime() - FarmPlot[plot][plRipeTime]) / 60;
        if (overdue >= FARM_RIPE_LOST_MIN) {
            FarmPlot[plot][plState] = PLOT_WITHERED;
            Farm_SetPlotObject(plot, 0);
            Farm_NotifyOwner(plot, "{FF4040}реколтата изсъхна, защото не я ожъна навреме.");
            return;
        }
        if (overdue >= FARM_RIPE_GRACE_MIN) {
            FarmPlot[plot][plHealth] -= 5;
            if (overdue == FARM_RIPE_GRACE_MIN) Farm_NotifyOwner(plot, "{FF4040}реколтата започва да се разваля – ожъни я!");
        }
    }

    if (FarmPlot[plot][plHealth] < 0) FarmPlot[plot][plHealth] = 0;
    if (FarmPlot[plot][plHealth] == 0 && FarmPlot[plot][plState] == PLOT_GROWING) {
        FarmPlot[plot][plState] = PLOT_WITHERED;
        Farm_SetPlotObject(plot, 0);
        Farm_NotifyOwner(plot, "{FF4040}растенията загинаха.");
    }
}

stock Farm_CheckGrowth(plot)
{
    new total = Farm_GetGrowTotal(plot), percent = FarmPlot[plot][plGrowth] * 100 / total;
    new stage = (percent < 34) ? 0 : ((percent < 67) ? 1 : 2);
    new crop = FarmPlot[plot][plCrop];

    if (stage != FarmPlot[plot][plStage]) {
        FarmPlot[plot][plStage] = stage;
        Farm_SetPlotObject(plot, CropInfo[crop][cpModels][stage]);
    }
    if (FarmPlot[plot][plGrowth] >= total) {
        FarmPlot[plot][plState] = PLOT_RIPE;
        FarmPlot[plot][plRipeTime] = gettime();
        FarmPlot[plot][plCrows] = false;
        FarmPlot[plot][plWeeds] = false;
        Farm_NotifyOwner(plot, "{33CC66}реколтата узря! Ожъни я, преди да се развали.");
    }
}

// ============================================================
//  Админ редактор
// ============================================================

CMD:farmedit(playerid, params[])
{
    if (!Admin_Check(playerid, ADMIN_ADMIN)) return 1;

    new sub[16];
    if (!GetParam(params, 0, sub)) {
        SendUsage(playerid, "/farmedit [job | buyer | well | vehicle | addplot | delplot | list]");
        SendClientMessage(playerid, COLOR_GREY, "job - вход на работата, buyer - изкупвач, well - кладенец, vehicle - място за техниката");
        return 1;
    }

    new Float:x, Float:y, Float:z, Float:a;
    GetPlayerPos(playerid, x, y, z);
    GetPlayerFacingAngle(playerid, a);

    if (!strcmp(sub, "job", true) || !strcmp(sub, "buyer", true) || !strcmp(sub, "well", true) || !strcmp(sub, "vehicle", true)) {
        for (new i = 0; sub[i] != EOS; i++) sub[i] = tolower(sub[i]);
        Job_SetPoint(JOB_FARM, sub, x, y, z, a);
        Farm_RefreshPoints();
        SendClientMessage(playerid, COLOR_ADMIN, "[Ферма] Точката \"%s\" е поставена тук.", sub);
        Log_Add("farmedit", PlayerData[playerid][pName], sub, "");
        return 1;
    }

    if (!strcmp(sub, "addplot", true)) {
        new q[192];
        format(q, sizeof (q), "INSERT INTO farm_plots (x, y, z) VALUES (%f, %f, %f)", x, y, z);
        Database_Exec(q);

        new dbid = 0;
        new DBResult:result = Database_Query("SELECT MAX(id) AS id FROM farm_plots");
        if (result != DBResult:0) {
            dbid = DB_GetFieldIntByName(result, "id");
            DB_FreeResultSet(result);
        }
        new plot = Farm_AddPlotToMemory(dbid, x, y, z);
        if (plot == -1) return SendError(playerid, "Достигнат е максималният брой парцели.");
        SendClientMessage(playerid, COLOR_ADMIN, "[Ферма] Добавен парцел #%d.", plot + 1);
        return 1;
    }

    if (!strcmp(sub, "delplot", true)) {
        for (new i = 0; i < FARM_MAX_PLOTS; i++) {
            if (!FarmPlot[i][plExists]) continue;
            if (!IsPlayerInRangeOfPoint(playerid, 3.0, FarmPlot[i][plX], FarmPlot[i][plY], FarmPlot[i][plZ])) continue;
            if (FarmPlot[i][plOwner] != INVALID_PLAYER_ID) return SendError(playerid, "Парцелът е зает от играч.");

            new q[96];
            format(q, sizeof (q), "DELETE FROM farm_plots WHERE id = %d", FarmPlot[i][plDBID]);
            Database_Exec(q);
            Farm_SetPlotObject(i, 0);
            DestroyDynamic3DTextLabel(FarmPlot[i][plLabel]);
            FarmPlot[i][plExists] = false;
            SendClientMessage(playerid, COLOR_ADMIN, "[Ферма] Парцел #%d е премахнат.", i + 1);
            return 1;
        }
        return SendError(playerid, "Няма парцел в радиус 3 м.");
    }

    if (!strcmp(sub, "list", true)) {
        new count = 0;
        for (new i = 0; i < FARM_MAX_PLOTS; i++) if (FarmPlot[i][plExists]) count++;
        new names[][] = {"job", "buyer", "well", "vehicle"};
        SendClientMessage(playerid, COLOR_ADMIN, "[Ферма] Парцели: %d/%d", count, FARM_MAX_PLOTS);
        for (new n = 0; n < sizeof (names); n++) {
            if (Job_GetPoint(JOB_FARM, names[n], x, y, z, a)) {
                SendClientMessage(playerid, COLOR_WHITE, "%s: %.1f, %.1f, %.1f", names[n], x, y, z);
            } else {
                SendClientMessage(playerid, COLOR_RED, "%s: не е поставена", names[n]);
            }
        }
        return 1;
    }

    SendUsage(playerid, "/farmedit [job | buyer | well | vehicle | addplot | delplot | list]");
    return 1;
}

CMD:farm(playerid, params[])
{
    #pragma unused params
    Farm_ShowInfo(playerid);
    return 1;
}
