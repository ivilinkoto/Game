// Общи помощни функции

// ---------- Съобщения ----------
// Текстът минава като аргумент към "%s", за да не се тълкува "%" от играча.

stock SendError(playerid, const msg[])
{
    SendClientMessage(playerid, COLOR_RED, "[!] {FFFFFF}%s", msg);
    return 1;
}

stock SendInfo(playerid, const msg[])
{
    SendClientMessage(playerid, COLOR_BRAND, "[HOTFIX] {FFFFFF}%s", msg);
    return 1;
}

stock ClearChat(playerid, lines = 20)
{
    for (new i = 0; i < lines; i++) {
        SendClientMessage(playerid, COLOR_WHITE, " ");
    }
}

// Кик със забавяне, за да стигнат съобщенията до клиента.
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

// Ivan_Petrov -> "Ivan Petrov"
stock GetRPName(playerid, dest[], size = sizeof (dest))
{
    format(dest, size, "%s", PlayerData[playerid][pName]);
    for (new i = 0; dest[i] != EOS; i++) {
        if (dest[i] == '_') dest[i] = ' ';
    }
}

// Формат Име_Фамилия: латински букви, главни начални, едно "_".
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

// ---------- Проверки на въведен текст ----------

stock bool:IsNumeric(const str[])
{
    if (str[0] == EOS) return false;
    for (new i = 0; str[i] != EOS; i++) {
        if (str[i] < '0' || str[i] > '9') return false;
    }
    return true;
}

// Проста проверка: нещо@нещо.нещо, само латиница, цифри и . _ - +
stock bool:IsValidEmail(const email[])
{
    new len = strlen(email), at = -1, lastDot = -1;
    if (len < 6 || len > 64) return false;

    for (new i = 0; i < len; i++) {
        new c = email[i];
        if (c == '@') {
            if (at != -1) return false;
            at = i;
        } else if (c == '.') {
            if (at != -1) lastDot = i;
        } else if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '+')) {
            return false;
        }
    }
    // поне 1 символ преди @, поне 1 между @ и точката, поне 2 след точката
    return (at >= 1 && lastDot > at + 1 && lastDot <= len - 3);
}

// Код: само латински букви и цифри
stock bool:IsAlphaNumeric(const str[])
{
    if (str[0] == EOS) return false;
    for (new i = 0; str[i] != EOS; i++) {
        new c = str[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))) return false;
    }
    return true;
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
