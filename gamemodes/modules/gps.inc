// GPS: категории с места, подредени по разстояние. Показва чекпойнт на картата.
// Компаниите, бензиностанциите, автокъщите и гаражът се взимат направо от играта.
// "Други" са ръчни точки от админ: /gpsadd, /gpsdel, /gpslist.

#define GPS_MAX_LIST        40
#define GPS_CP_RADIUS       8.0

enum {
    GPS_CAT_COMPANIES,
    GPS_CAT_FUEL,
    GPS_CAT_DEALERS,
    GPS_CAT_GARAGE,
    GPS_CAT_OTHER,
    GPS_CAT_COUNT
}

static const GpsCategoryNames[GPS_CAT_COUNT][] = {"Компании", "Бензиностанции", "Автокъщи", "Моят гараж", "Други"};

// Списъкът, показан на играча: вид + индекс (за "Други" – ID от базата)
static GpsListType[MAX_PLAYERS][GPS_MAX_LIST];
static GpsListIndex[MAX_PLAYERS][GPS_MAX_LIST];
static GpsListCount[MAX_PLAYERS];
static bool:GpsActive[MAX_PLAYERS];
static bool:GpsTpMode[MAX_PLAYERS];      // /tp: изборът телепортира вместо да води

stock Gps_Reset(playerid)
{
    GpsActive[playerid] = false;
}

// /tp (админ): същите категории, но изборът телепортира
stock Gps_ShowTeleport(playerid)
{
    GpsTpMode[playerid] = true;
    Gps_ShowCategoriesEx(playerid, true);
}

stock Gps_ShowCategories(playerid)
{
    GpsTpMode[playerid] = false;
    Gps_ShowCategoriesEx(playerid, false);
}

static stock Gps_ShowCategoriesEx(playerid, bool:tp)
{
    new body[256];
    for (new c = 0; c < GPS_CAT_COUNT; c++) {
        if (c > 0) strcat(body, "\n");
        strcat(body, GpsCategoryNames[c]);
    }
    if (tp) {
        ShowPlayerDialog(playerid, DIALOG_GPS_CATEGORY, DIALOG_STYLE_LIST, C_ADMIN "Телепорт", "%s", "Избери", "Затвори", body);
        return;
    }
    if (GpsActive[playerid]) strcat(body, "\n" C_RED "Изключи GPS");
    ShowPlayerDialog(playerid, DIALOG_GPS_CATEGORY, DIALOG_STYLE_LIST, C_BRAND "GPS", "%s", "Избери", "Назад", body);
}

// Позиция и име на място от списъка
static stock Gps_GetPlace(type, index, &Float:x, &Float:y, &Float:z, name[], size)
{
    switch (type) {
        case GPS_CAT_COMPANIES: {
            Company_GetPos(index, x, y, z);
            Company_GetName(index, name, size);
        }
        case GPS_CAT_FUEL: {
            Fuel_GetPos(index, x, y, z);
            format(name, size, "Бензиностанция %d", index + 1);
        }
        case GPS_CAT_DEALERS: {
            Dealer_GetPos(index, x, y, z);
            Dealer_GetName(index, name, size);
        }
        default: return 0;
    }
    return 1;
}

// Компании / бензиностанции / автокъщи – подредени по разстояние
static stock Gps_ShowGameList(playerid, type)
{
    new total;
    switch (type) {
        case GPS_CAT_COMPANIES: total = Company_Total();
        case GPS_CAT_FUEL:      total = Fuel_Total();
        case GPS_CAT_DEALERS:   total = Dealer_Total();
        default: return;
    }
    if (total > GPS_MAX_LIST) total = GPS_MAX_LIST;
    if (total == 0) {
        ShowPlayerDialog(playerid, DIALOG_MENU_INFO, DIALOG_STYLE_MSGBOX, C_BRAND "GPS", C_WHITE "Няма места в тази категория.", "Добре", "");
        return;
    }

    new idx[GPS_MAX_LIST], Float:dist[GPS_MAX_LIST], Float:x, Float:y, Float:z, name[48];
    for (new i = 0; i < total; i++) {
        Gps_GetPlace(type, i, x, y, z, name, sizeof (name));
        idx[i] = i;
        dist[i] = GetPlayerDistanceFromPoint(playerid, x, y, z);
    }
    for (new a = 0; a < total - 1; a++) {
        for (new b = a + 1; b < total; b++) {
            if (dist[b] < dist[a]) {
                new t = idx[a]; idx[a] = idx[b]; idx[b] = t;
                new Float:tf = dist[a]; dist[a] = dist[b]; dist[b] = tf;
            }
        }
    }

    new body[2400], line[112];
    format(body, sizeof (body), "Място\tРазстояние\n");
    for (new k = 0; k < total; k++) {
        Gps_GetPlace(type, idx[k], x, y, z, name, sizeof (name));
        format(line, sizeof (line), "%s\t" C_GREY "%.1f км\n", name, dist[k] / 1000.0);
        strcat(body, line);
        GpsListType[playerid][k] = type;
        GpsListIndex[playerid][k] = idx[k];
    }
    GpsListCount[playerid] = total;
    new title[64];
    format(title, sizeof (title), C_BRAND "GPS" C_WHITE " | %s", GpsCategoryNames[type]);
    ShowPlayerDialog(playerid, DIALOG_GPS_POINTS, DIALOG_STYLE_TABLIST_HEADERS, title, "%s", "Покажи", "Назад", body);
}

// "Други" – ръчните точки от базата
static stock Gps_ShowCustom(playerid)
{
    new Float:px, Float:py, Float:pz;
    GetPlayerPos(playerid, px, py, pz);

    new q[256];
    format(q, sizeof (q),
        "SELECT id, name, x, y, z FROM gps_points " \
        "ORDER BY ((x - %f) * (x - %f) + (y - %f) * (y - %f)) LIMIT %d",
        px, px, py, py, GPS_MAX_LIST);
    new DBResult:r = Database_Query(q);
    if (r == DBResult:0) return;

    new rows = DB_GetRowCount(r);
    if (rows == 0) {
        DB_FreeResultSet(r);
        ShowPlayerDialog(playerid, DIALOG_MENU_INFO, DIALOG_STYLE_MSGBOX, C_BRAND "GPS",
            C_WHITE "Още няма други точки.", "Добре", "");
        return;
    }

    new body[2048], line[96], name[48], Float:x, Float:y, Float:z;
    format(body, sizeof (body), "Място\tРазстояние\n");
    for (new i = 0; i < rows && i < GPS_MAX_LIST; i++) {
        GpsListType[playerid][i] = GPS_CAT_OTHER;
        GpsListIndex[playerid][i] = DB_GetFieldIntByName(r, "id");
        DB_GetFieldStringByName(r, "name", name, sizeof (name));
        x = DB_GetFieldFloatByName(r, "x");
        y = DB_GetFieldFloatByName(r, "y");
        z = DB_GetFieldFloatByName(r, "z");
        new Float:dist = GetPlayerDistanceFromPoint(playerid, x, y, z);
        format(line, sizeof (line), "%s\t" C_GREY "%.1f км\n", name, dist / 1000.0);
        strcat(body, line);
        DB_SelectNextRow(r);
    }
    GpsListCount[playerid] = (rows < GPS_MAX_LIST) ? rows : GPS_MAX_LIST;
    DB_FreeResultSet(r);

    ShowPlayerDialog(playerid, DIALOG_GPS_POINTS, DIALOG_STYLE_TABLIST_HEADERS, C_BRAND "GPS" C_WHITE " | Други",
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
        Route_Set(playerid, x, y, z, ROUTE_SRC_GPS);
        SendClientMessage(playerid, COLOR_BRAND, "[GPS] " C_WHITE "Маршрут до " C_BRAND "%s" C_WHITE " (%.1f км). Следвай маршрута на картата.",
            name, GetPlayerDistanceFromPoint(playerid, x, y, z) / 1000.0);
    }
    DB_FreeResultSet(r);
}

// /tp до ръчна точка от базата
static stock Gps_TeleportToPoint(playerid, pointId)
{
    new q[96];
    format(q, sizeof (q), "SELECT x, y, z FROM gps_points WHERE id = %d", pointId);
    new DBResult:r = Database_Query(q);
    if (r == DBResult:0) return;
    if (DB_GetRowCount(r) > 0) {
        Admin_TeleportTo(playerid, DB_GetFieldFloatByName(r, "x"), DB_GetFieldFloatByName(r, "y"), DB_GetFieldFloatByName(r, "z"));
    }
    DB_FreeResultSet(r);
}

// Маршрут до произволна точка (напр. гаража).
stock Gps_SetPoint(playerid, const name[], Float:x, Float:y, Float:z)
{
    DisablePlayerCheckpoint(playerid);
    SetPlayerCheckpoint(playerid, x, y, z, GPS_CP_RADIUS);
    GpsActive[playerid] = true;
    Route_Set(playerid, x, y, z, ROUTE_SRC_GPS);
    SendClientMessage(playerid, COLOR_BRAND, "[GPS] " C_WHITE "Маршрут до " C_BRAND "%s" C_WHITE " (%.1f км). Следвай маршрута на картата.",
        name, GetPlayerDistanceFromPoint(playerid, x, y, z) / 1000.0);
}

stock Gps_Cancel(playerid)
{
    if (!GpsActive[playerid]) return;
    DisablePlayerCheckpoint(playerid);
    GpsActive[playerid] = false;
    Route_Clear(playerid, ROUTE_SRC_GPS);
}

stock Gps_OnEnterCheckpoint(playerid)
{
    if (!GpsActive[playerid]) return 0;
    Gps_Cancel(playerid);
    PlayerPlaySound(playerid, 1058, 0.0, 0.0, 0.0);
    SendClientMessage(playerid, COLOR_BRAND, "[GPS] " C_WHITE "Пристигна!");
    return 1;
}

stock Gps_OnDialogResponse(playerid, dialogid, response, listitem)
{
    switch (dialogid) {
        case DIALOG_GPS_CATEGORY: {
            if (!response) {
                if (!GpsTpMode[playerid]) Menu_Show(playerid);
                return 1;
            }
            if (listitem == GPS_CAT_COUNT) {
                Gps_Cancel(playerid);
                SendClientMessage(playerid, COLOR_BRAND, "[GPS] " C_WHITE "Изключен.");
                return 1;
            }
            switch (listitem) {
                case GPS_CAT_GARAGE: {
                    new Float:x, Float:y, Float:z, Float:a;
                    City_GetEntrance(PlayerData[playerid][pGarageCity], x, y, z, a);
                    if (GpsTpMode[playerid]) Admin_TeleportTo(playerid, x, y, z);
                    else Gps_SetPoint(playerid, "моят гараж", x, y, z);
                }
                case GPS_CAT_OTHER: Gps_ShowCustom(playerid);
                default: Gps_ShowGameList(playerid, listitem);
            }
            return 1;
        }
        case DIALOG_GPS_POINTS: {
            if (!response) {
                if (GpsTpMode[playerid]) Gps_ShowTeleport(playerid);
                else Gps_ShowCategories(playerid);
                return 1;
            }
            if (listitem < 0 || listitem >= GpsListCount[playerid]) return 1;
            new type = GpsListType[playerid][listitem], index = GpsListIndex[playerid][listitem];
            if (type == GPS_CAT_OTHER) {
                if (GpsTpMode[playerid]) Gps_TeleportToPoint(playerid, index);
                else Gps_SetTarget(playerid, index);
            } else {
                new Float:x, Float:y, Float:z, name[48];
                if (Gps_GetPlace(type, index, x, y, z, name, sizeof (name))) {
                    if (GpsTpMode[playerid]) Admin_TeleportTo(playerid, x, y, z);
                    else Gps_SetPoint(playerid, name, x, y, z);
                }
            }
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
    if (IsNull(params)) return SendUsage(playerid, "/gpsadd [име] - добавя точка тук (категория \"Други\")");
    new name[48];
    format(name, sizeof (name), "%s", params);

    new Float:x, Float:y, Float:z, safe[96], q[256];
    GetPlayerPos(playerid, x, y, z);
    SQL_Escape(name, safe);
    format(q, sizeof (q), "INSERT INTO gps_points (category, name, x, y, z) VALUES (%d, '%s', %f, %f, %f)", GPS_CAT_OTHER, safe, x, y, z);
    Database_Exec(q);
    SendClientMessage(playerid, COLOR_BRAND, "[GPS] " C_WHITE "Добавена точка " C_BRAND "%s" C_WHITE " в \"Други\".", name);
    return 1;
}

CMD:gpslist(playerid, params[])
{
    #pragma unused params
    if (!Admin_Check(playerid, 3)) return 1;
    new DBResult:r = Database_Query("SELECT id, name FROM gps_points ORDER BY id");
    if (r == DBResult:0) return 1;
    new rows = DB_GetRowCount(r), name[48];
    if (rows == 0) SendInfo(playerid, "Няма GPS точки.");
    for (new i = 0; i < rows; i++) {
        DB_GetFieldStringByName(r, "name", name, sizeof (name));
        SendClientMessage(playerid, COLOR_GREY, "#%d: %s", DB_GetFieldIntByName(r, "id"), name);
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
