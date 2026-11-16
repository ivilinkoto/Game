// Админи: нива 1-5, отделна админ парола, банове и основни команди.
//
// Първи собственик: /rcon login ПАРОЛА -> /makeadmin [твоето ID] 5 -> /alogin
// При първото /alogin всеки админ създава своя админ парола.

static const AdminLevelNames[][] = {"Играч", "Хелпър", "Модератор", "Администратор", "Гл. администратор", "Собственик"};

// ---------- Админ панел (диалог) ----------
// /apanel или Меню > Админ панел. Показва само командите, до които играчът има достъп.
// Избираш команда -> въвеждаш параметрите (ако има) -> изпълнява се.

#define ADMIN_PANEL_MAX     24

enum E_ADMIN_CMD {
    acName[16],
    acLevel,
    acUsage[80],        // "" = без параметри
    acDesc[80]
}

static const AdminCmds[][E_ADMIN_CMD] = {
    {"admins",    1, "",                                    "Администратори онлайн"},
    {"a",         1, "[текст]",                             "Админ чат"},
    {"goto",      1, "[ID или ник]",                        "Отиди при играч"},
    {"gethere",   1, "[ID или ник]",                        "Доведи играч при себе си"},
    {"kick",      1, "[ID] [причина]",                      "Изритай играч"},
    {"ban",       2, "[ID] [дни, 0 = завинаги] [причина]",  "Бан на играч"},
    {"unban",     2, "[ник]",                               "Махни бан"},
    {"veh",       3, "[модел 400-611]",                     "Извикай превозно средство"},
    {"dv",        3, "",                                    "Махни извиканото превозно средство"},
    {"gpsadd",    3, "[категория 0-3] [име]",               "Добави GPS точка тук"},
    {"gpslist",   3, "",                                    "Списък с GPS точки"},
    {"gpsdel",    3, "[номер от списъка]",                  "Изтрий GPS точка"},
    {"givemoney", 4, "[ID] [сума]",                         "Дай пари на играч"},
    {"setmoney",  4, "[ID] [сума]",                         "Задай парите на играч"},
    {"complist",  3, "",                                    "Компании: номера и разстояние"},
    {"gotocomp",  3, "[номер на компания]",                 "Отиди до база на компания"},
    {"compos",    5, "[номер на компания]",                 "Премести базата на компания тук"},
    {"dealerpos", 5, "[0 LS | 1 SF | 2 LV]",                "Премести автокъщата тук"},
    {"garagepos", 5, "[0 LS | 1 SF | 2 LV]",                "Премести входа на гаражите тук"},
    {"setlevel",  4, "[ID] [ниво 1-5]",                     "Задай нивото на играч"},
    {"makeadmin", 5, "[ID] [ниво 0-5]",                     "Смени админ нивото на играч"}
};

static AdminPanelMap[MAX_PLAYERS][ADMIN_PANEL_MAX];
static AdminPanelCmd[MAX_PLAYERS];

stock Admin_GetLevelName(level, dest[], size = sizeof (dest))
{
    if (level < 0 || level >= sizeof (AdminLevelNames)) level = 0;
    format(dest, size, "%s", AdminLevelNames[level]);
}

// true = има право. Иначе пише защо няма.
stock bool:Admin_Check(playerid, level)
{
    if (PlayerData[playerid][pAdmin] < level) {
        SendError(playerid, "Нямаш достъп до тази команда.");
        return false;
    }
    if (!PlayerData[playerid][pAdminAuthed]) {
        SendError(playerid, "Първо влез като админ: /alogin");
        return false;
    }
    return true;
}

stock Admin_Broadcast(minLevel, const msg[])
{
    for (new i = 0; i < MAX_PLAYERS; i++) {
        if (IsPlayerLogged(i) && PlayerData[i][pAdmin] >= minLevel && PlayerData[i][pAdminAuthed]) {
            SendClientMessage(i, COLOR_ADMIN, "[A] %s", msg);
        }
    }
}

stock Admin_OnPlayerDisconnect(playerid)
{
    if (PlayerData[playerid][pAdminVehicle] != INVALID_VEHICLE_ID) {
        DestroyVehicle(PlayerData[playerid][pAdminVehicle]);
        PlayerData[playerid][pAdminVehicle] = INVALID_VEHICLE_ID;
    }
}

// ---------- Админ парола ----------

stock Admin_OnLoginResult(playerid, bool:success)
{
    if (!success) {
        SendError(playerid, "Грешна админ парола.");
        new msg[96];
        format(msg, sizeof (msg), "%s (%d) въведе грешна админ парола.", PlayerData[playerid][pName], playerid);
        Admin_Broadcast(4, msg);
        return;
    }
    PlayerData[playerid][pAdminAuthed] = true;
    new lvl[24];
    Admin_GetLevelName(PlayerData[playerid][pAdmin], lvl);
    SendClientMessage(playerid, COLOR_ADMIN, "[A] Влезе като %s. Команди: /ahelp, панел: /apanel или Меню > Админ панел", lvl);
    Admin_ShowPanel(playerid);
}

stock Admin_OnPassHashed(playerid, const hash[])
{
    PlayerData[playerid][pAdminCreating] = false;
    format(PlayerData[playerid][pAdminHash], BCRYPT_HASH_LENGTH, "%s", hash);
    new q[160];
    format(q, sizeof (q), "UPDATE accounts SET admin_password = '%s' WHERE id = %d", hash, PlayerData[playerid][pID]);
    Database_Exec(q);
    PlayerData[playerid][pAdminAuthed] = true;
    SendClientMessage(playerid, COLOR_ADMIN, "[A] Админ паролата е създадена и си влязъл. Команди: /ahelp, панел: /apanel");
    Admin_ShowPanel(playerid);
}

CMD:alogin(playerid, params[])
{
    #pragma unused params
    if (PlayerData[playerid][pAdmin] < 1) return SendError(playerid, "Не си администратор.");
    if (PlayerData[playerid][pAdminAuthed]) return SendInfo(playerid, "Вече си влязъл като админ.");

    if (PlayerData[playerid][pAdminHash][0] == EOS) {
        ShowPlayerDialog(playerid, DIALOG_ADMIN_CREATE, DIALOG_STYLE_PASSWORD, C_BRAND "Админ" C_WHITE " | Нова парола",
            C_WHITE "Създай своя админ парола (%d-%d символа).\nТя трябва да е различна от паролата на акаунта.",
            "Създай", "Отказ", PASSWORD_MIN, PASSWORD_MAX);
    } else {
        ShowPlayerDialog(playerid, DIALOG_ADMIN_LOGIN, DIALOG_STYLE_PASSWORD, C_BRAND "Админ" C_WHITE " | Вход",
            C_WHITE "Въведи админ паролата си:", "Вход", "Отказ");
    }
    return 1;
}

stock Admin_OnDialogResponse(playerid, dialogid, response, listitem, const inputtext[])
{
    switch (dialogid) {
        case DIALOG_ADMIN_HELP: {
            if (response && listitem >= 0 && listitem < PlayerData[playerid][pAdmin]) Admin_ShowHelpPage(playerid, listitem + 1);
            return 1;
        }
        case DIALOG_ADMIN_HELP_PAGE: {
            if (response && PlayerData[playerid][pAdmin] > 1) Admin_ShowHelpLevels(playerid);
            return 1;
        }
        case DIALOG_ADMIN_PANEL: {
            if (!response) return 1;
            if (listitem < 0 || listitem >= ADMIN_PANEL_MAX) return 1;
            new idx = AdminPanelMap[playerid][listitem];
            if (idx < 0 || idx >= sizeof (AdminCmds) || AdminCmds[idx][acLevel] > PlayerData[playerid][pAdmin]) return 1;
            AdminPanelCmd[playerid] = idx;
            if (AdminCmds[idx][acUsage][0] == EOS) {
                Admin_RunCommand(playerid, idx, "");
            } else {
                Admin_ShowParams(playerid, "");
            }
            return 1;
        }
        case DIALOG_ADMIN_PARAMS: {
            if (!response) {
                Admin_ShowPanel(playerid);
                return 1;
            }
            if (inputtext[0] == EOS) {
                Admin_ShowParams(playerid, "Не си въвел параметри.");
                return 1;
            }
            Admin_RunCommand(playerid, AdminPanelCmd[playerid], inputtext);
            return 1;
        }
        case DIALOG_ADMIN_LOGIN: {
            if (!response) return 1;
            Pass_Verify(playerid, PASS_ADMIN_LOGIN, inputtext, PlayerData[playerid][pAdminHash]);
            return 1;
        }
        case DIALOG_ADMIN_CREATE: {
            if (!response || PlayerData[playerid][pAdminCreating]) return 1;
            new len = strlen(inputtext);
            if (len < PASSWORD_MIN || len > PASSWORD_MAX) {
                SendError(playerid, "Паролата е с грешна дължина.");
                return 1;
            }
            PlayerData[playerid][pAdminCreating] = true;
            Pass_Hash(playerid, PASS_ADMIN_CREATE, inputtext);
            return 1;
        }
    }
    return 0;
}

// ---------- Банове ----------

// Проверява при вход. true = играчът е баннат (и вече е изритан).
stock bool:Admin_CheckBan(playerid)
{
    new q[200];
    format(q, sizeof (q),
        "SELECT reason, admin_name, expires_at FROM bans WHERE name = '%s' AND (expires_at = 0 OR expires_at > %d) ORDER BY id DESC LIMIT 1",
        PlayerData[playerid][pName], gettime());
    new DBResult:r = Database_Query(q);
    if (r == DBResult:0) return false;

    new bool:banned = false;
    if (DB_GetRowCount(r) > 0) {
        banned = true;
        new reason[96], admin[MAX_PLAYER_NAME + 1], expires = DB_GetFieldIntByName(r, "expires_at"), until[48];
        DB_GetFieldStringByName(r, "reason", reason, sizeof (reason));
        DB_GetFieldStringByName(r, "admin_name", admin, sizeof (admin));
        if (expires == 0) {
            format(until, sizeof (until), "завинаги");
        } else {
            new left = expires - gettime();
            format(until, sizeof (until), "още %d дни и %d часа", left / 86400, (left % 86400) / 3600);
        }
        ShowPlayerDialog(playerid, DIALOG_KICK_INFO, DIALOG_STYLE_MSGBOX, C_RED "Баннат си",
            C_WHITE "Акаунтът " C_BRAND "%s" C_WHITE " е баннат.\n\n" \
            "Причина: %s\nАдмин: %s\nСрок: %s",
            "Изход", "", PlayerData[playerid][pName], reason, admin, until);
        KickEx(playerid);
    }
    DB_FreeResultSet(r);
    return banned;
}

// ---------- Команди ----------


stock Admin_ShowPanel(playerid)
{
    if (PlayerData[playerid][pAdmin] < 1) return SendError(playerid, "Не си администратор.");
    if (!PlayerData[playerid][pAdminAuthed]) {
        // Първо вход - после панелът се отваря сам.
        return CallLocalFunction("cmd_alogin", "is", playerid, "\1");
    }

    new body[2048], line[128], n = 0, lvl[24];
    format(body, sizeof (body), "Команда\tКакво прави\n");
    for (new i = 0; i < sizeof (AdminCmds) && n < ADMIN_PANEL_MAX; i++) {
        if (AdminCmds[i][acLevel] > PlayerData[playerid][pAdmin]) continue;
        format(line, sizeof (line), C_BRAND "/%s\t" C_WHITE "%s\n", AdminCmds[i][acName], AdminCmds[i][acDesc]);
        strcat(body, line);
        AdminPanelMap[playerid][n++] = i;
    }
    Admin_GetLevelName(PlayerData[playerid][pAdmin], lvl);
    new title[64];
    format(title, sizeof (title), C_BRAND "Админ панел" C_WHITE " | %s", lvl);
    ShowPlayerDialog(playerid, DIALOG_ADMIN_PANEL, DIALOG_STYLE_TABLIST_HEADERS, title, "%s", "Избери", "Затвори", body);
    return 1;
}

static stock Admin_ShowParams(playerid, const error[])
{
    new idx = AdminPanelCmd[playerid], err[96];
    if (error[0] != EOS) format(err, sizeof (err), "\n\n" C_RED "%s", error);
    ShowPlayerDialog(playerid, DIALOG_ADMIN_PARAMS, DIALOG_STYLE_INPUT, C_BRAND "Админ панел" C_WHITE " | Параметри",
        C_WHITE "%s\n\nКоманда: " C_BRAND "/%s %s" C_WHITE "\n\nВъведи параметрите (без името на командата):%s",
        "Изпълни", "Назад", AdminCmds[idx][acDesc], AdminCmds[idx][acName], AdminCmds[idx][acUsage], err);
}

static stock Admin_RunCommand(playerid, idx, const params[])
{
    new funcname[32];
    format(funcname, sizeof (funcname), "cmd_%s", AdminCmds[idx][acName]);
    if (IsNull(params)) {
        CallLocalFunction(funcname, "is", playerid, "\1");
    } else {
        CallLocalFunction(funcname, "is", playerid, params);
    }
}

// Информационен прозорец: всички команди за твоето ниво с описание (само за четене).
// За изпълнение от прозорец - /apanel или Меню > Админ панел.
// Мобилният клиент сам познава кодировката на дълги прозорци и понякога греши
// (кирилицата излиза като тайландски), затова помощта е на кратки страници по ниво.
static stock Admin_ShowHelpLevels(playerid)
{
    new body[512], line[96], lvl[48];
    format(body, sizeof (body), "Ниво\tКоманди\n");
    for (new level = 1; level <= PlayerData[playerid][pAdmin]; level++) {
        new n = 0;
        for (new i = 0; i < sizeof (AdminCmds); i++) if (AdminCmds[i][acLevel] == level) n++;
        Admin_GetLevelName(level, lvl);
        format(line, sizeof (line), "%d. %s\t%d\n", level, lvl, n);
        strcat(body, line);
    }
    ShowPlayerDialog(playerid, DIALOG_ADMIN_HELP, DIALOG_STYLE_TABLIST_HEADERS, C_BRAND "Админ команди", "%s", "Покажи", "Затвори", body);
}

static stock Admin_ShowHelpPage(playerid, level)
{
    new body[1400], line[200], lvl[48], title[96];
    for (new i = 0; i < sizeof (AdminCmds); i++) {
        if (AdminCmds[i][acLevel] != level) continue;
        format(line, sizeof (line), C_BRAND "/%s %s\n" C_WHITE "    %s\n", AdminCmds[i][acName], AdminCmds[i][acUsage], AdminCmds[i][acDesc]);
        strcat(body, line);
    }
    Admin_GetLevelName(level, lvl);
    format(title, sizeof (title), C_BRAND "Команди" C_WHITE " | %s", lvl);
    ShowPlayerDialog(playerid, DIALOG_ADMIN_HELP_PAGE, DIALOG_STYLE_MSGBOX, title, "%s", "Назад", "Затвори", body);
}

// Информационни страници: командите по ниво с описание (само за четене).
// За изпълнение от прозорец - /apanel или Меню > Админ панел.
CMD:ahelp(playerid, params[])
{
    #pragma unused params
    if (!Admin_Check(playerid, 1)) return 1;
    if (PlayerData[playerid][pAdmin] == 1) Admin_ShowHelpPage(playerid, 1);
    else Admin_ShowHelpLevels(playerid);
    return 1;
}

CMD:apanel(playerid, params[])
{
    #pragma unused params
    return Admin_ShowPanel(playerid);
}

CMD:setlevel(playerid, params[])
{
    if (!Admin_Check(playerid, 4)) return 1;
    new target[24], levelStr[4];
    if (!GetParam(params, 0, target) || !GetParam(params, 1, levelStr) || !IsNumeric(levelStr)) {
        return SendUsage(playerid, "/setlevel [ID] [1-5]");
    }
    new targetid = FindPlayer(target), level = strval(levelStr);
    if (targetid == INVALID_PLAYER_ID) return SendError(playerid, "Няма такъв играч.");
    if (level < 1 || level > MAX_LEVEL) return SendError(playerid, "Нивото е от 1 до 5.");

    PlayerData[targetid][pLevel] = level;
    PlayerData[targetid][pExp] = LevelExp[level - 1];
    SetPlayerScore(targetid, level);
    new q[128];
    format(q, sizeof (q), "UPDATE accounts SET level = %d, exp = %d WHERE id = %d", level, PlayerData[targetid][pExp], PlayerData[targetid][pID]);
    Database_Exec(q);

    new msg[96];
    format(msg, sizeof (msg), "%s смени нивото на %s на %d.", PlayerData[playerid][pName], PlayerData[targetid][pName], level);
    Admin_Broadcast(4, msg);
    if (targetid != playerid) SendClientMessage(targetid, COLOR_BRAND, "[CARGO] " C_WHITE "Администратор смени нивото ти на " C_BRAND "%d" C_WHITE ".", level);
    return 1;
}

CMD:a(playerid, params[])
{
    if (!Admin_Check(playerid, 1)) return 1;
    if (IsNull(params)) return SendUsage(playerid, "/a [текст] - админ чат");
    new msg[160];
    format(msg, sizeof (msg), "%s: %s", PlayerData[playerid][pName], params);
    Admin_Broadcast(1, msg);
    return 1;
}

CMD:admins(playerid, params[])
{
    #pragma unused params
    new count = 0, lvl[24];
    SendClientMessage(playerid, COLOR_ADMIN, "Администратори онлайн:");
    for (new i = 0; i < MAX_PLAYERS; i++) {
        if (!IsPlayerLogged(i) || PlayerData[i][pAdmin] < 1) continue;
        Admin_GetLevelName(PlayerData[i][pAdmin], lvl);
        SendClientMessage(playerid, COLOR_WHITE, " %s (%d) - %s", PlayerData[i][pName], i, lvl);
        count++;
    }
    if (count == 0) SendClientMessage(playerid, COLOR_GREY, " Няма.");
    return 1;
}

CMD:kick(playerid, params[])
{
    if (!Admin_Check(playerid, 1)) return 1;
    new target[24], reason[96];
    if (!GetParam(params, 0, target)) return SendUsage(playerid, "/kick [ID] [причина]");
    if (!GetParamRest(params, 1, reason)) format(reason, sizeof (reason), "без причина");
    new targetid = FindPlayer(target);
    if (targetid == INVALID_PLAYER_ID) return SendError(playerid, "Няма такъв играч.");
    if (PlayerData[targetid][pAdmin] > PlayerData[playerid][pAdmin]) return SendError(playerid, "Не можеш да изриташ по-висш админ.");

    SendClientMessageToAll(COLOR_ADMIN, "%s изрита %s. Причина: %s", PlayerData[playerid][pName], PlayerData[targetid][pName], reason);
    KickEx(targetid);
    return 1;
}

CMD:ban(playerid, params[])
{
    if (!Admin_Check(playerid, 2)) return 1;
    new target[24], daysStr[8], reason[96];
    if (!GetParam(params, 0, target) || !GetParam(params, 1, daysStr) || !IsNumeric(daysStr) || !GetParamRest(params, 2, reason)) {
        return SendUsage(playerid, "/ban [ID] [дни, 0 = завинаги] [причина]");
    }
    new targetid = FindPlayer(target);
    if (targetid == INVALID_PLAYER_ID) return SendError(playerid, "Няма такъв играч.");
    if (targetid == playerid) return SendError(playerid, "Не можеш да баннеш себе си.");
    if (PlayerData[targetid][pAdmin] >= PlayerData[playerid][pAdmin]) return SendError(playerid, "Не можеш да баннеш админ със същото или по-високо ниво.");

    new days = strval(daysStr);
    if (days < 0 || days > 3650) return SendError(playerid, "Дните са от 0 до 3650.");
    new expires = (days == 0) ? 0 : gettime() + days * 86400;

    new safe[192], q[400];
    SQL_Escape(reason, safe);
    format(q, sizeof (q), "INSERT INTO bans (name, reason, admin_name, created_at, expires_at) VALUES ('%s', '%s', '%s', %d, %d)",
        PlayerData[targetid][pName], safe, PlayerData[playerid][pName], gettime(), expires);
    Database_Exec(q);

    if (days == 0) {
        SendClientMessageToAll(COLOR_ADMIN, "%s банна завинаги %s. Причина: %s", PlayerData[playerid][pName], PlayerData[targetid][pName], reason);
    } else {
        SendClientMessageToAll(COLOR_ADMIN, "%s банна %s за %d дни. Причина: %s", PlayerData[playerid][pName], PlayerData[targetid][pName], days, reason);
    }
    KickEx(targetid);
    return 1;
}

CMD:unban(playerid, params[])
{
    if (!Admin_Check(playerid, 2)) return 1;
    if (IsNull(params) || !IsValidNickname(params)) return SendUsage(playerid, "/unban [ник]");
    new q[160];
    format(q, sizeof (q), "DELETE FROM bans WHERE name = '%s'", params);
    Database_Exec(q);
    new msg[96];
    format(msg, sizeof (msg), "%s махна бана на %s.", PlayerData[playerid][pName], params);
    Admin_Broadcast(1, msg);
    return 1;
}

CMD:goto(playerid, params[])
{
    if (!Admin_Check(playerid, 1)) return 1;
    if (IsNull(params)) return SendUsage(playerid, "/goto [ID]");
    new targetid = FindPlayer(params);
    if (targetid == INVALID_PLAYER_ID) return SendError(playerid, "Няма такъв играч.");
    new Float:x, Float:y, Float:z;
    GetPlayerPos(targetid, x, y, z);
    SetPlayerInterior(playerid, GetPlayerInterior(targetid));
    SetPlayerVirtualWorld(playerid, GetPlayerVirtualWorld(targetid));
    if (IsPlayerInAnyVehicle(playerid) && GetPlayerState(playerid) == PLAYER_STATE_DRIVER) {
        SetVehiclePos(GetPlayerVehicleID(playerid), x + 2.0, y, z + 0.5);
    } else {
        SetPlayerPos(playerid, x + 1.0, y, z + 0.5);
    }
    return 1;
}

CMD:gethere(playerid, params[])
{
    if (!Admin_Check(playerid, 1)) return 1;
    if (IsNull(params)) return SendUsage(playerid, "/gethere [ID]");
    new targetid = FindPlayer(params);
    if (targetid == INVALID_PLAYER_ID) return SendError(playerid, "Няма такъв играч.");
    new Float:x, Float:y, Float:z;
    GetPlayerPos(playerid, x, y, z);
    SetPlayerInterior(targetid, GetPlayerInterior(playerid));
    SetPlayerVirtualWorld(targetid, GetPlayerVirtualWorld(playerid));
    SetPlayerPos(targetid, x + 1.0, y, z + 0.5);
    SendClientMessage(targetid, COLOR_ADMIN, "Администратор %s те премести при себе си.", PlayerData[playerid][pName]);
    return 1;
}

CMD:veh(playerid, params[])
{
    if (!Admin_Check(playerid, 3)) return 1;
    if (IsNull(params) || !IsNumeric(params)) return SendUsage(playerid, "/veh [модел 400-611]  (напр. 403 Linerunner, 514 Tanker, 515 Roadtrain)");
    new model = strval(params);
    if (model < 400 || model > 611) return SendError(playerid, "Моделът е от 400 до 611.");

    if (PlayerData[playerid][pAdminVehicle] != INVALID_VEHICLE_ID) {
        DestroyVehicle(PlayerData[playerid][pAdminVehicle]);
    }
    new Float:x, Float:y, Float:z, Float:a;
    GetPlayerPos(playerid, x, y, z);
    GetPlayerFacingAngle(playerid, a);
    new vehicleid = CreateVehicle(model, x, y, z + 1.0, a, -1, -1, -1);
    if (vehicleid == INVALID_VEHICLE_ID) {
        PlayerData[playerid][pAdminVehicle] = INVALID_VEHICLE_ID;
        return SendError(playerid, "Не може да се създаде превозното средство.");
    }
    LinkVehicleToInterior(vehicleid, GetPlayerInterior(playerid));
    SetVehicleVirtualWorld(vehicleid, GetPlayerVirtualWorld(playerid));
    PutPlayerInVehicle(playerid, vehicleid, 0);
    Engine_Set(vehicleid, true);
    PlayerData[playerid][pAdminVehicle] = vehicleid;
    return 1;
}

CMD:dv(playerid, params[])
{
    #pragma unused params
    if (!Admin_Check(playerid, 3)) return 1;
    if (PlayerData[playerid][pAdminVehicle] == INVALID_VEHICLE_ID) return SendError(playerid, "Нямаш извикано превозно средство.");
    DestroyVehicle(PlayerData[playerid][pAdminVehicle]);
    PlayerData[playerid][pAdminVehicle] = INVALID_VEHICLE_ID;
    SendInfo(playerid, "Превозното средство е премахнато.");
    return 1;
}

static stock Admin_MoneyCommand(playerid, const params[], bool:set)
{
    new target[24], amountStr[12];
    if (!GetParam(params, 0, target) || !GetParam(params, 1, amountStr) || !IsNumeric(amountStr)) {
        if (set) return SendUsage(playerid, "/setmoney [ID] [сума]");
        return SendUsage(playerid, "/givemoney [ID] [сума]");
    }
    new targetid = FindPlayer(target);
    if (targetid == INVALID_PLAYER_ID) return SendError(playerid, "Няма такъв играч.");
    new amount = strval(amountStr);

    if (set) Money_Set(targetid, amount);
    else Money_Give(targetid, amount);

    new money[16], msg[128], action[12];
    FormatMoney(PlayerData[targetid][pMoney], money);
    if (set) format(action, sizeof (action), "смени");
    else format(action, sizeof (action), "даде");
    format(msg, sizeof (msg), "%s %s пари на %s (%d). Сега: $%s",
        PlayerData[playerid][pName], action, PlayerData[targetid][pName], amount, money);
    Admin_Broadcast(4, msg);
    return 1;
}

CMD:givemoney(playerid, params[])
{
    if (!Admin_Check(playerid, 4)) return 1;
    return Admin_MoneyCommand(playerid, params, false);
}

CMD:setmoney(playerid, params[])
{
    if (!Admin_Check(playerid, 4)) return 1;
    return Admin_MoneyCommand(playerid, params, true);
}

CMD:makeadmin(playerid, params[])
{
    // Собственик (ниво 5) или влязъл с RCON
    if (!IsPlayerAdmin(playerid) && !Admin_Check(playerid, 5)) return 1;

    new target[24], levelStr[4];
    if (!GetParam(params, 0, target) || !GetParam(params, 1, levelStr) || !IsNumeric(levelStr)) {
        return SendUsage(playerid, "/makeadmin [ID] [0-5]");
    }
    new targetid = FindPlayer(target), level = strval(levelStr);
    if (targetid == INVALID_PLAYER_ID) return SendError(playerid, "Няма такъв играч.");
    if (level < 0 || level > 5) return SendError(playerid, "Нивото е от 0 до 5.");

    PlayerData[targetid][pAdmin] = level;
    PlayerData[targetid][pAdminAuthed] = false;
    new q[96];
    format(q, sizeof (q), "UPDATE accounts SET admin = %d WHERE id = %d", level, PlayerData[targetid][pID]);
    Database_Exec(q);

    new lvl[24];
    Admin_GetLevelName(level, lvl);
    SendClientMessage(playerid, COLOR_ADMIN, "[A] %s вече е %s.", PlayerData[targetid][pName], lvl);
    if (targetid != playerid) SendClientMessage(targetid, COLOR_ADMIN, "[A] Вече си %s. Влез с /alogin.", lvl);
    else if (level > 0) SendClientMessage(playerid, COLOR_ADMIN, "[A] Влез с /alogin.");
    return 1;
}
