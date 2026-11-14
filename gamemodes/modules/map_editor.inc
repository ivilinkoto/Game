// Редактор за mapping в играта (удобен и на телефон).
//
// Картите и обектите се пазят в базата и се зареждат при старт на сървъра.
//
// Команди (админ ниво 3+):
//   /mapnew [име]          – нова карта (става текуща)
//   /maps                  – списък с карти, избор на текуща
//   /mapdel [име]          – изтрива карта с всички обекти (ниво 5)
//   /mo [модел]            – нов обект пред теб (става избран)
//   /msel                  – избира най-близкия обект от текущата карта
//   /medit                 – бутони на екрана за местене и въртене
//   /mhere                 – мести избрания обект на твоето място
//   /mcopy                 – копие на избрания обект (на същото място; мести го с /medit)
//   /mdel                  – изтрива избрания обект
//   /minfo                 – информация за избрания обект
//   /mtex [слот] [модел] [txd] [текстура]   – текстура
//   /mtext [слот] [размер шрифт] [текст]     – надпис (| = нов ред, {RRGGBB} = цвят)
//   /mclear [слот]         – маха текстурата/надписа от слота
//   /mexport               – записва текущата карта като код в scriptfiles/map_[име].txt

#define MAP_MAX_OBJECTS     3000
#define MAP_NAME_LEN        32

enum E_MAP_OBJECT {
    bool:moExists,
    moDBID,
    moMap,
    moModel,
    Float:moX,
    Float:moY,
    Float:moZ,
    Float:moRX,
    Float:moRY,
    Float:moRZ,
    moWorld,
    moInterior,
    moDynamic
}
static MapObj[MAP_MAX_OBJECTS][E_MAP_OBJECT];

static const Float:MapPosSteps[] = {0.05, 0.1, 0.5, 1.0, 5.0};
static const Float:MapRotSteps[] = {1.0, 5.0, 15.0, 45.0, 90.0};

// Бутони на панела
enum {
    MAPBTN_XM, MAPBTN_XP, MAPBTN_YM, MAPBTN_YP, MAPBTN_ZM, MAPBTN_ZP, MAPBTN_STEP,
    MAPBTN_RXM, MAPBTN_RXP, MAPBTN_RYM, MAPBTN_RYP, MAPBTN_RZM, MAPBTN_RZP, MAPBTN_DONE,
    MAPBTN_COUNT
}
static const MapBtnText[MAPBTN_COUNT][] = {
    "X-", "X+", "Y-", "Y+", "Z-", "Z+", "STEP",
    "RX-", "RX+", "RY-", "RY+", "RZ-", "RZ+", "DONE"
};
static Text:gMapBtn[MAPBTN_COUNT];

static MapEditMap[MAX_PLAYERS],
    MapEditObj[MAX_PLAYERS],
    MapEditStep[MAX_PLAYERS],
    bool:MapEditing[MAX_PLAYERS],
    PlayerText:MapInfoTD[MAX_PLAYERS],
    MapEditMapName[MAX_PLAYERS][MAP_NAME_LEN],
    MapListIDs[MAX_PLAYERS][20],
    MapListCount[MAX_PLAYERS];

// ============================================================
//  Зареждане
// ============================================================

stock Map_Init()
{
    Database_Exec("CREATE TABLE IF NOT EXISTS maps (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL UNIQUE COLLATE NOCASE, author TEXT NOT NULL DEFAULT '', created_at INTEGER NOT NULL DEFAULT 0)");
    Database_Exec("CREATE TABLE IF NOT EXISTS map_objects (id INTEGER PRIMARY KEY AUTOINCREMENT, map_id INTEGER NOT NULL, model INTEGER NOT NULL, x REAL, y REAL, z REAL, rx REAL, ry REAL, rz REAL, world INTEGER NOT NULL DEFAULT -1, interior INTEGER NOT NULL DEFAULT -1)");
    new q[512];
    q[0] = EOS;
    strcat(q, "CREATE TABLE IF NOT EXISTS map_materials (object_id INTEGER NOT NULL, idx INTEGER NOT NULL, kind INTEGER NOT NULL, ");
    strcat(q, "model INTEGER NOT NULL DEFAULT 0, txd TEXT NOT NULL DEFAULT '', texture TEXT NOT NULL DEFAULT '', ");
    strcat(q, "text TEXT NOT NULL DEFAULT '', fontsize INTEGER NOT NULL DEFAULT 24, PRIMARY KEY (object_id, idx))");
    Database_Exec(q);
    Database_Exec("CREATE INDEX IF NOT EXISTS idx_map_objects_map ON map_objects (map_id)");

    for (new i = 0; i < MAP_MAX_OBJECTS; i++) MapObj[i][moExists] = false;

    new count = 0;
    new DBResult:result = Database_Query("SELECT * FROM map_objects ORDER BY id");
    if (result != DBResult:0) {
        if (DB_GetRowCount(result) > 0) {
            do {
                if (Map_SpawnObject(DB_GetFieldIntByName(result, "id"), DB_GetFieldIntByName(result, "map_id"),
                    DB_GetFieldIntByName(result, "model"),
                    DB_GetFieldFloatByName(result, "x"), DB_GetFieldFloatByName(result, "y"), DB_GetFieldFloatByName(result, "z"),
                    DB_GetFieldFloatByName(result, "rx"), DB_GetFieldFloatByName(result, "ry"), DB_GetFieldFloatByName(result, "rz"),
                    DB_GetFieldIntByName(result, "world"), DB_GetFieldIntByName(result, "interior")) != -1) count++;
            } while (DB_SelectNextRow(result));
        }
        DB_FreeResultSet(result);
    }

    // Текстури и надписи
    result = Database_Query("SELECT * FROM map_materials");
    if (result != DBResult:0) {
        if (DB_GetRowCount(result) > 0) {
            new txd[32], texture[32], text[256];
            do {
                new obj = Map_FindByDBID(DB_GetFieldIntByName(result, "object_id"));
                if (obj == -1) continue;
                DB_GetFieldStringByName(result, "txd", txd);
                DB_GetFieldStringByName(result, "texture", texture);
                DB_GetFieldStringByName(result, "text", text);
                Map_ApplyMaterial(obj, DB_GetFieldIntByName(result, "idx"), DB_GetFieldIntByName(result, "kind"),
                    DB_GetFieldIntByName(result, "model"), txd, texture, text, DB_GetFieldIntByName(result, "fontsize"));
            } while (DB_SelectNextRow(result));
        }
        DB_FreeResultSet(result);
    }
    printf("[MAP] Loaded %d object(s).", count);

    // Панел с бутони
    for (new b = 0; b < MAPBTN_COUNT; b++) {
        new col = b % 7, row = b / 7;
        new Float:x = 395.0 + float(col) * 36.0, Float:y = 360.0 + float(row) * 22.0;
        gMapBtn[b] = TextDrawCreate(x, y, MapBtnText[b]);
        TextDrawFont(gMapBtn[b], TEXT_DRAW_FONT_2);
        TextDrawLetterSize(gMapBtn[b], 0.20, 1.10);
        TextDrawAlignment(gMapBtn[b], TEXT_DRAW_ALIGN_CENTRE);
        TextDrawColour(gMapBtn[b], COLOR_WHITE);
        TextDrawSetOutline(gMapBtn[b], 0);
        TextDrawSetShadow(gMapBtn[b], 0);
        TextDrawUseBox(gMapBtn[b], true);
        TextDrawBoxColour(gMapBtn[b], (b == MAPBTN_DONE) ? 0x33CC66FF : ((b == MAPBTN_STEP) ? COLOR_BRAND : 0x2A2A2AEE));
        TextDrawTextSize(gMapBtn[b], 14.0, 32.0);
        TextDrawSetSelectable(gMapBtn[b], true);
    }
}

stock Map_ResetPlayer(playerid)
{
    MapEditMap[playerid] = 0;
    MapEditObj[playerid] = -1;
    MapEditStep[playerid] = 2;
    MapEditing[playerid] = false;
    MapInfoTD[playerid] = INVALID_PLAYER_TEXT_DRAW;
    MapEditMapName[playerid][0] = EOS;
    MapListCount[playerid] = 0;
}

stock Map_SpawnObject(dbid, mapid, model, Float:x, Float:y, Float:z, Float:rx, Float:ry, Float:rz, world, interior)
{
    for (new i = 0; i < MAP_MAX_OBJECTS; i++) {
        if (MapObj[i][moExists]) continue;
        MapObj[i][moExists] = true;
        MapObj[i][moDBID] = dbid;
        MapObj[i][moMap] = mapid;
        MapObj[i][moModel] = model;
        MapObj[i][moX] = x;
        MapObj[i][moY] = y;
        MapObj[i][moZ] = z;
        MapObj[i][moRX] = rx;
        MapObj[i][moRY] = ry;
        MapObj[i][moRZ] = rz;
        MapObj[i][moWorld] = world;
        MapObj[i][moInterior] = interior;
        MapObj[i][moDynamic] = CreateDynamicObject(model, x, y, z, rx, ry, rz, world, interior, -1, 300.0);
        return i;
    }
    return -1;
}

stock Map_FindByDBID(dbid)
{
    for (new i = 0; i < MAP_MAX_OBJECTS; i++) {
        if (MapObj[i][moExists] && MapObj[i][moDBID] == dbid) return i;
    }
    return -1;
}

// kind: 0 = текстура, 1 = надпис
stock Map_ApplyMaterial(obj, idx, kind, model, const txd[], const texture[], const text[], fontsize)
{
    if (kind == 0) {
        SetDynamicObjectMaterial(MapObj[obj][moDynamic], idx, model, txd, texture, 0);
    } else {
        SetDynamicObjectMaterialText(MapObj[obj][moDynamic], idx, text, 130, "Arial", fontsize, 1, 0xFFFFFFFF, 0, 1);
    }
}

stock Map_SaveObject(obj)
{
    new q[384];
    format(q, sizeof (q), "UPDATE map_objects SET x = %f, y = %f, z = %f, rx = %f, ry = %f, rz = %f WHERE id = %d",
        MapObj[obj][moX], MapObj[obj][moY], MapObj[obj][moZ], MapObj[obj][moRX], MapObj[obj][moRY], MapObj[obj][moRZ], MapObj[obj][moDBID]);
    Database_Exec(q);
}

stock Map_CreateObject(mapid, model, Float:x, Float:y, Float:z, Float:rx, Float:ry, Float:rz, world, interior)
{
    new q[384];
    format(q, sizeof (q), "INSERT INTO map_objects (map_id, model, x, y, z, rx, ry, rz, world, interior) VALUES (%d, %d, %f, %f, %f, %f, %f, %f, %d, %d)",
        mapid, model, x, y, z, rx, ry, rz, world, interior);
    Database_Exec(q);
    new dbid = 0;
    new DBResult:result = Database_Query("SELECT MAX(id) AS id FROM map_objects");
    if (result != DBResult:0) {
        dbid = DB_GetFieldIntByName(result, "id");
        DB_FreeResultSet(result);
    }
    return Map_SpawnObject(dbid, mapid, model, x, y, z, rx, ry, rz, world, interior);
}

stock Map_DestroyObject(obj)
{
    if (!MapObj[obj][moExists]) return;
    DestroyDynamicObject(MapObj[obj][moDynamic]);
    MapObj[obj][moExists] = false;
}

// ============================================================
//  Проверки за командите
// ============================================================

static stock bool:Map_RequireMap(playerid)
{
    if (!Admin_Check(playerid, ADMIN_ADMIN)) return false;
    if (MapEditMap[playerid] == 0) {
        SendError(playerid, "Първо избери карта: /mapnew [име] или /maps.");
        return false;
    }
    return true;
}

static stock bool:Map_RequireObject(playerid)
{
    if (!Map_RequireMap(playerid)) return false;
    new obj = MapEditObj[playerid];
    if (obj < 0 || !MapObj[obj][moExists]) {
        SendError(playerid, "Няма избран обект: /mo [модел] или /msel.");
        return false;
    }
    return true;
}

stock Map_Select(playerid, obj)
{
    MapEditObj[playerid] = obj;
    Map_UpdateInfo(playerid);
    SendClientMessage(playerid, COLOR_ADMIN, "[Map] Избран обект #%d (модел %d). /medit - мести, /mdel - изтрий.", MapObj[obj][moDBID], MapObj[obj][moModel]);
}

// ============================================================
//  Панел за местене
// ============================================================

stock Map_UpdateInfo(playerid)
{
    if (MapInfoTD[playerid] == INVALID_PLAYER_TEXT_DRAW) return;
    new obj = MapEditObj[playerid];
    if (obj < 0 || !MapObj[obj][moExists]) {
        PlayerTextDrawSetString(playerid, MapInfoTD[playerid], "NO OBJECT");
        return;
    }
    PlayerTextDrawSetString(playerid, MapInfoTD[playerid], "OBJ %d  MODEL %d  STEP %.2fm / %.0fdeg~n~X %.2f  Y %.2f  Z %.2f~n~RX %.0f  RY %.0f  RZ %.0f",
        MapObj[obj][moDBID], MapObj[obj][moModel], MapPosSteps[MapEditStep[playerid]], MapRotSteps[MapEditStep[playerid]],
        MapObj[obj][moX], MapObj[obj][moY], MapObj[obj][moZ], MapObj[obj][moRX], MapObj[obj][moRY], MapObj[obj][moRZ]);
}

stock Map_StartEdit(playerid)
{
    if (MapEditing[playerid]) return;
    MapEditing[playerid] = true;

    new PlayerText:t = CreatePlayerTextDraw(playerid, 503.0, 318.0, "_");
    PlayerTextDrawFont(playerid, t, TEXT_DRAW_FONT_1);
    PlayerTextDrawLetterSize(playerid, t, 0.20, 1.0);
    PlayerTextDrawAlignment(playerid, t, TEXT_DRAW_ALIGN_CENTRE);
    PlayerTextDrawColour(playerid, t, COLOR_WHITE);
    PlayerTextDrawSetOutline(playerid, t, 1);
    PlayerTextDrawSetShadow(playerid, t, 0);
    PlayerTextDrawBackgroundColour(playerid, t, 0x000000FF);
    MapInfoTD[playerid] = t;
    Map_UpdateInfo(playerid);
    PlayerTextDrawShow(playerid, t);

    for (new b = 0; b < MAPBTN_COUNT; b++) TextDrawShowForPlayer(playerid, gMapBtn[b]);
    SelectTextDraw(playerid, COLOR_GOLD);
    SendClientMessage(playerid, COLOR_ADMIN, "[Map] Бутоните местят избрания обект. X = изток/запад, Y = север/юг, Z = нагоре/надолу. DONE - край.");
}

stock Map_StopEdit(playerid)
{
    if (!MapEditing[playerid]) return;
    MapEditing[playerid] = false;
    for (new b = 0; b < MAPBTN_COUNT; b++) TextDrawHideForPlayer(playerid, gMapBtn[b]);
    if (MapInfoTD[playerid] != INVALID_PLAYER_TEXT_DRAW) {
        PlayerTextDrawDestroy(playerid, MapInfoTD[playerid]);
        MapInfoTD[playerid] = INVALID_PLAYER_TEXT_DRAW;
    }
    CancelSelectTextDraw(playerid);
}

// Връща true, ако натискането е обработено тук.
stock bool:Map_OnClickTextDraw(playerid, Text:clickedid)
{
    if (!MapEditing[playerid]) return false;

    if (clickedid == INVALID_TEXT_DRAW) {
        Map_StopEdit(playerid);
        return true;
    }

    new btn = -1;
    for (new b = 0; b < MAPBTN_COUNT; b++) {
        if (clickedid == gMapBtn[b]) {
            btn = b;
            break;
        }
    }
    if (btn == -1) return false;

    if (btn == MAPBTN_DONE) {
        Map_StopEdit(playerid);
        SendClientMessage(playerid, COLOR_ADMIN, "[Map] Редакцията е записана.");
        return true;
    }
    if (btn == MAPBTN_STEP) {
        MapEditStep[playerid] = (MapEditStep[playerid] + 1) % sizeof (MapPosSteps);
        Map_UpdateInfo(playerid);
        return true;
    }

    new obj = MapEditObj[playerid];
    if (obj < 0 || !MapObj[obj][moExists]) return true;

    new Float:p = MapPosSteps[MapEditStep[playerid]], Float:r = MapRotSteps[MapEditStep[playerid]];
    switch (btn) {
        case MAPBTN_XM:  MapObj[obj][moX] -= p;
        case MAPBTN_XP:  MapObj[obj][moX] += p;
        case MAPBTN_YM:  MapObj[obj][moY] -= p;
        case MAPBTN_YP:  MapObj[obj][moY] += p;
        case MAPBTN_ZM:  MapObj[obj][moZ] -= p;
        case MAPBTN_ZP:  MapObj[obj][moZ] += p;
        case MAPBTN_RXM: MapObj[obj][moRX] -= r;
        case MAPBTN_RXP: MapObj[obj][moRX] += r;
        case MAPBTN_RYM: MapObj[obj][moRY] -= r;
        case MAPBTN_RYP: MapObj[obj][moRY] += r;
        case MAPBTN_RZM: MapObj[obj][moRZ] -= r;
        case MAPBTN_RZP: MapObj[obj][moRZ] += r;
    }
    SetDynamicObjectPos(MapObj[obj][moDynamic], MapObj[obj][moX], MapObj[obj][moY], MapObj[obj][moZ]);
    SetDynamicObjectRot(MapObj[obj][moDynamic], MapObj[obj][moRX], MapObj[obj][moRY], MapObj[obj][moRZ]);
    Map_SaveObject(obj);
    Map_UpdateInfo(playerid);
    return true;
}

stock Map_OnPlayerDisconnect(playerid)
{
    MapEditing[playerid] = false;
    MapInfoTD[playerid] = INVALID_PLAYER_TEXT_DRAW;
}

// ============================================================
//  Команди
// ============================================================

CMD:mapnew(playerid, params[])
{
    if (!Admin_Check(playerid, ADMIN_ADMIN)) return 1;
    new name[MAP_NAME_LEN];
    if (!GetParam(params, 0, name) || !IsAlphaNumeric(name)) return SendUsage(playerid, "/mapnew [име] (латински букви и цифри, напр. ferma)");

    new q[256];
    format(q, sizeof (q), "INSERT OR IGNORE INTO maps (name, author, created_at) VALUES ('%s', '%s', %d)", name, PlayerData[playerid][pName], gettime());
    Database_Exec(q);
    if (!Map_SetCurrent(playerid, name)) return SendError(playerid, "Картата не можа да се създаде.");
    SendClientMessage(playerid, COLOR_ADMIN, "[Map] Текуща карта: \"%s\". Добави обект с /mo [модел].", name);
    return 1;
}

stock bool:Map_SetCurrent(playerid, const name[])
{
    new q[160], id = 0;
    format(q, sizeof (q), "SELECT id, name FROM maps WHERE name = '%s' LIMIT 1", name);
    new DBResult:result = Database_Query(q);
    if (result != DBResult:0) {
        if (DB_GetRowCount(result) > 0) {
            id = DB_GetFieldIntByName(result, "id");
            DB_GetFieldStringByName(result, "name", MapEditMapName[playerid], MAP_NAME_LEN);
        }
        DB_FreeResultSet(result);
    }
    if (id == 0) return false;
    MapEditMap[playerid] = id;
    MapEditObj[playerid] = -1;
    return true;
}

CMD:maps(playerid, params[])
{
    #pragma unused params
    if (!Admin_Check(playerid, ADMIN_ADMIN)) return 1;

    new DBResult:result = Database_Query("SELECT m.id, m.name, m.author, (SELECT COUNT(*) FROM map_objects o WHERE o.map_id = m.id) AS cnt FROM maps m ORDER BY m.name LIMIT 20");
    new body[1500], line[128], count = 0;
    body[0] = EOS;
    if (result != DBResult:0) {
        if (DB_GetRowCount(result) > 0) {
            new name[MAP_NAME_LEN], author[MAX_PLAYER_NAME + 1];
            do {
                DB_GetFieldStringByName(result, "name", name);
                DB_GetFieldStringByName(result, "author", author);
                format(line, sizeof (line), C_WHITE "%s " C_GREY "- %d обекта, от %s\n", name, DB_GetFieldIntByName(result, "cnt"), author);
                strcat(body, line);
                MapListIDs[playerid][count++] = DB_GetFieldIntByName(result, "id");
            } while (count < 20 && DB_SelectNextRow(result));
        }
        DB_FreeResultSet(result);
    }
    MapListCount[playerid] = count;
    if (count == 0) return SendInfo(playerid, "Няма карти. Създай с /mapnew [име].");
    ShowPlayerDialog(playerid, DIALOG_MAP_LIST, DIALOG_STYLE_LIST, C_RED "Карти " C_WHITE "| избери текуща", "%s", "Избери", "Затвори", body);
    return 1;
}

stock Map_OnDialogResponse(playerid, dialogid, response, listitem)
{
    if (dialogid != DIALOG_MAP_LIST) return 0;
    if (!response || listitem < 0 || listitem >= MapListCount[playerid]) return 1;

    new q[128], name[MAP_NAME_LEN];
    format(q, sizeof (q), "SELECT name FROM maps WHERE id = %d", MapListIDs[playerid][listitem]);
    new DBResult:result = Database_Query(q);
    if (result != DBResult:0) {
        if (DB_GetRowCount(result) > 0) DB_GetFieldStringByName(result, "name", name);
        DB_FreeResultSet(result);
    }
    if (name[0] != EOS && Map_SetCurrent(playerid, name)) {
        SendClientMessage(playerid, COLOR_ADMIN, "[Map] Текуща карта: \"%s\".", name);
    }
    return 1;
}

CMD:mapdel(playerid, params[])
{
    if (!Admin_Check(playerid, ADMIN_OWNER)) return 1;
    new name[MAP_NAME_LEN];
    if (!GetParam(params, 0, name) || !IsAlphaNumeric(name)) return SendUsage(playerid, "/mapdel [име]");

    new q[160], id = 0;
    format(q, sizeof (q), "SELECT id FROM maps WHERE name = '%s' LIMIT 1", name);
    new DBResult:result = Database_Query(q);
    if (result != DBResult:0) {
        if (DB_GetRowCount(result) > 0) id = DB_GetFieldIntByName(result, "id");
        DB_FreeResultSet(result);
    }
    if (id == 0) return SendError(playerid, "Няма такава карта.");

    new removed = 0;
    for (new i = 0; i < MAP_MAX_OBJECTS; i++) {
        if (MapObj[i][moExists] && MapObj[i][moMap] == id) {
            Map_DestroyObject(i);
            removed++;
        }
    }
    format(q, sizeof (q), "DELETE FROM map_materials WHERE object_id IN (SELECT id FROM map_objects WHERE map_id = %d)", id);
    Database_Exec(q);
    format(q, sizeof (q), "DELETE FROM map_objects WHERE map_id = %d", id);
    Database_Exec(q);
    format(q, sizeof (q), "DELETE FROM maps WHERE id = %d", id);
    Database_Exec(q);

    for (new p = 0; p < MAX_PLAYERS; p++) {
        if (MapEditMap[p] == id) {
            MapEditMap[p] = 0;
            MapEditObj[p] = -1;
        }
    }
    SendClientMessage(playerid, COLOR_ADMIN, "[Map] Картата \"%s\" е изтрита (%d обекта).", name, removed);
    Log_Add("mapdel", PlayerData[playerid][pName], name, "");
    return 1;
}

CMD:mo(playerid, params[])
{
    if (!Map_RequireMap(playerid)) return 1;
    new model;
    if (!GetParamInt(params, 0, model) || model < 300 || model > 20000) return SendUsage(playerid, "/mo [модел на обекта]");

    new Float:x, Float:y, Float:z, Float:a;
    GetPlayerPos(playerid, x, y, z);
    GetPlayerFacingAngle(playerid, a);
    x -= 2.0 * floatsin(a, degrees);
    y += 2.0 * floatcos(a, degrees);

    new obj = Map_CreateObject(MapEditMap[playerid], model, x, y, z - 1.0, 0.0, 0.0, a,
        GetPlayerVirtualWorld(playerid), GetPlayerInterior(playerid));
    if (obj == -1) return SendError(playerid, "Достигнат е максималният брой обекти.");
    Map_Select(playerid, obj);
    return 1;
}

CMD:msel(playerid, params[])
{
    #pragma unused params
    if (!Map_RequireMap(playerid)) return 1;
    new best = -1, Float:bestDist = 15.0;
    for (new i = 0; i < MAP_MAX_OBJECTS; i++) {
        if (!MapObj[i][moExists] || MapObj[i][moMap] != MapEditMap[playerid]) continue;
        new Float:d = GetPlayerDistanceFromPoint(playerid, MapObj[i][moX], MapObj[i][moY], MapObj[i][moZ]);
        if (d < bestDist) {
            bestDist = d;
            best = i;
        }
    }
    if (best == -1) return SendError(playerid, "Няма обект от текущата карта в радиус 15 м.");
    Map_Select(playerid, best);
    return 1;
}

CMD:medit(playerid, params[])
{
    #pragma unused params
    if (!Map_RequireObject(playerid)) return 1;
    Map_StartEdit(playerid);
    return 1;
}

CMD:mhere(playerid, params[])
{
    #pragma unused params
    if (!Map_RequireObject(playerid)) return 1;
    new obj = MapEditObj[playerid], Float:x, Float:y, Float:z;
    GetPlayerPos(playerid, x, y, z);
    MapObj[obj][moX] = x;
    MapObj[obj][moY] = y;
    MapObj[obj][moZ] = z - 1.0;
    SetDynamicObjectPos(MapObj[obj][moDynamic], x, y, z - 1.0);
    Map_SaveObject(obj);
    Map_UpdateInfo(playerid);
    SendClientMessage(playerid, COLOR_ADMIN, "[Map] Обектът е преместен при теб.");
    return 1;
}

CMD:mcopy(playerid, params[])
{
    #pragma unused params
    if (!Map_RequireObject(playerid)) return 1;
    new src = MapEditObj[playerid];
    new obj = Map_CreateObject(MapEditMap[playerid], MapObj[src][moModel], MapObj[src][moX], MapObj[src][moY], MapObj[src][moZ],
        MapObj[src][moRX], MapObj[src][moRY], MapObj[src][moRZ], MapObj[src][moWorld], MapObj[src][moInterior]);
    if (obj == -1) return SendError(playerid, "Достигнат е максималният брой обекти.");

    // Копираме и текстурите
    new q[256];
    format(q, sizeof (q), "INSERT INTO map_materials (object_id, idx, kind, model, txd, texture, text, fontsize) " \
        "SELECT %d, idx, kind, model, txd, texture, text, fontsize FROM map_materials WHERE object_id = %d",
        MapObj[obj][moDBID], MapObj[src][moDBID]);
    Database_Exec(q);
    Map_ReapplyMaterials(obj);

    Map_Select(playerid, obj);
    SendClientMessage(playerid, COLOR_GREY, "Копието е на същото място – премести го с /medit.");
    return 1;
}

stock Map_ReapplyMaterials(obj)
{
    new q[128];
    format(q, sizeof (q), "SELECT * FROM map_materials WHERE object_id = %d", MapObj[obj][moDBID]);
    new DBResult:result = Database_Query(q);
    if (result == DBResult:0) return;
    if (DB_GetRowCount(result) > 0) {
        new txd[32], texture[32], text[256];
        do {
            DB_GetFieldStringByName(result, "txd", txd);
            DB_GetFieldStringByName(result, "texture", texture);
            DB_GetFieldStringByName(result, "text", text);
            Map_ApplyMaterial(obj, DB_GetFieldIntByName(result, "idx"), DB_GetFieldIntByName(result, "kind"),
                DB_GetFieldIntByName(result, "model"), txd, texture, text, DB_GetFieldIntByName(result, "fontsize"));
        } while (DB_SelectNextRow(result));
    }
    DB_FreeResultSet(result);
}

CMD:mdel(playerid, params[])
{
    #pragma unused params
    if (!Map_RequireObject(playerid)) return 1;
    new obj = MapEditObj[playerid], q[128];
    format(q, sizeof (q), "DELETE FROM map_materials WHERE object_id = %d", MapObj[obj][moDBID]);
    Database_Exec(q);
    format(q, sizeof (q), "DELETE FROM map_objects WHERE id = %d", MapObj[obj][moDBID]);
    Database_Exec(q);
    SendClientMessage(playerid, COLOR_ADMIN, "[Map] Обект #%d е изтрит.", MapObj[obj][moDBID]);
    Map_DestroyObject(obj);
    MapEditObj[playerid] = -1;
    Map_UpdateInfo(playerid);
    return 1;
}

CMD:minfo(playerid, params[])
{
    #pragma unused params
    if (!Map_RequireObject(playerid)) return 1;
    new obj = MapEditObj[playerid];
    SendClientMessage(playerid, COLOR_ADMIN, "[Map] Обект #%d, модел %d, карта \"%s\"", MapObj[obj][moDBID], MapObj[obj][moModel], MapEditMapName[playerid]);
    SendClientMessage(playerid, COLOR_WHITE, "Позиция: %.3f, %.3f, %.3f | Ротация: %.1f, %.1f, %.1f",
        MapObj[obj][moX], MapObj[obj][moY], MapObj[obj][moZ], MapObj[obj][moRX], MapObj[obj][moRY], MapObj[obj][moRZ]);
    return 1;
}

CMD:mtex(playerid, params[])
{
    if (!Map_RequireObject(playerid)) return 1;
    new idx, model, txd[32], texture[32];
    if (!GetParamInt(params, 0, idx) || !GetParamInt(params, 1, model) || !GetParam(params, 2, txd) || !GetParam(params, 3, texture) || idx < 0 || idx > 15) {
        return SendUsage(playerid, "/mtex [слот 0-15] [модел] [txd] [текстура]  напр. /mtex 0 19297 matlights invisible");
    }
    new obj = MapEditObj[playerid], eTxd[64], eTex[64], q[384];
    SQL_Escape(txd, eTxd);
    SQL_Escape(texture, eTex);
    format(q, sizeof (q), "INSERT OR REPLACE INTO map_materials (object_id, idx, kind, model, txd, texture) VALUES (%d, %d, 0, %d, '%s', '%s')",
        MapObj[obj][moDBID], idx, model, eTxd, eTex);
    Database_Exec(q);
    Map_ApplyMaterial(obj, idx, 0, model, txd, texture, "", 24);
    SendClientMessage(playerid, COLOR_ADMIN, "[Map] Текстура в слот %d: %d %s %s", idx, model, txd, texture);
    return 1;
}

CMD:mtext(playerid, params[])
{
    if (!Map_RequireObject(playerid)) return 1;
    new idx, fontsize, text[200];
    if (!GetParamInt(params, 0, idx) || !GetParamInt(params, 1, fontsize) || !GetParamRest(params, 2, text) || idx < 0 || idx > 15 || fontsize < 8 || fontsize > 120) {
        return SendUsage(playerid, "/mtext [слот] [размер 8-120] [текст]  (| = нов ред, {FF6A00} = цвят)");
    }
    for (new i = 0; text[i] != EOS; i++) {
        if (text[i] == '|') text[i] = '\n';
    }
    new obj = MapEditObj[playerid], eText[400], q[600];
    SQL_Escape(text, eText);
    format(q, sizeof (q), "INSERT OR REPLACE INTO map_materials (object_id, idx, kind, text, fontsize) VALUES (%d, %d, 1, '%s', %d)",
        MapObj[obj][moDBID], idx, eText, fontsize);
    Database_Exec(q);
    Map_ApplyMaterial(obj, idx, 1, 0, "", "", text, fontsize);
    SendClientMessage(playerid, COLOR_ADMIN, "[Map] Надпис в слот %d.", idx);
    return 1;
}

CMD:mclear(playerid, params[])
{
    if (!Map_RequireObject(playerid)) return 1;
    new idx;
    if (!GetParamInt(params, 0, idx) || idx < 0 || idx > 15) return SendUsage(playerid, "/mclear [слот 0-15]");
    new obj = MapEditObj[playerid], q[128];
    format(q, sizeof (q), "DELETE FROM map_materials WHERE object_id = %d AND idx = %d", MapObj[obj][moDBID], idx);
    Database_Exec(q);
    RemoveDynamicObjectMaterial(MapObj[obj][moDynamic], idx);
    RemoveDynamicObjectMaterialText(MapObj[obj][moDynamic], idx);
    SendClientMessage(playerid, COLOR_ADMIN, "[Map] Слот %d е изчистен.", idx);
    return 1;
}

CMD:mexport(playerid, params[])
{
    #pragma unused params
    if (!Map_RequireMap(playerid)) return 1;

    new filename[64];
    format(filename, sizeof (filename), "map_%s.txt", MapEditMapName[playerid]);
    new File:f = fopen(filename, io_write);
    if (!f) return SendError(playerid, "Файлът не може да се създаде.");

    new line[512], count = 0;
    format(line, sizeof (line), "// Карта \"%s\" - износ от HOTFIX ONLINE\r\nnew tmpobj;\r\n", MapEditMapName[playerid]);
    fwrite(f, line);
    for (new i = 0; i < MAP_MAX_OBJECTS; i++) {
        if (!MapObj[i][moExists] || MapObj[i][moMap] != MapEditMap[playerid]) continue;
        format(line, sizeof (line), "tmpobj = CreateDynamicObject(%d, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %d, %d);\r\n",
            MapObj[i][moModel], MapObj[i][moX], MapObj[i][moY], MapObj[i][moZ], MapObj[i][moRX], MapObj[i][moRY], MapObj[i][moRZ],
            MapObj[i][moWorld], MapObj[i][moInterior]);
        fwrite(f, line);
        count++;

        new q[128];
        format(q, sizeof (q), "SELECT * FROM map_materials WHERE object_id = %d", MapObj[i][moDBID]);
        new DBResult:result = Database_Query(q);
        if (result != DBResult:0) {
            if (DB_GetRowCount(result) > 0) {
                new txd[32], texture[32], text[256];
                do {
                    DB_GetFieldStringByName(result, "txd", txd);
                    DB_GetFieldStringByName(result, "texture", texture);
                    DB_GetFieldStringByName(result, "text", text);
                    if (DB_GetFieldIntByName(result, "kind") == 0) {
                        format(line, sizeof (line), "SetDynamicObjectMaterial(tmpobj, %d, %d, \"%s\", \"%s\", 0);\r\n",
                            DB_GetFieldIntByName(result, "idx"), DB_GetFieldIntByName(result, "model"), txd, texture);
                    } else {
                        format(line, sizeof (line), "SetDynamicObjectMaterialText(tmpobj, %d, \"%s\", 130, \"Arial\", %d, 1, 0xFFFFFFFF, 0, 1);\r\n",
                            DB_GetFieldIntByName(result, "idx"), text, DB_GetFieldIntByName(result, "fontsize"));
                    }
                    fwrite(f, line);
                } while (DB_SelectNextRow(result));
            }
            DB_FreeResultSet(result);
        }
    }
    fclose(f);
    SendClientMessage(playerid, COLOR_ADMIN, "[Map] Изнесени %d обекта в scriptfiles/%s", count, filename);
    return 1;
}
