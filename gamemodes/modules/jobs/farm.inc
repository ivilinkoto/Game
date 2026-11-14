// Работа: ФЕРМЕР – памучно поле (начална – без ниво, без кметство)
//
//  1. Говориш с Иван (ALT) -> започваш смяна (работни дрехи).
//  2. Отиваш до храст памук и натискаш ALT (или Y) – героят бере няколко секунди.
//  3. Снопът отива в чувала на гърба (до 5 снопа).
//  4. С чувала отиваш до склада и натискаш ALT – плаща се на сноп.
//  5. Обраните храсти поникват наново след няколко минути (по-бързо в дъжд, по-бавно в жега).
//
//  Нашето различно:
//   - "Златен сноп": рядък храст, плаща се x5.
//   - Серия: без дълги паузи бонусът расте (+5% на 10 снопа, до +25%).
//   - Скил 2: трактор – засява наново обраните места, докато минаваш през тях (+$ за всяко).
//   - Скил 3: комбайн – жъне храстите, през които минаваш; разтоварваш в склада.
//
//  Админ: /farmedit – застани на мястото и избери: Иван, ъгъл 1 и 2 на полето, склад, паркинг.
//  Храстите се подреждат сами в редове между двата ъгъла.

// ------------------------------------------------------------
//  Настройки
// ------------------------------------------------------------

#define FARM_MAX_PLANTS         150
#define FARM_ROW_SPACING        3.0     // между редовете (м)
#define FARM_PLANT_SPACING      2.5     // между храстите в реда (м)
#define FARM_PLANT_MODEL        19473   // храст (смени, ако има по-подходящ модел)
#define FARM_PLANT_Z_OFFSET     -1.0    // от височината на играча до земята

#define FARM_SACK_SIZE          5       // снопа в чувала
#define FARM_HARVEST_MS         5000    // колко време се бере (скил 1)
#define FARM_HARVEST_MS_FAST    3500    // скил 2+
#define FARM_BUNDLE_PRICE       30      // $ за сноп на скил 1
#define FARM_GOLD_MULTIPLIER    5
#define FARM_GOLD_CHANCE        3       // % шанс храстът да е златен
#define FARM_RESPAWN_SECONDS    180     // нормално време
#define FARM_RESPAWN_RAIN       110
#define FARM_RESPAWN_HOT        260
#define FARM_STREAK_RESET       180     // секунди без беране -> серията се нулира
#define FARM_TRACTOR_PAY        6       // $ за засадено място с трактора
#define FARM_COMBINE_CAPACITY   30      // снопа в бункера на комбайна

#define FARM_SKIN_MALE          158
#define FARM_SKIN_FEMALE        157
#define FARM_TRACTOR_MODEL      531
#define FARM_COMBINE_MODEL      532
#define FARM_SACK_SLOT          9
#define FARM_SACK_MODEL         2060

// ------------------------------------------------------------
//  Данни
// ------------------------------------------------------------

enum E_PLANT {
    bool:ptExists,
    Float:ptX,
    Float:ptY,
    Float:ptZ,
    bool:ptAlive,
    bool:ptGold,
    ptRespawnAt,        // unix време
    ptObject,
    ptHarvester,        // кой го бере в момента
    STREAMER_TAG_3D_TEXT_LABEL:ptLabel,
    bool:ptHasLabel
}
static FarmPlant[FARM_MAX_PLANTS][E_PLANT];
static gFarmPlantCount;

static bool:gFarmFieldSet, bool:gFarmStoreSet, bool:gFarmParkSet;
static Float:gFieldAX, Float:gFieldAY, Float:gFieldAZ, Float:gFieldBX, Float:gFieldBY, Float:gFieldBZ;
static Float:gStoreX, Float:gStoreY, Float:gStoreZ;
static Float:gParkX, Float:gParkY, Float:gParkZ, Float:gParkA;
static STREAMER_TAG_3D_TEXT_LABEL:gFarmStoreLabel, STREAMER_TAG_3D_TEXT_LABEL:gFarmFieldLabel, STREAMER_TAG_3D_TEXT_LABEL:gFarmParkLabel;
static bool:gFarmLabelsCreated;

static bool:FarmOnShift[MAX_PLAYERS],
    FarmSack[MAX_PLAYERS],              // обикновени снопове в чувала
    FarmSackGold[MAX_PLAYERS],          // златни снопове в чувала
    FarmHarvestPlant[MAX_PLAYERS],      // кой храст бере (-1 ако не бере)
    FarmStreak[MAX_PLAYERS],
    FarmLastHarvest[MAX_PLAYERS],
    FarmVehicle[MAX_PLAYERS],
    FarmBunker[MAX_PLAYERS],            // снопове в комбайна
    FarmBunkerGold[MAX_PLAYERS],
    FarmKeyTick[MAX_PLAYERS],
    FarmMenuAction[MAX_PLAYERS][8];

// ============================================================
//  Инициализация
// ============================================================

stock Farm_Init()
{
    // Почистване от старите версии на фермата
    Database_Exec("DROP TABLE IF EXISTS farm_plots");
    Database_Exec("DELETE FROM job_points WHERE job = 1 AND name LIKE 'spot%'");
    Database_Exec("DELETE FROM job_points WHERE job = 1 AND name IN ('buyer', 'well', 'vehicle')");

    for (new i = 0; i < FARM_MAX_PLANTS; i++) {
        FarmPlant[i][ptExists] = false;
        FarmPlant[i][ptHarvester] = INVALID_PLAYER_ID;
    }

    new Float:a;
    gFarmFieldSet = Job_GetPoint(JOB_FARM, "fieldA", gFieldAX, gFieldAY, gFieldAZ, a)
        && Job_GetPoint(JOB_FARM, "fieldB", gFieldBX, gFieldBY, gFieldBZ, a);
    gFarmStoreSet = Job_GetPoint(JOB_FARM, "store", gStoreX, gStoreY, gStoreZ, a);
    gFarmParkSet = Job_GetPoint(JOB_FARM, "park", gParkX, gParkY, gParkZ, gParkA);

    Farm_BuildField();
    Farm_CreateLabels();
}

// Махаме картата "ferma" от старата версия (след като редакторът на карти е зареден).
stock Farm_SeedMap()
{
    if (Map_GetID("ferma") == 0) return;
    Map_ClearMap("ferma");
    Database_Exec("DELETE FROM maps WHERE name = 'ferma'");
    print("[FARM] Old farm map removed.");
}

stock Farm_CreateLabels()
{
    if (gFarmLabelsCreated) {
        DestroyDynamic3DTextLabel(gFarmStoreLabel);
        DestroyDynamic3DTextLabel(gFarmFieldLabel);
        DestroyDynamic3DTextLabel(gFarmParkLabel);
    }
    gFarmStoreLabel = CreateDynamic3DTextLabel("{FF6A00}Склад за памук\n{FFFFFF}Разтовари чувала: ALT", COLOR_WHITE,
        gStoreX, gStoreY, gFarmStoreSet ? (gStoreZ + 0.6) : -100.0, 15.0);
    gFarmFieldLabel = CreateDynamic3DTextLabel("{FF6A00}Памучно поле\n{FFFFFF}Бери: ALT до храст", COLOR_WHITE,
        (gFieldAX + gFieldBX) / 2.0, (gFieldAY + gFieldBY) / 2.0, gFarmFieldSet ? ((gFieldAZ + gFieldBZ) / 2.0 + 2.0) : -100.0, 40.0);
    gFarmParkLabel = CreateDynamic3DTextLabel("{FF6A00}Техника на фермата", COLOR_WHITE,
        gParkX, gParkY, gFarmParkSet ? (gParkZ + 0.6) : -100.0, 15.0);
    gFarmLabelsCreated = true;
}

// Подрежда храстите в редове между двата ъгъла на полето.
stock Farm_BuildField()
{
    // Старите храсти
    for (new i = 0; i < FARM_MAX_PLANTS; i++) {
        if (!FarmPlant[i][ptExists]) continue;
        Farm_HidePlant(i);
        FarmPlant[i][ptExists] = false;
    }
    gFarmPlantCount = 0;
    if (!gFarmFieldSet) return;

    new Float:minX = (gFieldAX < gFieldBX) ? gFieldAX : gFieldBX, Float:maxX = (gFieldAX < gFieldBX) ? gFieldBX : gFieldAX;
    new Float:minY = (gFieldAY < gFieldBY) ? gFieldAY : gFieldBY, Float:maxY = (gFieldAY < gFieldBY) ? gFieldBY : gFieldAY;
    new Float:dx = gFieldBX - gFieldAX, Float:dy = gFieldBY - gFieldAY, Float:len2 = dx * dx + dy * dy;

    for (new Float:x = minX + 1.0; x <= maxX - 1.0; x += FARM_ROW_SPACING) {
        for (new Float:y = minY + 1.0; y <= maxY - 1.0; y += FARM_PLANT_SPACING) {
            if (gFarmPlantCount >= FARM_MAX_PLANTS) break;

            // Височина: по правата между двата ъгъла
            new Float:t = 0.5;
            if (len2 > 1.0) {
                t = ((x - gFieldAX) * dx + (y - gFieldAY) * dy) / len2;
                if (t < 0.0) t = 0.0;
                if (t > 1.0) t = 1.0;
            }
            new i = gFarmPlantCount++;
            FarmPlant[i][ptExists] = true;
            FarmPlant[i][ptX] = x;
            FarmPlant[i][ptY] = y;
            FarmPlant[i][ptZ] = gFieldAZ + (gFieldBZ - gFieldAZ) * t;
            FarmPlant[i][ptHarvester] = INVALID_PLAYER_ID;
            FarmPlant[i][ptObject] = 0;
            FarmPlant[i][ptHasLabel] = false;
            Farm_GrowPlant(i);
        }
    }
    printf("[FARM] Cotton field: %d plants.", gFarmPlantCount);
}

stock Farm_GrowPlant(i)
{
    Farm_HidePlant(i);
    FarmPlant[i][ptAlive] = true;
    FarmPlant[i][ptGold] = (random(100) < FARM_GOLD_CHANCE);
    FarmPlant[i][ptObject] = CreateDynamicObject(FARM_PLANT_MODEL,
        FarmPlant[i][ptX], FarmPlant[i][ptY], FarmPlant[i][ptZ] + FARM_PLANT_Z_OFFSET, 0.0, 0.0, float(random(360)), 0, 0, -1, 120.0);
    if (FarmPlant[i][ptGold]) {
        FarmPlant[i][ptLabel] = CreateDynamic3DTextLabel("{FFD700}* Златен сноп *", COLOR_WHITE,
            FarmPlant[i][ptX], FarmPlant[i][ptY], FarmPlant[i][ptZ] + 0.6, 25.0);
        FarmPlant[i][ptHasLabel] = true;
    }
}

stock Farm_HidePlant(i)
{
    if (FarmPlant[i][ptObject] != 0) {
        DestroyDynamicObject(FarmPlant[i][ptObject]);
        FarmPlant[i][ptObject] = 0;
    }
    if (FarmPlant[i][ptHasLabel]) {
        DestroyDynamic3DTextLabel(FarmPlant[i][ptLabel]);
        FarmPlant[i][ptHasLabel] = false;
    }
}

stock Farm_GetRespawnSeconds()
{
    switch (Weather_GetKind()) {
        case WEATHER_KIND_RAIN: return FARM_RESPAWN_RAIN;
        case WEATHER_KIND_HOT:  return FARM_RESPAWN_HOT;
    }
    return FARM_RESPAWN_SECONDS;
}

// Обран храст
stock Farm_PickPlant(i)
{
    Farm_HidePlant(i);
    FarmPlant[i][ptAlive] = false;
    FarmPlant[i][ptHarvester] = INVALID_PLAYER_ID;
    FarmPlant[i][ptRespawnAt] = gettime() + Farm_GetRespawnSeconds();
}

stock Farm_GetNearestPlant(playerid, Float:range, bool:alive)
{
    new best = -1, Float:bestDist = range;
    for (new i = 0; i < gFarmPlantCount; i++) {
        if (!FarmPlant[i][ptExists] || FarmPlant[i][ptAlive] != alive) continue;
        new Float:d = GetPlayerDistanceFromPoint(playerid, FarmPlant[i][ptX], FarmPlant[i][ptY], FarmPlant[i][ptZ]);
        if (d < bestDist) {
            bestDist = d;
            best = i;
        }
    }
    return best;
}

// ============================================================
//  Играч
// ============================================================

stock Farm_ResetPlayer(playerid)
{
    FarmOnShift[playerid] = false;
    FarmSack[playerid] = 0;
    FarmSackGold[playerid] = 0;
    FarmHarvestPlant[playerid] = -1;
    FarmStreak[playerid] = 0;
    FarmLastHarvest[playerid] = 0;
    FarmVehicle[playerid] = INVALID_VEHICLE_ID;
    FarmBunker[playerid] = 0;
    FarmBunkerGold[playerid] = 0;
    FarmKeyTick[playerid] = 0;
}

stock Farm_StreakBonus(playerid)
{
    new bonus = (FarmStreak[playerid] / 10) * 5;
    return (bonus > 25) ? 25 : bonus;
}

stock Farm_BundlePrice(playerid)
{
    return FARM_BUNDLE_PRICE * Job_GetPayPercent(playerid, JOB_FARM) / 100;
}

stock Farm_UpdateSack(playerid)
{
    if (FarmSack[playerid] + FarmSackGold[playerid] > 0) {
        SetPlayerAttachedObject(playerid, FARM_SACK_SLOT, FARM_SACK_MODEL, 1, 0.1, -0.25, 0.0, 0.0, 90.0, 0.0, 0.8, 0.8, 0.8);
    } else {
        RemovePlayerAttachedObject(playerid, FARM_SACK_SLOT);
    }
}

stock Farm_StartShift(playerid)
{
    if (!gFarmFieldSet || !gFarmStoreSet) {
        SendClientMessage(playerid, COLOR_GREY, "Иван: \"Полето още не е готово. Ела по-късно.\" {A0A0A0}(админ: /farmedit)");
        return;
    }
    FarmOnShift[playerid] = true;
    FarmStreak[playerid] = 0;
    FarmLastHarvest[playerid] = gettime();
    SetPlayerSkin(playerid, (PlayerData[playerid][pGender] == GENDER_FEMALE) ? FARM_SKIN_FEMALE : FARM_SKIN_MALE);
    Gps_SetTarget(playerid, "Памучно поле", (gFieldAX + gFieldBX) / 2.0, (gFieldAY + gFieldBY) / 2.0, (gFieldAZ + gFieldBZ) / 2.0);

    SendClientMessage(playerid, COLOR_GREEN, "[Ферма] {FFFFFF}Смяната започна! Отиди до храст памук и натисни {FFD54A}ALT{FFFFFF}.");
    SendClientMessage(playerid, COLOR_GREY, "Чувалът събира %d снопа. Пълния чувал занеси в склада (ALT). Златните снопове се плащат x%d!",
        FARM_SACK_SIZE, FARM_GOLD_MULTIPLIER);
}

stock Farm_EndShift(playerid, bool:silent = false)
{
    if (!FarmOnShift[playerid]) return;
    Farm_CancelHarvest(playerid);
    FarmOnShift[playerid] = false;

    if (FarmVehicle[playerid] != INVALID_VEHICLE_ID) {
        DestroyVehicle(FarmVehicle[playerid]);
        FarmVehicle[playerid] = INVALID_VEHICLE_ID;
    }
    new lost = FarmSack[playerid] + FarmSackGold[playerid] + FarmBunker[playerid] + FarmBunkerGold[playerid];
    FarmSack[playerid] = 0;
    FarmSackGold[playerid] = 0;
    FarmBunker[playerid] = 0;
    FarmBunkerGold[playerid] = 0;

    if (IsPlayerConnected(playerid)) {
        RemovePlayerAttachedObject(playerid, FARM_SACK_SLOT);
        SetPlayerSkin(playerid, PlayerData[playerid][pSkin]);
        if (!silent) {
            if (lost > 0) SendClientMessage(playerid, COLOR_GREEN, "[Ферма] {FFFFFF}Смяната приключи. Неразтоварените %d снопа остават във фермата.", lost);
            else SendClientMessage(playerid, COLOR_GREEN, "[Ферма] {FFFFFF}Смяната приключи. Ела пак!");
        }
    }
}

stock Farm_SpawnVehicle(playerid, model)
{
    if (!gFarmParkSet) {
        SendError(playerid, "Паркингът за техниката още не е поставен (админ: /farmedit).");
        return;
    }
    if (FarmVehicle[playerid] != INVALID_VEHICLE_ID) DestroyVehicle(FarmVehicle[playerid]);

    new vehicleid = CreateVehicle(model, gParkX, gParkY, gParkZ + 0.5, gParkA, 1, 1, -1);
    if (vehicleid == INVALID_VEHICLE_ID || vehicleid == 0) {
        FarmVehicle[playerid] = INVALID_VEHICLE_ID;
        SendError(playerid, "Техниката не може да бъде създадена.");
        return;
    }
    FarmVehicle[playerid] = vehicleid;
    SetVehicleNumberPlate(vehicleid, "FARM");
    PutPlayerInVehicle(playerid, vehicleid, 0);

    if (model == FARM_TRACTOR_MODEL) {
        SendClientMessage(playerid, COLOR_GREEN, "[Ферма] {FFFFFF}Трактор: карай през обраните места – засява ги наново ($%d за всяко).", FARM_TRACTOR_PAY);
    } else {
        SendClientMessage(playerid, COLOR_GREEN, "[Ферма] {FFFFFF}Комбайн: карай през храстите – жъне ги. Разтовари бункера в склада (до %d снопа).", FARM_COMBINE_CAPACITY);
    }
}

// ============================================================
//  Беране
// ============================================================

stock Farm_TryHarvest(playerid)
{
    if (FarmHarvestPlant[playerid] != -1) return;
    if (FarmSack[playerid] + FarmSackGold[playerid] >= FARM_SACK_SIZE) {
        GameTextForPlayer(playerid, "~r~Sack is full!~n~~w~Go to the store", 2500, 3);
        return;
    }
    new plant = Farm_GetNearestPlant(playerid, 1.8, true);
    if (plant == -1) return;
    if (FarmPlant[plant][ptHarvester] != INVALID_PLAYER_ID) {
        SendClientMessage(playerid, COLOR_GREY, "Някой вече бере този храст.");
        return;
    }

    FarmPlant[plant][ptHarvester] = playerid;
    FarmHarvestPlant[playerid] = plant;
    TogglePlayerControllable(playerid, false);
    ApplyAnimation(playerid, "BOMBER", "BOM_Plant_Loop", 4.1, true, false, false, false, 0);

    new ms = (Job_GetSkill(playerid, JOB_FARM) >= 2) ? FARM_HARVEST_MS_FAST : FARM_HARVEST_MS;
    GameTextForPlayer(playerid, "~y~Picking cotton...", ms, 3);
    SetTimerEx("Farm_HarvestDone", ms, false, "ii", playerid, plant);
}

forward Farm_HarvestDone(playerid, plant);
public Farm_HarvestDone(playerid, plant)
{
    if (!IsPlayerConnected(playerid) || FarmHarvestPlant[playerid] != plant) return 0;
    FarmHarvestPlant[playerid] = -1;
    TogglePlayerControllable(playerid, true);
    ClearAnimations(playerid);

    if (!FarmPlant[plant][ptAlive] || FarmPlant[plant][ptHarvester] != playerid) return 0;

    new bool:gold = FarmPlant[plant][ptGold];
    Farm_PickPlant(plant);

    if (gold) FarmSackGold[playerid]++;
    else FarmSack[playerid]++;

    // Серия
    if (gettime() - FarmLastHarvest[playerid] > FARM_STREAK_RESET) FarmStreak[playerid] = 0;
    FarmStreak[playerid]++;
    FarmLastHarvest[playerid] = gettime();

    Farm_UpdateSack(playerid);
    new total = FarmSack[playerid] + FarmSackGold[playerid];
    if (gold) {
        GameTextForPlayer(playerid, "~y~GOLDEN BUNDLE!", 2500, 3);
        SendClientMessage(playerid, COLOR_GOLD, "[Ферма] Златен сноп! Плаща се x%d.", FARM_GOLD_MULTIPLIER);
    } else {
        GameTextForPlayer(playerid, "~w~Sack: ~y~%d/%d", 1500, 3, total, FARM_SACK_SIZE);
    }
    if (total >= FARM_SACK_SIZE) {
        SendClientMessage(playerid, COLOR_GREEN, "[Ферма] {FFFFFF}Чувалът е пълен. Занеси го в склада.");
        Gps_SetTarget(playerid, "Склад за памук", gStoreX, gStoreY, gStoreZ);
    }
    return 1;
}

stock Farm_CancelHarvest(playerid)
{
    new plant = FarmHarvestPlant[playerid];
    if (plant == -1) return;
    FarmHarvestPlant[playerid] = -1;
    if (plant >= 0 && plant < FARM_MAX_PLANTS && FarmPlant[plant][ptHarvester] == playerid) {
        FarmPlant[plant][ptHarvester] = INVALID_PLAYER_ID;
    }
    if (IsPlayerConnected(playerid)) {
        TogglePlayerControllable(playerid, true);
        ClearAnimations(playerid);
    }
}

// Разтоварване в склада (чувал или бункер на комбайна)
stock Farm_Unload(playerid, normal, gold, bool:fromCombine)
{
    if (normal + gold <= 0) return 0;

    new price = Farm_BundlePrice(playerid);
    new pay = normal * price + gold * price * FARM_GOLD_MULTIPLIER;
    new bonus = Farm_StreakBonus(playerid);
    pay += pay * bonus / 100;

    Money_Give(playerid, pay);
    Market_Add(GOODS_COTTON, (normal + gold) * 2);
    Auth_SaveAccount(playerid);

    if (bonus > 0) {
        SendClientMessage(playerid, COLOR_GREEN, "[Ферма] {FFFFFF}Разтовари %d снопа. Получи {33CC66}$%d{FFFFFF} (серия: +%d%%).",
            normal + gold, pay, bonus);
    } else {
        SendClientMessage(playerid, COLOR_GREEN, "[Ферма] {FFFFFF}Разтовари %d снопа. Получи {33CC66}$%d{FFFFFF}.", normal + gold, pay);
    }

    // EXP: 1 на пълен чувал (или на всеки 5 снопа от комбайна), +1 за злато
    new exp = (normal + gold) / FARM_SACK_SIZE;
    if (exp < 1) exp = 1;
    if (gold > 0) exp++;
    Job_AddExp(playerid, JOB_FARM, exp);

    new details[48];
    if (fromCombine) format(details, sizeof (details), "%d снопа: $%d (комбайн)", normal + gold, pay);
    else format(details, sizeof (details), "%d снопа: $%d", normal + gold, pay);
    Log_Add("farm", PlayerData[playerid][pName], "", details);
    return pay;
}

stock bool:Farm_TryUnloadSack(playerid)
{
    if (GetPlayerDistanceFromPoint(playerid, gStoreX, gStoreY, gStoreZ) > 3.0) return false;
    if (FarmSack[playerid] + FarmSackGold[playerid] == 0) {
        SendClientMessage(playerid, COLOR_GREY, "Чувалът ти е празен.");
        return true;
    }
    ApplyAnimation(playerid, "CARRY", "putdwn", 4.1, false, false, false, false, 0);
    Farm_Unload(playerid, FarmSack[playerid], FarmSackGold[playerid], false);
    FarmSack[playerid] = 0;
    FarmSackGold[playerid] = 0;
    Farm_UpdateSack(playerid);
    Gps_Stop(playerid);
    return true;
}

// ALT / Y. Връща true, ако бутонът е използван от фермата.
stock bool:Farm_OnKeyStateChange(playerid, KEY:newkeys, KEY:oldkeys)
{
    if (!FarmOnShift[playerid] || IsPlayerInAnyVehicle(playerid)) return false;
    new bool:alt = ((newkeys & KEY_WALK) && !(oldkeys & KEY_WALK)),
        bool:yes = ((newkeys & KEY_YES) && !(oldkeys & KEY_YES));
    if (!alt && !yes) return false;

    // До склада – разтоварване
    if (gFarmStoreSet && GetPlayerDistanceFromPoint(playerid, gStoreX, gStoreY, gStoreZ) <= 3.0) {
        Farm_TryUnloadSack(playerid);
        return true;
    }
    // До храст – беране
    if (Farm_GetNearestPlant(playerid, 1.8, true) != -1) {
        new now = GetTickCount();
        if (now - FarmKeyTick[playerid] < 500) return true;
        FarmKeyTick[playerid] = now;
        Farm_TryHarvest(playerid);
        return true;
    }
    return false;
}

// ============================================================
//  Таймер (всяка секунда)
// ============================================================

stock Farm_OnTick()
{
    new now = gettime();

    // Поникване
    for (new i = 0; i < gFarmPlantCount; i++) {
        if (FarmPlant[i][ptExists] && !FarmPlant[i][ptAlive] && now >= FarmPlant[i][ptRespawnAt]) Farm_GrowPlant(i);
    }

    // Техника
    for (new p = 0; p < MAX_PLAYERS; p++) {
        if (!FarmOnShift[p] || FarmVehicle[p] == INVALID_VEHICLE_ID || !IsPlayerConnected(p)) continue;
        if (GetVehicleModel(FarmVehicle[p]) == 0) {
            FarmVehicle[p] = INVALID_VEHICLE_ID;
            continue;
        }
        if (GetPlayerVehicleID(p) != FarmVehicle[p]) continue;
        new model = GetVehicleModel(FarmVehicle[p]);

        if (model == FARM_TRACTOR_MODEL) {
            // Засява обраните места наоколо
            new plant = Farm_GetNearestPlant(p, 3.5, false);
            if (plant != -1) {
                Farm_GrowPlant(plant);
                Money_Give(p, FARM_TRACTOR_PAY);
                GameTextForPlayer(p, "~g~+$%d ~w~seeded", 1000, 3, FARM_TRACTOR_PAY);
            }
        } else if (model == FARM_COMBINE_MODEL) {
            // Жъне храстите наоколо
            if (FarmBunker[p] + FarmBunkerGold[p] < FARM_COMBINE_CAPACITY) {
                new plant = Farm_GetNearestPlant(p, 3.5, true);
                if (plant != -1 && FarmPlant[plant][ptHarvester] == INVALID_PLAYER_ID) {
                    if (FarmPlant[plant][ptGold]) FarmBunkerGold[p]++;
                    else FarmBunker[p]++;
                    Farm_PickPlant(plant);
                    FarmStreak[p]++;
                    FarmLastHarvest[p] = now;
                    GameTextForPlayer(p, "~w~Bunker: ~y~%d/%d", 1000, 3, FarmBunker[p] + FarmBunkerGold[p], FARM_COMBINE_CAPACITY);
                    if (FarmBunker[p] + FarmBunkerGold[p] >= FARM_COMBINE_CAPACITY) {
                        SendClientMessage(p, COLOR_GREEN, "[Ферма] {FFFFFF}Бункерът е пълен. Закарай комбайна до склада.");
                        Gps_SetTarget(p, "Склад за памук", gStoreX, gStoreY, gStoreZ);
                    }
                }
            }
            // Разтоварване до склада
            if (FarmBunker[p] + FarmBunkerGold[p] > 0 && GetPlayerDistanceFromPoint(p, gStoreX, gStoreY, gStoreZ) <= 8.0) {
                Farm_Unload(p, FarmBunker[p], FarmBunkerGold[p], true);
                FarmBunker[p] = 0;
                FarmBunkerGold[p] = 0;
                Gps_Stop(p);
            }
        }
    }
}

stock Farm_OnPlayerDisconnect(playerid)
{
    Farm_EndShift(playerid, true);
    Farm_ResetPlayer(playerid);
}

stock Farm_OnPlayerDeath(playerid)
{
    Farm_CancelHarvest(playerid);
    if (FarmSack[playerid] + FarmSackGold[playerid] > 0) {
        FarmSack[playerid] = 0;
        FarmSackGold[playerid] = 0;
        SendClientMessage(playerid, COLOR_RED, "[Ферма] Изгуби чувала с памук.");
    }
}

stock Farm_OnPlayerSpawn(playerid)
{
    if (FarmOnShift[playerid]) {
        SetPlayerSkin(playerid, (PlayerData[playerid][pGender] == GENDER_FEMALE) ? FARM_SKIN_FEMALE : FARM_SKIN_MALE);
        Farm_UpdateSack(playerid);
    }
}

// ============================================================
//  Иван (NPC)
// ============================================================

stock Farm_OnNpcInteract(playerid)
{
    new body[512], count = 0, skill = Job_GetSkill(playerid, JOB_FARM);
    body[0] = EOS;
    if (!FarmOnShift[playerid]) {
        strcat(body, C_GREEN "» " C_WHITE "Започни смяна\n");
        FarmMenuAction[playerid][count++] = 1;
    } else {
        strcat(body, C_RED "» " C_WHITE "Приключи смяната\n");
        FarmMenuAction[playerid][count++] = 2;
        if (skill >= 2) {
            strcat(body, C_BRAND "» " C_WHITE "Вземи трактор\n");
            FarmMenuAction[playerid][count++] = 3;
        } else {
            strcat(body, C_GREY "» Трактор (скил 2)\n");
            FarmMenuAction[playerid][count++] = 0;
        }
        if (skill >= 3) {
            strcat(body, C_BRAND "» " C_WHITE "Вземи комбайн\n");
            FarmMenuAction[playerid][count++] = 4;
        } else {
            strcat(body, C_GREY "» Комбайн (скил 3)\n");
            FarmMenuAction[playerid][count++] = 0;
        }
    }
    strcat(body, C_BLUE "» " C_WHITE "Моят напредък\n");
    FarmMenuAction[playerid][count++] = 5;
    strcat(body, C_BLUE "» " C_WHITE "Как се работи");
    FarmMenuAction[playerid][count] = 6;

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
    format(line, sizeof (line), "\n" C_WHITE "Цена за сноп: " C_GREEN "$%d" C_WHITE " (златен: " C_GOLD "$%d" C_WHITE ")\n",
        Farm_BundlePrice(playerid), Farm_BundlePrice(playerid) * FARM_GOLD_MULTIPLIER);
    strcat(body, line);
    if (FarmOnShift[playerid]) {
        format(line, sizeof (line), C_WHITE "Серия: " C_GOLD "%d снопа" C_WHITE " (бонус +%d%%)\n", FarmStreak[playerid], Farm_StreakBonus(playerid));
        strcat(body, line);
        format(line, sizeof (line), C_WHITE "В чувала: " C_GOLD "%d/%d\n", FarmSack[playerid] + FarmSackGold[playerid], FARM_SACK_SIZE);
        strcat(body, line);
    }
    strcat(body, "\n" C_BRAND "Скил\n");
    strcat(body, C_WHITE "1: беране на ръка\n");
    strcat(body, C_WHITE "2: по-бързо беране + трактор (засява обраните места)\n");
    strcat(body, C_WHITE "3: комбайн (жъне всичко по пътя си)\n");
    strcat(body, C_GREY "EXP: +1 за пълен чувал, +1 за златен сноп.");
    ShowPlayerDialog(playerid, DIALOG_FARM_INFO, DIALOG_STYLE_MSGBOX, C_BRAND "Ферма " C_WHITE "| Напредък", "%s", "Назад", "", body);
}

stock Farm_ShowHelp(playerid)
{
    new body[1500], line[160];
    body[0] = EOS;
    strcat(body, C_WHITE "1. Започни смяна при " C_BRAND "Иван" C_WHITE ".\n");
    strcat(body, C_WHITE "2. Отиди до храст памук и натисни " C_GOLD "ALT" C_WHITE " - береш няколко секунди.\n");
    format(line, sizeof (line), C_WHITE "3. Чувалът на гърба събира " C_GOLD "%d снопа" C_WHITE ".\n", FARM_SACK_SIZE);
    strcat(body, line);
    strcat(body, C_WHITE "4. Отиди до " C_BRAND "склада" C_WHITE " и натисни " C_GOLD "ALT" C_WHITE " - плаща се на сноп.\n\n");
    format(line, sizeof (line), C_GOLD "Златен сноп" C_WHITE " - рядък, плаща се x%d. Търси надписа над храста!\n", FARM_GOLD_MULTIPLIER);
    strcat(body, line);
    strcat(body, C_WHITE "Серия - береш без дълги паузи и бонусът расте до " C_GREEN "+25%" C_WHITE ".\n");
    strcat(body, C_WHITE "Обраните храсти поникват наново - по-бързо в дъжд, по-бавно в жега.\n");
    ShowPlayerDialog(playerid, DIALOG_FARM_INFO, DIALOG_STYLE_MSGBOX, C_BRAND "Ферма " C_WHITE "| Как се работи", "%s", "Назад", "", body);
}

stock Farm_OnDialogResponse(playerid, dialogid, response, listitem)
{
    switch (dialogid) {
        case DIALOG_FARM_NPC: {
            if (!response || listitem < 0 || listitem > 5) return 1;
            switch (FarmMenuAction[playerid][listitem]) {
                case 1: Farm_StartShift(playerid);
                case 2: Farm_EndShift(playerid);
                case 3: Farm_SpawnVehicle(playerid, FARM_TRACTOR_MODEL);
                case 4: Farm_SpawnVehicle(playerid, FARM_COMBINE_MODEL);
                case 5: Farm_ShowProgress(playerid);
                case 6: Farm_ShowHelp(playerid);
                default: SendError(playerid, "Скилът ти още не стига за това.");
            }
            return 1;
        }
        case DIALOG_FARM_INFO: {
            Farm_OnNpcInteract(playerid);
            return 1;
        }
        case DIALOG_FARM_EDIT: {
            if (response) Farm_OnEditSelect(playerid, listitem);
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

stock Farm_EditLine(body[], size, const name[], bool:isSet)
{
    new line[96];
    if (isSet) format(line, sizeof (line), C_GREEN "» " C_WHITE "%s " C_GREEN "(поставен)\n", name);
    else format(line, sizeof (line), C_GREY "» " C_WHITE "%s " C_GREY "(не е поставен)\n", name);
    strcat(body, line, size);
}

stock Farm_ShowEditMenu(playerid)
{
    new body[1024], Float:x, Float:y, Float:z, Float:a;
    body[0] = EOS;
    strcat(body, C_BRAND "» " C_WHITE "Иван (NPC) - тук\n");
    Farm_EditLine(body, sizeof (body), "Ъгъл 1 на полето", Job_GetPoint(JOB_FARM, "fieldA", x, y, z, a));
    Farm_EditLine(body, sizeof (body), "Ъгъл 2 на полето", Job_GetPoint(JOB_FARM, "fieldB", x, y, z, a));
    Farm_EditLine(body, sizeof (body), "Склад", gFarmStoreSet);
    Farm_EditLine(body, sizeof (body), "Паркинг за техниката", gFarmParkSet);
    new line[96];
    format(line, sizeof (line), C_BLUE "» " C_WHITE "Засади полето наново " C_GREY "(%d храста)", gFarmPlantCount);
    strcat(body, line);
    ShowPlayerDialog(playerid, DIALOG_FARM_EDIT, DIALOG_STYLE_LIST, C_RED "Ферма " C_WHITE "| Застани на мястото и избери",
        "%s", "Постави тук", "Затвори", body);
}

stock Farm_OnEditSelect(playerid, item)
{
    new Float:x, Float:y, Float:z, Float:a;
    GetPlayerPos(playerid, x, y, z);
    GetPlayerFacingAngle(playerid, a);

    switch (item) {
        case 0: {
            Job_SetPoint(JOB_FARM, "job", x, y, z, a);
            Npc_MoveRole(NPC_ROLE_FARM, x, y, z, a);
            SendClientMessage(playerid, COLOR_ADMIN, "[Ферма] Иван е преместен тук.");
        }
        case 1, 2: {
            if (item == 1) Job_SetPoint(JOB_FARM, "fieldA", x, y, z, a);
            else Job_SetPoint(JOB_FARM, "fieldB", x, y, z, a);
            new Float:t;
            gFarmFieldSet = Job_GetPoint(JOB_FARM, "fieldA", gFieldAX, gFieldAY, gFieldAZ, t)
                && Job_GetPoint(JOB_FARM, "fieldB", gFieldBX, gFieldBY, gFieldBZ, t);
            SendClientMessage(playerid, COLOR_ADMIN, "[Ферма] Ъгъл %d на полето е поставен.", item);
            if (gFarmFieldSet) {
                Farm_BuildField();
                SendClientMessage(playerid, COLOR_ADMIN, "[Ферма] Полето е засадено: %d храста.", gFarmPlantCount);
            }
        }
        case 3: {
            Job_SetPoint(JOB_FARM, "store", x, y, z, a);
            gStoreX = x; gStoreY = y; gStoreZ = z;
            gFarmStoreSet = true;
            SendClientMessage(playerid, COLOR_ADMIN, "[Ферма] Складът е поставен тук.");
        }
        case 4: {
            Job_SetPoint(JOB_FARM, "park", x, y, z, a);
            gParkX = x; gParkY = y; gParkZ = z; gParkA = a;
            gFarmParkSet = true;
            SendClientMessage(playerid, COLOR_ADMIN, "[Ферма] Паркингът е поставен тук (колата ще гледа накъдето гледаш ти).");
        }
        case 5: {
            Farm_BuildField();
            SendClientMessage(playerid, COLOR_ADMIN, "[Ферма] Полето е засадено наново: %d храста.", gFarmPlantCount);
        }
    }
    Farm_CreateLabels();
    Log_Add("farmedit", PlayerData[playerid][pName], "", "");
    Farm_ShowEditMenu(playerid);
}

CMD:farm(playerid, params[])
{
    #pragma unused params
    Farm_ShowHelp(playerid);
    return 1;
}
