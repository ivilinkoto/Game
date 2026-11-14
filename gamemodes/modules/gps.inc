// GPS – отваря се от менюто (Y -> GPS) или с /gps.
//
// Откъде идват местата:
//  1. Работи – автоматично от точките "job" на всяка работа (core/jobs.inc).
//     Нова работа със поставена точка "job" се появява в GPS сама.
//  2. Вградени места – списъкът GpsBuiltIn по-долу.
//  3. Места, добавени от админите в играта:
//       /gpsadd [категория 1-4] [име]  – добавя мястото, където стоиш
//       /gpslist                        – списък с номерата
//       /gpsdel [номер]                 – премахва място

#define GPS_MAX_LIST    20
#define GPS_NAME_LEN    40

enum {
    GPS_CAT_JOBS,
    GPS_CAT_IMPORTANT,
    GPS_CAT_SERVICES,
    GPS_CAT_FUN,
    MAX_GPS_CATEGORIES
}

static const GpsCategoryName[MAX_GPS_CATEGORIES][] = {
    "Работи",
    "Важни места",
    "Магазини и услуги",
    "Забавления"
};

// Вградени места. Добавяй ред във всеки от трите списъка (на една и съща позиция).
static const GpsBuiltInName[][] = {
    "Гара Unity (начало)",
    "Болница All Saints",
    "Кметство на Los Santos",
    "Полицейско управление (LSPD)"
};

static const GpsBuiltInCategory[] = {
    GPS_CAT_IMPORTANT,
    GPS_CAT_IMPORTANT,
    GPS_CAT_IMPORTANT,
    GPS_CAT_IMPORTANT
};

static const Float:GpsBuiltInPos[][3] = {
    {1742.9, -1861.0, 13.58},
    {1176.0, -1323.5, 14.10},
    {1481.0, -1772.3, 18.80},
    {1554.9, -1675.6, 16.20}
};

// Списъкът, показан на играча в момента (за да знаем кое е избрал)
static Float:GpsListX[MAX_PLAYERS][GPS_MAX_LIST],
    Float:GpsListY[MAX_PLAYERS][GPS_MAX_LIST],
    Float:GpsListZ[MAX_PLAYERS][GPS_MAX_LIST],
    GpsListName[MAX_PLAYERS][GPS_MAX_LIST][GPS_NAME_LEN],
    GpsListCount[MAX_PLAYERS],
    bool:GpsActive[MAX_PLAYERS],
    bool:GpsMainHasOff[MAX_PLAYERS];

stock Gps_Init()
{
    Database_Exec("CREATE TABLE IF NOT EXISTS gps_locations (id INTEGER PRIMARY KEY AUTOINCREMENT, category INTEGER NOT NULL, name TEXT NOT NULL, x REAL NOT NULL, y REAL NOT NULL, z REAL NOT NULL)");
}

stock Gps_ResetPlayer(playerid)
{
    GpsListCount[playerid] = 0;
    GpsActive[playerid] = false;
    GpsMainHasOff[playerid] = false;
}

// Задава цел. Може да се вика и от други модули (напр. фермата).
stock Gps_SetTarget(playerid, const name[], Float:x, Float:y, Float:z)
{
    DisablePlayerCheckpoint(playerid);
    SetPlayerCheckpoint(playerid, x, y, z, 4.0);
    GpsActive[playerid] = true;

    new Float:dist = GetPlayerDistanceFromPoint(playerid, x, y, z);
    SendClientMessage(playerid, COLOR_GREEN, "[GPS] {FFFFFF}Маршрут до {FFD54A}%s{FFFFFF} (%.1f км). Следвай червената точка на картата.",
        name, dist / 1000.0);
}

stock Gps_Stop(playerid)
{
    if (!GpsActive[playerid]) return;
    DisablePlayerCheckpoint(playerid);
    GpsActive[playerid] = false;
}

// Връща true, ако чекпойнтът е на GPS-а.
stock bool:Gps_OnEnterCheckpoint(playerid)
{
    if (!GpsActive[playerid]) return false;
    Gps_Stop(playerid);
    SendClientMessage(playerid, COLOR_GREEN, "[GPS] {FFFFFF}Пристигна!");
    return true;
}

// ------------------------------------------------------------
//  Менюта
// ------------------------------------------------------------

stock Gps_ShowMain(playerid)
{
    new body[512];
    body[0] = EOS;
    for (new c = 0; c < MAX_GPS_CATEGORIES; c++) {
        strcat(body, C_BRAND "» " C_WHITE);
        strcat(body, GpsCategoryName[c]);
        strcat(body, "\n");
    }
    GpsMainHasOff[playerid] = GpsActive[playerid];
    if (GpsActive[playerid]) strcat(body, C_RED "» " C_WHITE "Изключи GPS");
    ShowPlayerDialog(playerid, DIALOG_GPS_MAIN, DIALOG_STYLE_LIST, C_BRAND "GPS " C_WHITE "| Къде отиваш?", "%s", "Избери", "Затвори", body);
}

static stock Gps_AddToList(playerid, const name[], Float:x, Float:y, Float:z)
{
    new i = GpsListCount[playerid];
    if (i >= GPS_MAX_LIST) return;
    format(GpsListName[playerid][i], GPS_NAME_LEN, "%s", name);
    GpsListX[playerid][i] = x;
    GpsListY[playerid][i] = y;
    GpsListZ[playerid][i] = z;
    GpsListCount[playerid]++;
}

stock Gps_ShowCategory(playerid, category)
{
    GpsListCount[playerid] = 0;

    // 1. Работи – от точките "job"
    if (category == GPS_CAT_JOBS) {
        new Float:x, Float:y, Float:z, Float:a, jobName[32], name[GPS_NAME_LEN];
        for (new j = 1; j < MAX_JOBS; j++) {
            if (!Job_GetPoint(j, "job", x, y, z, a)) continue;
            Job_GetName(j, jobName);
            if (Job_GetType(j) == JOB_TYPE_STARTER) format(name, sizeof (name), "%s (начална)", jobName);
            else format(name, sizeof (name), "%s (стандартна)", jobName);
            Gps_AddToList(playerid, name, x, y, z);
        }
    }

    // 2. Вградени места
    for (new i = 0; i < sizeof (GpsBuiltInName); i++) {
        if (GpsBuiltInCategory[i] != category) continue;
        Gps_AddToList(playerid, GpsBuiltInName[i], GpsBuiltInPos[i][0], GpsBuiltInPos[i][1], GpsBuiltInPos[i][2]);
    }

    // 3. Места от админите
    new q[128];
    format(q, sizeof (q), "SELECT name, x, y, z FROM gps_locations WHERE category = %d ORDER BY name", category);
    new DBResult:result = Database_Query(q);
    if (result != DBResult:0) {
        if (DB_GetRowCount(result) > 0) {
            new name[GPS_NAME_LEN];
            do {
                DB_GetFieldStringByName(result, "name", name);
                Gps_AddToList(playerid, name, DB_GetFieldFloatByName(result, "x"), DB_GetFieldFloatByName(result, "y"), DB_GetFieldFloatByName(result, "z"));
            } while (DB_SelectNextRow(result));
        }
        DB_FreeResultSet(result);
    }

    if (GpsListCount[playerid] == 0) {
        ShowPlayerDialog(playerid, DIALOG_GPS_EMPTY, DIALOG_STYLE_MSGBOX, C_BRAND "GPS", C_GREY "В тази категория още няма места.", "Назад", "");
        return;
    }

    new body[2048], line[96];
    body[0] = EOS;
    for (new i = 0; i < GpsListCount[playerid]; i++) {
        new Float:dist = GetPlayerDistanceFromPoint(playerid, GpsListX[playerid][i], GpsListY[playerid][i], GpsListZ[playerid][i]);
        format(line, sizeof (line), C_WHITE "%s " C_GREY "- %.1f км\n", GpsListName[playerid][i], dist / 1000.0);
        strcat(body, line);
    }
    new title[64];
    format(title, sizeof (title), C_BRAND "GPS " C_WHITE "| %s", GpsCategoryName[category]);
    ShowPlayerDialog(playerid, DIALOG_GPS_LIST, DIALOG_STYLE_LIST, title, "%s", "Маршрут", "Назад", body);
}

stock Gps_OnDialogResponse(playerid, dialogid, response, listitem)
{
    switch (dialogid) {
        case DIALOG_GPS_MAIN: {
            if (!response) return 1;
            if (listitem >= 0 && listitem < MAX_GPS_CATEGORIES) {
                Gps_ShowCategory(playerid, listitem);
            } else if (listitem == MAX_GPS_CATEGORIES && GpsMainHasOff[playerid]) {
                Gps_Stop(playerid);
                SendClientMessage(playerid, COLOR_GREY, "[GPS] Маршрутът е изключен.");
            }
            return 1;
        }
        case DIALOG_GPS_LIST: {
            if (!response) {
                Gps_ShowMain(playerid);
                return 1;
            }
            if (listitem < 0 || listitem >= GpsListCount[playerid]) return 1;
            Gps_SetTarget(playerid, GpsListName[playerid][listitem],
                GpsListX[playerid][listitem], GpsListY[playerid][listitem], GpsListZ[playerid][listitem]);
            return 1;
        }
        case DIALOG_GPS_EMPTY: {
            Gps_ShowMain(playerid);
            return 1;
        }
    }
    return 0;
}

// ------------------------------------------------------------
//  Команди
// ------------------------------------------------------------

CMD:gps(playerid, params[])
{
    #pragma unused params
    Gps_ShowMain(playerid);
    return 1;
}

CMD:gpsadd(playerid, params[])
{
    if (!Admin_Check(playerid, ADMIN_ADMIN)) return 1;

    new category, name[GPS_NAME_LEN];
    if (!GetParamInt(params, 0, category) || !GetParamRest(params, 1, name) || category < 1 || category > MAX_GPS_CATEGORIES) {
        SendUsage(playerid, "/gpsadd [категория] [име]");
        for (new c = 0; c < MAX_GPS_CATEGORIES; c++) {
            SendClientMessage(playerid, COLOR_GREY, "  %d - %s", c + 1, GpsCategoryName[c]);
        }
        return 1;
    }

    new Float:x, Float:y, Float:z, eName[100], q[256];
    GetPlayerPos(playerid, x, y, z);
    SQL_Escape(name, eName);
    format(q, sizeof (q), "INSERT INTO gps_locations (category, name, x, y, z) VALUES (%d, '%s', %f, %f, %f)",
        category - 1, eName, x, y, z);
    Database_Exec(q);

    SendClientMessage(playerid, COLOR_ADMIN, "[GPS] Добавено: \"%s\" в категория \"%s\".", name, GpsCategoryName[category - 1]);
    Log_Add("gpsadd", PlayerData[playerid][pName], name, "");
    return 1;
}

CMD:gpslist(playerid, params[])
{
    #pragma unused params
    if (!Admin_Check(playerid, ADMIN_ADMIN)) return 1;

    new DBResult:result = Database_Query("SELECT id, category, name FROM gps_locations ORDER BY category, name");
    new count = 0;
    SendClientMessage(playerid, COLOR_ADMIN, "______ GPS места, добавени от админи ______");
    if (result != DBResult:0) {
        if (DB_GetRowCount(result) > 0) {
            new name[GPS_NAME_LEN], cat;
            do {
                DB_GetFieldStringByName(result, "name", name);
                cat = DB_GetFieldIntByName(result, "category");
                if (cat < 0 || cat >= MAX_GPS_CATEGORIES) cat = 0;
                SendClientMessage(playerid, COLOR_WHITE, "#%d %s {A0A0A0}(%s)", DB_GetFieldIntByName(result, "id"), name, GpsCategoryName[cat]);
                count++;
            } while (DB_SelectNextRow(result));
        }
        DB_FreeResultSet(result);
    }
    if (count == 0) SendClientMessage(playerid, COLOR_GREY, "Няма добавени места. Виж /gpsadd.");
    return 1;
}

CMD:gpsdel(playerid, params[])
{
    if (!Admin_Check(playerid, ADMIN_ADMIN)) return 1;
    new id;
    if (!GetParamInt(params, 0, id)) return SendUsage(playerid, "/gpsdel [номер от /gpslist]");

    new q[96];
    format(q, sizeof (q), "DELETE FROM gps_locations WHERE id = %d", id);
    Database_Exec(q);
    SendClientMessage(playerid, COLOR_ADMIN, "[GPS] Място #%d е премахнато (ако съществуваше).", id);
    return 1;
}
