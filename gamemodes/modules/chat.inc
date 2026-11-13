// RP чат: говор наблизо, /me, /do, викане, тихо, OOC и лични съобщения.

stock Chat_OnPlayerText(playerid, const text[])
{
    if (!PlayerData[playerid][pLogged]) return;

    new rpName[MAX_PLAYER_NAME + 1], msg[200];
    GetRPName(playerid, rpName);
    format(msg, sizeof (msg), "%s казва: %s", rpName, text);
    ProxMessage(playerid, CHAT_RADIUS_NORMAL, COLOR_WHITE, msg);
    SetPlayerChatBubble(playerid, "%s", COLOR_WHITE, CHAT_RADIUS_NORMAL, 5000, text);
}

CMD:me(playerid, params[])
{
    if (IsNull(params)) return SendUsage(playerid, "/me [действие]  (пример: /me отваря вратата)");
    new rpName[MAX_PLAYER_NAME + 1], msg[200];
    GetRPName(playerid, rpName);
    format(msg, sizeof (msg), "* %s %s", rpName, params);
    ProxMessage(playerid, CHAT_RADIUS_ME, COLOR_PURPLE, msg, false);
    return 1;
}

CMD:do(playerid, params[])
{
    if (IsNull(params)) return SendUsage(playerid, "/do [описание]  (пример: /do Вратата е заключена.)");
    new rpName[MAX_PLAYER_NAME + 1], msg[200];
    GetRPName(playerid, rpName);
    format(msg, sizeof (msg), "* %s (( %s ))", params, rpName);
    ProxMessage(playerid, CHAT_RADIUS_ME, COLOR_PURPLE, msg, false);
    return 1;
}

CMD:s(playerid, params[])
{
    if (IsNull(params)) return SendUsage(playerid, "/s [текст] - викаш");
    new rpName[MAX_PLAYER_NAME + 1], msg[200];
    GetRPName(playerid, rpName);
    format(msg, sizeof (msg), "%s вика: %s!", rpName, params);
    ProxMessage(playerid, CHAT_RADIUS_SHOUT, COLOR_WHITE, msg);
    return 1;
}

CMD:l(playerid, params[])
{
    if (IsNull(params)) return SendUsage(playerid, "/l [текст] - говориш тихо");
    new rpName[MAX_PLAYER_NAME + 1], msg[200];
    GetRPName(playerid, rpName);
    format(msg, sizeof (msg), "%s казва тихо: %s", rpName, params);
    ProxMessage(playerid, CHAT_RADIUS_LOW, COLOR_WHITE, msg);
    return 1;
}

// Локален OOC (извън ролята)
CMD:b(playerid, params[])
{
    if (IsNull(params)) return SendUsage(playerid, "/b [текст] - локален OOC чат");
    new rpName[MAX_PLAYER_NAME + 1], msg[200];
    GetRPName(playerid, rpName);
    format(msg, sizeof (msg), "(( %s [%d]: %s ))", rpName, playerid, params);
    ProxMessage(playerid, CHAT_RADIUS_NORMAL, COLOR_OOC, msg);
    return 1;
}

// Глобален OOC
CMD:o(playerid, params[])
{
    if (IsNull(params)) return SendUsage(playerid, "/o [текст] - OOC чат за всички");
    new rpName[MAX_PLAYER_NAME + 1];
    GetRPName(playerid, rpName);
    SendClientMessageToAll(COLOR_OOC, "(( [OOC] %s [%d]: %s ))", rpName, playerid, params);
    return 1;
}

CMD:pm(playerid, params[])
{
    new targetid, text[128];
    if (!GetParamRest(params, 1, text)) return SendUsage(playerid, "/pm [ID/име] [текст]");
    if (!GetParamPlayer(playerid, params, 0, targetid)) return 1;
    if (targetid == playerid) return SendError(playerid, "Не можеш да пишеш на себе си.");

    Chat_SendPM(playerid, targetid, text);
    return 1;
}

// Отговор на последното лично съобщение
CMD:r(playerid, params[])
{
    if (IsNull(params)) return SendUsage(playerid, "/r [текст] - отговор на последното ЛС");
    new targetid = PlayerData[playerid][pLastPM];
    if (targetid == INVALID_PLAYER_ID || !IsPlayerConnected(targetid) || !PlayerData[targetid][pLogged]) {
        return SendError(playerid, "Няма на кого да отговориш.");
    }
    Chat_SendPM(playerid, targetid, params);
    return 1;
}

stock Chat_SendPM(playerid, targetid, const text[])
{
    new from[MAX_PLAYER_NAME + 1], to[MAX_PLAYER_NAME + 1];
    GetRPName(playerid, from);
    GetRPName(targetid, to);
    SendClientMessage(targetid, COLOR_PM, "(( ЛС от %s [%d]: %s ))", from, playerid, text);
    SendClientMessage(playerid, COLOR_PM, "(( ЛС до %s [%d]: %s ))", to, targetid, text);
    PlayerData[targetid][pLastPM] = playerid;
}
