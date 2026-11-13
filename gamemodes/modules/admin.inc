// Админ система и банове.
//
// Всеки админ има отделна АДМИН парола. Командите работят само след /alogin.
// При първото /alogin админът сам си създава паролата.
//
// Нива: 1 Хелпър, 2 Модератор, 3 Администратор, 4 Главен администратор, 5 Собственик.
// Първия собственик го правиш с RCON:
//     /rcon login [паролата от config.json]
//     /makeadmin [твоето ID] 5

#define ADMIN_HELPER     1
#define ADMIN_MODERATOR  2
#define ADMIN_ADMIN      3
#define ADMIN_HEAD       4
#define ADMIN_OWNER      5

stock Admin_GetRankName(level, dest[], size = sizeof (dest))
{
    switch (level) {
        case ADMIN_HELPER:    format(dest, size, "Хелпър");
        case ADMIN_MODERATOR: format(dest, size, "Модератор");
        case ADMIN_ADMIN:     format(dest, size, "Администратор");
        case ADMIN_HEAD:      format(dest, size, "Главен администратор");
        case ADMIN_OWNER:     format(dest, size, "Собственик");
        default:              format(dest, size, "Играч");
    }
}

// Проверява нивото и админ входа; пише грешка, ако не стигат.
stock bool:Admin_Check(playerid, level)
{
    if (PlayerData[playerid][pAdmin] < level) {
        SendError(playerid, "Нямаш права за тази команда.");
        return false;
    }
    if (!PlayerData[playerid][pAdminLogged]) {
        SendError(playerid, "Първо влез в админ системата с /alogin.");
        return false;
    }
    return true;
}

// Не може срещу админ с равно или по-високо ниво (освен собственик и себе си).
stock bool:Admin_CanTarget(playerid, targetid)
{
    if (playerid == targetid || PlayerData[playerid][pAdmin] >= ADMIN_OWNER) return true;
    if (PlayerData[targetid][pAdmin] >= PlayerData[playerid][pAdmin]) {
        SendError(playerid, "Не можеш да го направиш на админ с равно или по-високо ниво.");
        return false;
    }
    return true;
}

stock Admin_Broadcast(const msg[])
{
    for (new i = 0; i < MAX_PLAYERS; i++) {
        if (IsPlayerConnected(i) && PlayerData[i][pLogged] && PlayerData[i][pAdmin] > 0 && PlayerData[i][pAdminLogged]) {
            SendClientMessage(i, COLOR_ADMIN, "%s", msg);
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

// ============================================================
//  Админ вход (/alogin)
// ============================================================

CMD:alogin(playerid, params[])
{
    #pragma unused params
    if (PlayerData[playerid][pAdmin] == 0) return 0; // за играчите командата "не съществува"
    if (PlayerData[playerid][pAdminLogged]) {
        SendInfo(playerid, "Вече си влязъл в админ системата.");
        return 1;
    }
    Admin_ShowLogin(playerid, "");
    return 1;
}

stock Admin_ShowLogin(playerid, const error[])
{
    new body[1024], rank[32];
    Admin_GetRankName(PlayerData[playerid][pAdmin], rank);

    if (PlayerData[playerid][pAdminPass][0] == EOS) {
        format(body, sizeof (body),
            C_WHITE "Здравей, " C_RED "%s" C_WHITE "!\n\n" \
            "Това е първото ти влизане в админ системата.\n" \
            "Създай " C_GOLD "отделна админ парола" C_WHITE " – различна от паролата на акаунта.\n\n" \
            C_BRAND "»" C_WHITE " Между " C_GOLD "%d" C_WHITE " и " C_GOLD "%d" C_WHITE " символа\n" \
            C_BRAND "»" C_WHITE " Никога не я казвай на никого\n" \
            C_BRAND "»" C_WHITE " Ще я въвеждаш при всяко влизане с /alogin",
            rank, MIN_PASSWORD_LENGTH, MAX_PASSWORD_LENGTH);
        Auth_AppendError(body, sizeof (body), error);
        ShowPlayerDialog(playerid, DIALOG_ALOGIN_CREATE, DIALOG_STYLE_PASSWORD, C_RED "Админ " C_WHITE "| Нова парола", "%s", "Напред", "Отказ", body);
    } else {
        format(body, sizeof (body),
            C_WHITE "Ранг: " C_RED "%s" C_WHITE " (ниво %d)\n\n" \
            "Въведи своята " C_GOLD "админ парола" C_WHITE ", за да отключиш админ командите.",
            rank, PlayerData[playerid][pAdmin]);
        Auth_AppendError(body, sizeof (body), error);
        ShowPlayerDialog(playerid, DIALOG_ALOGIN, DIALOG_STYLE_PASSWORD, C_RED "Админ " C_WHITE "| Вход", "%s", "Вход", "Отказ", body);
    }
}

stock Admin_LoginSuccess(playerid)
{
    PlayerData[playerid][pAdminLogged] = true;
    PlayerData[playerid][pAdminLoginAttempts] = 0;

    new rank[32], msg[128];
    Admin_GetRankName(PlayerData[playerid][pAdmin], rank);
    SendClientMessage(playerid, COLOR_ADMIN, "[Админ] Влезе като %s. Команди: /ahelp или админ панела в менюто (Y).", rank);
    format(msg, sizeof (msg), "[A] %s %s влезе в админ системата.", rank, PlayerData[playerid][pName]);
    Admin_Broadcast(msg);
    Log_Add("alogin", PlayerData[playerid][pName], "", "");
}

// Връща 1, ако диалогът е обработен тук.
stock Admin_OnDialogResponse(playerid, dialogid, response, const inputtext[])
{
    switch (dialogid) {
        case DIALOG_ALOGIN: {
            if (!response || PlayerData[playerid][pAdmin] == 0) return 1;

            if (Pass_IsLegacy(PlayerData[playerid][pAdminSalt])) {
                // Стара SHA-256 админ парола: проверяваме и я прехвърляме към BCrypt.
                if (Pass_LegacyCheck(inputtext, PlayerData[playerid][pAdminSalt], PlayerData[playerid][pAdminPass])) {
                    Pass_Hash(playerid, PASS_REHASH_ADMIN, inputtext);
                    Admin_OnLoginResult(playerid, true);
                } else {
                    Admin_OnLoginResult(playerid, false);
                }
            } else {
                Pass_Verify(playerid, PASS_ADMIN_LOGIN, inputtext, PlayerData[playerid][pAdminPass]);
            }
            return 1;
        }

        case DIALOG_ALOGIN_CREATE: {
            if (!response || PlayerData[playerid][pAdmin] == 0) return 1;

            new len = strlen(inputtext);
            if (len < MIN_PASSWORD_LENGTH || len > MAX_PASSWORD_LENGTH) {
                Admin_ShowLogin(playerid, "Паролата е твърде къса или твърде дълга.");
                return 1;
            }
            format(PlayerData[playerid][pTempPass], MAX_PASSWORD_LENGTH + 1, "%s", inputtext);

            // Не бива да е същата като паролата на акаунта
            if (Pass_IsLegacy(PlayerData[playerid][pSalt])) {
                Admin_OnSameCheck(playerid, Pass_LegacyCheck(inputtext, PlayerData[playerid][pSalt], PlayerData[playerid][pPassHash]));
            } else {
                Pass_Verify(playerid, PASS_ADMIN_SAME, inputtext, PlayerData[playerid][pPassHash]);
            }
            return 1;
        }

        case DIALOG_ALOGIN_CONFIRM: {
            if (PlayerData[playerid][pAdmin] == 0) return 1;

            if (!response) {
                PlayerData[playerid][pTempPass][0] = EOS;
                Admin_ShowLogin(playerid, "");
                return 1;
            }
            if (inputtext[0] == EOS || strcmp(inputtext, PlayerData[playerid][pTempPass], false) != 0) {
                PlayerData[playerid][pTempPass][0] = EOS;
                Admin_ShowLogin(playerid, "Паролите не съвпадат. Опитай отново.");
                return 1;
            }
            // Хешира се асинхронно – продължаваме в Admin_OnPassHashed.
            Pass_Hash(playerid, PASS_ADMIN_CREATE, PlayerData[playerid][pTempPass]);
            PlayerData[playerid][pTempPass][0] = EOS;
            return 1;
        }
    }
    return 0;
}

// ---------- Резултати от BCrypt (виж core/passwords.inc) ----------

stock Admin_OnLoginResult(playerid, bool:success)
{
    if (PlayerData[playerid][pAdmin] == 0 || PlayerData[playerid][pAdminLogged]) return;

    if (success) {
        Admin_LoginSuccess(playerid);
        return;
    }

    PlayerData[playerid][pAdminLoginAttempts]++;
    if (PlayerData[playerid][pAdminLoginAttempts] >= MAX_LOGIN_ATTEMPTS) {
        new msg[128];
        format(msg, sizeof (msg), "[A] ВНИМАНИЕ: %s сгреши админ паролата %d пъти и е изгонен.",
            PlayerData[playerid][pName], MAX_LOGIN_ATTEMPTS);
        Admin_Broadcast(msg);
        Log_Add("alogin_fail", PlayerData[playerid][pName], "", "kick");
        SendError(playerid, "Твърде много грешни опити за админ парола.");
        KickEx(playerid);
    } else {
        new err[80];
        format(err, sizeof (err), "Грешна админ парола! Остават опити: %d",
            MAX_LOGIN_ATTEMPTS - PlayerData[playerid][pAdminLoginAttempts]);
        Admin_ShowLogin(playerid, err);
    }
}

// same = true -> новата админ парола съвпада с паролата на акаунта
stock Admin_OnSameCheck(playerid, bool:same)
{
    if (same) {
        PlayerData[playerid][pTempPass][0] = EOS;
        Admin_ShowLogin(playerid, "Админ паролата трябва да е различна от паролата на акаунта.");
        return;
    }
    ShowPlayerDialog(playerid, DIALOG_ALOGIN_CONFIRM, DIALOG_STYLE_PASSWORD, C_RED "Админ " C_WHITE "| Нова парола",
        C_WHITE "Въведи админ паролата " C_GOLD "още веднъж" C_WHITE " за потвърждение.", "Запази", "Назад");
}

stock Admin_SaveAdminPassword(playerid, const hash[])
{
    format(PlayerData[playerid][pAdminPass], 65, "%s", hash);
    PlayerData[playerid][pAdminSalt][0] = EOS;

    new q[256];
    format(q, sizeof (q), "UPDATE accounts SET admin_password = '%s', admin_salt = '' WHERE id = %d",
        hash, PlayerData[playerid][pID]);
    Database_Exec(q);
}

stock Admin_OnPassHashed(playerid, const hash[])
{
    Admin_SaveAdminPassword(playerid, hash);
    SendClientMessage(playerid, COLOR_ADMIN, "[Админ] Админ паролата е създадена. Запомни я!");
    Admin_LoginSuccess(playerid);
}

// Стара админ парола е прехвърлена към BCrypt.
stock Admin_OnRehashed(playerid, const hash[])
{
    Admin_SaveAdminPassword(playerid, hash);
}

// ============================================================
//  Банове
// ============================================================

// При свързване. Връща true, ако играчът е баннат (и подготвя текста за прозореца).
stock bool:Bans_CheckPlayer(playerid)
{
    new ip[16], q[384];
    GetPlayerIp(playerid, ip, sizeof (ip));
    format(q, sizeof (q),
        "SELECT admin, reason, expire FROM bans WHERE (name = '%s' OR ip = '%s') AND (expire = 0 OR expire > %d) ORDER BY id DESC LIMIT 1",
        PlayerData[playerid][pName], ip, gettime());

    new DBResult:result = Database_Query(q);
    if (result == DBResult:0) return false;
    if (DB_GetRowCount(result) == 0) {
        DB_FreeResultSet(result);
        return false;
    }

    new admin[MAX_PLAYER_NAME + 1], reason[128], expire, term[64];
    DB_GetFieldStringByName(result, "admin", admin);
    DB_GetFieldStringByName(result, "reason", reason);
    expire = DB_GetFieldIntByName(result, "expire");
    DB_FreeResultSet(result);

    if (expire == 0) {
        format(term, sizeof (term), C_RED "завинаги");
    } else {
        format(term, sizeof (term), C_GOLD "още около %d ч.", (expire - gettime()) / 3600 + 1);
    }

    format(PlayerData[playerid][pBanInfo], 256,
        C_WHITE "Админ: " C_GOLD "%s\n" C_WHITE "Причина: " C_GOLD "%s\n" C_WHITE "Срок: %s",
        admin, reason, term);
    PlayerData[playerid][pBanned] = true;
    return true;
}

stock Bans_ShowDialog(playerid)
{
    new body[768];
    format(body, sizeof (body),
        C_RED "Достъпът ти до HOTFIX ONLINE е ограничен.\n\n" \
        "%s\n\n" \
        C_GREY "Ако смяташ, че е грешка, пиши на администрацията в Discord: %s",
        PlayerData[playerid][pBanInfo], SERVER_DISCORD);
    ShowPlayerDialog(playerid, DIALOG_INFO_KICK, DIALOG_STYLE_MSGBOX, C_BRAND "HOTFIX ONLINE " C_WHITE "| Бан", "%s", "Изход", "", body);
}

// days = 0 -> завинаги
stock Bans_Add(const name[], const ip[], const admin[], const reason[], days)
{
    new eReason[256], q[512];
    SQL_Escape(reason, eReason);
    new expire = (days > 0) ? (gettime() + days * 86400) : 0;
    format(q, sizeof (q),
        "INSERT INTO bans (name, ip, admin, reason, date, expire) VALUES ('%s', '%s', '%s', '%s', %d, %d)",
        name, ip, admin, eReason, gettime(), expire);
    Database_Exec(q);
}

// Връща true, ако е имало активен бан.
stock bool:Bans_Remove(const name[])
{
    new eName[64], q[256], count = 0;
    SQL_Escape(name, eName);
    format(q, sizeof (q),
        "SELECT COUNT(*) AS c FROM bans WHERE name = '%s' AND (expire = 0 OR expire > %d)", eName, gettime());
    new DBResult:result = Database_Query(q);
    if (result != DBResult:0) {
        count = DB_GetFieldIntByName(result, "c");
        DB_FreeResultSet(result);
    }
    if (count == 0) return false;

    // Пазим историята – само прекратяваме бана.
    format(q, sizeof (q),
        "UPDATE bans SET expire = %d WHERE name = '%s' AND (expire = 0 OR expire > %d)", gettime(), eName, gettime());
    Database_Exec(q);
    return true;
}

// ============================================================
//  Команди
// ============================================================

CMD:ahelp(playerid, params[])
{
    #pragma unused params
    if (!Admin_Check(playerid, ADMIN_HELPER)) return 1;
    Admin_ShowHelp(playerid, false);
    return 1;
}

stock Admin_ShowHelp(playerid, bool:fromMenu)
{
    new lvl = PlayerData[playerid][pAdmin], body[1500], rank[32];
    Admin_GetRankName(lvl, rank);
    format(body, sizeof (body), C_WHITE "Твоят ранг: " C_BRAND "%s" C_WHITE " (ниво %d)\n\n", rank, lvl);

    strcat(body, C_GOLD "Хелпър (1)\n" C_WHITE "/a [текст] - админ чат\n/kick [ID] [причина]\n/goto [ID]   /gethere [ID]\n\n");
    if (lvl >= ADMIN_MODERATOR) strcat(body, C_GOLD "Модератор (2)\n" C_WHITE "/ban [ID] [дни, 0 = завинаги] [причина]\n/sethp [ID] [0-100]   /setskin [ID] [скин]\n/veh [модел] - временна кола   /dv - махни кола\n\n");
    if (lvl >= ADMIN_ADMIN)     strcat(body, C_GOLD "Администратор (3)\n" C_WHITE "/unban [Име_Фамилия]\n/ann [текст] - обявление до всички\n\n");
    if (lvl >= ADMIN_HEAD)      strcat(body, C_GOLD "Главен администратор (4)\n" C_WHITE "/givemoney [ID] [сума]\n\n");
    if (lvl >= ADMIN_OWNER)     strcat(body, C_GOLD "Собственик (5)\n" C_WHITE "/makeadmin [ID] [0-5]\n");

    if (fromMenu) {
        ShowPlayerDialog(playerid, DIALOG_MENU_BACK, DIALOG_STYLE_MSGBOX, C_BRAND "HOTFIX ONLINE " C_WHITE "| Админ команди", "%s", "Назад", "", body);
    } else {
        ShowPlayerDialog(playerid, DIALOG_INFO, DIALOG_STYLE_MSGBOX, C_BRAND "HOTFIX ONLINE " C_WHITE "| Админ команди", "%s", "Затвори", "", body);
    }
}

// Админ чат
CMD:a(playerid, params[])
{
    if (!Admin_Check(playerid, ADMIN_HELPER)) return 1;
    if (IsNull(params)) return SendUsage(playerid, "/a [текст]");

    new rank[32], msg[200];
    Admin_GetRankName(PlayerData[playerid][pAdmin], rank);
    format(msg, sizeof (msg), "[A] %s %s: %s", rank, PlayerData[playerid][pName], params);
    Admin_Broadcast(msg);
    return 1;
}

CMD:kick(playerid, params[])
{
    if (!Admin_Check(playerid, ADMIN_HELPER)) return 1;

    new targetid, reason[96];
    if (!GetParamRest(params, 1, reason)) return SendUsage(playerid, "/kick [ID/име] [причина]");
    if (!GetParamPlayer(playerid, params, 0, targetid)) return 1;
    if (!Admin_CanTarget(playerid, targetid)) return 1;

    SendClientMessageToAll(COLOR_ADMIN, "[Админ] %s изгони %s. Причина: %s",
        PlayerData[playerid][pName], PlayerData[targetid][pName], reason);
    Log_Add("kick", PlayerData[playerid][pName], PlayerData[targetid][pName], reason);
    KickEx(targetid);
    return 1;
}

CMD:ban(playerid, params[])
{
    if (!Admin_Check(playerid, ADMIN_MODERATOR)) return 1;

    new targetid, days, reason[96];
    if (!GetParamInt(params, 1, days) || !GetParamRest(params, 2, reason) || days < 0 || days > 3650) {
        return SendUsage(playerid, "/ban [ID/име] [дни, 0 = завинаги] [причина]");
    }
    if (!GetParamPlayer(playerid, params, 0, targetid)) return 1;
    if (targetid == playerid) return SendError(playerid, "Не можеш да баннеш себе си.");
    if (!Admin_CanTarget(playerid, targetid)) return 1;

    new ip[16];
    GetPlayerIp(targetid, ip, sizeof (ip));
    Bans_Add(PlayerData[targetid][pName], ip, PlayerData[playerid][pName], reason, days);

    if (days == 0) {
        SendClientMessageToAll(COLOR_ADMIN, "[Админ] %s банна %s завинаги. Причина: %s",
            PlayerData[playerid][pName], PlayerData[targetid][pName], reason);
    } else {
        SendClientMessageToAll(COLOR_ADMIN, "[Админ] %s банна %s за %d дни. Причина: %s",
            PlayerData[playerid][pName], PlayerData[targetid][pName], days, reason);
    }

    new details[128];
    format(details, sizeof (details), "%d дни: %s", days, reason);
    Log_Add("ban", PlayerData[playerid][pName], PlayerData[targetid][pName], details);
    KickEx(targetid);
    return 1;
}

CMD:unban(playerid, params[])
{
    if (!Admin_Check(playerid, ADMIN_ADMIN)) return 1;

    new name[MAX_PLAYER_NAME + 1];
    if (!GetParam(params, 0, name)) return SendUsage(playerid, "/unban [Име_Фамилия]");
    if (!Bans_Remove(name)) return SendError(playerid, "Няма активен бан за това име.");

    SendClientMessage(playerid, COLOR_ADMIN, "[Админ] Банът на %s е премахнат.", name);
    Log_Add("unban", PlayerData[playerid][pName], name, "");
    return 1;
}

CMD:goto(playerid, params[])
{
    if (!Admin_Check(playerid, ADMIN_HELPER)) return 1;

    new targetid;
    if (IsNull(params)) return SendUsage(playerid, "/goto [ID/име]");
    if (!GetParamPlayer(playerid, params, 0, targetid)) return 1;

    new Float:x, Float:y, Float:z;
    GetPlayerPos(targetid, x, y, z);
    SetPlayerInterior(playerid, GetPlayerInterior(targetid));
    SetPlayerVirtualWorld(playerid, GetPlayerVirtualWorld(targetid));
    SetPlayerPos(playerid, x + 1.0, y, z);
    SendClientMessage(playerid, COLOR_ADMIN, "[Админ] Телепортира се до %s.", PlayerData[targetid][pName]);
    return 1;
}

CMD:gethere(playerid, params[])
{
    if (!Admin_Check(playerid, ADMIN_HELPER)) return 1;

    new targetid;
    if (IsNull(params)) return SendUsage(playerid, "/gethere [ID/име]");
    if (!GetParamPlayer(playerid, params, 0, targetid)) return 1;

    new Float:x, Float:y, Float:z;
    GetPlayerPos(playerid, x, y, z);
    SetPlayerInterior(targetid, GetPlayerInterior(playerid));
    SetPlayerVirtualWorld(targetid, GetPlayerVirtualWorld(playerid));
    SetPlayerPos(targetid, x + 1.0, y, z);
    SendClientMessage(targetid, COLOR_ADMIN, "[Админ] %s те телепортира до себе си.", PlayerData[playerid][pName]);
    SendClientMessage(playerid, COLOR_ADMIN, "[Админ] Телепортира %s до себе си.", PlayerData[targetid][pName]);
    return 1;
}

CMD:sethp(playerid, params[])
{
    if (!Admin_Check(playerid, ADMIN_MODERATOR)) return 1;

    new targetid, hp;
    if (!GetParamInt(params, 1, hp) || hp < 0 || hp > 100) return SendUsage(playerid, "/sethp [ID/име] [0-100]");
    if (!GetParamPlayer(playerid, params, 0, targetid)) return 1;

    SetPlayerHealth(targetid, float(hp));
    SendClientMessage(playerid, COLOR_ADMIN, "[Админ] Зададе %d HP на %s.", hp, PlayerData[targetid][pName]);
    return 1;
}

CMD:setskin(playerid, params[])
{
    if (!Admin_Check(playerid, ADMIN_MODERATOR)) return 1;

    new targetid, skin;
    if (!GetParamInt(params, 1, skin) || skin < 0 || skin > 311 || skin == 74) {
        return SendUsage(playerid, "/setskin [ID/име] [0-311]");
    }
    if (!GetParamPlayer(playerid, params, 0, targetid)) return 1;

    PlayerData[targetid][pSkin] = skin;
    SetPlayerSkin(targetid, skin);
    SendClientMessage(playerid, COLOR_ADMIN, "[Админ] Смени скина на %s на %d.", PlayerData[targetid][pName], skin);
    Auth_SaveAccount(targetid);
    return 1;
}

CMD:givemoney(playerid, params[])
{
    if (!Admin_Check(playerid, ADMIN_HEAD)) return 1;

    new targetid, amount;
    if (!GetParamInt(params, 1, amount)) return SendUsage(playerid, "/givemoney [ID/име] [сума] (може и минус)");
    if (!GetParamPlayer(playerid, params, 0, targetid)) return 1;

    Money_Give(targetid, amount);
    SendClientMessage(playerid, COLOR_ADMIN, "[Админ] Даде $%d на %s.", amount, PlayerData[targetid][pName]);
    SendClientMessage(targetid, COLOR_ADMIN, "[Админ] %s ти даде $%d.", PlayerData[playerid][pName], amount);

    new details[32];
    format(details, sizeof (details), "$%d", amount);
    Log_Add("givemoney", PlayerData[playerid][pName], PlayerData[targetid][pName], details);
    Auth_SaveAccount(targetid);
    return 1;
}

// Временна админ кола (изчезва при нова /veh или при излизане).
CMD:veh(playerid, params[])
{
    if (!Admin_Check(playerid, ADMIN_MODERATOR)) return 1;

    new model, c1 = -1, c2 = -1;
    if (!GetParamInt(params, 0, model) || model < 400 || model > 611) {
        return SendUsage(playerid, "/veh [модел 400-611] [цвят1] [цвят2]");
    }
    GetParamInt(params, 1, c1);
    GetParamInt(params, 2, c2);

    if (PlayerData[playerid][pAdminVehicle] != INVALID_VEHICLE_ID) {
        DestroyVehicle(PlayerData[playerid][pAdminVehicle]);
        PlayerData[playerid][pAdminVehicle] = INVALID_VEHICLE_ID;
    }

    new Float:x, Float:y, Float:z, Float:a;
    GetPlayerPos(playerid, x, y, z);
    GetPlayerFacingAngle(playerid, a);

    new vehicleid = CreateVehicle(model, x, y, z + 0.5, a, c1, c2, -1);
    if (vehicleid == INVALID_VEHICLE_ID || vehicleid == 0) {
        return SendError(playerid, "Колата не може да бъде създадена.");
    }
    SetVehicleVirtualWorld(vehicleid, GetPlayerVirtualWorld(playerid));
    LinkVehicleToInterior(vehicleid, GetPlayerInterior(playerid));
    SetVehicleNumberPlate(vehicleid, "ADMIN");
    PutPlayerInVehicle(playerid, vehicleid, 0);
    PlayerData[playerid][pAdminVehicle] = vehicleid;
    return 1;
}

CMD:dv(playerid, params[])
{
    #pragma unused params
    if (!Admin_Check(playerid, ADMIN_MODERATOR)) return 1;
    if (!IsPlayerInAnyVehicle(playerid)) return SendError(playerid, "Не си в превозно средство.");

    new vehicleid = GetPlayerVehicleID(playerid);
    for (new i = 0; i < MAX_PLAYERS; i++) {
        if (IsPlayerConnected(i) && PlayerData[i][pAdminVehicle] == vehicleid) {
            PlayerData[i][pAdminVehicle] = INVALID_VEHICLE_ID;
        }
    }
    DestroyVehicle(vehicleid);
    SendClientMessage(playerid, COLOR_ADMIN, "[Админ] Превозното средство е премахнато.");
    return 1;
}

CMD:ann(playerid, params[])
{
    if (!Admin_Check(playerid, ADMIN_ADMIN)) return 1;
    if (IsNull(params)) return SendUsage(playerid, "/ann [текст]");
    SendClientMessageToAll(COLOR_BRAND, "[Обявление] {FFFFFF}%s", params);
    return 1;
}

// Собственик или RCON админ
CMD:makeadmin(playerid, params[])
{
    // RCON админът (/rcon login) може винаги – така се прави първият собственик.
    if (!IsPlayerAdmin(playerid) && !Admin_Check(playerid, ADMIN_OWNER)) return 1;

    new targetid, level;
    if (!GetParamInt(params, 1, level) || level < 0 || level > ADMIN_OWNER) {
        return SendUsage(playerid, "/makeadmin [ID/име] [0-5]  (0 = премахва)");
    }
    if (!GetParamPlayer(playerid, params, 0, targetid)) return 1;

    PlayerData[targetid][pAdmin] = level;
    if (level == 0) PlayerData[targetid][pAdminLogged] = false;
    Auth_SaveAccount(targetid);

    new rank[32];
    Admin_GetRankName(level, rank);
    if (level == 0) {
        SendClientMessage(targetid, COLOR_ADMIN, "[Админ] %s премахна админ правата ти.", PlayerData[playerid][pName]);
    } else if (PlayerData[targetid][pAdminLogged]) {
        SendClientMessage(targetid, COLOR_ADMIN, "[Админ] %s ти даде ранг %s (ниво %d).", PlayerData[playerid][pName], rank, level);
    } else {
        SendClientMessage(targetid, COLOR_ADMIN, "[Админ] %s ти даде ранг %s (ниво %d). Влез с /alogin.", PlayerData[playerid][pName], rank, level);
    }
    if (targetid != playerid) {
        SendClientMessage(playerid, COLOR_ADMIN, "[Админ] %s вече е %s (ниво %d).", PlayerData[targetid][pName], rank, level);
    }

    new details[16];
    format(details, sizeof (details), "ниво %d", level);
    Log_Add("makeadmin", PlayerData[playerid][pName], PlayerData[targetid][pName], details);
    return 1;
}
