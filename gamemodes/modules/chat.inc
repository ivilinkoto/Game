// RP чат: говор наблизо, /me, /do, викане, тихо, OOC и лични съобщения.

// ---------- Дневник на чата (за контекста в репортите) ----------

#define CHAT_LOG_SIZE 60

enum E_CHAT_LOG {
    clTime,
    Float:clX,
    Float:clY,
    Float:clZ,
    clWorld,
    clText[144]
}
static gChatLog[CHAT_LOG_SIZE][E_CHAT_LOG], gChatLogPos;

stock Chat_Log(playerid, const text[])
{
    new Float:x, Float:y, Float:z;
    GetPlayerPos(playerid, x, y, z);
    gChatLog[gChatLogPos][clTime] = gettime();
    gChatLog[gChatLogPos][clX] = x;
    gChatLog[gChatLogPos][clY] = y;
    gChatLog[gChatLogPos][clZ] = z;
    gChatLog[gChatLogPos][clWorld] = GetPlayerVirtualWorld(playerid);
    format(gChatLog[gChatLogPos][clText], 144, "%s", text);
    gChatLogPos = (gChatLogPos + 1) % CHAT_LOG_SIZE;
}

// Добавя към dest последните maxLines съобщения, казани в радиус около играча.
stock Chat_GetNearbyLog(playerid, dest[], size, Float:radius, seconds, maxLines)
{
    new found[CHAT_LOG_SIZE], count = 0, now = gettime(), vw = GetPlayerVirtualWorld(playerid);

    // От най-новото към най-старото
    for (new n = 1; n <= CHAT_LOG_SIZE && count < maxLines; n++) {
        new idx = (gChatLogPos - n + CHAT_LOG_SIZE) % CHAT_LOG_SIZE;
        if (gChatLog[idx][clTime] == 0 || now - gChatLog[idx][clTime] > seconds) continue;
        if (gChatLog[idx][clWorld] != vw) continue;
        if (GetPlayerDistanceFromPoint(playerid, gChatLog[idx][clX], gChatLog[idx][clY], gChatLog[idx][clZ]) > radius) continue;
        found[count++] = idx;
    }
    if (count == 0) {
        strcat(dest, "(няма)", size);
        return;
    }
    // Записваме от най-старото към най-новото
    new line[180];
    for (new k = count - 1; k >= 0; k--) {
        new idx = found[k];
        format(line, sizeof (line), "[-%dс] %s\n", now - gChatLog[idx][clTime], gChatLog[idx][clText]);
        strcat(dest, line, size);
    }
}

stock Chat_OnPlayerText(playerid, const text[])
{
    if (!PlayerData[playerid][pLogged]) return;
    PlayerData[playerid][pLastActiveTick] = GetTickCount(); // говори = не е AFK

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
