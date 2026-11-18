// Опит и нива - без горна граница. Нивата отключват камиони и категории.
// Таблицата за първите нива е в core/config.inc (LevelExpTable).

// Общ опит, нужен за ниво level
stock Level_ExpFor(level)
{
    if (level <= 1) return 0;
    if (level <= LEVEL_TABLE_SIZE) return LevelExpTable[level - 1];
    new exp = LevelExpTable[LEVEL_TABLE_SIZE - 1];
    new step = LevelExpTable[LEVEL_TABLE_SIZE - 1] - LevelExpTable[LEVEL_TABLE_SIZE - 2];
    for (new l = LEVEL_TABLE_SIZE + 1; l <= level; l++) {
        step += 400;
        exp += step;
    }
    return exp;
}

stock Level_ForExp(exp)
{
    new level = 1;
    while (level < LEVEL_MAX_ADMIN && exp >= Level_ExpFor(level + 1)) level++;
    return level;
}

// Опит до следващото ниво
stock Level_ExpToNext(playerid)
{
    return Level_ExpFor(PlayerData[playerid][pLevel] + 1) - PlayerData[playerid][pExp];
}

stock Level_GiveExp(playerid, amount)
{
    if (!PlayerData[playerid][pLogged] || amount <= 0) return;

    PlayerData[playerid][pExp] += amount;
    new newLevel = Level_ForExp(PlayerData[playerid][pExp]);

    if (newLevel > PlayerData[playerid][pLevel]) {
        PlayerData[playerid][pLevel] = newLevel;
        SetPlayerScore(playerid, newLevel);
        PlayerPlaySound(playerid, 1058, 0.0, 0.0, 0.0);
        GameTextForPlayer(playerid, "~y~LEVEL UP!~n~~w~LEVEL %d", 4000, 1, newLevel);
        SendClientMessage(playerid, COLOR_BRAND, "[CARGO] " C_WHITE "Поздравления! Вече си " C_BRAND "ниво %d" C_WHITE ".", newLevel);
    }

    new q[128];
    format(q, sizeof (q), "UPDATE accounts SET exp = %d, level = %d WHERE id = %d",
        PlayerData[playerid][pExp], PlayerData[playerid][pLevel], PlayerData[playerid][pID]);
    Database_Exec(q);
}
