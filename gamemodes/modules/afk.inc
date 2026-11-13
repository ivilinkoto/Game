// AFK система.
//
// Играчът става AFK, когато:
//  - играта му е на пауза или минимизирана (клиентът спира да праща данни);
//  - не се е движил и не е натискал бутони AFK_IDLE_MINUTES минути;
//  - сам напише /afk [причина].
//
// Докато е AFK:
//  - над главата му стои надпис "AFK" с времето;
//  - времето НЕ се брои за PayDay;
//  - при AFK_KICK_MINUTES > 0 се кика след толкова минути (без админите).
// Връща се автоматично, щом помръдне или натисне бутон.

stock bool:Afk_IsAFK(playerid)
{
    return PlayerData[playerid][pAFK];
}

// От OnPlayerUpdate – клиентът е активен (играта не е на пауза).
stock Afk_OnPlayerUpdate(playerid)
{
    PlayerData[playerid][pLastUpdateTick] = GetTickCount();
}

// От OnPlayerKeyStateChange – натиснат бутон = активност.
stock Afk_OnKeyStateChange(playerid)
{
    PlayerData[playerid][pLastActiveTick] = GetTickCount();
}

// Всяка секунда за всеки влязъл играч.
stock Afk_OnPlayerTick(playerid)
{
    new now = GetTickCount();

    // Движение
    new Float:x, Float:y, Float:z;
    GetPlayerPos(playerid, x, y, z);
    new Float:dx = x - PlayerData[playerid][pActX],
        Float:dy = y - PlayerData[playerid][pActY],
        Float:dz = z - PlayerData[playerid][pActZ];
    if (dx * dx + dy * dy + dz * dz > 0.25) {
        PlayerData[playerid][pLastActiveTick] = now;
        PlayerData[playerid][pActX] = x;
        PlayerData[playerid][pActY] = y;
        PlayerData[playerid][pActZ] = z;
    }

    new bool:paused = (now - PlayerData[playerid][pLastUpdateTick] > AFK_PAUSE_MS),
        bool:idle = (now - PlayerData[playerid][pLastActiveTick] > AFK_IDLE_MINUTES * 60000),
        bool:manual = false;

    // Ръчното AFK приключва, щом играчът помръдне или натисне бутон.
    if (PlayerData[playerid][pAFKManual]) {
        if (PlayerData[playerid][pLastActiveTick] - PlayerData[playerid][pAFKStartTick] > 1000) {
            PlayerData[playerid][pAFKManual] = false;
        } else {
            manual = true;
        }
    }

    new bool:shouldBeAFK = (paused || idle || manual);

    if (shouldBeAFK && !PlayerData[playerid][pAFK]) {
        Afk_Start(playerid, false, "");
    } else if (!shouldBeAFK && PlayerData[playerid][pAFK]) {
        Afk_End(playerid);
    }

    if (PlayerData[playerid][pAFK]) {
        Afk_UpdateLabel(playerid);
        Afk_CheckKick(playerid);
    }
}

stock Afk_Start(playerid, bool:manual, const reason[])
{
    PlayerData[playerid][pAFK] = true;
    PlayerData[playerid][pAFKManual] = manual;
    PlayerData[playerid][pAFKSince] = gettime();
    PlayerData[playerid][pAFKStartTick] = GetTickCount();
    format(PlayerData[playerid][pAFKReason], 64, "%s", reason);

    if (PlayerData[playerid][pAFKLabel] != INVALID_3DTEXT_ID) {
        Delete3DTextLabel(PlayerData[playerid][pAFKLabel]);
    }
    PlayerData[playerid][pAFKLabel] = Create3DTextLabel("AFK", COLOR_BRAND, 0.0, 0.0, 0.0, 15.0,
        GetPlayerVirtualWorld(playerid), true);
    Attach3DTextLabelToPlayer(PlayerData[playerid][pAFKLabel], playerid, 0.0, 0.0, 0.45);
    Afk_UpdateLabel(playerid);

    if (manual) {
        SendClientMessage(playerid, COLOR_GREY, "Вече си AFK. Помръдни или натисни бутон, за да се върнеш.");
    }
}

stock Afk_End(playerid)
{
    new seconds = gettime() - PlayerData[playerid][pAFKSince];

    PlayerData[playerid][pAFK] = false;
    PlayerData[playerid][pAFKManual] = false;
    PlayerData[playerid][pAFKReason][0] = EOS;
    if (PlayerData[playerid][pAFKLabel] != INVALID_3DTEXT_ID) {
        Delete3DTextLabel(PlayerData[playerid][pAFKLabel]);
        PlayerData[playerid][pAFKLabel] = INVALID_3DTEXT_ID;
    }

    SendClientMessage(playerid, COLOR_BRAND, "[HOTFIX] {FFFFFF}Добре дошъл обратно! Беше AFK {FFD54A}%d мин. %d сек.{FFFFFF} Това време не се брои за PayDay.",
        seconds / 60, seconds % 60);
}

stock Afk_UpdateLabel(playerid)
{
    if (PlayerData[playerid][pAFKLabel] == INVALID_3DTEXT_ID) return;
    new seconds = gettime() - PlayerData[playerid][pAFKSince];
    if (PlayerData[playerid][pAFKReason][0] != EOS) {
        Update3DTextLabelText(PlayerData[playerid][pAFKLabel], COLOR_BRAND, "AFK {FFFFFF}%02d:%02d\n{A0A0A0}%s",
            seconds / 60, seconds % 60, PlayerData[playerid][pAFKReason]);
    } else {
        Update3DTextLabelText(PlayerData[playerid][pAFKLabel], COLOR_BRAND, "AFK {FFFFFF}%02d:%02d",
            seconds / 60, seconds % 60);
    }
}

stock Afk_CheckKick(playerid)
{
    if (AFK_KICK_MINUTES <= 0 || PlayerData[playerid][pAdmin] > 0) return;
    if (gettime() - PlayerData[playerid][pAFKSince] < AFK_KICK_MINUTES * 60) return;

    SendClientMessage(playerid, COLOR_RED, "Изгонен си, защото беше AFK повече от %d минути.", AFK_KICK_MINUTES);
    PlayerData[playerid][pAFKSince] = gettime(); // да не се повтаря съобщението до кика
    KickEx(playerid);
}

stock Afk_OnPlayerDisconnect(playerid)
{
    if (PlayerData[playerid][pAFKLabel] != INVALID_3DTEXT_ID) {
        Delete3DTextLabel(PlayerData[playerid][pAFKLabel]);
        PlayerData[playerid][pAFKLabel] = INVALID_3DTEXT_ID;
    }
}

CMD:afk(playerid, params[])
{
    if (PlayerData[playerid][pAFK]) {
        Afk_End(playerid);
        // Без това веднага би станал AFK отново заради бездействие.
        PlayerData[playerid][pLastActiveTick] = GetTickCount();
        return 1;
    }

    new reason[64];
    if (!IsNull(params)) format(reason, sizeof (reason), "%s", params);
    Afk_Start(playerid, true, reason);
    return 1;
}

CMD:afklist(playerid, params[])
{
    #pragma unused params
    new count = 0, seconds;
    SendClientMessage(playerid, COLOR_BRAND, "________ AFK играчи ________");
    for (new i = 0; i < MAX_PLAYERS; i++) {
        if (!IsPlayerConnected(i) || !PlayerData[i][pLogged] || !PlayerData[i][pAFK]) continue;
        seconds = gettime() - PlayerData[i][pAFKSince];
        SendClientMessage(playerid, COLOR_WHITE, "%s [%d] - %d мин.", PlayerData[i][pName], i, seconds / 60);
        count++;
    }
    if (count == 0) SendClientMessage(playerid, COLOR_GREY, "В момента никой не е AFK.");
    return 1;
}
