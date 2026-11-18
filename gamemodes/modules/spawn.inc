// Къде се появява играчът: гараж, дом или последното място (Меню > Настройки > Спаун).
//
// "Последно място" важи за първия спаун след влизане (докато е онлайн, след смърт - гараж).

enum {
    SPAWN_GARAGE,
    SPAWN_HOME,
    SPAWN_LAST,
    SPAWN_COUNT
}

stock const SpawnNames[SPAWN_COUNT][] = {"Гараж", "Дом", "Последно място"};

static SpawnPref[MAX_PLAYERS];
static bool:SpawnDone[MAX_PLAYERS];

stock Spawn_Init()
{
    Database_AddColumn("accounts", "spawn_pref", "INTEGER NOT NULL DEFAULT 0");
    // Еднократно: който е имал "спаун в дома" от менюто на къщата, получава "Дом"
    if (Database_GetMeta("spawn_pref_migrated") == 0) {
        Database_Exec("UPDATE accounts SET spawn_pref = 1 WHERE id IN (SELECT owner FROM houses WHERE spawn_home = 1 AND owner > 0)");
        Database_SetMeta("spawn_pref_migrated", 1);
    }
}

stock Spawn_OnPlayerConnect(playerid)
{
    SpawnPref[playerid] = SPAWN_GARAGE;
    SpawnDone[playerid] = false;
}

stock Spawn_OnLogin(playerid)
{
    new q[96];
    format(q, sizeof (q), "SELECT spawn_pref FROM accounts WHERE id = %d", PlayerData[playerid][pID]);
    new DBResult:r = Database_Query(q);
    if (r != DBResult:0) {
        if (DB_GetRowCount(r) > 0) SpawnPref[playerid] = DB_GetFieldIntByName(r, "spawn_pref");
        DB_FreeResultSet(r);
    }
    if (SpawnPref[playerid] < 0 || SpawnPref[playerid] >= SPAWN_COUNT) SpawnPref[playerid] = SPAWN_GARAGE;
}

stock Spawn_GetPref(playerid)
{
    return SpawnPref[playerid];
}

stock Spawn_SetPref(playerid, pref)
{
    if (pref < 0 || pref >= SPAWN_COUNT) return;
    SpawnPref[playerid] = pref;
    new q[96];
    format(q, sizeof (q), "UPDATE accounts SET spawn_pref = %d WHERE id = %d", pref, PlayerData[playerid][pID]);
    Database_Exec(q);
}

// Следващата възможност (за бутона в настройките)
stock Spawn_Cycle(playerid)
{
    Spawn_SetPref(playerid, (SpawnPref[playerid] + 1) % SPAWN_COUNT);
}

// "Гараж" / "Дом (нямаш дом - гараж)"
stock Spawn_Describe(playerid, dest[], size = sizeof (dest))
{
    if (SpawnPref[playerid] == SPAWN_HOME && House_GetOwned(playerid) == -1) {
        format(dest, size, C_BRAND "Дом " C_GREY "(нямаш - гараж)");
    } else {
        format(dest, size, C_BRAND "%s", SpawnNames[SpawnPref[playerid]]);
    }
}

// OnPlayerSpawn
stock Spawn_OnPlayerSpawn(playerid)
{
    new bool:first = !SpawnDone[playerid];
    SpawnDone[playerid] = true;

    if (SpawnPref[playerid] == SPAWN_HOME && House_EnterOwned(playerid)) {
        City_ShowGarageIcon(playerid);
        return;
    }
    if (SpawnPref[playerid] == SPAWN_LAST && first && PlayerData[playerid][pHasPos]) {
        City_ShowGarageIcon(playerid);
        SetPlayerInterior(playerid, 0);
        SetPlayerVirtualWorld(playerid, 0);
        SetPlayerPos(playerid, PlayerData[playerid][pPosX], PlayerData[playerid][pPosY], PlayerData[playerid][pPosZ]);
        SetPlayerFacingAngle(playerid, PlayerData[playerid][pPosA]);
        SetCameraBehindPlayer(playerid);
        return;
    }
    Garage_OnPlayerSpawn(playerid);
}
