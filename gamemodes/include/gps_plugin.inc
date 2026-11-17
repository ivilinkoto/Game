// Функции на плъгина samp-gps-plugin (github.com/AmyrAhmady/samp-gps-plugin).
// Плъгинът намира път по пътната мрежа на San Andreas (данните са в scriptfiles/GPS.dat).
// Тук са само функциите, които ползваме – по документацията на плъгина.
// Инсталира се от scripts/install-gps.sh (deploy го пуска сам, ако липсва).

#if defined _gps_plugin_included
    #endinput
#endif
#define _gps_plugin_included

// Имената и подписите са сверени с GPS.inc на плъгина (издание 1.4.1).
const MapNode:INVALID_MAP_NODE_ID = MapNode:-1;
const Path:INVALID_GPS_PATH_ID = Path:-1;

native GetClosestMapNodeToPoint(Float:x, Float:y, Float:z, &MapNode:nodeid, MapNode:ignorednode = INVALID_MAP_NODE_ID);
native FindPathThreaded(MapNode:source, MapNode:target, const callback[], const format[] = "", {Float, _}:...);
native GetPathSize(Path:pathid, &size);
native GetPathNode(Path:pathid, index, &MapNode:nodeid);
native GetMapNodePos(MapNode:nodeid, &Float:x, &Float:y, &Float:z);
native DestroyPath(Path:pathid);
