// Автошколата в San Fierro - единствената на сървъра.
//
// - Вход отвън (ALT пеша) -> интериорът от maps/school.inc (собствен виртуален свят).
// - На рецепцията стои рецепционист (NPC): ALT пред плота -> избор на курс (licenses.inc).
// - Теорията се пише на бюро в стаята за теория; практиката започва навън, при изпитния камион.
// - Админ: Места на картата -> "Премести входа на автошколата тук" и "Изпитният камион тръгва оттук".

#define SCHOOL_ENTER_RANGE      2.5
#define SCHOOL_DESK_RANGE       2.0

// По подразбиране: сградата на автошколата в Doherty, San Fierro (админ може да ги премести)
static Float:SchoolEntrance[4] = {-2026.6, -100.6, 35.2, 90.0};
static Float:SchoolVehSpawn[4] = {-2017.0, -100.6, 35.2, 90.0};

static SchoolPickup = -1;
static Text3D:SchoolLabel = Text3D:-1;
static SchoolMapIcon = -1;
static SchoolActor = INVALID_ACTOR_ID;
static SchoolSeatNext;
static SchoolDoorObj[sizeof (SchoolDoors)];
static bool:SchoolDoorOpen[sizeof (SchoolDoors)];

stock School_Init()
{
    Database_Exec("CREATE TABLE IF NOT EXISTS school_points (id INTEGER PRIMARY KEY, x REAL NOT NULL, y REAL NOT NULL, z REAL NOT NULL, a REAL NOT NULL)");
    new DBResult:r = Database_Query("SELECT id, x, y, z, a FROM school_points");
    if (r != DBResult:0) {
        new rows = DB_GetRowCount(r);
        for (new i = 0; i < rows; i++) {
            new id = DB_GetFieldIntByName(r, "id");
            if (id == 0) {
                SchoolEntrance[0] = DB_GetFieldFloatByName(r, "x");
                SchoolEntrance[1] = DB_GetFieldFloatByName(r, "y");
                SchoolEntrance[2] = DB_GetFieldFloatByName(r, "z");
                SchoolEntrance[3] = DB_GetFieldFloatByName(r, "a");
            } else if (id == 1) {
                SchoolVehSpawn[0] = DB_GetFieldFloatByName(r, "x");
                SchoolVehSpawn[1] = DB_GetFieldFloatByName(r, "y");
                SchoolVehSpawn[2] = DB_GetFieldFloatByName(r, "z");
                SchoolVehSpawn[3] = DB_GetFieldFloatByName(r, "a");
            }
            DB_SelectNextRow(r);
        }
        DB_FreeResultSet(r);
    }

    School_CreateMap();
    School_CreateOutside();
    for (new d = 0; d < sizeof (SchoolDoors); d++) {
        SchoolDoorObj[d] = CreateDynamicObject(SCHOOL_DOOR_MODEL, SCHOOL_X + SchoolDoors[d][0], SCHOOL_Y + SchoolDoors[d][1], SCHOOL_Z,
            0.0, 0.0, SchoolDoors[d][2], SCHOOL_WORLD, -1, -1, 200.0, 200.0);
        SchoolDoorOpen[d] = false;
    }

    // Рецепционистката
    SchoolActor = CreateActor(SCHOOL_NPC_SKIN, SCHOOL_X + SCHOOL_DESK_X, SCHOOL_Y + SCHOOL_NPC_Y, SCHOOL_Z + 1.0, 180.0);
    if (SchoolActor != INVALID_ACTOR_ID) {
        SetActorVirtualWorld(SchoolActor, SCHOOL_WORLD);
        SetActorInvulnerable(SchoolActor, true);
    }
    CreateDynamic3DTextLabel(C_WHITE "Рецепционист", COLOR_WHITE, SCHOOL_X + SCHOOL_DESK_X, SCHOOL_Y + SCHOOL_NPC_Y, SCHOOL_Z + 2.15, 8.0,
        INVALID_PLAYER_ID, INVALID_VEHICLE_ID, 0, SCHOOL_WORLD, -1);
}

static stock School_CreateOutside()
{
    if (SchoolPickup != -1) DestroyDynamicPickup(SchoolPickup);
    if (SchoolLabel != Text3D:-1) DestroyDynamic3DTextLabel(SchoolLabel);
    if (SchoolMapIcon != -1) DestroyDynamicMapIcon(SchoolMapIcon);

    SchoolPickup = CreateDynamicPickup(1239, 1, SchoolEntrance[0], SchoolEntrance[1], SchoolEntrance[2], 0, 0);
    SchoolLabel = CreateDynamic3DTextLabel(C_BRAND "АВТОШКОЛА\n" C_WHITE "Категории C, CE и ADR\n" C_GREY "ALT - влез", COLOR_WHITE,
        SchoolEntrance[0], SchoolEntrance[1], SchoolEntrance[2] + 0.8, 20.0, INVALID_PLAYER_ID, INVALID_VEHICLE_ID, 1, 0, 0);
    SchoolMapIcon = CreateDynamicMapIcon(SchoolEntrance[0], SchoolEntrance[1], SchoolEntrance[2], 36, 0, 0, 0, -1, 3000.0, MAPICON_LOCAL);
}

static stock School_SavePoint(id, const Float:p[4])
{
    new q[200];
    format(q, sizeof (q), "INSERT OR REPLACE INTO school_points (id, x, y, z, a) VALUES (%d, %f, %f, %f, %f)", id, p[0], p[1], p[2], p[3]);
    Database_Exec(q);
}

// ---------- За другите модули ----------

stock bool:School_IsInside(playerid)
{
    return (GetPlayerVirtualWorld(playerid) == SCHOOL_WORLD);
}

stock School_GetEntrance(&Float:x, &Float:y, &Float:z)
{
    x = SchoolEntrance[0];
    y = SchoolEntrance[1];
    z = SchoolEntrance[2];
}

stock School_GetVehicleSpawn(&Float:x, &Float:y, &Float:z, &Float:a)
{
    x = SchoolVehSpawn[0];
    y = SchoolVehSpawn[1];
    z = SchoolVehSpawn[2];
    a = SchoolVehSpawn[3];
}

// ---------- Автоматични врати ----------

// Всяка секунда: врата се отваря, щом някой е до отвора, и се затваря, щом няма никой
stock School_DoorTick()
{
    for (new d = 0; d < sizeof (SchoolDoors); d += 2) {
        // Двете крила на един отвор: центърът е между пантите
        new Float:cx = SCHOOL_X + (SchoolDoors[d][0] + SchoolDoors[d + 1][0]) / 2.0;
        new Float:cy = SCHOOL_Y + SchoolDoors[d][1];
        new bool:near = false;
        for (new i = 0; i < MAX_PLAYERS; i++) {
            if (!IsPlayerConnected(i) || GetPlayerVirtualWorld(i) != SCHOOL_WORLD) continue;
            if (IsPlayerInRangeOfPoint(i, 2.8, cx, cy, SCHOOL_Z + 1.0)) {
                near = true;
                break;
            }
        }
        if (near == SchoolDoorOpen[d]) continue;
        for (new k = d; k <= d + 1; k++) {
            SchoolDoorOpen[k] = near;
            // Отворена: и двете крила сочат на север (навътре в стаята)
            SetDynamicObjectRot(SchoolDoorObj[k], 0.0, 0.0, near ? 90.0 : SchoolDoors[k][2]);
        }
    }
}

// ---------- Вход и изход ----------

static stock School_Enter(playerid)
{
    new Float:x = SCHOOL_X + SCHOOL_DOOR_X, Float:y = SCHOOL_Y + SCHOOL_DOOR_Y + 0.6, Float:z = SCHOOL_Z + 1.0;
    Gps_Cancel(playerid);
    SetPlayerVirtualWorld(playerid, SCHOOL_WORLD);
    SetPlayerInterior(playerid, 0);
    Streamer_UpdateEx(playerid, x, y, z, SCHOOL_WORLD, 0);
    SetPlayerPos(playerid, x, y, z);
    SetPlayerFacingAngle(playerid, 0.0);
    SetCameraBehindPlayer(playerid);
    GameTextForPlayer(playerid, "~y~DRIVING SCHOOL", 2500, 1);
}

static stock School_Exit(playerid)
{
    new Float:a = SchoolEntrance[3];
    // 1.5 м пред вратата, с гръб към нея
    new Float:x = SchoolEntrance[0] + 1.5 * floatsin(-a, degrees), Float:y = SchoolEntrance[1] + 1.5 * floatcos(-a, degrees);
    SetPlayerVirtualWorld(playerid, 0);
    SetPlayerInterior(playerid, 0);
    Streamer_UpdateEx(playerid, x, y, SchoolEntrance[2], 0, 0);
    SetPlayerPos(playerid, x, y, SchoolEntrance[2]);
    SetPlayerFacingAngle(playerid, a);
    SetCameraBehindPlayer(playerid);
}

// ---------- Теория и изпит ----------

// Сяда на свободно бюро в стаята за теория (замразен, докато пише)
stock School_SeatForTheory(playerid)
{
    if (!School_IsInside(playerid)) return;
    new s = SchoolSeatNext++ % sizeof (SchoolSeats);
    SetPlayerPos(playerid, SCHOOL_X + SchoolSeats[s][0], SCHOOL_Y + SchoolSeats[s][1], SCHOOL_Z + 1.0);
    SetPlayerFacingAngle(playerid, 0.0);
    SetCameraBehindPlayer(playerid);
    TogglePlayerControllable(playerid, false);
}

stock School_StandUp(playerid)
{
    TogglePlayerControllable(playerid, true);
}

// От стаята за теория - навън, до изпитния камион
stock School_LeaveForExam(playerid, Float:x, Float:y, Float:z, Float:a)
{
    TogglePlayerControllable(playerid, true);
    SetPlayerVirtualWorld(playerid, 0);
    SetPlayerInterior(playerid, 0);
    Streamer_UpdateEx(playerid, x, y, z, 0, 0);
    SetPlayerPos(playerid, x + 3.0 * floatcos(-a, degrees), y - 3.0 * floatsin(-a, degrees), z);
}

// ---------- ALT пеша ----------

stock School_OnKeyStateChange(playerid, KEY:newkeys, KEY:oldkeys)
{
    if (!((newkeys & KEY_WALK) && !(oldkeys & KEY_WALK))) return 0;
    if (!PlayerData[playerid][pLogged] || IsPlayerInAnyVehicle(playerid)) return 0;

    if (School_IsInside(playerid)) {
        if (IsPlayerInRangeOfPoint(playerid, SCHOOL_DESK_RANGE, SCHOOL_X + SCHOOL_DESK_X, SCHOOL_Y + SCHOOL_DESK_Y, SCHOOL_Z + 1.0)) {
            if (SchoolActor != INVALID_ACTOR_ID) ApplyActorAnimation(SchoolActor, "PED", "IDLE_CHAT", 4.1, false, false, false, false, 0);
            License_ShowSchool(playerid);
            return 1;
        }
        if (IsPlayerInRangeOfPoint(playerid, SCHOOL_ENTER_RANGE, SCHOOL_X + SCHOOL_DOOR_X, SCHOOL_Y + SCHOOL_DOOR_Y, SCHOOL_Z + 1.0)) {
            School_Exit(playerid);
            return 1;
        }
        return 0;
    }
    if (GetPlayerVirtualWorld(playerid) == 0
        && IsPlayerInRangeOfPoint(playerid, SCHOOL_ENTER_RANGE, SchoolEntrance[0], SchoolEntrance[1], SchoolEntrance[2])) {
        School_Enter(playerid);
        return 1;
    }
    return 0;
}

// ---------- Админ ----------

stock School_SetEntrance(playerid)
{
    GetPlayerPos(playerid, SchoolEntrance[0], SchoolEntrance[1], SchoolEntrance[2]);
    GetPlayerFacingAngle(playerid, SchoolEntrance[3]);
    School_SavePoint(0, SchoolEntrance);
    School_CreateOutside();
    SendClientMessage(playerid, COLOR_ADMIN, "[A] Входът на автошколата е тук. Гледай навън от вратата - натам излизат играчите.");
}

stock School_SetVehicleSpawn(playerid)
{
    if (IsPlayerInAnyVehicle(playerid)) {
        new v = GetPlayerVehicleID(playerid);
        GetVehiclePos(v, SchoolVehSpawn[0], SchoolVehSpawn[1], SchoolVehSpawn[2]);
        GetVehicleZAngle(v, SchoolVehSpawn[3]);
    } else {
        GetPlayerPos(playerid, SchoolVehSpawn[0], SchoolVehSpawn[1], SchoolVehSpawn[2]);
        GetPlayerFacingAngle(playerid, SchoolVehSpawn[3]);
    }
    School_SavePoint(1, SchoolVehSpawn);
    SendClientMessage(playerid, COLOR_ADMIN, "[A] Изпитният камион ще тръгва оттук, с лице натам, накъдето гледаш. Тук е и финалът.");
}
