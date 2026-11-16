// Общи помощни функции

// ---------- Съобщения ----------
// Текстът на играча винаги минава като аргумент към "%s",
// за да не се тълкува "%" в него.

stock SendError(playerid, const msg[])
{
    SendClientMessage(playerid, COLOR_RED, "[!] " C_WHITE "%s", msg);
    return 1;
}

stock SendInfo(playerid, const msg[])
{
    SendClientMessage(playerid, COLOR_BRAND, "[CARGO] " C_WHITE "%s", msg);
    return 1;
}

stock SendUsage(playerid, const usage[])
{
    SendClientMessage(playerid, COLOR_GREY, "Използване: %s", usage);
    return 1;
}

stock ClearChat(playerid, lines = 20)
{
    for (new i = 0; i < lines; i++) {
        SendClientMessage(playerid, COLOR_WHITE, " ");
    }
}

// Кик със забавяне, за да стигнат съобщенията/диалогът до клиента.
forward Timer_DelayedKick(playerid, seq);
public Timer_DelayedKick(playerid, seq)
{
    if (IsPlayerConnected(playerid) && PlayerData[playerid][pConnSeq] == seq) {
        Kick(playerid);
    }
    return 1;
}

stock KickEx(playerid)
{
    SetTimerEx("Timer_DelayedKick", 500, false, "ii", playerid, PlayerData[playerid][pConnSeq]);
}

// ---------- Текст ----------

// Командният процесор подава "\1" вместо празен низ.
stock bool:IsNull(const str[])
{
    return (str[0] == EOS || (str[0] == '\1' && str[1] == EOS));
}

stock bool:IsNumeric(const str[])
{
    new i = 0;
    if (str[0] == '-') i = 1;
    if (str[i] == EOS) return false;
    for (; str[i] != EOS; i++) {
        if (str[i] < '0' || str[i] > '9') return false;
    }
    return true;
}

// Ник: 3-20 символа, латински букви, цифри, "_" и ".". Без RP формат.
stock bool:IsValidNickname(const name[])
{
    new len = strlen(name);
    if (len < 3 || len > 20) return false;
    for (new i = 0; i < len; i++) {
        new c = name[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '.')) {
            return false;
        }
    }
    return true;
}

// Удвоява ' за SQL низове.
stock SQL_Escape(const src[], dest[], size = sizeof (dest))
{
    new j = 0;
    for (new i = 0; src[i] != EOS && j < size - 2; i++) {
        if (src[i] == '\'') dest[j++] = '\'';
        dest[j++] = src[i];
    }
    dest[j] = EOS;
}

// 1234567 -> "1,234,567"
stock FormatMoney(amount, dest[], size = sizeof (dest))
{
    new raw[16], neg = (amount < 0);
    format(raw, sizeof (raw), "%d", neg ? -amount : amount);
    new len = strlen(raw), j = 0;
    if (neg && j < size - 1) dest[j++] = '-';
    for (new i = 0; i < len && j < size - 1; i++) {
        if (i > 0 && (len - i) % 3 == 0 && j < size - 1) dest[j++] = ',';
        dest[j++] = raw[i];
    }
    dest[j] = EOS;
}

// ---------- Параметри на команди ----------

// Взима N-тата дума (от 0) от params.
stock GetParam(const params[], index, dest[], size = sizeof (dest))
{
    dest[0] = EOS;
    if (IsNull(params)) return 0;
    new pos = 0, word = 0;
    while (params[pos] != EOS) {
        while (params[pos] == ' ') pos++;
        if (params[pos] == EOS) break;
        new start = pos;
        while (params[pos] != EOS && params[pos] != ' ') pos++;
        if (word == index) {
            new len = pos - start;
            if (len > size - 1) len = size - 1;
            for (new i = 0; i < len; i++) dest[i] = params[start + i];
            dest[len] = EOS;
            return 1;
        }
        word++;
    }
    return 0;
}

// Всичко след N-тата дума (за текст с интервали, напр. причина).
stock GetParamRest(const params[], index, dest[], size = sizeof (dest))
{
    dest[0] = EOS;
    if (IsNull(params)) return 0;
    new pos = 0, word = 0;
    while (params[pos] != EOS) {
        while (params[pos] == ' ') pos++;
        if (params[pos] == EOS) break;
        if (word == index) {
            format(dest, size, "%s", params[pos]);
            return 1;
        }
        while (params[pos] != EOS && params[pos] != ' ') pos++;
        word++;
    }
    return 0;
}

// ID или част от името -> playerid (само влезли играчи).
stock FindPlayer(const param[])
{
    if (param[0] == EOS) return INVALID_PLAYER_ID;
    if (IsNumeric(param)) {
        new id = strval(param);
        if (id >= 0 && id < MAX_PLAYERS && IsPlayerLogged(id)) return id;
        return INVALID_PLAYER_ID;
    }
    new found = INVALID_PLAYER_ID;
    for (new i = 0; i < MAX_PLAYERS; i++) {
        if (!IsPlayerLogged(i)) continue;
        if (strfind(PlayerData[i][pName], param, true) != -1) {
            if (found != INVALID_PLAYER_ID) return INVALID_PLAYER_ID; // повече от един
            found = i;
        }
    }
    return found;
}
