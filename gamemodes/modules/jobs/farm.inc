// Работа: ФЕРМА (памук) – по спецификация docs/specs/farm.md (v1)
//
// Начална работа: без ниво, без кметство. Фермата е при Blueberry.
//
//   Иван (NPC, ALT)      – смяна: депозит $200 за гребло, работни дрехи, HUD
//   Храст (ALT до него)  – беране с лента за прогрес; брояч "N от 10" над храста
//   Изчерпан храст       – кълн -> стрък -> узрял (3 етапа), дъждът ускорява, жегата забавя
//   Петър (NPC, ALT)     – изкупвач, цена според пазара
//
// Админ: /farmedit – Иван/изкупвач тук, добави/изтрий храст, пробни модели, избор на модели.

// ------------------------------------------------------------
//  Настройки
// ------------------------------------------------------------

#define FARM_MAX_BUSHES         40
#define FARM_BUSH_HARVESTS      10      // беране от един узрял храст
#define FARM_STAGE_SECONDS      120     // всеки етап на растеж (нормално време)
#define FARM_INTERACT_RANGE     2.0
#define FARM_CANCEL_RANGE       1.5     // отдалечиш ли се толкова – беренето прекъсва

#define FARM_DEPOSIT            200
#define FARM_DOUBLE_CHANCE_2    10      // % шанс за +2 на скил 2
#define FARM_DOUBLE_CHANCE_3    20      // % шанс за +2 на скил 3
#define FARM_GOLD_CHANCE        1       // % шанс за златна шушулка
#define FARM_GOLD_VALUE         10      // златна = толкова обикновени
#define FARM_STREAK_RESET       300     // секунди без беране -> серията се нулира
#define FARM_STREAK_STEP        20      // на всеки толкова броя ...
#define FARM_STREAK_PERCENT     5       // ... +5%
#define FARM_STREAK_MAX         25
#define FARM_EXP_PER_UNITS      10      // +1 EXP на всеки 10 продадени

#define FARM_SKIN_MALE          158
#define FARM_SKIN_FEMALE        157
#define FARM_RAKE_SLOT          8
#define FARM_RAKE_MODEL         18890

// Модели по подразбиране (сменят се от играта: /farmedit)
#define FARM_DEF_MODEL_RIPE     19473
#define FARM_DEF_MODEL_STALK    19473
#define FARM_DEF_MODEL_SPROUT   874

// Етапи на храста
#define BUSH_SPROUT     1
#define BUSH_STALK      2
#define BUSH_RIPE       3

// Модели за пробния ред (/farmedit -> "Пробни модели тук")
static const FarmTestModels[] = {19473, 3409, 804, 805, 806, 870, 874, 2244};

// Време за беране по скил (мин, макс, в ms)
static const FarmHarvestMin[] = {6000, 5000, 4000};
static const FarmHarvestMax[] = {9000, 7000, 6000};

// ------------------------------------------------------------
//  Данни
// ------------------------------------------------------------

enum E_BUSH {
    bool:bsExists,
    bsDBID,
    Float:bsX,
    Float:bsY,
    Float:bsZ,
    Float:bsA,
    bsStage,
    bsCount,            // оставащи берения (когато е узрял)
    bsNextStage,        // unix време за следващия етап
    bsObject,
    bsHarvester,
    STREAMER_TAG_3D_TEXT_LABEL:bsLabel
}
static FarmBush[FARM_MAX_BUSHES][E_BUSH];

static gModelRipe, gModelStalk, gModelSprout;

static bool:FarmOnShift[MAX_PLAYERS],
    FarmCotton[MAX_PLAYERS],            // обикновен памук в джоба
    FarmGold[MAX_PLAYERS],              // златни шушулки
    FarmStreak[MAX_PLAYERS],
    FarmBatchBonus[MAX_PLAYERS],        // най-високият бонус от серия в тази партида
    FarmLastPick[MAX_PLAYERS],
    FarmExpCarry[MAX_PLAYERS],          // продадени бройки, които още не са дали EXP
    FarmBush_Picking[MAX_PLAYERS],      // храстът, който бере (-1)
    FarmPickStart[MAX_PLAYERS],
    FarmPickDuration[MAX_PLAYERS],
    FarmPickTimer[MAX_PLAYERS],
    Float:FarmPickX[MAX_PLAYERS],
    Float:FarmPickY[MAX_PLAYERS],
    Float:FarmPickZ[MAX_PLAYERS],
    FarmKeyTick[MAX_PLAYERS],
    FarmMenuAction[MAX_PLAYERS][8],
    FarmModelEdit[MAX_PLAYERS],
    PlayerText:FarmBarBg[MAX_PLAYERS],
    PlayerText:FarmBarFill[MAX_PLAYERS],
    PlayerText:FarmBarText[MAX_PLAYERS],
    PlayerText:FarmHud[MAX_PLAYERS];

// ============================================================
//  Инициализация
// ============================================================

stock Farm_Init()
{
    // Почистване от предишните версии
    Database_Exec("DROP TABLE IF EXISTS farm_plots");
    Database_Exec("DELETE FROM job_points WHERE job = 1 AND name <> 'job'");
    Database_Exec("CREATE TABLE IF NOT EXISTS farm_bushes (id INTEGER PRIMARY KEY AUTOINCREMENT, x REAL NOT NULL, y REAL NOT NULL, z REAL NOT NULL, a REAL NOT NULL DEFAULT 0)");

    gModelRipe = Settings_GetInt("farm_model_ripe", FARM_DEF_MODEL_RIPE);
    gModelStalk = Settings_GetInt("farm_model_stalk", FARM_DEF_MODEL_STALK);
    gModelSprout = Settings_GetInt("farm_model_sprout", FARM_DEF_MODEL_SPROUT);

    for (new i = 0; i < FARM_MAX_BUSHES; i++) {
        FarmBush[i][bsExists] = false;
        FarmBush[i][bsHarvester] = INVALID_PLAYER_ID;
    }

    new DBResult:result = Database_Query("SELECT * FROM farm_bushes ORDER BY id");
    new count = 0;
    if (result != DBResult:0) {
        if (DB_GetRowCount(result) > 0) {
            do {
                if (Farm_AddBushToMemory(DB_GetFieldIntByName(result, "id"),
                    DB_GetFieldFloatByName(result, "x"), DB_GetFieldFloatByName(result, "y"),
                    DB_GetFieldFloatByName(result, "z"), DB_GetFieldFloatByName(result, "a")) != -1) count++;
            } while (DB_SelectNextRow(result));
        }
        DB_FreeResultSet(result);
    }
    printf("[FARM] %d cotton bush(es) loaded.", count);
}

// Махаме картата "ferma" от предишната версия (след като редакторът на карти е зареден).
stock Farm_SeedMap()
{
    if (Map_GetID("ferma") == 0) return;
    Map_ClearMap("ferma");
    Database_Exec("DELETE FROM maps WHERE name = 'ferma'");
}

stock Farm_ResetPlayer(playerid)
{
    FarmOnShift[playerid] = false;
    FarmCotton[playerid] = 0;
    FarmGold[playerid] = 0;
    FarmStreak[playerid] = 0;
    FarmBatchBonus[playerid] = 0;
    FarmLastPick[playerid] = 0;
    FarmExpCarry[playerid] = 0;
    FarmBush_Picking[playerid] = -1;
    FarmKeyTick[playerid] = 0;
    FarmBarBg[playerid] = INVALID_PLAYER_TEXT_DRAW;
    FarmBarFill[playerid] = INVALID_PLAYER_TEXT_DRAW;
    FarmBarText[playerid] = INVALID_PLAYER_TEXT_DRAW;
    FarmHud[playerid] = INVALID_PLAYER_TEXT_DRAW;
}

// ============================================================
//  Храсти
// ============================================================

stock Farm_AddBushToMemory(dbid, Float:x, Float:y, Float:z, Float:a)
{
    for (new i = 0; i < FARM_MAX_BUSHES; i++) {
        if (FarmBush[i][bsExists]) continue;
        FarmBush[i][bsExists] = true;
        FarmBush[i][bsDBID] = dbid;
        FarmBush[i][bsX] = x;
        FarmBush[i][bsY] = y;
        FarmBush[i][bsZ] = z;
        FarmBush[i][bsA] = a;
        FarmBush[i][bsObject] = 0;
        FarmBush[i][bsHarvester] = INVALID_PLAYER_ID;
        FarmBush[i][bsLabel] = CreateDynamic3DTextLabel(" ", COLOR_WHITE, x, y, z + 0.4, 12.0);
        Farm_SetStage(i, BUSH_RIPE);
        return i;
    }
    return -1;
}

stock Farm_RemoveBush(i)
{
    if (!FarmBush[i][bsExists]) return;
    if (FarmBush[i][bsObject] != 0) DestroyDynamicObject(FarmBush[i][bsObject]);
    DestroyDynamic3DTextLabel(FarmBush[i][bsLabel]);
    FarmBush[i][bsExists] = false;
}

stock Farm_StageModel(stage)
{
    switch (stage) {
        case BUSH_SPROUT: return gModelSprout;
        case BUSH_STALK:  return gModelStalk;
    }
    return gModelRipe;
}

stock Farm_StageSeconds()
{
    switch (Weather_GetKind()) {
        case WEATHER_KIND_RAIN: return FARM_STAGE_SECONDS * 70 / 100;
        case WEATHER_KIND_HOT:  return FARM_STAGE_SECONDS * 130 / 100;
    }
    return FARM_STAGE_SECONDS;
}

stock Farm_SetStage(i, stage)
{
    FarmBush[i][bsStage] = stage;
    if (stage == BUSH_RIPE) FarmBush[i][bsCount] = FARM_BUSH_HARVESTS;
    else FarmBush[i][bsNextStage] = gettime() + Farm_StageSeconds();

    if (FarmBush[i][bsObject] != 0) DestroyDynamicObject(FarmBush[i][bsObject]);
    // Обектът стои на земята: позицията е на играча (около 1 м над земята)
    FarmBush[i][bsObject] = CreateDynamicObject(Farm_StageModel(stage),
        FarmBush[i][bsX], FarmBush[i][bsY], FarmBush[i][bsZ] - 1.0, 0.0, 0.0, FarmBush[i][bsA], 0, 0, -1, 100.0);
    Farm_UpdateBushLabel(i);
}

stock Farm_UpdateBushLabel(i)
{
    new text[96];
    if (FarmBush[i][bsStage] == BUSH_RIPE) {
        format(text, sizeof (text), "{FFFFFF}Памук {FF6A00}(%d от %d)\n{A0A0A0}ALT - бери", FarmBush[i][bsCount], FARM_BUSH_HARVESTS);
    } else {
        // Оставащо време до узряване (текущият етап + следващите)
        new left = FarmBush[i][bsNextStage] - gettime();
        if (FarmBush[i][bsStage] == BUSH_SPROUT) left += Farm_StageSeconds();
        if (left < 0) left = 0;
        format(text, sizeof (text), "{A0A0A0}Расте: %d:%02d", left / 60, left % 60);
    }
    UpdateDynamic3DTextLabelText(FarmBush[i][bsLabel], COLOR_WHITE, text);
}

stock Farm_GetNearestRipeBush(playerid, Float:range)
{
    new best = -1, Float:bestDist = range;
    for (new i = 0; i < FARM_MAX_BUSHES; i++) {
        if (!FarmBush[i][bsExists] || FarmBush[i][bsStage] != BUSH_RIPE) continue;
        new Float:d = GetPlayerDistanceFromPoint(playerid, FarmBush[i][bsX], FarmBush[i][bsY], FarmBush[i][bsZ]);
        if (d < bestDist) {
            bestDist = d;
            best = i;
        }
    }
    return best;
}

stock Farm_GetNearestBush(playerid, Float:range)
{
    new best = -1, Float:bestDist = range;
    for (new i = 0; i < FARM_MAX_BUSHES; i++) {
        if (!FarmBush[i][bsExists]) continue;
        new Float:d = GetPlayerDistanceFromPoint(playerid, FarmBush[i][bsX], FarmBush[i][bsY], FarmBush[i][bsZ]);
        if (d < bestDist) {
            bestDist = d;
            best = i;
        }
    }
    return best;
}

// ============================================================
//  Интерфейс: лента за прогрес и HUD
// ============================================================

#define FARM_BAR_X      245.0
#define FARM_BAR_Y      360.0
#define FARM_BAR_W      150.0

stock Farm_ShowBar(playerid)
{
    Farm_HideBar(playerid);

    new PlayerText:bg = CreatePlayerTextDraw(playerid, FARM_BAR_X, FARM_BAR_Y, "_");
    PlayerTextDrawUseBox(playerid, bg, true);
    PlayerTextDrawBoxColour(playerid, bg, 0x000000AA);
    PlayerTextDrawLetterSize(playerid, bg, 0.0, 1.2);
    PlayerTextDrawTextSize(playerid, bg, FARM_BAR_X + FARM_BAR_W, 0.0);
    FarmBarBg[playerid] = bg;

    new PlayerText:fill = CreatePlayerTextDraw(playerid, FARM_BAR_X, FARM_BAR_Y, "_");
    PlayerTextDrawUseBox(playerid, fill, true);
    PlayerTextDrawBoxColour(playerid, fill, COLOR_BRAND);
    PlayerTextDrawLetterSize(playerid, fill, 0.0, 1.2);
    PlayerTextDrawTextSize(playerid, fill, FARM_BAR_X + 1.0, 0.0);
    FarmBarFill[playerid] = fill;

    new PlayerText:t = CreatePlayerTextDraw(playerid, FARM_BAR_X + FARM_BAR_W / 2.0, FARM_BAR_Y - 14.0, "PICKING COTTON...");
    PlayerTextDrawFont(playerid, t, TEXT_DRAW_FONT_2);
    PlayerTextDrawLetterSize(playerid, t, 0.22, 1.0);
    PlayerTextDrawAlignment(playerid, t, TEXT_DRAW_ALIGN_CENTRE);
    PlayerTextDrawColour(playerid, t, COLOR_WHITE);
    PlayerTextDrawSetOutline(playerid, t, 1);
    PlayerTextDrawSetShadow(playerid, t, 0);
    PlayerTextDrawBackgroundColour(playerid, t, 0x000000FF);
    FarmBarText[playerid] = t;

    PlayerTextDrawShow(playerid, bg);
    PlayerTextDrawShow(playerid, fill);
    PlayerTextDrawShow(playerid, t);
}

stock Farm_UpdateBar(playerid, percent)
{
    if (FarmBarFill[playerid] == INVALID_PLAYER_TEXT_DRAW) return;
    if (percent < 1) percent = 1;
    if (percent > 100) percent = 100;
    PlayerTextDrawTextSize(playerid, FarmBarFill[playerid], FARM_BAR_X + FARM_BAR_W * float(percent) / 100.0, 0.0);
    PlayerTextDrawShow(playerid, FarmBarFill[playerid]); // повторното показване прилага новия размер
}

stock Farm_HideBar(playerid)
{
    if (FarmBarBg[playerid] != INVALID_PLAYER_TEXT_DRAW) PlayerTextDrawDestroy(playerid, FarmBarBg[playerid]);
    if (FarmBarFill[playerid] != INVALID_PLAYER_TEXT_DRAW) PlayerTextDrawDestroy(playerid, FarmBarFill[playerid]);
    if (FarmBarText[playerid] != INVALID_PLAYER_TEXT_DRAW) PlayerTextDrawDestroy(playerid, FarmBarText[playerid]);
    FarmBarBg[playerid] = INVALID_PLAYER_TEXT_DRAW;
    FarmBarFill[playerid] = INVALID_PLAYER_TEXT_DRAW;
    FarmBarText[playerid] = INVALID_PLAYER_TEXT_DRAW;
}

stock Farm_StreakBonus(playerid)
{
    new bonus = (FarmStreak[playerid] / FARM_STREAK_STEP) * FARM_STREAK_PERCENT;
    return (bonus > FARM_STREAK_MAX) ? FARM_STREAK_MAX : bonus;
}

stock Farm_ShowHud(playerid)
{
    if (FarmHud[playerid] == INVALID_PLAYER_TEXT_DRAW) {
        new PlayerText:h = CreatePlayerTextDraw(playerid, 608.0, 150.0, "_");
        PlayerTextDrawFont(playerid, h, TEXT_DRAW_FONT_2);
        PlayerTextDrawLetterSize(playerid, h, 0.22, 1.05);
        PlayerTextDrawAlignment(playerid, h, TEXT_DRAW_ALIGN_RIGHT);
        PlayerTextDrawColour(playerid, h, COLOR_WHITE);
        PlayerTextDrawSetOutline(playerid, h, 1);
        PlayerTextDrawSetShadow(playerid, h, 0);
        PlayerTextDrawBackgroundColour(playerid, h, 0x000000FF);
        FarmHud[playerid] = h;
    }
    if (FarmGold[playerid] > 0) {
        PlayerTextDrawSetString(playerid, FarmHud[playerid], "~y~FARM~n~~w~COTTON: ~y~%d ~w~+ ~y~%d GOLD~n~~w~SKILL: ~y~%d~n~~w~STREAK: ~y~+%d%%",
            FarmCotton[playerid], FarmGold[playerid], Job_GetSkill(playerid, JOB_FARM), Farm_StreakBonus(playerid));
    } else {
        PlayerTextDrawSetString(playerid, FarmHud[playerid], "~y~FARM~n~~w~COTTON: ~y~%d~n~~w~SKILL: ~y~%d~n~~w~STREAK: ~y~+%d%%",
            FarmCotton[playerid], Job_GetSkill(playerid, JOB_FARM), Farm_StreakBonus(playerid));
    }
    PlayerTextDrawShow(playerid, FarmHud[playerid]);
}

stock Farm_HideHud(playerid)
{
    if (FarmHud[playerid] != INVALID_PLAYER_TEXT_DRAW) {
        PlayerTextDrawDestroy(playerid, FarmHud[playerid]);
        FarmHud[playerid] = INVALID_PLAYER_TEXT_DRAW;
    }
}

// ============================================================
//  Смяна
// ============================================================

stock Farm_AttachRake(playerid)
{
    SetPlayerAttachedObject(playerid, FARM_RAKE_SLOT, FARM_RAKE_MODEL, 6, 0.07, 0.03, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0);
}

stock Farm_ApplyOutfit(playerid)
{
    SetPlayerSkin(playerid, (PlayerData[playerid][pGender] == GENDER_FEMALE) ? FARM_SKIN_FEMALE : FARM_SKIN_MALE);
    Farm_AttachRake(playerid);
}

stock Farm_StartShift(playerid)
{
    if (FarmOnShift[playerid]) return;
    if (Money_Get(playerid) < FARM_DEPOSIT) {
        SendClientMessage(playerid, COLOR_GREY, "Иван: \"Греблото е срещу $%d депозит. Нямаш толкова в себе си.\"", FARM_DEPOSIT);
        return;
    }
    Money_Give(playerid, -FARM_DEPOSIT);
    FarmOnShift[playerid] = true;
    FarmStreak[playerid] = 0;
    FarmBatchBonus[playerid] = 0;
    FarmLastPick[playerid] = gettime();
    Farm_ApplyOutfit(playerid);
    Farm_ShowHud(playerid);

    SendClientMessage(playerid, COLOR_GREEN, "[Ферма] {FFFFFF}Смяната започна. Остави {FFD54A}$%d{FFFFFF} депозит за греблото – връща се в края.", FARM_DEPOSIT);
    SendClientMessage(playerid, COLOR_WHITE, "Отиди до узрял храст памук и натисни {FFD54A}ALT{FFFFFF}. Продавай при {FFD54A}Петър{FFFFFF} – изкупвача.");
}

stock Farm_EndShift(playerid, bool:silent = false)
{
    if (!FarmOnShift[playerid]) return;
    Farm_CancelPick(playerid, false);
    FarmOnShift[playerid] = false;

    Money_Give(playerid, FARM_DEPOSIT);
    if (IsPlayerConnected(playerid)) {
        RemovePlayerAttachedObject(playerid, FARM_RAKE_SLOT);
        SetPlayerSkin(playerid, PlayerData[playerid][pSkin]);
        Farm_HideHud(playerid);
        if (!silent) {
            SendClientMessage(playerid, COLOR_GREEN, "[Ферма] {FFFFFF}Смяната приключи. Върнат депозит: {33CC66}$%d{FFFFFF}.", FARM_DEPOSIT);
            if (FarmCotton[playerid] + FarmGold[playerid] > 0) {
                SendClientMessage(playerid, COLOR_GREY, "Още носиш непродаден памук – можеш да го продадеш при Петър и след смяната.");
            }
        }
    }
}

// ============================================================
//  Беране
// ============================================================

stock Farm_TryPick(playerid)
{
    if (FarmBush_Picking[playerid] != -1) return;

    new bush = Farm_GetNearestRipeBush(playerid, FARM_INTERACT_RANGE);
    if (bush == -1) return;
    if (!FarmOnShift[playerid]) {
        SendClientMessage(playerid, COLOR_GREY, "Нямаш гребло. Започни смяна при Иван.");
        return;
    }
    if (FarmBush[bush][bsHarvester] != INVALID_PLAYER_ID) {
        SendClientMessage(playerid, COLOR_GREY, "Някой вече бере този храст.");
        return;
    }

    new skill = Job_GetSkill(playerid, JOB_FARM) - 1;
    new duration = FarmHarvestMin[skill] + random(FarmHarvestMax[skill] - FarmHarvestMin[skill] + 1);

    FarmBush[bush][bsHarvester] = playerid;
    FarmBush_Picking[playerid] = bush;
    FarmPickStart[playerid] = GetTickCount();
    FarmPickDuration[playerid] = duration;
    GetPlayerPos(playerid, FarmPickX[playerid], FarmPickY[playerid], FarmPickZ[playerid]);

    ApplyAnimation(playerid, "BOMBER", "BOM_Plant_Loop", 4.1, true, false, false, false, 0);
    Farm_ShowBar(playerid);
    FarmPickTimer[playerid] = SetTimerEx("Farm_PickTick", 200, true, "i", playerid);
}

forward Farm_PickTick(playerid);
public Farm_PickTick(playerid)
{
    new bush = FarmBush_Picking[playerid];
    if (!IsPlayerConnected(playerid) || bush == -1) return 0;

    // Отдалечил се / влязъл в кола -> прекъсва
    if (IsPlayerInAnyVehicle(playerid) ||
        GetPlayerDistanceFromPoint(playerid, FarmPickX[playerid], FarmPickY[playerid], FarmPickZ[playerid]) > FARM_CANCEL_RANGE) {
        Farm_CancelPick(playerid, true);
        return 0;
    }

    new elapsed = GetTickCount() - FarmPickStart[playerid];
    if (elapsed < FarmPickDuration[playerid]) {
        Farm_UpdateBar(playerid, elapsed * 100 / FarmPickDuration[playerid]);
        return 1;
    }
    Farm_FinishPick(playerid);
    return 0;
}

stock Farm_CancelPick(playerid, bool:notify)
{
    new bush = FarmBush_Picking[playerid];
    if (bush == -1) return;
    KillTimer(FarmPickTimer[playerid]);
    FarmBush_Picking[playerid] = -1;
    if (FarmBush[bush][bsHarvester] == playerid) FarmBush[bush][bsHarvester] = INVALID_PLAYER_ID;
    if (IsPlayerConnected(playerid)) {
        Farm_HideBar(playerid);
        ClearAnimations(playerid);
        if (notify) GameTextForPlayer(playerid, "~r~Interrupted", 1500, 3);
    }
}

stock Farm_FinishPick(playerid)
{
    new bush = FarmBush_Picking[playerid];
    KillTimer(FarmPickTimer[playerid]);
    FarmBush_Picking[playerid] = -1;
    Farm_HideBar(playerid);
    ClearAnimations(playerid);

    if (bush == -1 || FarmBush[bush][bsHarvester] != playerid || FarmBush[bush][bsStage] != BUSH_RIPE) return;
    FarmBush[bush][bsHarvester] = INVALID_PLAYER_ID;

    // Серия
    new now = gettime();
    if (now - FarmLastPick[playerid] > FARM_STREAK_RESET) FarmStreak[playerid] = 0;
    FarmLastPick[playerid] = now;

    // Добив
    new skill = Job_GetSkill(playerid, JOB_FARM), amount = 1;
    if ((skill == 2 && random(100) < FARM_DOUBLE_CHANCE_2) || (skill >= 3 && random(100) < FARM_DOUBLE_CHANCE_3)) amount = 2;

    if (random(100) < FARM_GOLD_CHANCE) {
        FarmGold[playerid]++;
        GameTextForPlayer(playerid, "~y~GOLDEN BOLL!", 3000, 3);
        new rpName[MAX_PLAYER_NAME + 1], msg[144];
        GetRPName(playerid, rpName);
        format(msg, sizeof (msg), "[Ферма] %s намери златна памучна шушулка!", rpName);
        ProxMessage(playerid, 60.0, COLOR_GOLD, msg, false);
    } else {
        FarmCotton[playerid] += amount;
        if (amount == 2) GameTextForPlayer(playerid, "~g~+2 ~w~cotton", 1500, 3);
        else GameTextForPlayer(playerid, "~g~+1 ~w~cotton", 1500, 3);
    }
    FarmStreak[playerid] += amount;
    new bonus = Farm_StreakBonus(playerid);
    if (bonus > FarmBatchBonus[playerid]) FarmBatchBonus[playerid] = bonus;

    // Храстът
    FarmBush[bush][bsCount]--;
    if (FarmBush[bush][bsCount] <= 0) Farm_SetStage(bush, BUSH_SPROUT);
    else Farm_UpdateBushLabel(bush);

    Farm_ShowHud(playerid);
}

// ALT (или Y). Връща true, ако бутонът е използван от фермата.
stock bool:Farm_OnKeyStateChange(playerid, KEY:newkeys, KEY:oldkeys)
{
    if (IsPlayerInAnyVehicle(playerid)) return false;
    new bool:alt = ((newkeys & KEY_WALK) && !(oldkeys & KEY_WALK)),
        bool:yes = ((newkeys & KEY_YES) && !(oldkeys & KEY_YES));
    if (!alt && !yes) return false;
    if (Farm_GetNearestRipeBush(playerid, FARM_INTERACT_RANGE) == -1) return false;

    new now = GetTickCount();
    if (now - FarmKeyTick[playerid] < 500) return true;
    FarmKeyTick[playerid] = now;
    Farm_TryPick(playerid);
    return true;
}

// ============================================================
//  Изкупвач
// ============================================================

stock Farm_SaleValue(playerid)
{
    new units = FarmCotton[playerid] + FarmGold[playerid] * FARM_GOLD_VALUE;
    new value = units * Market_GetPrice(GOODS_COTTON) * Job_GetPayPercent(playerid, JOB_FARM) / 100;
    value += value * FarmBatchBonus[playerid] / 100;
    return value;
}

stock Farm_OnBuyerInteract(playerid)
{
    if (FarmCotton[playerid] + FarmGold[playerid] == 0) {
        SendClientMessage(playerid, COLOR_GREY, "Петър: \"Нямаш памук. Ела, като набереш.\" {A0A0A0}(цена сега: $%d за брой)", Market_GetPrice(GOODS_COTTON));
        return;
    }
    new body[768], line[128];
    format(body, sizeof (body), C_GREY "Петър: " C_WHITE "\"Да видим какво носиш...\"\n\n");
    format(line, sizeof (line), C_WHITE "Памук: " C_GOLD "%d бр." C_WHITE " x " C_GREEN "$%d\n", FarmCotton[playerid], Market_GetPrice(GOODS_COTTON));
    strcat(body, line);
    if (FarmGold[playerid] > 0) {
        format(line, sizeof (line), C_WHITE "Златни шушулки: " C_GOLD "%d" C_WHITE " (всяка = %d бр.)\n", FarmGold[playerid], FARM_GOLD_VALUE);
        strcat(body, line);
    }
    format(line, sizeof (line), C_WHITE "Бонус скил: " C_GREEN "%d%%" C_WHITE "   Бонус серия: " C_GREEN "+%d%%\n\n",
        Job_GetPayPercent(playerid, JOB_FARM), FarmBatchBonus[playerid]);
    strcat(body, line);
    format(line, sizeof (line), C_WHITE "Общо: " C_GREEN "$%d", Farm_SaleValue(playerid));
    strcat(body, line);
    ShowPlayerDialog(playerid, DIALOG_FARM_SELL, DIALOG_STYLE_MSGBOX, C_BRAND "Петър - Изкупвач", "%s", "Продай", "Отказ", body);
}

stock Farm_Sell(playerid)
{
    new units = FarmCotton[playerid] + FarmGold[playerid] * FARM_GOLD_VALUE;
    if (units <= 0) return;
    new value = Farm_SaleValue(playerid), gold = FarmGold[playerid];

    Market_Add(GOODS_COTTON, FarmCotton[playerid] + FarmGold[playerid]);
    Money_Give(playerid, value);
    SendClientMessage(playerid, COLOR_GREEN, "[Ферма] {FFFFFF}Продаде памука за {33CC66}$%d{FFFFFF}.", value);

    // EXP: +1 на всеки 10 продадени (остатъкът се пази), +1 за всяка златна
    FarmExpCarry[playerid] += FarmCotton[playerid];
    new exp = FarmExpCarry[playerid] / FARM_EXP_PER_UNITS + gold;
    FarmExpCarry[playerid] %= FARM_EXP_PER_UNITS;

    FarmCotton[playerid] = 0;
    FarmGold[playerid] = 0;
    FarmBatchBonus[playerid] = Farm_StreakBonus(playerid);
    if (exp > 0) Job_AddExp(playerid, JOB_FARM, exp);
    if (FarmOnShift[playerid]) Farm_ShowHud(playerid);
    Auth_SaveAccount(playerid);

    new details[48];
    format(details, sizeof (details), "%d бр.: $%d", units, value);
    Log_Add("farm_sell", PlayerData[playerid][pName], "", details);
}

// ============================================================
//  Таймер и събития
// ============================================================

// Всяка секунда
stock Farm_OnTick()
{
    new now = gettime(), bool:updateLabels = (now % 5 == 0);
    for (new i = 0; i < FARM_MAX_BUSHES; i++) {
        if (!FarmBush[i][bsExists] || FarmBush[i][bsStage] == BUSH_RIPE) continue;
        if (now >= FarmBush[i][bsNextStage]) {
            Farm_SetStage(i, FarmBush[i][bsStage] + 1);
        } else if (updateLabels) {
            Farm_UpdateBushLabel(i);
        }
    }
}

stock Farm_OnPlayerDisconnect(playerid)
{
    Farm_CancelPick(playerid, false);
    Farm_EndShift(playerid, true);   // връща депозита преди записа на акаунта
    Farm_ResetPlayer(playerid);
}

stock Farm_OnPlayerDeath(playerid)
{
    Farm_CancelPick(playerid, false);
}

stock Farm_OnPlayerSpawn(playerid)
{
    if (FarmOnShift[playerid]) {
        Farm_ApplyOutfit(playerid);
        Farm_ShowHud(playerid);
    }
}

// ============================================================
//  Иван (NPC)
// ============================================================

stock Farm_OnNpcInteract(playerid)
{
    new body[512], count = 0;
    body[0] = EOS;
    if (!FarmOnShift[playerid]) {
        format(body, sizeof (body), C_GREEN "» " C_WHITE "Започни смяна " C_GREY "(депозит $%d за гребло)\n", FARM_DEPOSIT);
        FarmMenuAction[playerid][count++] = 1;
    } else {
        strcat(body, C_RED "» " C_WHITE "Приключи смяната " C_GREY "(връща депозита)\n");
        FarmMenuAction[playerid][count++] = 2;
    }
    strcat(body, C_BLUE "» " C_WHITE "Моят напредък\n");
    FarmMenuAction[playerid][count++] = 3;
    strcat(body, C_BLUE "» " C_WHITE "Как се работи");
    FarmMenuAction[playerid][count] = 4;
    ShowPlayerDialog(playerid, DIALOG_FARM_NPC, DIALOG_STYLE_LIST, C_BRAND "Иван - Фермер", "%s", "Избери", "Затвори", body);
}

stock Farm_ShowProgress(playerid)
{
    new body[1024], line[192];
    new skill = Job_GetSkill(playerid, JOB_FARM), maxSkill = Job_GetMaxSkill(JOB_FARM);

    format(body, sizeof (body), C_WHITE "Скил: " C_GOLD "%d/%d" C_WHITE " (%d EXP)", skill, maxSkill, PlayerJobExp[playerid][JOB_FARM]);
    if (skill < maxSkill) {
        format(line, sizeof (line), C_GREY " - следващ при %d EXP", Job_GetSkillExp(JOB_FARM, skill + 1));
        strcat(body, line);
    }
    format(line, sizeof (line), "\n" C_WHITE "Беране: " C_GOLD "%d-%d сек." C_WHITE "   Шанс за +2: " C_GOLD "%d%%\n",
        FarmHarvestMin[skill - 1] / 1000, FarmHarvestMax[skill - 1] / 1000,
        (skill >= 3) ? FARM_DOUBLE_CHANCE_3 : ((skill == 2) ? FARM_DOUBLE_CHANCE_2 : 0));
    strcat(body, line);
    format(line, sizeof (line), C_WHITE "Цена сега: " C_GREEN "$%d" C_WHITE " за брой (бонус скил %d%%)\n",
        Market_GetPrice(GOODS_COTTON), Job_GetPayPercent(playerid, JOB_FARM));
    strcat(body, line);
    format(line, sizeof (line), C_WHITE "В теб: " C_GOLD "%d памук" C_WHITE ", " C_GOLD "%d златни\n", FarmCotton[playerid], FarmGold[playerid]);
    strcat(body, line);
    ShowPlayerDialog(playerid, DIALOG_FARM_INFO, DIALOG_STYLE_MSGBOX, C_BRAND "Ферма " C_WHITE "| Напредък", "%s", "Назад", "", body);
}

stock Farm_ShowHelp(playerid)
{
    new body[1500], line[160];
    body[0] = EOS;
    format(line, sizeof (line), C_WHITE "1. Започни смяна при " C_BRAND "Иван" C_WHITE " - $%d депозит за гребло (връща се).\n", FARM_DEPOSIT);
    strcat(body, line);
    strcat(body, C_WHITE "2. Отиди до узрял храст и натисни " C_GOLD "ALT" C_WHITE ". Не мърдай, докато лентата се напълни.\n");
    format(line, sizeof (line), C_WHITE "3. Всеки храст дава " C_GOLD "%d" C_WHITE " пъти, после пораства наново (~%d мин.).\n", FARM_BUSH_HARVESTS, FARM_STAGE_SECONDS * 3 / 60);
    strcat(body, line);
    strcat(body, C_WHITE "4. Продай памука при " C_BRAND "Петър" C_WHITE " - изкупвача.\n\n");
    format(line, sizeof (line), C_GOLD "Златна шушулка" C_WHITE " - %d%% шанс, струва колкото %d броя.\n", FARM_GOLD_CHANCE, FARM_GOLD_VALUE);
    strcat(body, line);
    strcat(body, C_WHITE "Серия - береш без дълги паузи и бонусът расте до " C_GREEN "+25%" C_WHITE ".\n");
    strcat(body, C_WHITE "Дъждът ускорява растежа, жегата го забавя.\n");
    strcat(body, C_WHITE "Скил 2 и 3 - по-бързо беране и шанс за двоен добив.\n\n");
    strcat(body, C_GREY "Внимание: непродаденият памук се губи при излизане от сървъра.");
    ShowPlayerDialog(playerid, DIALOG_FARM_INFO, DIALOG_STYLE_MSGBOX, C_BRAND "Ферма " C_WHITE "| Как се работи", "%s", "Назад", "", body);
}

// ============================================================
//  Диалози
// ============================================================

stock Farm_OnDialogResponse(playerid, dialogid, response, listitem, const inputtext[])
{
    switch (dialogid) {
        case DIALOG_FARM_NPC: {
            if (!response || listitem < 0 || listitem > 3) return 1;
            switch (FarmMenuAction[playerid][listitem]) {
                case 1: Farm_StartShift(playerid);
                case 2: Farm_EndShift(playerid);
                case 3: Farm_ShowProgress(playerid);
                case 4: Farm_ShowHelp(playerid);
            }
            return 1;
        }
        case DIALOG_FARM_INFO: {
            Farm_OnNpcInteract(playerid);
            return 1;
        }
        case DIALOG_FARM_SELL: {
            if (response) Farm_Sell(playerid);
            return 1;
        }
        case DIALOG_FARM_EDIT: {
            if (response) Farm_OnEditSelect(playerid, listitem);
            return 1;
        }
        case DIALOG_FARM_MODEL: {
            if (!response) {
                Farm_ShowEditMenu(playerid);
                return 1;
            }
            new model = IsNumeric(inputtext) ? strval(inputtext) : 0;
            if (model < 300 || model > 20000) {
                SendError(playerid, "Невалиден модел.");
                return 1;
            }
            Farm_SetModel(playerid, FarmModelEdit[playerid], model);
            Farm_ShowEditMenu(playerid);
            return 1;
        }
    }
    return 0;
}

// ============================================================
//  Админ – /farmedit
// ============================================================

CMD:farmedit(playerid, params[])
{
    #pragma unused params
    if (!Admin_Check(playerid, ADMIN_ADMIN)) return 1;
    Farm_ShowEditMenu(playerid);
    return 1;
}

stock Farm_ShowEditMenu(playerid)
{
    new body[1024], line[128], count = 0;
    for (new i = 0; i < FARM_MAX_BUSHES; i++) if (FarmBush[i][bsExists]) count++;

    body[0] = EOS;
    strcat(body, C_BRAND "» " C_WHITE "Иван (фермер) - тук\n");
    strcat(body, C_BRAND "» " C_WHITE "Петър (изкупвач) - тук\n");
    format(line, sizeof (line), C_GREEN "» " C_WHITE "Добави храст тук " C_GREY "(сега: %d/%d)\n", count, FARM_MAX_BUSHES);
    strcat(body, line);
    strcat(body, C_RED "» " C_WHITE "Изтрий най-близкия храст\n");
    strcat(body, C_BLUE "» " C_WHITE "Пробни модели тук " C_GREY "(ред от модели за избор, за 3 мин.)\n");
    format(line, sizeof (line), C_BLUE "» " C_WHITE "Модел: узрял храст " C_GREY "(%d)\n", gModelRipe);
    strcat(body, line);
    format(line, sizeof (line), C_BLUE "» " C_WHITE "Модел: стрък " C_GREY "(%d)\n", gModelStalk);
    strcat(body, line);
    format(line, sizeof (line), C_BLUE "» " C_WHITE "Модел: кълн " C_GREY "(%d)", gModelSprout);
    strcat(body, line);
    ShowPlayerDialog(playerid, DIALOG_FARM_EDIT, DIALOG_STYLE_LIST, C_RED "Ферма " C_WHITE "| Редактор", "%s", "Избери", "Затвори", body);
}

stock Farm_OnEditSelect(playerid, item)
{
    new Float:x, Float:y, Float:z, Float:a;
    GetPlayerPos(playerid, x, y, z);
    GetPlayerFacingAngle(playerid, a);

    switch (item) {
        case 0: {
            Job_SetPoint(JOB_FARM, "job", x, y, z, a);
            Npc_PlaceRole(NPC_ROLE_FARM, "Иван - Фермер", 158, x, y, z, a, 3);
            SendClientMessage(playerid, COLOR_ADMIN, "[Ферма] Иван е тук.");
        }
        case 1: {
            Npc_PlaceRole(NPC_ROLE_FARM_BUYER, "Петър - Изкупвач", 161, x, y, z, a, 1);
            SendClientMessage(playerid, COLOR_ADMIN, "[Ферма] Петър (изкупвач) е тук.");
        }
        case 2: {
            new q[192];
            format(q, sizeof (q), "INSERT INTO farm_bushes (x, y, z, a) VALUES (%f, %f, %f, %f)", x, y, z, a);
            Database_Exec(q);
            new dbid = 0;
            new DBResult:result = Database_Query("SELECT MAX(id) AS id FROM farm_bushes");
            if (result != DBResult:0) {
                dbid = DB_GetFieldIntByName(result, "id");
                DB_FreeResultSet(result);
            }
            if (Farm_AddBushToMemory(dbid, x, y, z, a) == -1) {
                SendError(playerid, "Достигнат е максималният брой храсти.");
            } else {
                // Отстъпваме, за да не стоим в храста
                SetPlayerPos(playerid, x - 1.2 * floatsin(a, degrees), y + 1.2 * floatcos(a, degrees), z);
                SendClientMessage(playerid, COLOR_ADMIN, "[Ферма] Храстът е добавен.");
            }
        }
        case 3: {
            new bush = Farm_GetNearestBush(playerid, 3.0);
            if (bush == -1) {
                SendError(playerid, "Няма храст в радиус 3 м.");
            } else {
                new q[96];
                format(q, sizeof (q), "DELETE FROM farm_bushes WHERE id = %d", FarmBush[bush][bsDBID]);
                Database_Exec(q);
                Farm_RemoveBush(bush);
                SendClientMessage(playerid, COLOR_ADMIN, "[Ферма] Храстът е изтрит.");
            }
        }
        case 4: Farm_ShowTestModels(playerid, x, y, z, a);
        case 5, 6, 7: {
            FarmModelEdit[playerid] = item - 5;
            ShowPlayerDialog(playerid, DIALOG_FARM_MODEL, DIALOG_STYLE_INPUT, C_RED "Ферма " C_WHITE "| Модел",
                C_WHITE "Въведи номер на модел (виж пробните модели - номерът е над всеки).", "Запази", "Назад");
            return;
        }
    }
    Log_Add("farmedit", PlayerData[playerid][pName], "", "");
    Farm_ShowEditMenu(playerid);
}

// which: 0 = узрял, 1 = стрък, 2 = кълн
stock Farm_SetModel(playerid, which, model)
{
    switch (which) {
        case 0: {
            gModelRipe = model;
            Settings_SetInt("farm_model_ripe", model);
        }
        case 1: {
            gModelStalk = model;
            Settings_SetInt("farm_model_stalk", model);
        }
        default: {
            gModelSprout = model;
            Settings_SetInt("farm_model_sprout", model);
        }
    }
    // Прилагаме веднага
    for (new i = 0; i < FARM_MAX_BUSHES; i++) {
        if (!FarmBush[i][bsExists]) continue;
        if (FarmBush[i][bsObject] != 0) DestroyDynamicObject(FarmBush[i][bsObject]);
        FarmBush[i][bsObject] = CreateDynamicObject(Farm_StageModel(FarmBush[i][bsStage]),
            FarmBush[i][bsX], FarmBush[i][bsY], FarmBush[i][bsZ] - 1.0, 0.0, 0.0, FarmBush[i][bsA], 0, 0, -1, 100.0);
    }
    SendClientMessage(playerid, COLOR_ADMIN, "[Ферма] Моделът е сменен на %d.", model);
}

// Ред от пробни модели пред админа, с номер над всеки. Изчезват след 3 минути.
static gFarmTestObj[sizeof (FarmTestModels)], STREAMER_TAG_3D_TEXT_LABEL:gFarmTestLabel[sizeof (FarmTestModels)], bool:gFarmTestActive;

stock Farm_ShowTestModels(playerid, Float:x, Float:y, Float:z, Float:a)
{
    Farm_ClearTestModels();
    // Редът е 3 м пред админа, перпендикулярно на погледа
    new Float:cx = x - 3.0 * floatsin(a, degrees), Float:cy = y + 3.0 * floatcos(a, degrees);
    new Float:rx = floatcos(a, degrees), Float:ry = floatsin(a, degrees);
    new n = sizeof (FarmTestModels), text[16];
    for (new i = 0; i < n; i++) {
        new Float:off = (float(i) - float(n - 1) / 2.0) * 1.6;
        new Float:px = cx + rx * off, Float:py = cy + ry * off;
        gFarmTestObj[i] = CreateDynamicObject(FarmTestModels[i], px, py, z - 1.0, 0.0, 0.0, a, -1, -1, -1, 100.0);
        format(text, sizeof (text), "{FFD54A}%d", FarmTestModels[i]);
        gFarmTestLabel[i] = CreateDynamic3DTextLabel(text, COLOR_WHITE, px, py, z + 0.8, 15.0);
    }
    gFarmTestActive = true;
    SetTimer("Farm_ClearTestModelsTimer", 180000, false);
    SendClientMessage(playerid, COLOR_ADMIN, "[Ферма] Пред теб има %d пробни модела с номерата им. Избери с /farmedit -> Модел.", n);
}

forward Farm_ClearTestModelsTimer();
public Farm_ClearTestModelsTimer()
{
    Farm_ClearTestModels();
    return 1;
}

stock Farm_ClearTestModels()
{
    if (!gFarmTestActive) return;
    for (new i = 0; i < sizeof (FarmTestModels); i++) {
        DestroyDynamicObject(gFarmTestObj[i]);
        DestroyDynamic3DTextLabel(gFarmTestLabel[i]);
    }
    gFarmTestActive = false;
}

CMD:farm(playerid, params[])
{
    #pragma unused params
    Farm_ShowHelp(playerid);
    return 1;
}
