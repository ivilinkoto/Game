// GPS: точки по категории, подредени по разстояние. Показва чекпойнт на картата.
// Точките се добавят от админ в играта: /gpsadd, /gpsdel, /gpslist.

#define GPS_MAX_LIST        40
#define GPS_CP_RADIUS       8.0

enum {
    GPS_CAT_INDUSTRY,
    GPS_CAT_FUEL,
    GPS_CAT_DEPOT,
    GPS_CAT_OTHER,
    GPS_CAT_COUNT
}

static const GpsCategoryNames[GPS_CAT_COUNT][] = {"Индустрии", "Бензиностанции", "Депа", "Други"};

static GpsListIds[MAX_PLAYERS][GPS_MAX_LIST];
static bool:GpsActive[MAX_PLAYERS];

stock Gps_Reset(playerid)
{
    GpsActive[playerid] = false;
}

stock Gps_ShowCategories(playerid)
{
    new body[256];
    for (new c = 0; c < GPS_CAT_COUNT; c++) {
        if (c > 0) strcat(body, "\n");
        strcat(body, GpsCategoryNames[c]);
    }
    if (GpsActive[playerid]) strcat(body, "\n" C_RED "Изключи GPS");
    ShowPlayerDialog(playerid, DIALOG_GPS_CATEGORY, DIALOG_STYLE_LIST, C_BRAND "GPS", "%s", "Избери", "Назад", body);
}

static stock Gps_ShowPoints(playerid, category)
{
    new Float:px, Float:py, Float:pz;
    GetPlayerPos(playerid, px, py, pz);

    // SQLite подрежда по разстояние (квадрат на разстоянието е достатъчен за реда).
    new q[256];
    format(q, sizeof (q),
        "SELECT id, name, x, y, z FROM gps_points WHERE category = %d " \
        "ORDER BY ((x - %f) * (x - %f) + (y - %f) * (y - %f)) LIMIT %d",
        category, px, px, py, py, GPS_MAX_LIST);
    new DBResult:r = Database_Query(q);
    if (r == DBResult:0) return;

    new rows = DB_GetRowCount(r);
    if (rows == 0) {
        DB_FreeResultSet(r);
        ShowPlayerDialog(playerid, DIALOG_MENU_INFO, DIALOG_STYLE_MSGBOX, C_BRAND "GPS",
            C_WHITE "В тази категория още няма точки.", "Добре", "");
        return;
    }

    new body[2048], line[96], name[48], Float:x, Float:y, Float:z;
    format(body, sizeof (body), "Място\tРазстояние\n");
    for (new i = 0; i < rows && i < GPS_MAX_LIST; i++) {
        GpsListIds[playerid][i] = DB_GetFieldIntByName(r, "id");
        DB_GetFieldStringByName(r, "name", name, sizeof (name));
        x = DB_GetFieldFloatByName(r, "x");
        y = DB_GetFieldFloatByName(r, "y");
        z = DB_GetFieldFloatByName(r, "z");
        new Float:dist = GetPlayerDistanceFromPoint(playerid, x, y, z);
        format(line, sizeof (line), "%s\t" C_GREY "%.1f км\n", name, dist / 1000.0);
        strcat(body, line);
        DB_SelectNextRow(r);
    }
    DB_FreeResultSet(r);

    ShowPlayerDialog(playerid, DIALOG_GPS_POINTS, DIALOG_STYLE_TABLIST_HEADERS, C_BRAND "GPS" C_WHITE " | Избери място",
        "%s", "Покажи", "Назад", body);
}

static stock Gps_SetTarget(playerid, pointId)
{
    new q[96];
    format(q, sizeof (q), "SELECT name, x, y, z FROM gps_points WHERE id = %d", pointId);
    new DBResult:r = Database_Query(q);
    if (r == DBResult:0) return;
    if (DB_GetRowCount(r) > 0) {
        new name[48];
        DB_GetFieldStringByName(r, "name", name, sizeof (name));
        new Float:x = DB_GetFieldFloatByName(r, "x");
        new Float:y = DB_GetFieldFloatByName(r, "y");
        new Float:z = DB_GetFieldFloatByName(r, "z");
        DisablePlayerCheckpoint(playerid);
        SetPlayerCheckpoint(playerid, x, y, z, GPS_CP_RADIUS);
        GpsActive[playerid] = true;
        SendClientMessage(playerid, COLOR_BRAND, "[GPS] " C_WHITE "Маршрут до " C_BRAND "%s" C_WHITE " (%.1f км). Следвай червения маркер на картата.",
            name, GetPlayerDistanceFromPoint(playerid, x, y, z) / 1000.0);
    }
    DB_FreeResultSet(r);
}

// Маршрут до произволна точка (напр. гаража).
stock Gps_SetPoint(playerid, const name[], Float:x, Float:y, Float:z)
{
    DisablePlayerCheckpoint(playerid);
    SetPlayerCheckpoint(playerid, x, y, z, GPS_CP_RADIUS);
    GpsActive[playerid] = true;
    SendClientMessage(playerid, COLOR_BRAND, "[GPS] " C_WHITE "Маршрут до " C_BRAND "%s" C_WHITE " (%.1f км). Следвай червения маркер на картата.",
        name, GetPlayerDistanceFromPoint(playerid, x, y, z) / 1000.0);
}

stock Gps_Cancel(playerid)
{
    if (!GpsActive[playerid]) return;
    DisablePlayerCheckpoint(playerid);
    GpsActive[playerid] = false;
}

stock Gps_OnEnterCheckpoint(playerid)
{
    if (!GpsActive[playerid]) return 0;
    Gps_Cancel(playerid);
    PlayerPlaySound(playerid, 1058, 0.0, 0.0, 0.0);
    SendClientMessage(playerid, COLOR_BRAND, "[GPS] " C_WHITE "Пристигна!");
    return 1;
}

static GpsCategory[MAX_PLAYERS];

stock Gps_OnDialogResponse(playerid, dialogid, response, listitem)
{
    switch (dialogid) {
        case DIALOG_GPS_CATEGORY: {
            if (!response) {
                Menu_Show(playerid);
                return 1;
            }
            if (listitem == GPS_CAT_COUNT) {
                Gps_Cancel(playerid);
                SendClientMessage(playerid, COLOR_BRAND, "[GPS] " C_WHITE "Изключен.");
                return 1;
            }
            if (listitem < 0 || listitem >= GPS_CAT_COUNT) return 1;
            GpsCategory[playerid] = listitem;
            Gps_ShowPoints(playerid, listitem);
            return 1;
        }
        case DIALOG_GPS_POINTS: {
            if (!response) {
                Gps_ShowCategories(playerid);
                return 1;
            }
            if (listitem < 0 || listitem >= GPS_MAX_LIST) return 1;
            Gps_SetTarget(playerid, GpsListIds[playerid][listitem]);
            return 1;
        }
    }
    return 0;
}

CMD:gps(playerid, params[])
{
    #pragma unused params
    Gps_ShowCategories(playerid);
    return 1;
}

// ---------- Админ ----------

CMD:gpsadd(playerid, params[])
{
    if (!Admin_Check(playerid, 3)) return 1;
    new catStr[4], name[48];
    if (!GetParam(params, 0, catStr) || !IsNumeric(catStr) || !GetParamRest(params, 1, name)) {
        SendUsage(playerid, "/gpsadd [категория] [име]");
        SendClientMessage(playerid, COLOR_GREY, "Категории: 0 Индустрии · 1 Бензиностанции · 2 Депа · 3 Други");
        return 1;
    }
    new cat = strval(catStr);
    if (cat < 0 || cat >= GPS_CAT_COUNT) return SendError(playerid, "Категорията е от 0 до 3.");

    new Float:x, Float:y, Float:z, safe[96], q[256];
    GetPlayerPos(playerid, x, y, z);
    SQL_Escape(name, safe);
    format(q, sizeof (q), "INSERT INTO gps_points (category, name, x, y, z) VALUES (%d, '%s', %f, %f, %f)", cat, safe, x, y, z);
    Database_Exec(q);
    SendClientMessage(playerid, COLOR_BRAND, "[GPS] " C_WHITE "Добавена точка " C_BRAND "%s" C_WHITE " в „%s“.", name, GpsCategoryNames[cat]);
    return 1;
}

CMD:gpslist(playerid, params[])
{
    #pragma unused params
    if (!Admin_Check(playerid, 3)) return 1;
    new DBResult:r = Database_Query("SELECT id, category, name FROM gps_points ORDER BY category, id");
    if (r == DBResult:0) return 1;
    new rows = DB_GetRowCount(r), name[48];
    if (rows == 0) SendInfo(playerid, "Няма GPS точки.");
    for (new i = 0; i < rows; i++) {
        DB_GetFieldStringByName(r, "name", name, sizeof (name));
        new cat = DB_GetFieldIntByName(r, "category");
        if (cat < 0 || cat >= GPS_CAT_COUNT) cat = GPS_CAT_OTHER;
        SendClientMessage(playerid, COLOR_GREY, "#%d · %s · %s", DB_GetFieldIntByName(r, "id"), GpsCategoryNames[cat], name);
        DB_SelectNextRow(r);
    }
    DB_FreeResultSet(r);
    return 1;
}

CMD:gpsdel(playerid, params[])
{
    if (!Admin_Check(playerid, 3)) return 1;
    if (IsNull(params) || !IsNumeric(params)) return SendUsage(playerid, "/gpsdel [номер от /gpslist]");
    new q[64];
    format(q, sizeof (q), "DELETE FROM gps_points WHERE id = %d", strval(params));
    Database_Exec(q);
    SendInfo(playerid, "GPS точката е изтрита (ако е съществувала).");
    return 1;
}
