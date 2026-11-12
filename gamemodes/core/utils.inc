// Общи помощни функции

// ---------- Съобщения ----------
// Потребителският текст винаги минава като аргумент към "%s",
// за да не се тълкува "%" от играча като форматиране.

stock SendMsg(playerid, colour, const msg[])
{
    SendClientMessage(playerid, colour, "%s", msg);
    return 1;
}

stock SendError(playerid, const msg[])
{
    SendClientMessage(playerid, COLOR_RED, "[Грешка] {FFFFFF}%s", msg);
    return 1;
}

stock SendInfo(playerid, const msg[])
{
    SendClientMessage(playerid, COLOR_LIGHTBLUE, "[Инфо] {FFFFFF}%s", msg);
    return 1;
}

stock SendUsage(playerid, const msg[])
{
    SendClientMessage(playerid, COLOR_GREY, "Използване: %s", msg);
    return 1;
}

stock ClearChat(playerid, lines = 20)
{
    for (new i = 0; i < lines; i++) {
        SendClientMessage(playerid, COLOR_WHITE, " ");
    }
}

// Кик със забавяне, за да стигнат съобщенията до клиента преди това.
forward Timer_DelayedKick(playerid);
public Timer_DelayedKick(playerid)
{
    Kick(playerid);
    return 1;
}

stock KickEx(playerid)
{
    SetTimerEx("Timer_DelayedKick", 500, false, "i", playerid);
}

// ---------- Имена ----------

// Име_Фамилия -> "Име Фамилия"
stock GetRPName(playerid, dest[], size = sizeof (dest))
{
    format(dest, size, "%s", PlayerData[playerid][pName]);
    for (new i = 0; dest[i] != EOS; i++) {
        if (dest[i] == '_') dest[i] = ' ';
    }
}

// Проверява формат Име_Фамилия (само латински букви, главни начални)
stock bool:IsValidRPName(const name[])
{
    new len = strlen(name), underscore = -1;
    if (len < 5 || len > MAX_PLAYER_NAME) return false;

    for (new i = 0; i < len; i++) {
        new c = name[i];
        if (c == '_') {
            if (underscore != -1) return false;
            underscore = i;
        } else if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))) {
            return false;
        }
    }
    if (underscore < 2 || underscore > len - 3) return false;
    if (name[0] < 'A' || name[0] > 'Z') return false;
    if (name[underscore + 1] < 'A' || name[underscore + 1] > 'Z') return false;
    return true;
}

// ---------- Параметри на команди ----------

// Празен параметър (командният процесор подава "\1" вместо "")
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

// Връща думата с номер index (от 0), разделени с интервали.
stock bool:GetParam(const src[], index, dest[], size = sizeof (dest))
{
    dest[0] = EOS;
    if (IsNull(src)) return false;

    new pos = 0, len = strlen(src), current = 0;
    while (pos < len) {
        while (pos < len && src[pos] == ' ') pos++;
        if (pos >= len) break;

        new start = pos;
        while (pos < len && src[pos] != ' ') pos++;

        if (current == index) {
            strmid(dest, src, start, pos, size);
            return true;
        }
        current++;
    }
    return false;
}

// Връща всичко от думата с номер index до края (за причини, текст и т.н.)
stock bool:GetParamRest(const src[], index, dest[], size = sizeof (dest))
{
    dest[0] = EOS;
    if (IsNull(src)) return false;

    new pos = 0, len = strlen(src), current = 0;
    while (pos < len) {
        while (pos < len && src[pos] == ' ') pos++;
        if (pos >= len) break;

        if (current == index) {
            strmid(dest, src, pos, len, size);
            return true;
        }
        while (pos < len && src[pos] != ' ') pos++;
        current++;
    }
    return false;
}

// Взима цяло число от параметър. Връща false, ако липсва или не е число.
stock bool:GetParamInt(const src[], index, &value)
{
    new buf[16];
    if (!GetParam(src, index, buf) || !IsNumeric(buf)) return false;
    value = strval(buf);
    return true;
}

// Намира играч по ID или част от името. INVALID_PLAYER_ID ако няма/има няколко.
stock FindPlayer(const str[])
{
    if (IsNumeric(str)) {
        new id = strval(str);
        if (0 <= id < MAX_PLAYERS && IsPlayerConnected(id)) return id;
        return INVALID_PLAYER_ID;
    }

    new name[MAX_PLAYER_NAME + 1], found = INVALID_PLAYER_ID;
    for (new i = 0; i < MAX_PLAYERS; i++) {
        if (!IsPlayerConnected(i)) continue;
        GetPlayerName(i, name, sizeof (name));
        if (strfind(name, str, true) != -1) {
            if (found != INVALID_PLAYER_ID) return INVALID_PLAYER_ID; // нееднозначно
            found = i;
        }
    }
    return found;
}

// Взима играч от параметър и пише грешка, ако няма такъв.
stock bool:GetParamPlayer(playerid, const src[], index, &targetid)
{
    new buf[MAX_PLAYER_NAME + 1];
    if (!GetParam(src, index, buf)) return false;

    targetid = FindPlayer(buf);
    if (targetid == INVALID_PLAYER_ID || !PlayerData[targetid][pLogged]) {
        SendError(playerid, "Няма такъв играч (или не е влязъл в акаунта си).");
        targetid = INVALID_PLAYER_ID;
        return false;
    }
    return true;
}

// ---------- Разстояния ----------

stock bool:IsPlayerNearPlayer(playerid, targetid, Float:radius)
{
    new Float:x, Float:y, Float:z;
    GetPlayerPos(targetid, x, y, z);
    if (GetPlayerVirtualWorld(playerid) != GetPlayerVirtualWorld(targetid)) return false;
    if (GetPlayerInterior(playerid) != GetPlayerInterior(targetid)) return false;
    return (GetPlayerDistanceFromPoint(playerid, x, y, z) <= radius);
}

// Изпраща съобщение на всички наоколо, по-далечните виждат по-бледо.
stock ProxMessage(playerid, Float:radius, colour, const msg[], bool:fade = true)
{
    new Float:x, Float:y, Float:z;
    GetPlayerPos(playerid, x, y, z);
    new vw = GetPlayerVirtualWorld(playerid),
        interior = GetPlayerInterior(playerid);

    for (new i = 0; i < MAX_PLAYERS; i++) {
        if (!IsPlayerConnected(i) || !PlayerData[i][pLogged]) continue;
        if (GetPlayerVirtualWorld(i) != vw || GetPlayerInterior(i) != interior) continue;

        new Float:dist = GetPlayerDistanceFromPoint(i, x, y, z);
        if (dist > radius) continue;

        new c = colour;
        if (fade) {
            if (dist > radius * 0.75) c = COLOR_FADE3;
            else if (dist > radius * 0.5) c = COLOR_FADE2;
            else if (dist > radius * 0.25) c = COLOR_FADE1;
        }
        SendClientMessage(i, c, "%s", msg);
    }
}

// ---------- SQL ----------

// Удвоява апострофите, за да е безопасен текстът в SQL заявка.
stock SQL_Escape(const src[], dest[], size = sizeof (dest))
{
    new j = 0;
    for (new i = 0; src[i] != EOS && j < size - 2; i++) {
        if (src[i] == '\'') dest[j++] = '\'';
        dest[j++] = src[i];
    }
    dest[j] = EOS;
}

stock GenerateSalt(dest[], length, size = sizeof (dest))
{
    static const chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    if (length >= size) length = size - 1;
    for (new i = 0; i < length; i++) {
        dest[i] = chars[random(sizeof (chars) - 1)];
    }
    dest[length] = EOS;
}

// ---------- Форматиране ----------

// 1234567 -> "1,234,567"
stock FormatNumber(value, dest[], size = sizeof (dest))
{
    new tmp[16];
    format(tmp, sizeof (tmp), "%d", value);

    new len = strlen(tmp), start = (tmp[0] == '-') ? 1 : 0, j = 0;
    for (new i = 0; i < len && j < size - 1; i++) {
        dest[j++] = tmp[i];
        new remaining = len - i - 1;
        if (i >= start && remaining > 0 && remaining % 3 == 0 && j < size - 1) {
            dest[j++] = ',';
        }
    }
    dest[j] = EOS;
}
