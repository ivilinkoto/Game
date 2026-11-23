// Увод за нови играчи: пристигане на гарата, Наставник и 7 задачи.
//
// - Новите играчи се появяват на гарата на избрания град (докато нямат гараж и бус).
//   Там има площадче (мапинг), безплатни велосипеди и NPC "Наставник" (ALT).
// - Задачи: 1 лична карта (кметството), 2 медицинско (болницата), 3 книжка B (автошколата),
//   4 първият бус (гаражът), 5 първа доставка, 6 храна и гориво, 7 кариера (борса или компания).
// - Кметството е вграденият интериор "Planning Department" (интериор 3), отделен свят за всеки град.
// - Местата се местят от админ панела: гара, вход на кметството, служителката в него, болница.
// - Старите акаунти са минали увода (имат документи) - еднократно при старта.

#define TUT_ID          1
#define TUT_MEDICAL     2
#define TUT_LICENSE     3
#define TUT_GARAGE      4
#define TUT_DELIVERY    5
#define TUT_CARE        6
#define TUT_CAREER      7
#define TUT_DONE        100
#define TUT_STEPS       7

#define HALL_INTERIOR       3
#define HALL_WORLD_BASE     6000            // свят = база + град
#define HALL_ENTRY_X        384.8086
#define HALL_ENTRY_Y        173.8050
#define HALL_ENTRY_Z        1008.3828
#define HALL_ENTRY_A        90.0
#define WORLD_RANGE         2.5
#define STATION_BIKES       5
#define STATION_OBJECTS     24
#define MENTOR_SKIN         17
#define CLERK_SKIN          150
#define DOCTOR_SKIN         276

static const TutStepNames[TUT_STEPS][] = {
    "Вземи лична карта от кметството",
    "Мини медицински преглед в болницата",
    "Вземи шофьорска книжка B в автошколата",
    "Вземи първия си бус от гаража",
    "Направи първата си доставка",
    "Купи си храна и зареди гориво",
    "Разгледай борсата на товарите или кандидатствай в компания"
};
static const TutRewardMoney[TUT_STEPS] = {200, 200, 500, 0, 500, 300, 1500};
static const TutRewardExp[TUT_STEPS] = {20, 20, 40, 20, 40, 20, 80};

// Места по градове: гара, вход на кметството, болница. Служителката в кметството е една позиция.
enum {
    WP_STATION,
    WP_HALL,
    WP_HOSPITAL,
    WP_KINDS
}
static const WpNames[WP_KINDS][] = {"Гара", "Кметство", "Болница"};
static Float:WorldPoint[WP_KINDS][MAX_CITIES][4] = {
    {   // гари: Unity Station, Cranberry Station, Linden Station
        {1742.9, -1859.6, 13.41, 0.0},
        {-1974.4, 137.6, 27.69, 90.0},
        {2848.4, 1291.0, 11.39, 90.0}
    },
    {   // кметства
        {1481.0, -1771.4, 18.80, 0.0},
        {-2765.8, 375.5, 6.34, 270.0},
        {2388.6, 2466.0, 10.82, 180.0}
    },
    {   // болници: All Saints, SF Medical Center, LV Hospital
        {1172.2, -1323.4, 15.40, 270.0},
        {-2655.0, 638.2, 14.45, 180.0},
        {1607.5, 1817.6, 10.82, 0.0}
    }
};
static Float:ClerkPos[4] = {359.7, 173.6, 1008.38, 270.0};

static TutStep[MAX_PLAYERS];
static bool:TutFood[MAX_PLAYERS];
static bool:TutFuel[MAX_PLAYERS];
static HallBusy[MAX_PLAYERS];                       // чака на опашката (gettime() до кога)

static WpPickup[WP_KINDS][MAX_CITIES];
static Text3D:WpLabel[WP_KINDS][MAX_CITIES];
static WpIcon[WP_KINDS][MAX_CITIES];
static StationObj[MAX_CITIES][STATION_OBJECTS];
static StationBike[MAX_CITIES][STATION_BIKES];
static MentorActor[MAX_CITIES] = {INVALID_ACTOR_ID, ...};
static Text3D:MentorLabel[MAX_CITIES] = {Text3D:-1, ...};
static Text3D:StationLabel[MAX_CITIES][2];
static DoctorActor[MAX_CITIES] = {INVALID_ACTOR_ID, ...};
static ClerkActor[MAX_CITIES] = {INVALID_ACTOR_ID, ...};
static Text3D:ClerkLabel[MAX_CITIES] = {Text3D:-1, ...};

// ---------- Начало ----------

stock Tutorial_Init()
{
    Database_AddColumn("accounts", "tutorial_step", "INTEGER NOT NULL DEFAULT 100");
    Database_Exec("CREATE TABLE IF NOT EXISTS world_points (kind INTEGER NOT NULL, city INTEGER NOT NULL, x REAL NOT NULL, y REAL NOT NULL, z REAL NOT NULL, a REAL NOT NULL, PRIMARY KEY (kind, city))");

    // Еднократно: старите акаунти получават документи (уводът е за новите)
    if (Database_GetMeta("tutorial_migrated") == 0) {
        Database_Exec("UPDATE accounts SET tutorial_step = 100, id_number = 7000000 + id * 13, id_issued = registered_at, birth = 19900101, med_issued = registered_at WHERE id_number = 0");
        Database_SetMeta("tutorial_migrated", 1);
    }

    new DBResult:r = Database_Query("SELECT * FROM world_points");
    if (r != DBResult:0) {
        new rows = DB_GetRowCount(r);
        for (new i = 0; i < rows; i++) {
            new kind = DB_GetFieldIntByName(r, "kind"), city = DB_GetFieldIntByName(r, "city");
            if (kind == 99) {
                ClerkPos[0] = DB_GetFieldFloatByName(r, "x");
                ClerkPos[1] = DB_GetFieldFloatByName(r, "y");
                ClerkPos[2] = DB_GetFieldFloatByName(r, "z");
                ClerkPos[3] = DB_GetFieldFloatByName(r, "a");
            } else if (kind >= 0 && kind < WP_KINDS && city >= 0 && city < MAX_CITIES) {
                WorldPoint[kind][city][0] = DB_GetFieldFloatByName(r, "x");
                WorldPoint[kind][city][1] = DB_GetFieldFloatByName(r, "y");
                WorldPoint[kind][city][2] = DB_GetFieldFloatByName(r, "z");
                WorldPoint[kind][city][3] = DB_GetFieldFloatByName(r, "a");
            }
            DB_SelectNextRow(r);
        }
        DB_FreeResultSet(r);
    }

    for (new k = 0; k < WP_KINDS; k++) {
        for (new c = 0; c < MAX_CITIES; c++) {
            WpPickup[k][c] = -1;
            WpLabel[k][c] = Text3D:-1;
            WpIcon[k][c] = -1;
            Tutorial_BuildPoint(k, c);
        }
    }
    for (new c = 0; c < MAX_CITIES; c++) Tutorial_BuildClerk(c);
}

stock Tutorial_OnPlayerConnect(playerid)
{
    TutStep[playerid] = TUT_DONE;
    TutFood[playerid] = false;
    TutFuel[playerid] = false;
    HallBusy[playerid] = 0;
}

stock Tutorial_OnLogin(playerid)
{
    new q[96];
    format(q, sizeof (q), "SELECT tutorial_step FROM accounts WHERE id = %d", PlayerData[playerid][pID]);
    new DBResult:r = Database_Query(q);
    if (r != DBResult:0) {
        if (DB_GetRowCount(r) > 0) TutStep[playerid] = DB_GetFieldIntByName(r, "tutorial_step");
        DB_FreeResultSet(r);
    }
    // Старите играчи без бус (не би трябвало) - получават го
    if (TutStep[playerid] > TUT_GARAGE && !PlayerData[playerid][pStarterGiven]) Veh_GiveStarter(playerid);
}

// Нов акаунт: без категории и без бус - уводът започва
stock Tutorial_OnRegister(playerid)
{
    TutStep[playerid] = TUT_ID;
    new q[128];
    format(q, sizeof (q), "UPDATE accounts SET tutorial_step = %d, licenses = 0 WHERE id = %d", TUT_ID, PlayerData[playerid][pID]);
    Database_Exec(q);
}

static stock Tutorial_SaveStep(playerid)
{
    new q[96];
    format(q, sizeof (q), "UPDATE accounts SET tutorial_step = %d WHERE id = %d", TutStep[playerid], PlayerData[playerid][pID]);
    Database_Exec(q);
}

stock Tutorial_GetStep(playerid)
{
    return TutStep[playerid];
}

// Има ли лична карта и медицинско (нужни за изпитите)
stock Tutorial_HasDocs(playerid)
{
    return (Docs_HasId(playerid) && Docs_HasMedical(playerid)) ? 1 : 0;
}

// ---------- Места на картата ----------

stock Tutorial_GetPoint(kind, city, &Float:x, &Float:y, &Float:z)
{
    x = WorldPoint[kind][city][0];
    y = WorldPoint[kind][city][1];
    z = WorldPoint[kind][city][2];
}

static stock Float:Tut_OffX(kind, city, Float:dx, Float:dy)
{
    new Float:a = WorldPoint[kind][city][3];
    return WorldPoint[kind][city][0] + dy * floatsin(-a, degrees) + dx * floatcos(-a, degrees);
}

static stock Float:Tut_OffY(kind, city, Float:dx, Float:dy)
{
    new Float:a = WorldPoint[kind][city][3];
    return WorldPoint[kind][city][1] + dy * floatcos(-a, degrees) - dx * floatsin(-a, degrees);
}

static stock Tutorial_StationObj(city, &n, model, Float:dx, Float:dy, Float:dz, Float:rzAdd)
{
    if (n >= STATION_OBJECTS) return;
    StationObj[city][n++] = CreateDynamicObject(model, Tut_OffX(WP_STATION, city, dx, dy), Tut_OffY(WP_STATION, city, dx, dy),
        WorldPoint[WP_STATION][city][2] - 1.0 + dz, 0.0, 0.0, WorldPoint[WP_STATION][city][3] + rzAdd, 0, 0, -1, 200.0, 200.0);
}

static stock Tutorial_BuildPoint(kind, city)
{
    if (WpPickup[kind][city] != -1) DestroyDynamicPickup(WpPickup[kind][city]);
    if (WpLabel[kind][city] != Text3D:-1) DestroyDynamic3DTextLabel(WpLabel[kind][city]);
    if (WpIcon[kind][city] != -1) DestroyDynamicMapIcon(WpIcon[kind][city]);
    WpPickup[kind][city] = -1;
    WpLabel[kind][city] = Text3D:-1;
    WpIcon[kind][city] = -1;
    new Float:x = WorldPoint[kind][city][0], Float:y = WorldPoint[kind][city][1], Float:z = WorldPoint[kind][city][2];

    switch (kind) {
        case WP_STATION: Tutorial_BuildStation(city);
        case WP_HALL: {
            WpPickup[kind][city] = CreateDynamicPickup(1318, 1, x, y, z, 0, 0);
            WpLabel[kind][city] = CreateDynamic3DTextLabel(C_BRAND "КМЕТСТВО\n" C_WHITE "Лични карти и документи\n" C_GREY "ALT - влез", COLOR_WHITE,
                x, y, z + 0.8, 20.0, INVALID_PLAYER_ID, INVALID_VEHICLE_ID, 1, 0, 0);
            WpIcon[kind][city] = CreateDynamicMapIcon(x, y, z, 52, 0, 0, 0, -1, 3000.0, MAPICON_LOCAL);
        }
        case WP_HOSPITAL: {
            if (DoctorActor[city] != INVALID_ACTOR_ID) DestroyActor(DoctorActor[city]);
            DoctorActor[city] = CreateActor(DOCTOR_SKIN, Tut_OffX(kind, city, 1.2, 0.0), Tut_OffY(kind, city, 1.2, 0.0), z, WorldPoint[kind][city][3] + 180.0);
            if (DoctorActor[city] != INVALID_ACTOR_ID) SetActorInvulnerable(DoctorActor[city], true);
            WpPickup[kind][city] = CreateDynamicPickup(1240, 1, x, y, z, 0, 0);
            WpLabel[kind][city] = CreateDynamic3DTextLabel(C_RED "БОЛНИЦА\n" C_WHITE "Медицински преглед\n" C_GREY "ALT - при доктора", COLOR_WHITE,
                x, y, z + 0.8, 20.0, INVALID_PLAYER_ID, INVALID_VEHICLE_ID, 1, 0, 0);
            WpIcon[kind][city] = CreateDynamicMapIcon(x, y, z, 22, 0, 0, 0, -1, 3000.0, MAPICON_LOCAL);
        }
    }
}

// Площадчето пред гарата: табела, пейки, лампи, саксии, велосипеди и Наставникът
static stock Tutorial_BuildStation(city)
{
    for (new i = 0; i < STATION_OBJECTS; i++) {
        if (StationObj[city][i] != 0) DestroyDynamicObject(StationObj[city][i]);
        StationObj[city][i] = 0;
    }
    for (new i = 0; i < STATION_BIKES; i++) {
        if (StationBike[city][i] != 0 && StationBike[city][i] != INVALID_VEHICLE_ID) DestroyVehicle(StationBike[city][i]);
        StationBike[city][i] = 0;
    }
    if (MentorActor[city] != INVALID_ACTOR_ID) DestroyActor(MentorActor[city]);
    if (MentorLabel[city] != Text3D:-1) DestroyDynamic3DTextLabel(MentorLabel[city]);
    for (new i = 0; i < 2; i++) {
        if (StationLabel[city][i] != Text3D:0) DestroyDynamic3DTextLabel(StationLabel[city][i]);
        StationLabel[city][i] = Text3D:0;
    }

    new n = 0;
    // Табела "Добре дошли": две колони и оранжево табло
    Tutorial_StationObj(city, n, 19353, -1.6, 6.0, 1.75, 90.0);
    if (n > 0) SetDynamicObjectMaterial(StationObj[city][n - 1], 0, 18646, "matcolours", "white", 0xFF2E3640);
    Tutorial_StationObj(city, n, 1290, -3.6, 6.2, 0.0, 0.0);
    Tutorial_StationObj(city, n, 1290, 0.4, 6.2, 0.0, 0.0);
    // Пейки и саксии от двете страни
    Tutorial_StationObj(city, n, 1280, -5.0, 2.0, 0.5, 90.0);
    Tutorial_StationObj(city, n, 1280, 5.0, 2.0, 0.5, 270.0);
    Tutorial_StationObj(city, n, 948, -5.0, 3.6, 0.0, 0.0);
    Tutorial_StationObj(city, n, 948, -5.0, 0.4, 0.0, 0.0);
    Tutorial_StationObj(city, n, 948, 5.0, 3.6, 0.0, 0.0);
    Tutorial_StationObj(city, n, 948, 5.0, 0.4, 0.0, 0.0);
    // Лампи по ъглите
    Tutorial_StationObj(city, n, 1226, -6.5, -2.0, 0.0, 0.0);
    Tutorial_StationObj(city, n, 1226, 6.5, -2.0, 0.0, 180.0);

    new Float:sx = Tut_OffX(WP_STATION, city, -1.6, 6.0), Float:sy = Tut_OffY(WP_STATION, city, -1.6, 6.0);
    StationLabel[city][0] = CreateDynamic3DTextLabel(C_BRAND "ДОБРЕ ДОШЛИ В\nCARGO STATE", COLOR_WHITE, sx, sy, WorldPoint[WP_STATION][city][2] + 2.0, 30.0,
        INVALID_PLAYER_ID, INVALID_VEHICLE_ID, 0, 0, 0);

    // Велосипеди в редица вдясно (сами се връщат, ако ги оставиш)
    for (new i = 0; i < STATION_BIKES; i++) {
        new Float:dx = 8.0, Float:dy = -1.0 + i * 1.3;
        StationBike[city][i] = CreateVehicle((i % 2 == 0) ? 510 : 509, Tut_OffX(WP_STATION, city, dx, dy), Tut_OffY(WP_STATION, city, dx, dy),
            WorldPoint[WP_STATION][city][2], WorldPoint[WP_STATION][city][3] + 90.0, random(126), random(126), 180);
    }
    StationLabel[city][1] = CreateDynamic3DTextLabel(C_GREEN "БЕЗПЛАТНИ ВЕЛОСИПЕДИ\n" C_WHITE "за придвижване без книжка", COLOR_WHITE,
        Tut_OffX(WP_STATION, city, 8.0, 1.6), Tut_OffY(WP_STATION, city, 8.0, 1.6), WorldPoint[WP_STATION][city][2] + 1.0, 15.0,
        INVALID_PLAYER_ID, INVALID_VEHICLE_ID, 0, 0, 0);

    // Наставникът - вляво пред табелата
    new Float:mx = Tut_OffX(WP_STATION, city, -2.5, 3.0), Float:my = Tut_OffY(WP_STATION, city, -2.5, 3.0);
    MentorActor[city] = CreateActor(MENTOR_SKIN, mx, my, WorldPoint[WP_STATION][city][2], WorldPoint[WP_STATION][city][3] + 200.0);
    if (MentorActor[city] != INVALID_ACTOR_ID) SetActorInvulnerable(MentorActor[city], true);
    MentorLabel[city] = CreateDynamic3DTextLabel(C_BRAND "Наставник\n" C_WHITE "ALT - задачи за начинаещи", COLOR_WHITE, mx, my, WorldPoint[WP_STATION][city][2] + 1.1, 15.0,
        INVALID_PLAYER_ID, INVALID_VEHICLE_ID, 0, 0, 0);
}

static stock Tutorial_BuildClerk(city)
{
    if (ClerkActor[city] != INVALID_ACTOR_ID) DestroyActor(ClerkActor[city]);
    if (ClerkLabel[city] != Text3D:-1) DestroyDynamic3DTextLabel(ClerkLabel[city]);
    ClerkActor[city] = CreateActor(CLERK_SKIN, ClerkPos[0], ClerkPos[1], ClerkPos[2], ClerkPos[3]);
    if (ClerkActor[city] != INVALID_ACTOR_ID) {
        SetActorVirtualWorld(ClerkActor[city], HALL_WORLD_BASE + city);
        SetActorInvulnerable(ClerkActor[city], true);
    }
    ClerkLabel[city] = CreateDynamic3DTextLabel(C_BRAND "Гише: лични карти\n" C_WHITE "ALT", COLOR_WHITE, ClerkPos[0], ClerkPos[1], ClerkPos[2] + 1.1, 10.0,
        INVALID_PLAYER_ID, INVALID_VEHICLE_ID, 0, HALL_WORLD_BASE + city, HALL_INTERIOR);
}

// Админ: мести точка (kind) на града city тук
stock Tutorial_SetPoint(playerid, kind, city)
{
    GetPlayerPos(playerid, WorldPoint[kind][city][0], WorldPoint[kind][city][1], WorldPoint[kind][city][2]);
    GetPlayerFacingAngle(playerid, WorldPoint[kind][city][3]);
    new q[200];
    format(q, sizeof (q), "INSERT OR REPLACE INTO world_points (kind, city, x, y, z, a) VALUES (%d, %d, %f, %f, %f, %f)",
        kind, city, WorldPoint[kind][city][0], WorldPoint[kind][city][1], WorldPoint[kind][city][2], WorldPoint[kind][city][3]);
    Database_Exec(q);
    Tutorial_BuildPoint(kind, city);
    SendClientMessage(playerid, COLOR_ADMIN, "[A] %s (%s) е тук. Гледай натам, накъдето да е лицето на мястото.", WpNames[kind], CityNames[city]);
}

stock Tutorial_SetClerk(playerid)
{
    if (GetPlayerInterior(playerid) != HALL_INTERIOR) return SendError(playerid, "Влез в кметството и застани зад гишето.");
    GetPlayerPos(playerid, ClerkPos[0], ClerkPos[1], ClerkPos[2]);
    GetPlayerFacingAngle(playerid, ClerkPos[3]);
    new q[200];
    format(q, sizeof (q), "INSERT OR REPLACE INTO world_points (kind, city, x, y, z, a) VALUES (99, 0, %f, %f, %f, %f)", ClerkPos[0], ClerkPos[1], ClerkPos[2], ClerkPos[3]);
    Database_Exec(q);
    for (new c = 0; c < MAX_CITIES; c++) Tutorial_BuildClerk(c);
    SendClientMessage(playerid, COLOR_ADMIN, "[A] Служителката в кметствата стои тук и гледа натам, накъдето гледаш ти.");
    return 1;
}

// ---------- Спаун на гарата ----------

// Новак без гараж и бус - появява се на гарата
stock Tutorial_NeedsStation(playerid)
{
    return (TutStep[playerid] != TUT_DONE && TutStep[playerid] <= TUT_GARAGE) ? 1 : 0;
}

stock Tutorial_SpawnAtStation(playerid)
{
    new city = PlayerData[playerid][pGarageCity];
    PlayerData[playerid][pInGarage] = false;
    SetPlayerVirtualWorld(playerid, 0);
    SetPlayerInterior(playerid, 0);
    SetPlayerPos(playerid, WorldPoint[WP_STATION][city][0], WorldPoint[WP_STATION][city][1], WorldPoint[WP_STATION][city][2]);
    SetPlayerFacingAngle(playerid, WorldPoint[WP_STATION][city][3]);
    SetCameraBehindPlayer(playerid);
    if (TutStep[playerid] == TUT_ID) {
        GameTextForPlayer(playerid, "~y~WELCOME TO~n~~w~SAN ANDREAS", 4000, 1);
        SendClientMessage(playerid, COLOR_BRAND, "[УВОД] " C_WHITE "Пристигна с влака в " C_BRAND "%s" C_WHITE ". Говори с " C_BRAND "Наставника" C_WHITE " (ALT) до табелата.", CityNames[city]);
    }
}

// ---------- Задачи ----------

static stock Tutorial_Target(playerid, &Float:x, &Float:y, &Float:z, name[], size)
{
    new city = PlayerData[playerid][pGarageCity];
    switch (TutStep[playerid]) {
        case TUT_ID: {
            Tutorial_GetPoint(WP_HALL, city, x, y, z);
            format(name, size, "кметството");
        }
        case TUT_MEDICAL: {
            Tutorial_GetPoint(WP_HOSPITAL, city, x, y, z);
            format(name, size, "болницата");
        }
        case TUT_LICENSE: {
            School_GetEntrance(x, y, z);
            format(name, size, "автошколата");
        }
        case TUT_GARAGE: {
            new Float:a;
            City_GetEntrance(city, x, y, z, a);
            format(name, size, "гаража ти");
        }
        default: return 0;
    }
    return 1;
}

static stock Tutorial_ShowTask(playerid)
{
    new step = TutStep[playerid];
    if (step == TUT_DONE) return;
    new Float:x, Float:y, Float:z, name[32];
    if (Tutorial_Target(playerid, x, y, z, name, sizeof (name))) Gps_SetPoint(playerid, name, x, y, z);
    SendClientMessage(playerid, COLOR_BRAND, "[ЗАДАЧА %d/%d] " C_WHITE "%s.", step, TUT_STEPS, TutStepNames[step - 1]);
}

// Завършва текущата задача (ако е step) и дава следващата
static stock Tutorial_Complete(playerid, step)
{
    if (TutStep[playerid] != step) return;
    new money = TutRewardMoney[step - 1], exp = TutRewardExp[step - 1];
    PlayerPlaySound(playerid, 1058, 0.0, 0.0, 0.0);
    GameTextForPlayer(playerid, "~g~TASK COMPLETE!", 3000, 1);
    if (money > 0) Money_Give(playerid, money);
    Level_GiveExp(playerid, exp);
    SendClientMessage(playerid, COLOR_GREEN, "[ЗАДАЧА] " C_WHITE "Изпълнена: %s. Награда: " C_GREEN "$%d" C_WHITE ", опит " C_BRAND "+%d", TutStepNames[step - 1], money, exp);

    if (step >= TUT_STEPS) {
        TutStep[playerid] = TUT_DONE;
        Tutorial_SaveStep(playerid);
        ShowPlayerDialog(playerid, DIALOG_TUT_INFO, DIALOG_STYLE_MSGBOX, C_GREEN "Поздравления!",
            C_WHITE "Завърши всички задачи за начинаещи!\n\n" \
            C_WHITE "Вече си истински шофьор на " C_BRAND "CARGO STATE" C_WHITE ".\n" \
            C_WHITE "Карай, качвай нива, вземи по-големи категории и камиони,\n" \
            C_WHITE "купи си дом и компания. Приятен път!", "Да карам!", "");
        return;
    }
    TutStep[playerid] = step + 1;
    Tutorial_SaveStep(playerid);
    Tutorial_ShowTask(playerid);
}

// ---------- Наставникът ----------

stock Tutorial_ShowMentor(playerid)
{
    new step = TutStep[playerid];
    if (step == TUT_DONE) {
        ShowPlayerDialog(playerid, DIALOG_TUT_INFO, DIALOG_STYLE_MSGBOX, C_BRAND "Наставник",
            C_WHITE "Ти вече си опитен шофьор - нямам какво повече да те уча.\n\nВелосипедите са на твое разположение. Приятен път!", "Добре", "");
        return;
    }
    new title[64];
    format(title, sizeof (title), C_BRAND "Наставник" C_WHITE " | задача %d/%d", step, TUT_STEPS);
    if (step == TUT_LICENSE) {
        ShowPlayerDialog(playerid, DIALOG_TUT_MENTOR, DIALOG_STYLE_LIST, title,
            "Какво да правя?\nЗакарай ме до автошколата\nПокажи маршрута\n" C_GREY "Пропусни увода (за опитни играчи)", "Избери", "Затвори");
    } else {
        ShowPlayerDialog(playerid, DIALOG_TUT_MENTOR, DIALOG_STYLE_LIST, title,
            "Какво да правя?\n" C_GREY "-\nПокажи маршрута\n" C_GREY "Пропусни увода (за опитни играчи)", "Избери", "Затвори");
    }
}

static stock Tutorial_Hint(playerid)
{
    new step = TutStep[playerid], hint[300];
    switch (step) {
        case TUT_ID: format(hint, sizeof (hint), C_WHITE "Първо ти трябва " C_BRAND "лична карта" C_WHITE ". Вземи велосипед и иди до кметството -\nмаршрутът е на картата. Вътре иди на гишето и натисни ALT.");
        case TUT_MEDICAL: format(hint, sizeof (hint), C_WHITE "За книжката ти трябва " C_BRAND "медицинско" C_WHITE ". Иди до болницата\nи говори с доктора на входа (ALT).");
        case TUT_LICENSE: format(hint, sizeof (hint), C_WHITE "Сега - " C_BRAND "книжка B" C_WHITE " в автошколата в San Fierro.\nМога да те закарам дотам. На рецепцията избери категория B.");
        case TUT_GARAGE: format(hint, sizeof (hint), C_WHITE "Имаш книжка! Първият ти " C_BRAND "бус" C_WHITE " те чака в гаража ти.\nВлез в гаража (ALT на входа).");
        case TUT_DELIVERY: format(hint, sizeof (hint), C_WHITE "Седни в буса, натисни " C_BRAND "N" C_WHITE ", избери поръчка и я достави.\nТоварене и разтоварване - в червения маркер на компаниите.");
        case TUT_CARE: format(hint, sizeof (hint), C_WHITE "Грижи се за себе си и за буса: " C_BRAND "купи храна" C_WHITE " (магазин, бензиностанция)\nи " C_BRAND "зареди гориво" C_WHITE " на бензиностанция (ALT в буса).");
        default: format(hint, sizeof (hint), C_WHITE "Последно: отвори " C_BRAND "Борсата на товарите" C_WHITE " от менюто (Y),\nили " C_BRAND "кандидатствай" C_WHITE " в компания (ALT на базата й).");
    }
    ShowPlayerDialog(playerid, DIALOG_TUT_INFO, DIALOG_STYLE_MSGBOX, C_BRAND "Наставник", "%s", "Добре", "", hint);
}

// Меню > Задачи
stock Tutorial_ShowList(playerid)
{
    new body[1200], line[160];
    format(body, sizeof (body), "Задача\tСтатус\n");
    for (new i = 1; i <= TUT_STEPS; i++) {
        if (TutStep[playerid] == TUT_DONE || TutStep[playerid] > i) format(line, sizeof (line), C_GREY "%d. %s\t" C_GREEN "готово\n", i, TutStepNames[i - 1]);
        else if (TutStep[playerid] == i) format(line, sizeof (line), C_WHITE "%d. %s\t" C_BRAND "сега\n", i, TutStepNames[i - 1]);
        else format(line, sizeof (line), C_GREY "%d. %s\t" C_GREY "-\n", i, TutStepNames[i - 1]);
        strcat(body, line);
    }
    ShowPlayerDialog(playerid, DIALOG_TUT_LIST, DIALOG_STYLE_TABLIST_HEADERS, C_BRAND "Задачи за начинаещи", "%s", "Добре", "Назад", body);
}

// Пропуска увода: документи, книжка B и бус, без наградите
static stock Tutorial_Skip(playerid)
{
    Docs_IssueId(playerid);
    Docs_IssueMedical(playerid);
    License_SetBits(playerid, License_GetBits(playerid) | LIC_BIT(LIC_B));
    TutStep[playerid] = TUT_DONE;
    Tutorial_SaveStep(playerid);
    Veh_GiveStarter(playerid);
    SendClientMessage(playerid, COLOR_BRAND, "[УВОД] " C_WHITE "Уводът е пропуснат: имаш лична карта, медицинско, книжка B и бус в гаража.");
    new Float:x, Float:y, Float:z, Float:a;
    City_GetEntrance(PlayerData[playerid][pGarageCity], x, y, z, a);
    Gps_SetPoint(playerid, "гаража ти", x, y, z);
}

// ---------- Кметство и болница ----------

static stock Tutorial_EnterHall(playerid, city)
{
    SetPlayerInterior(playerid, HALL_INTERIOR);
    SetPlayerVirtualWorld(playerid, HALL_WORLD_BASE + city);
    SetPlayerPos(playerid, HALL_ENTRY_X, HALL_ENTRY_Y, HALL_ENTRY_Z);
    SetPlayerFacingAngle(playerid, HALL_ENTRY_A);
    SetCameraBehindPlayer(playerid);
    GameTextForPlayer(playerid, "~y~CITY HALL", 2500, 1);
}

static stock Tutorial_ExitHall(playerid, city)
{
    new Float:a = WorldPoint[WP_HALL][city][3];
    SetPlayerInterior(playerid, 0);
    SetPlayerVirtualWorld(playerid, 0);
    SetPlayerPos(playerid, Tut_OffX(WP_HALL, city, 0.0, 1.5), Tut_OffY(WP_HALL, city, 0.0, 1.5), WorldPoint[WP_HALL][city][2]);
    SetPlayerFacingAngle(playerid, a);
    SetCameraBehindPlayer(playerid);
}

static stock Tutorial_HallCity(playerid)
{
    new w = GetPlayerVirtualWorld(playerid) - HALL_WORLD_BASE;
    if (GetPlayerInterior(playerid) != HALL_INTERIOR || w < 0 || w >= MAX_CITIES) return -1;
    return w;
}

// Гишето: опашка (номер), после личната карта
static stock Tutorial_Clerk(playerid)
{
    if (Docs_HasId(playerid)) {
        ShowPlayerDialog(playerid, DIALOG_TUT_INFO, DIALOG_STYLE_MSGBOX, C_BRAND "Гише: лични карти",
            C_WHITE "Вече имаш лична карта. Можеш да я видиш от Меню > Документи.", "Добре", "");
        return;
    }
    if (HallBusy[playerid] > gettime()) {
        SendClientMessage(playerid, COLOR_GREY, "[КМЕТСТВО] " C_WHITE "Чакай да те извикат...");
        return;
    }
    new ticket = 10 + random(80);
    HallBusy[playerid] = gettime() + 8;
    SendClientMessage(playerid, COLOR_BRAND, "[КМЕТСТВО] " C_WHITE "Взе номер " C_BRAND "%d" C_WHITE ". Изчакай да те извикат на гишето.", ticket);
    GameTextForPlayer(playerid, "~y~TICKET %d~n~~w~PLEASE WAIT", 3000, 3, ticket);
    SetTimerEx("Tutorial_ClerkCall", 7000, false, "iii", playerid, PlayerData[playerid][pConnSeq], ticket);
}

forward Tutorial_ClerkCall(playerid, seq, ticket);
public Tutorial_ClerkCall(playerid, seq, ticket)
{
    if (!IsPlayerConnected(playerid) || PlayerData[playerid][pConnSeq] != seq) return 0;
    if (Tutorial_HallCity(playerid) == -1) return 0;
    PlayerPlaySound(playerid, 1137, 0.0, 0.0, 0.0);
    ShowPlayerDialog(playerid, DIALOG_TUT_IDCARD, DIALOG_STYLE_MSGBOX, C_BRAND "Гише 2",
        C_WHITE "Номер " C_BRAND "%d" C_WHITE ", заповядайте на гише 2.\n\n" \
        C_WHITE "- Добър ден! За лична карта ли сте? Трябва ми само снимка и подпис.\n" \
        C_WHITE "Първата лична карта е " C_GREEN "безплатна" C_WHITE ".",
        "Подпиши", "Откажи", ticket);
    return 1;
}

static stock Tutorial_Doctor(playerid)
{
    if (Docs_HasMedical(playerid)) {
        ShowPlayerDialog(playerid, DIALOG_TUT_INFO, DIALOG_STYLE_MSGBOX, C_RED "Доктор",
            C_WHITE "Ти вече имаш медицинско. Бъди здрав и почивай достатъчно!", "Добре", "");
        return;
    }
    if (!Docs_HasId(playerid)) {
        ShowPlayerDialog(playerid, DIALOG_TUT_INFO, DIALOG_STYLE_MSGBOX, C_RED "Доктор",
            C_WHITE "Без лична карта не мога да те прегледам. Първо иди в кметството.", "Добре", "");
        return;
    }
    ShowPlayerDialog(playerid, DIALOG_TUT_DOCTOR, DIALOG_STYLE_LIST, C_RED "Доктор: имате ли хронични заболявания?",
        "Не, здрав съм\nИмам алергия\nНе знам", "Отговори", "Отказ");
}

forward Tutorial_DoctorDone(playerid, seq);
public Tutorial_DoctorDone(playerid, seq)
{
    if (!IsPlayerConnected(playerid) || PlayerData[playerid][pConnSeq] != seq) return 0;
    TogglePlayerControllable(playerid, true);
    Docs_IssueMedical(playerid);
    Docs_Show(playerid, playerid, DOC_MEDICAL);
    SendClientMessage(playerid, COLOR_GREEN, "[БОЛНИЦА] " C_WHITE "Годен си за шофьор! Получи медицинско свидетелство.");
    Tutorial_Complete(playerid, TUT_MEDICAL);
    return 1;
}

// ---------- ALT пеша ----------

stock Tutorial_OnKeyStateChange(playerid, KEY:newkeys, KEY:oldkeys)
{
    if (!((newkeys & KEY_WALK) && !(oldkeys & KEY_WALK))) return 0;
    if (!PlayerData[playerid][pLogged] || IsPlayerInAnyVehicle(playerid)) return 0;

    new hall = Tutorial_HallCity(playerid);
    if (hall != -1) {
        if (IsPlayerInRangeOfPoint(playerid, 2.5, ClerkPos[0], ClerkPos[1], ClerkPos[2])) {
            Tutorial_Clerk(playerid);
            return 1;
        }
        if (IsPlayerInRangeOfPoint(playerid, WORLD_RANGE, HALL_ENTRY_X, HALL_ENTRY_Y, HALL_ENTRY_Z)) {
            Tutorial_ExitHall(playerid, hall);
            return 1;
        }
        return 0;
    }
    if (GetPlayerVirtualWorld(playerid) != 0 || GetPlayerInterior(playerid) != 0) return 0;
    for (new c = 0; c < MAX_CITIES; c++) {
        if (IsPlayerInRangeOfPoint(playerid, WORLD_RANGE, WorldPoint[WP_HALL][c][0], WorldPoint[WP_HALL][c][1], WorldPoint[WP_HALL][c][2])) {
            Tutorial_EnterHall(playerid, c);
            return 1;
        }
        if (IsPlayerInRangeOfPoint(playerid, WORLD_RANGE + 1.0, WorldPoint[WP_HOSPITAL][c][0], WorldPoint[WP_HOSPITAL][c][1], WorldPoint[WP_HOSPITAL][c][2])) {
            Tutorial_Doctor(playerid);
            return 1;
        }
        new Float:mx = Tut_OffX(WP_STATION, c, -2.5, 3.0), Float:my = Tut_OffY(WP_STATION, c, -2.5, 3.0);
        if (IsPlayerInRangeOfPoint(playerid, WORLD_RANGE, mx, my, WorldPoint[WP_STATION][c][2])) {
            Tutorial_ShowMentor(playerid);
            return 1;
        }
    }
    return 0;
}

// ---------- Куки от другите модули ----------

stock Tutorial_OnLicense(playerid, cat)
{
    if (cat != LIC_B || TutStep[playerid] != TUT_LICENSE) return;
    Tutorial_Complete(playerid, TUT_LICENSE);
    ShowPlayerDialog(playerid, DIALOG_TUT_RIDE, DIALOG_STYLE_MSGBOX, C_BRAND "Наставник",
        C_WHITE "Браво, вече имаш книжка B!\n\nПървият ти бус те чака в гаража в " C_BRAND "%s" C_WHITE ".\nДа те закарам ли дотам?",
        "Закарай ме", "Ще отида сам", CityNames[PlayerData[playerid][pGarageCity]]);
}

// Влизане в гаража: на задача 4 буса е там
stock Tutorial_OnEnterGarage(playerid)
{
    if (TutStep[playerid] != TUT_GARAGE) return;
    Veh_GiveStarter(playerid);
    Tutorial_Complete(playerid, TUT_GARAGE);
    SendClientMessage(playerid, COLOR_BRAND, "[УВОД] " C_WHITE "Седни в буса, запали с " C_BRAND "CTRL" C_WHITE " и излез с " C_BRAND "H" C_WHITE ". После " C_BRAND "N" C_WHITE " - поръчки.");
}

stock Tutorial_OnDelivery(playerid)
{
    Tutorial_Complete(playerid, TUT_DELIVERY);
}

stock Tutorial_OnFood(playerid)
{
    if (TutStep[playerid] != TUT_CARE) return;
    TutFood[playerid] = true;
    if (TutFuel[playerid]) Tutorial_Complete(playerid, TUT_CARE);
    else SendClientMessage(playerid, COLOR_BRAND, "[ЗАДАЧА] " C_WHITE "Храна - готово. Остава да заредиш гориво.");
}

stock Tutorial_OnRefuel(playerid)
{
    if (TutStep[playerid] != TUT_CARE) return;
    TutFuel[playerid] = true;
    if (TutFood[playerid]) Tutorial_Complete(playerid, TUT_CARE);
    else SendClientMessage(playerid, COLOR_BRAND, "[ЗАДАЧА] " C_WHITE "Гориво - готово. Остава да си купиш храна.");
}

stock Tutorial_OnCareer(playerid)
{
    Tutorial_Complete(playerid, TUT_CAREER);
}

// ---------- Диалози ----------

stock Tutorial_OnDialogResponse(playerid, dialogid, response, listitem)
{
    switch (dialogid) {
        case DIALOG_TUT_INFO: return 1;
        case DIALOG_TUT_LIST: {
            if (!response) Menu_Show(playerid);
            return 1;
        }
        case DIALOG_TUT_MENTOR: {
            if (!response) return 1;
            switch (listitem) {
                case 0: Tutorial_Hint(playerid);
                case 1: {
                    if (TutStep[playerid] != TUT_LICENSE) return 1;
                    new Float:x, Float:y, Float:z;
                    School_GetEntrance(x, y, z);
                    if (IsPlayerInAnyVehicle(playerid)) RemovePlayerFromVehicle(playerid);
                    SetPlayerVirtualWorld(playerid, 0);
                    SetPlayerInterior(playerid, 0);
                    SetPlayerPos(playerid, x, y, z);
                    GameTextForPlayer(playerid, "~y~DRIVING SCHOOL", 3000, 1);
                    SendClientMessage(playerid, COLOR_BRAND, "[УВОД] " C_WHITE "Ето я автошколата. Влез (ALT) и избери " C_BRAND "категория B" C_WHITE " на рецепцията.");
                }
                case 2: Tutorial_ShowTask(playerid);
                case 3: ShowPlayerDialog(playerid, DIALOG_TUT_SKIP, DIALOG_STYLE_MSGBOX, C_BRAND "Пропусни увода",
                    C_WHITE "Получаваш лична карта, медицинско, книжка B и бус,\nно " C_RED "без наградите" C_WHITE " за задачите.\n\nСигурен ли си?", "Пропусни", "Назад");
            }
            return 1;
        }
        case DIALOG_TUT_SKIP: {
            if (response && TutStep[playerid] != TUT_DONE) Tutorial_Skip(playerid);
            else if (!response) Tutorial_ShowMentor(playerid);
            return 1;
        }
        case DIALOG_TUT_IDCARD: {
            if (!response) return 1;
            if (Tutorial_HallCity(playerid) == -1) return 1;
            if (!Docs_IssueId(playerid)) return 1;
            Docs_Show(playerid, playerid, DOC_ID);
            SendClientMessage(playerid, COLOR_GREEN, "[КМЕТСТВО] " C_WHITE "Получи лична карта! Виж я по всяко време от " C_BRAND "Меню > Документи" C_WHITE ".");
            Tutorial_Complete(playerid, TUT_ID);
            return 1;
        }
        case DIALOG_TUT_DOCTOR: {
            if (!response) return 1;
            TogglePlayerControllable(playerid, false);
            GameTextForPlayer(playerid, "~w~MEDICAL CHECK...", 5000, 3);
            SendClientMessage(playerid, COLOR_RED, "[БОЛНИЦА] " C_WHITE "Доктор: \"Дишайте дълбоко... кръвно - добре... зрение - отлично.\"");
            SetTimerEx("Tutorial_DoctorDone", 5000, false, "ii", playerid, PlayerData[playerid][pConnSeq]);
            return 1;
        }
        case DIALOG_TUT_RIDE: {
            if (!response) {
                Tutorial_ShowTask(playerid);
                return 1;
            }
            new Float:x, Float:y, Float:z, Float:a;
            City_GetEntrance(PlayerData[playerid][pGarageCity], x, y, z, a);
            if (IsPlayerInAnyVehicle(playerid)) RemovePlayerFromVehicle(playerid);
            SetPlayerVirtualWorld(playerid, 0);
            SetPlayerInterior(playerid, 0);
            SetPlayerPos(playerid, x, y, z);
            SetPlayerFacingAngle(playerid, a);
            SetCameraBehindPlayer(playerid);
            SendClientMessage(playerid, COLOR_BRAND, "[УВОД] " C_WHITE "Ето го гаражът ти. Влез с " C_BRAND "ALT" C_WHITE " - бусът те чака.");
            return 1;
        }
    }
    return 0;
}
