// Маршрут на картата: при всяка отметка (GPS, курс, отметка на картата) целият път
// по улиците се рисува на мини картата и голямата карта.
//
// Пътят се търси от плъгина GPS (в отделна нишка, без да спира сървъра), а се рисува
// с малки квадратчета (gang zones), видими само за играча. Обновява се, докато караш.
//
// Нужен е плъгинът GPS (plugins/GPS.so + scriptfiles/GPS.dat). Без него маршрутът е изключен:
// включва се с #define ROUTE_ENABLED в cargo.pwn, след като плъгинът е инсталиран.

#define ROUTE_MAX_ZONES     600
#define ROUTE_MAX_NODES     3000
#define ROUTE_STEP          14.0        // разстояние между квадратчетата (м)
#define ROUTE_DOT           4.0         // половин страна на квадратчето (м)
#define ROUTE_COLOR         0xF5A623BB
#define ROUTE_ARRIVE        35.0        // толкова близо до целта – маршрутът изчезва
#define ROUTE_RECALC_MOVE   60.0        // преизчисляване, ако си се отдалечил толкова от последната точка

enum {
    ROUTE_SRC_NONE,
    ROUTE_SRC_GPS,
    ROUTE_SRC_JOB,
    ROUTE_SRC_MAP
}

#if defined ROUTE_ENABLED

static RouteZones[MAX_PLAYERS][ROUTE_MAX_ZONES];
static RouteZoneCount[MAX_PLAYERS];
static RouteSource[MAX_PLAYERS];
static Float:RouteTarget[MAX_PLAYERS][3];
static Float:RouteFrom[MAX_PLAYERS][3];
static RouteSeq[MAX_PLAYERS];
static bool:RouteBusy[MAX_PLAYERS];
static RouteBusySince[MAX_PLAYERS];

static Float:RouteNodeBuf[ROUTE_MAX_NODES][2];

stock Route_OnPlayerConnect(playerid)
{
    RouteZoneCount[playerid] = 0;
    RouteSource[playerid] = ROUTE_SRC_NONE;
    RouteBusy[playerid] = false;
    RouteSeq[playerid]++;
}

static stock Route_ClearZones(playerid)
{
    for (new i = 0; i < RouteZoneCount[playerid]; i++) {
        PlayerGangZoneDestroy(playerid, RouteZones[playerid][i]);
    }
    RouteZoneCount[playerid] = 0;
}

// Маха маршрута (само ако е от този източник; ROUTE_SRC_NONE = какъвто и да е)
stock Route_Clear(playerid, source = ROUTE_SRC_NONE)
{
    if (source != ROUTE_SRC_NONE && RouteSource[playerid] != source) return;
    RouteSource[playerid] = ROUTE_SRC_NONE;
    RouteSeq[playerid]++;           // чакащите резултати вече не важат
    Route_ClearZones(playerid);
}

// Нова цел – търси и рисува пътя
stock Route_Set(playerid, Float:x, Float:y, Float:z, source)
{
    RouteSource[playerid] = source;
    RouteTarget[playerid][0] = x;
    RouteTarget[playerid][1] = y;
    RouteTarget[playerid][2] = z;
    Route_Calculate(playerid);
}

static stock Route_Calculate(playerid)
{
    if (RouteSource[playerid] == ROUTE_SRC_NONE) return;
    new Float:px, Float:py, Float:pz;
    if (PlayerData[playerid][pInGarage]) {
        // В гаража – от входа в града
        new Float:a;
        City_GetEntrance(PlayerData[playerid][pGarageCity], px, py, pz, a);
    } else {
        GetPlayerPos(playerid, px, py, pz);
    }
    RouteFrom[playerid][0] = px;
    RouteFrom[playerid][1] = py;
    RouteFrom[playerid][2] = pz;

    new MapNode:from, MapNode:to;
    if (GetClosestMapNodeToPoint(px, py, pz, from) != 0) return;
    if (GetClosestMapNodeToPoint(RouteTarget[playerid][0], RouteTarget[playerid][1], RouteTarget[playerid][2], to) != 0) return;

    RouteSeq[playerid]++;
    RouteBusy[playerid] = true;
    RouteBusySince[playerid] = gettime();
    FindPathThreaded(from, to, "Route_OnPathFound", "ii", playerid, RouteSeq[playerid]);
}

forward Route_OnPathFound(Path:pathid, playerid, seq);
public Route_OnPathFound(Path:pathid, playerid, seq)
{
    if (!IsPlayerConnected(playerid) || RouteSeq[playerid] != seq || RouteSource[playerid] == ROUTE_SRC_NONE) {
        DestroyPath(pathid);
        return 0;
    }
    RouteBusy[playerid] = false;

    new size;
    GetPathSize(pathid, size);
    if (size > ROUTE_MAX_NODES) size = ROUTE_MAX_NODES;

    // Точките на пътя + общата дължина
    new Float:total = 0.0, MapNode:node, Float:x, Float:y, Float:z;
    for (new i = 0; i < size; i++) {
        GetPathNode(pathid, i, node);
        GetMapNodePos(node, x, y, z);
        RouteNodeBuf[i][0] = x;
        RouteNodeBuf[i][1] = y;
        if (i > 0) total += VectorSize(x - RouteNodeBuf[i - 1][0], y - RouteNodeBuf[i - 1][1], 0.0);
    }
    DestroyPath(pathid);

    Route_ClearZones(playerid);
    if (size < 2) return 1;

    // Стъпката расте при дълъг път, за да не минем лимита на квадратчетата
    new Float:step = ROUTE_STEP;
    if (total / step > float(ROUTE_MAX_ZONES)) step = total / float(ROUTE_MAX_ZONES);

    new Float:carry = 0.0;
    for (new i = 1; i < size && RouteZoneCount[playerid] < ROUTE_MAX_ZONES; i++) {
        new Float:ax = RouteNodeBuf[i - 1][0], Float:ay = RouteNodeBuf[i - 1][1];
        new Float:dx = RouteNodeBuf[i][0] - ax, Float:dy = RouteNodeBuf[i][1] - ay;
        new Float:len = VectorSize(dx, dy, 0.0);
        if (len < 0.01) continue;
        new Float:d = carry;
        while (d <= len && RouteZoneCount[playerid] < ROUTE_MAX_ZONES) {
            new Float:cx = ax + dx * d / len, Float:cy = ay + dy * d / len;
            new zone = CreatePlayerGangZone(playerid, cx - ROUTE_DOT, cy - ROUTE_DOT, cx + ROUTE_DOT, cy + ROUTE_DOT);
            if (zone == INVALID_GANG_ZONE) break;
            PlayerGangZoneShow(playerid, zone, ROUTE_COLOR);
            RouteZones[playerid][RouteZoneCount[playerid]++] = zone;
            d += step;
        }
        carry = d - len;
    }
    return 1;
}

// Всяка секунда: пристигане и преизчисляване, ако играчът се е отклонил/придвижил
stock Route_Tick(playerid)
{
    if (RouteBusy[playerid] && gettime() - RouteBusySince[playerid] > 5) RouteBusy[playerid] = false;   // отговорът се е загубил
    if (RouteSource[playerid] == ROUTE_SRC_NONE || RouteBusy[playerid] || PlayerData[playerid][pInGarage]) return;
    new Float:x, Float:y, Float:z;
    GetPlayerPos(playerid, x, y, z);

    if (VectorSize(x - RouteTarget[playerid][0], y - RouteTarget[playerid][1], 0.0) < ROUTE_ARRIVE) {
        // Отметката от картата изчезва при пристигане; GPS/курсът се махат от своите модули
        if (RouteSource[playerid] == ROUTE_SRC_MAP) Route_Clear(playerid);
        else Route_ClearZones(playerid);
        return;
    }
    if (VectorSize(x - RouteFrom[playerid][0], y - RouteFrom[playerid][1], 0.0) > ROUTE_RECALC_MOVE) {
        Route_Calculate(playerid);
    }
}

// Отметка на картата (не за админите с включен телепорт – виж admin.inc)
stock Route_OnClickMap(playerid, Float:x, Float:y, Float:z)
{
    Route_Set(playerid, x, y, z, ROUTE_SRC_MAP);
}

#else

// Плъгинът не е включен – маршрутът не прави нищо.
stock Route_OnPlayerConnect(playerid) { RouteDummy(playerid); }
stock Route_Clear(playerid, source = ROUTE_SRC_NONE) { RouteDummy(playerid + source); }
stock Route_Set(playerid, Float:x, Float:y, Float:z, source) { RouteDummy(playerid + source + floatround(x + y + z)); }
stock Route_Tick(playerid) { RouteDummy(playerid); }
stock Route_OnClickMap(playerid, Float:x, Float:y, Float:z) { RouteDummy(playerid + floatround(x + y + z)); }
static stock RouteDummy(value) { return value; }

#endif
