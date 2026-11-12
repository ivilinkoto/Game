// Лек команден процесор (без плъгини).
//
// Дефиниране на команда:
//     CMD:pay(playerid, params[]) { ... return 1; }
//
// Имената на командите са с малки букви, най-много 27 символа.
// Ако няма параметри, params е "\1" – провери с IsNull(params).

#define CMD:%1(%2) forward cmd_%1(%2); public cmd_%1(%2)

stock Commands_Process(playerid, cmdtext[])
{
    if (!PlayerData[playerid][pLogged]) {
        SendError(playerid, "Трябва първо да влезеш в акаунта си.");
        return 1;
    }

    new cmd[28], pos = 1, i = 0;
    while (cmdtext[pos] != EOS && cmdtext[pos] != ' ') {
        if (i >= sizeof (cmd) - 1) {
            SendError(playerid, "Непозната команда. Виж /help.");
            return 1;
        }
        cmd[i++] = tolower(cmdtext[pos++]);
    }
    cmd[i] = EOS;

    if (i == 0) {
        SendError(playerid, "Непозната команда. Виж /help.");
        return 1;
    }

    while (cmdtext[pos] == ' ') pos++;

    new funcname[32];
    format(funcname, sizeof (funcname), "cmd_%s", cmd);

    new result;
    if (cmdtext[pos] == EOS) {
        result = CallLocalFunction(funcname, "is", playerid, "\1");
    } else {
        result = CallLocalFunction(funcname, "is", playerid, cmdtext[pos]);
    }

    if (!result) {
        SendError(playerid, "Непозната команда. Виж /help.");
    }
    return 1;
}
