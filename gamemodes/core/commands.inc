// Команден процесор (без плъгини).
//
// Нова команда:
//     CMD:primer(playerid, params[])
//     {
//         if (IsNull(params)) return SendUsage(playerid, "/primer [текст]");
//         ...
//         return 1;
//     }
//
// Имената са с малки букви, до 27 символа. Без параметри params е "\1".

#define CMD:%1(%2) forward cmd_%1(%2); public cmd_%1(%2)

stock Commands_Process(playerid, cmdtext[])
{
    if (!PlayerData[playerid][pLogged]) {
        SendError(playerid, "Първо влез в акаунта си.");
        return 1;
    }

    new cmd[28], pos = 1, i = 0;
    while (cmdtext[pos] != EOS && cmdtext[pos] != ' ') {
        if (i >= sizeof (cmd) - 1) {
            SendError(playerid, "Непозната команда. Отвори менюто с Y или /menu.");
            return 1;
        }
        cmd[i++] = tolower(cmdtext[pos++]);
    }
    cmd[i] = EOS;

    if (i == 0) {
        SendError(playerid, "Непозната команда. Отвори менюто с Y или /menu.");
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
        SendError(playerid, "Непозната команда. Отвори менюто с Y или /menu.");
    }
    return 1;
}
