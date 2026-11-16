// Опит и нива (1–5). Нивата отключват камионите в етап 2.
// Прагът е в core/config.inc (LevelExp).

stock Level_ForExp(exp)
{
    new level = 1;
    for (new i = 0; i < MAX_LEVEL; i++) {
        if (exp >= LevelExp[i]) level = i + 1;
    }
    return level;
}

// Опит до следващото ниво (0 = максимално ниво).
stock Level_ExpToNext(playerid)
{
    new level = PlayerData[playerid][pLevel];
    if (level >= MAX_LEVEL) return 0;
    return LevelExp[level] - PlayerData[playerid][pExp];
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
