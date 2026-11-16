// Чат: общ чат, лични съобщения (/pm, /r) и CB радио (/cb, /cbch).
// Не е RP сървър – без /me, /do и т.н.

#define CHAT_COOLDOWN_MS    1000

static stock bool:Chat_CheckSpam(playerid)
{
    new now = GetTickCount();
    if (now - PlayerData[playerid][pLastMsgTick] < CHAT_COOLDOWN_MS) {
        SendError(playerid, "Не пиши толкова бързо.");
        return false;
    }
    PlayerData[playerid][pLastMsgTick] = now;
    return true;
}

// Общ чат
stock Chat_OnPlayerText(playerid, const text[])
{
    if (!PlayerData[playerid][pLogged]) return 0;
    if (!Chat_CheckSpam(playerid)) return 0;

    new color = GetPlayerColor(playerid);
    if (color == 0) color = COLOR_WHITE;
    SendClientMessageToAll(COLOR_WHITE, "{%06x}%s" C_GREY " [%d]" C_WHITE ": %s",
        color >>> 8, PlayerData[playerid][pName], playerid, text);
    return 0;
}

// ---------- Лични съобщения ----------

static stock Chat_SendPM(playerid, targetid, const text[])
{
    if (targetid == playerid) return SendError(playerid, "Не можеш да пишеш на себе си.");
    if (!Chat_CheckSpam(playerid)) return 1;

    SendClientMessage(targetid, COLOR_PM, "[ЛС от %s (%d)] %s", PlayerData[playerid][pName], playerid, text);
    SendClientMessage(playerid, COLOR_PM, "[ЛС до %s (%d)] %s", PlayerData[targetid][pName], targetid, text);
    PlayerPlaySound(targetid, 1085, 0.0, 0.0, 0.0);
    PlayerData[targetid][pLastPM] = playerid;
    return 1;
}

CMD:pm(playerid, params[])
{
    new target[24], text[144];
    if (!GetParam(params, 0, target) || !GetParamRest(params, 1, text)) {
        return SendUsage(playerid, "/pm [ID или ник] [текст]");
    }
    new targetid = FindPlayer(target);
    if (targetid == INVALID_PLAYER_ID) return SendError(playerid, "Няма такъв играч (или има няколко с това име).");
    return Chat_SendPM(playerid, targetid, text);
}

CMD:r(playerid, params[])
{
    if (IsNull(params)) return SendUsage(playerid, "/r [текст] – отговор на последното ЛС");
    new targetid = PlayerData[playerid][pLastPM];
    if (targetid == INVALID_PLAYER_ID || !IsPlayerLogged(targetid)) return SendError(playerid, "Няма на кого да отговориш.");
    return Chat_SendPM(playerid, targetid, params);
}

// ---------- CB радио ----------
// Чува се от всички, които са на същия канал и са с включено радио –
// без значение къде са на картата.

stock CB_Send(playerid, const text[])
{
    if (!PlayerData[playerid][pCBOn]) return SendError(playerid, "CB радиото ти е изключено. Включи го: Меню > Настройки.");
    if (!Chat_CheckSpam(playerid)) return 1;

    new channel = PlayerData[playerid][pCBChannel];
    for (new i = 0; i < MAX_PLAYERS; i++) {
        if (!IsPlayerLogged(i) || !PlayerData[i][pCBOn] || PlayerData[i][pCBChannel] != channel) continue;
        SendClientMessage(i, COLOR_CB, "[CB %d] %s: " C_WHITE "%s", channel, PlayerData[playerid][pName], text);
    }
    return 1;
}

stock CB_CountListeners(channel)
{
    new count = 0;
    for (new i = 0; i < MAX_PLAYERS; i++) {
        if (IsPlayerLogged(i) && PlayerData[i][pCBOn] && PlayerData[i][pCBChannel] == channel) count++;
    }
    return count;
}

stock bool:CB_SetChannel(playerid, channel)
{
    if (channel < 1 || channel > CB_MAX_CHANNEL) return false;
    PlayerData[playerid][pCBChannel] = channel;
    PlayerData[playerid][pCBOn] = true;
    SendClientMessage(playerid, COLOR_CB, "[CB] " C_WHITE "Канал " C_CB "%d" C_WHITE ". Слушат: %d.", channel, CB_CountListeners(channel));
    return true;
}

CMD:cb(playerid, params[])
{
    if (IsNull(params)) {
        new cbState[8];
        if (PlayerData[playerid][pCBOn]) format(cbState, sizeof (cbState), "вкл.");
        else format(cbState, sizeof (cbState), "изкл.");
        SendClientMessage(playerid, COLOR_CB, "[CB] " C_WHITE "Канал %d (%s). Пиши: /cb [текст] · смени канала: /cbch [1-%d]",
            PlayerData[playerid][pCBChannel], cbState, CB_MAX_CHANNEL);
        return 1;
    }
    return CB_Send(playerid, params);
}

CMD:cbch(playerid, params[])
{
    if (IsNull(params) || !IsNumeric(params)) {
        new usage[48];
        format(usage, sizeof (usage), "/cbch [1-%d]  (по подразбиране %d)", CB_MAX_CHANNEL, CB_DEFAULT_CHANNEL);
        return SendUsage(playerid, usage);
    }
    if (!CB_SetChannel(playerid, strval(params))) {
        return SendError(playerid, "Няма такъв канал.");
    }
    return 1;
}
