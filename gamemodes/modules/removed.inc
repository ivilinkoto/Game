// Махнати обекти от картата на играта (напр. портата пред автошколата).
//
// Админ: Места на картата -> "Махни обекта пред мен". Пази се в базата и се маха за всеки
// играч при влизане (RemoveBuildingForPlayer). Маха всичко, чийто център е в радиуса.

#define MAX_REMOVED_BUILDINGS   100
#define REMOVE_AHEAD            3.0         // точката е толкова пред админа
#define REMOVE_RADIUS           5.0

static Float:RemovedPos[MAX_REMOVED_BUILDINGS][4];     // x, y, z, радиус
static RemovedCount;

stock Removed_Init()
{
    Database_Exec("CREATE TABLE IF NOT EXISTS removed_buildings (id INTEGER PRIMARY KEY AUTOINCREMENT, x REAL NOT NULL, y REAL NOT NULL, z REAL NOT NULL, radius REAL NOT NULL)");
    RemovedCount = 0;
    new DBResult:r = Database_Query("SELECT x, y, z, radius FROM removed_buildings ORDER BY id");
    if (r == DBResult:0) return;
    new rows = DB_GetRowCount(r);
    for (new i = 0; i < rows && RemovedCount < MAX_REMOVED_BUILDINGS; i++) {
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
    for (new i = 0; i < RemovedCount; i++) {
        RemoveBuildingForPlayer(playerid, -1, RemovedPos[i][0], RemovedPos[i][1], RemovedPos[i][2], RemovedPos[i][3]);
    }
}

stock Removed_AddAhead(playerid)
{
    if (RemovedCount >= MAX_REMOVED_BUILDINGS) return SendError(playerid, "Достигнат е максимумът.");
    new Float:x, Float:y, Float:z, Float:a;
    GetPlayerPos(playerid, x, y, z);
    GetPlayerFacingAngle(playerid, a);
    x += REMOVE_AHEAD * floatsin(-a, degrees);
    y += REMOVE_AHEAD * floatcos(-a, degrees);

    new k = RemovedCount++;
    RemovedPos[k][0] = x;
    RemovedPos[k][1] = y;
    RemovedPos[k][2] = z;
    RemovedPos[k][3] = REMOVE_RADIUS;
    new q[200];
    format(q, sizeof (q), "INSERT INTO removed_buildings (x, y, z, radius) VALUES (%f, %f, %f, %f)", x, y, z, REMOVE_RADIUS);
    Database_Exec(q);
    for (new i = 0; i < MAX_PLAYERS; i++) {
        if (IsPlayerConnected(i)) RemoveBuildingForPlayer(i, -1, x, y, z, REMOVE_RADIUS);
    }
    SendClientMessage(playerid, COLOR_ADMIN, "[A] Махнато е всичко от картата в %.0f м пред теб. Ако още го виждаш, излез и влез отново.", REMOVE_RADIUS);
    return 1;
}

stock Removed_RestoreAll(playerid)
{
    Database_Exec("DELETE FROM removed_buildings");
    RemovedCount = 0;
    SendClientMessage(playerid, COLOR_ADMIN, "[A] Всички махнати обекти са върнати. Вижда се след рестарт на сървъра и повторно влизане.");
}
