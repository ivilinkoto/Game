// Махнати обекти от картата на играта (напр. портата пред автошколата).
//
// Админ: Места на картата -> "Махни обекта пред мен" (с радиус). Пази се в базата и се маха за
// всеки играч при влизане (RemoveBuildingForPlayer). Маха всичко, чийто център е в радиуса.
// Играта показва промяната сигурно чак след повторно влизане.

#define MAX_REMOVED_BUILDINGS   100
#define REMOVE_AHEAD            3.0         // точката е толкова пред админа

static Float:RemovedPos[MAX_REMOVED_BUILDINGS][4];     // x, y, z, радиус
static RemovedID[MAX_REMOVED_BUILDINGS];
static RemovedCount;

// Махнати винаги, по модел и точни координати (от Prineside model ID database):
// модел, x, y, z, радиус
enum E_REMOVED_FIXED {
    rfModel,
    Float:rfX,
    Float:rfY,
    Float:rfZ,
    Float:rfRadius
}
static const RemovedFixed[][E_REMOVED_FIXED] = {
    {11014, -2076.44, -107.93, 36.9688, 1.0}       // портата и покривът на двора на автошколата (drivingschlgrg_SFS)
};

stock Removed_Init()
{
    Database_Exec("CREATE TABLE IF NOT EXISTS removed_buildings (id INTEGER PRIMARY KEY AUTOINCREMENT, x REAL NOT NULL, y REAL NOT NULL, z REAL NOT NULL, radius REAL NOT NULL)");
    // Еднократно: връщаме всичко, махнато с радиус до момента (беше махнало земя и сгради)
    if (Database_GetMeta("removed_reset_1") == 0) {
        Database_Exec("DELETE FROM removed_buildings");
        Database_SetMeta("removed_reset_1", 1);
    }
    RemovedCount = 0;
    new DBResult:r = Database_Query("SELECT id, x, y, z, radius FROM removed_buildings ORDER BY id");
    if (r == DBResult:0) return;
    new rows = DB_GetRowCount(r);
    for (new i = 0; i < rows && RemovedCount < MAX_REMOVED_BUILDINGS; i++) {
        RemovedID[RemovedCount] = DB_GetFieldIntByName(r, "id");
        RemovedPos[RemovedCount][0] = DB_GetFieldFloatByName(r, "x");
        RemovedPos[RemovedCount][1] = DB_GetFieldFloatByName(r, "y");
        RemovedPos[RemovedCount][2] = DB_GetFieldFloatByName(r, "z");
        RemovedPos[RemovedCount][3] = DB_GetFieldFloatByName(r, "radius");
        RemovedCount++;
        DB_SelectNextRow(r);
    }
    DB_FreeResultSet(r);
}

// OnPlayerConnect - преди играчът да е заредил картата
stock Removed_OnPlayerConnect(playerid)
{
    for (new i = 0; i < sizeof (RemovedFixed); i++) {
        RemoveBuildingForPlayer(playerid, RemovedFixed[i][rfModel], RemovedFixed[i][rfX], RemovedFixed[i][rfY], RemovedFixed[i][rfZ], RemovedFixed[i][rfRadius]);
    }
    for (new i = 0; i < RemovedCount; i++) {
        RemoveBuildingForPlayer(playerid, -1, RemovedPos[i][0], RemovedPos[i][1], RemovedPos[i][2], RemovedPos[i][3]);
    }
}

stock Removed_AddAhead(playerid, Float:radius)
{
    if (RemovedCount >= MAX_REMOVED_BUILDINGS) return SendError(playerid, "Достигнат е максимумът.");
    new Float:x, Float:y, Float:z, Float:a;
    GetPlayerPos(playerid, x, y, z);
    GetPlayerFacingAngle(playerid, a);
    x += REMOVE_AHEAD * floatsin(-a, degrees);
    y += REMOVE_AHEAD * floatcos(-a, degrees);

    new q[200];
    format(q, sizeof (q), "INSERT INTO removed_buildings (x, y, z, radius) VALUES (%f, %f, %f, %f)", x, y, z, radius);
    Database_Exec(q);
    new k = RemovedCount++;
    RemovedPos[k][0] = x;
    RemovedPos[k][1] = y;
    RemovedPos[k][2] = z;
    RemovedPos[k][3] = radius;
    RemovedID[k] = 0;
    new DBResult:r = Database_Query("SELECT last_insert_rowid() AS id");
    if (r != DBResult:0) {
        if (DB_GetRowCount(r) > 0) RemovedID[k] = DB_GetFieldIntByName(r, "id");
        DB_FreeResultSet(r);
    }
    for (new i = 0; i < MAX_PLAYERS; i++) {
        if (IsPlayerConnected(i)) RemoveBuildingForPlayer(i, -1, x, y, z, radius);
    }
    SendClientMessage(playerid, COLOR_ADMIN, "[A] Махнато е всичко с център до %.0f м от точка 3 м пред теб. Излез и влез отново, за да го видиш.", radius);
    return 1;
}

stock Removed_RestoreLast(playerid)
{
    if (RemovedCount == 0) return SendError(playerid, "Няма махнати обекти.");
    new k = --RemovedCount, q[96];
    format(q, sizeof (q), "DELETE FROM removed_buildings WHERE id = %d", RemovedID[k]);
    Database_Exec(q);
    SendClientMessage(playerid, COLOR_ADMIN, "[A] Последният махнат обект е върнат. Вижда се след рестарт на сървъра и повторно влизане.");
    return 1;
}
