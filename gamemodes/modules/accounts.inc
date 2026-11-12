// Регистрация, вход, зареждане и записване на акаунти.

stock Accounts_OnPlayerConnect(playerid)
{
    ResetPlayerData(playerid);
    GetPlayerName(playerid, PlayerData[playerid][pName], MAX_PLAYER_NAME + 1);
    ClearChat(playerid);

    if (!IsValidRPName(PlayerData[playerid][pName])) {
        SendClientMessage(playerid, COLOR_RED, "Името ти не е подходящо за RolePlay сървър.");
        SendClientMessage(playerid, COLOR_WHITE, "Използвай формат {FFC400}Име_Фамилия{FFFFFF} с латински букви, напр. {FFC400}Ivan_Petrov");
        SendClientMessage(playerid, COLOR_WHITE, "Смени името си в клиента и влез отново.");
        KickEx(playerid);
        return 1;
    }

    if (Bans_CheckPlayer(playerid)) {
        return 1; // Bans_CheckPlayer вече е кикнал играча
    }

    new q[256];
    format(q, sizeof (q), "SELECT password, salt FROM accounts WHERE name = '%s' LIMIT 1", PlayerData[playerid][pName]);
    new DBResult:result = Database_Query(q);
    if (result != DBResult:0) {
        if (DB_GetRowCount(result) > 0) {
            PlayerData[playerid][pHasAccount] = true;
            DB_GetFieldStringByName(result, "password", PlayerData[playerid][pPassHash], 65);
            DB_GetFieldStringByName(result, "salt", PlayerData[playerid][pSalt], 17);
        }
        DB_FreeResultSet(result);
    }

    SendClientMessage(playerid, COLOR_WHITE, "Добре дошъл в {FFC400}%s{FFFFFF}!", SERVER_NAME);
    return 1;
}

stock Accounts_OnPlayerRequestClass(playerid)
{
    if (PlayerData[playerid][pLogged]) {
        Accounts_SpawnPlayer(playerid);
        return 1;
    }
    if (PlayerData[playerid][pAuthShown]) return 1;

    PlayerData[playerid][pAuthShown] = true;
    TogglePlayerSpectating(playerid, true);
    // Малко забавяне – камерата не се задава надеждно веднага след spectate.
    SetTimerEx("Accounts_ShowAuth", 300, false, "i", playerid);
    return 1;
}

forward Accounts_ShowAuth(playerid);
public Accounts_ShowAuth(playerid)
{
    if (!IsPlayerConnected(playerid) || PlayerData[playerid][pLogged]) return 0;

    // Изглед към Los Santos
    SetPlayerCameraPos(playerid, 1500.0, -1700.0, 120.0);
    SetPlayerCameraLookAt(playerid, 1750.0, -1850.0, 30.0);

    if (PlayerData[playerid][pHasAccount]) {
        Accounts_ShowLoginDialog(playerid, "");
    } else {
        Accounts_ShowRegisterDialog(playerid, "");
    }
    return 1;
}

stock Accounts_ShowLoginDialog(playerid, const error[])
{
    new body[512], rpName[MAX_PLAYER_NAME + 1];
    GetRPName(playerid, rpName);
    format(body, sizeof (body),
        "{FFFFFF}Добре дошъл отново, {FFC400}%s{FFFFFF}!\n\nВъведи паролата си, за да влезеш:\n\n{FF4040}%s",
        rpName, error);
    ShowPlayerDialog(playerid, DIALOG_LOGIN, DIALOG_STYLE_PASSWORD, "Вход", "%s", "Вход", "Изход", body);
}

stock Accounts_ShowRegisterDialog(playerid, const error[])
{
    new body[512], rpName[MAX_PLAYER_NAME + 1];
    GetRPName(playerid, rpName);
    format(body, sizeof (body),
        "{FFFFFF}Здравей, {FFC400}%s{FFFFFF}! Това име не е регистрирано.\n\nИзмисли парола ({FFC400}%d-%d{FFFFFF} символа):\n\n{FF4040}%s",
        rpName, MIN_PASSWORD_LENGTH, MAX_PASSWORD_LENGTH, error);
    ShowPlayerDialog(playerid, DIALOG_REGISTER, DIALOG_STYLE_PASSWORD, "Регистрация", "%s", "Напред", "Изход", body);
}

stock Accounts_HashPassword(const password[], const salt[], dest[], size = sizeof (dest))
{
    SHA256_PassHash(password, salt, dest, size);
}

// Връща 1, ако диалогът е обработен тук.
stock Accounts_OnDialogResponse(playerid, dialogid, response, listitem, const inputtext[])
{
    switch (dialogid) {
        case DIALOG_LOGIN: {
            if (!response) {
                KickEx(playerid);
                return 1;
            }
            new hash[65];
            Accounts_HashPassword(inputtext, PlayerData[playerid][pSalt], hash);

            if (inputtext[0] != EOS && !strcmp(hash, PlayerData[playerid][pPassHash], false)) {
                Accounts_Load(playerid);
            } else {
                PlayerData[playerid][pLoginAttempts]++;
                if (PlayerData[playerid][pLoginAttempts] >= MAX_LOGIN_ATTEMPTS) {
                    SendClientMessage(playerid, COLOR_RED, "Твърде много грешни опити. Изгонен си.");
                    KickEx(playerid);
                } else {
                    new err[64];
                    format(err, sizeof (err), "Грешна парола! Остават опити: %d",
                        MAX_LOGIN_ATTEMPTS - PlayerData[playerid][pLoginAttempts]);
                    Accounts_ShowLoginDialog(playerid, err);
                }
            }
            return 1;
        }

        case DIALOG_REGISTER: {
            if (!response) {
                KickEx(playerid);
                return 1;
            }
            new len = strlen(inputtext);
            if (len < MIN_PASSWORD_LENGTH || len > MAX_PASSWORD_LENGTH) {
                Accounts_ShowRegisterDialog(playerid, "Паролата е с невалидна дължина.");
                return 1;
            }
            GenerateSalt(PlayerData[playerid][pSalt], 16, 17);
            Accounts_HashPassword(inputtext, PlayerData[playerid][pSalt], PlayerData[playerid][pPassHash], 65);

            ShowPlayerDialog(playerid, DIALOG_REGISTER_CONFIRM, DIALOG_STYLE_PASSWORD, "Регистрация",
                "{FFFFFF}Въведи паролата още веднъж за потвърждение:", "Напред", "Назад");
            return 1;
        }

        case DIALOG_REGISTER_CONFIRM: {
            if (!response) {
                Accounts_ShowRegisterDialog(playerid, "");
                return 1;
            }
            new hash[65];
            Accounts_HashPassword(inputtext, PlayerData[playerid][pSalt], hash);
            if (inputtext[0] == EOS || strcmp(hash, PlayerData[playerid][pPassHash], false) != 0) {
                Accounts_ShowRegisterDialog(playerid, "Паролите не съвпадат. Опитай отново.");
                return 1;
            }
            ShowPlayerDialog(playerid, DIALOG_GENDER, DIALOG_STYLE_LIST, "Пол на героя", "Мъж\nЖена", "Избери", "");
            return 1;
        }

        case DIALOG_GENDER: {
            // Ако натисне ESC, по подразбиране е мъж.
            PlayerData[playerid][pGender] = (response && listitem == 1) ? 1 : 0;
            Accounts_Create(playerid);
            return 1;
        }
    }
    return 0;
}

stock Accounts_Create(playerid)
{
    new ip[16], q[512];
    GetPlayerIp(playerid, ip, sizeof (ip));

    new skin = (PlayerData[playerid][pGender] == 1) ? DEFAULT_SKIN_FEMALE : DEFAULT_SKIN_MALE;

    format(q, sizeof (q),
        "INSERT INTO accounts (name, password, salt, gender, money, skin, pos_x, pos_y, pos_z, pos_a, register_ip, last_ip, register_date, last_login) "
        "VALUES ('%s', '%s', '%s', %d, %d, %d, %f, %f, %f, %f, '%s', '%s', %d, %d)",
        PlayerData[playerid][pName], PlayerData[playerid][pPassHash], PlayerData[playerid][pSalt],
        PlayerData[playerid][pGender], START_MONEY, skin,
        SPAWN_X, SPAWN_Y, SPAWN_Z, SPAWN_A,
        ip, ip, gettime(), gettime());
    Database_Exec(q);

    // Взимаме ID-то на новия акаунт
    new DBResult:result = Database_Query("SELECT last_insert_rowid() AS id");
    if (result != DBResult:0) {
        PlayerData[playerid][pID] = DB_GetFieldIntByName(result, "id");
        DB_FreeResultSet(result);
    }

    if (PlayerData[playerid][pID] <= 0) {
        SendError(playerid, "Регистрацията не успя. Свържи се с администратор.");
        KickEx(playerid);
        return 0;
    }

    PlayerData[playerid][pHasAccount] = true;
    PlayerData[playerid][pMoney] = START_MONEY;
    PlayerData[playerid][pSkin] = skin;
    PlayerData[playerid][pLevel] = 1;
    PlayerData[playerid][pPosX] = SPAWN_X;
    PlayerData[playerid][pPosY] = SPAWN_Y;
    PlayerData[playerid][pPosZ] = SPAWN_Z;
    PlayerData[playerid][pPosA] = SPAWN_A;
    PlayerData[playerid][pLogged] = true;

    printf("[АКАУНТ] Нова регистрация: %s (ID %d, IP %s)", PlayerData[playerid][pName], PlayerData[playerid][pID], ip);

    ClearChat(playerid);
    SendClientMessage(playerid, COLOR_GREEN, "Регистрацията е успешна! Добре дошъл в %s.", SERVER_NAME);
    SendClientMessage(playerid, COLOR_WHITE, "Получаваш {33CC33}$%d{FFFFFF} начален капитал. Напиши {FFC400}/help{FFFFFF} за помощ.", START_MONEY);

    Accounts_SpawnPlayer(playerid);
    return 1;
}

stock Accounts_Load(playerid)
{
    new q[128];
    format(q, sizeof (q), "SELECT * FROM accounts WHERE name = '%s' LIMIT 1", PlayerData[playerid][pName]);
    new DBResult:result = Database_Query(q);
    if (result == DBResult:0 || DB_GetRowCount(result) == 0) {
        if (result != DBResult:0) DB_FreeResultSet(result);
        SendError(playerid, "Акаунтът не може да се зареди. Свържи се с администратор.");
        KickEx(playerid);
        return 0;
    }

    PlayerData[playerid][pID]             = DB_GetFieldIntByName(result, "id");
    PlayerData[playerid][pGender]         = DB_GetFieldIntByName(result, "gender");
    PlayerData[playerid][pMoney]          = DB_GetFieldIntByName(result, "money");
    PlayerData[playerid][pBank]           = DB_GetFieldIntByName(result, "bank");
    PlayerData[playerid][pLevel]          = DB_GetFieldIntByName(result, "level");
    PlayerData[playerid][pExp]            = DB_GetFieldIntByName(result, "exp");
    PlayerData[playerid][pSkin]           = DB_GetFieldIntByName(result, "skin");
    PlayerData[playerid][pAdmin]          = DB_GetFieldIntByName(result, "admin");
    PlayerData[playerid][pVip]            = DB_GetFieldIntByName(result, "vip");
    PlayerData[playerid][pVipExpire]      = DB_GetFieldIntByName(result, "vip_expire");
    PlayerData[playerid][pCoins]          = DB_GetFieldIntByName(result, "coins");
    PlayerData[playerid][pPlayedMinutes]  = DB_GetFieldIntByName(result, "played_minutes");
    PlayerData[playerid][pPaydayMinutes]  = DB_GetFieldIntByName(result, "payday_minutes");
    PlayerData[playerid][pPosX]           = DB_GetFieldFloatByName(result, "pos_x");
    PlayerData[playerid][pPosY]           = DB_GetFieldFloatByName(result, "pos_y");
    PlayerData[playerid][pPosZ]           = DB_GetFieldFloatByName(result, "pos_z");
    PlayerData[playerid][pPosA]           = DB_GetFieldFloatByName(result, "pos_a");
    PlayerData[playerid][pInterior]       = DB_GetFieldIntByName(result, "interior");
    PlayerData[playerid][pVirtualWorld]   = DB_GetFieldIntByName(result, "virtual_world");
    DB_FreeResultSet(result);

    // Няма запазена позиция -> начален спаун
    if (PlayerData[playerid][pPosX] == 0.0 && PlayerData[playerid][pPosY] == 0.0) {
        PlayerData[playerid][pPosX] = SPAWN_X;
        PlayerData[playerid][pPosY] = SPAWN_Y;
        PlayerData[playerid][pPosZ] = SPAWN_Z;
        PlayerData[playerid][pPosA] = SPAWN_A;
        PlayerData[playerid][pInterior] = 0;
        PlayerData[playerid][pVirtualWorld] = 0;
    }

    PlayerData[playerid][pLogged] = true;

    new ip[16];
    GetPlayerIp(playerid, ip, sizeof (ip));
    format(q, sizeof (q), "UPDATE accounts SET last_login = %d, last_ip = '%s' WHERE id = %d",
        gettime(), ip, PlayerData[playerid][pID]);
    Database_Exec(q);

    ClearChat(playerid);
    new rpName[MAX_PLAYER_NAME + 1];
    GetRPName(playerid, rpName);
    SendClientMessage(playerid, COLOR_GREEN, "Успешен вход! Добре дошъл отново, %s.", rpName);

    if (PlayerData[playerid][pAdmin] > 0) {
        new rank[32];
        Admin_GetRankName(PlayerData[playerid][pAdmin], rank);
        SendClientMessage(playerid, COLOR_ADMIN, "Влезе като %s (ниво %d). Команди: /ahelp", rank, PlayerData[playerid][pAdmin]);
    }

    Vip_CheckExpire(playerid);
    Vip_ShowLoginInfo(playerid);
    Donations_ProcessForPlayer(playerid);

    Accounts_SpawnPlayer(playerid);
    return 1;
}

// Записва всичко в базата. savePos = false при заплата и т.н.
stock Accounts_Save(playerid, bool:savePos)
{
    if (!PlayerData[playerid][pLogged] || PlayerData[playerid][pID] <= 0) return 0;

    if (savePos && PlayerData[playerid][pFirstSpawnDone]) {
        new PLAYER_STATE:pState = GetPlayerState(playerid);
        if (pState != PLAYER_STATE_WASTED && pState != PLAYER_STATE_SPECTATING) {
            new Float:x, Float:y, Float:z, Float:a;
            GetPlayerPos(playerid, x, y, z);
            GetPlayerFacingAngle(playerid, a);
            PlayerData[playerid][pPosX] = x;
            PlayerData[playerid][pPosY] = y;
            PlayerData[playerid][pPosZ] = z;
            PlayerData[playerid][pPosA] = a;
            PlayerData[playerid][pInterior] = GetPlayerInterior(playerid);
            PlayerData[playerid][pVirtualWorld] = GetPlayerVirtualWorld(playerid);
        }
    }

    new q[1024], part[512];
    format(q, sizeof (q),
        "UPDATE accounts SET gender = %d, money = %d, bank = %d, level = %d, exp = %d, skin = %d, admin = %d, vip = %d, vip_expire = %d, coins = %d,",
        PlayerData[playerid][pGender], PlayerData[playerid][pMoney], PlayerData[playerid][pBank],
        PlayerData[playerid][pLevel], PlayerData[playerid][pExp], PlayerData[playerid][pSkin],
        PlayerData[playerid][pAdmin], PlayerData[playerid][pVip], PlayerData[playerid][pVipExpire],
        PlayerData[playerid][pCoins]);
    format(part, sizeof (part),
        " played_minutes = %d, payday_minutes = %d, pos_x = %f, pos_y = %f, pos_z = %f, pos_a = %f, interior = %d, virtual_world = %d WHERE id = %d",
        PlayerData[playerid][pPlayedMinutes], PlayerData[playerid][pPaydayMinutes],
        PlayerData[playerid][pPosX], PlayerData[playerid][pPosY], PlayerData[playerid][pPosZ], PlayerData[playerid][pPosA],
        PlayerData[playerid][pInterior], PlayerData[playerid][pVirtualWorld], PlayerData[playerid][pID]);
    strcat(q, part);
    Database_Exec(q);
    return 1;
}

stock Accounts_SpawnPlayer(playerid)
{
    SetSpawnInfo(playerid, NO_TEAM, PlayerData[playerid][pSkin],
        PlayerData[playerid][pPosX], PlayerData[playerid][pPosY], PlayerData[playerid][pPosZ], PlayerData[playerid][pPosA]);
    TogglePlayerSpectating(playerid, false);
}

stock Accounts_OnPlayerSpawn(playerid)
{
    if (!PlayerData[playerid][pLogged]) {
        // Опит за спаун без вход
        SendError(playerid, "Трябва да влезеш в акаунта си.");
        KickEx(playerid);
        return 1;
    }

    SetPlayerSkin(playerid, PlayerData[playerid][pSkin]);
    SetPlayerScore(playerid, PlayerData[playerid][pLevel]);
    SetPlayerColor(playerid, COLOR_WHITE);
    SyncPlayerMoney(playerid);

    if (!PlayerData[playerid][pFirstSpawnDone]) {
        PlayerData[playerid][pFirstSpawnDone] = true;
        SetPlayerInterior(playerid, PlayerData[playerid][pInterior]);
        SetPlayerVirtualWorld(playerid, PlayerData[playerid][pVirtualWorld]);
        SetCameraBehindPlayer(playerid);
    } else if (PlayerData[playerid][pDied]) {
        PlayerData[playerid][pDied] = false;
        SetPlayerInterior(playerid, 0);
        SetPlayerVirtualWorld(playerid, 0);

        new fee = HOSPITAL_FEE;
        if (fee > PlayerData[playerid][pMoney]) fee = PlayerData[playerid][pMoney];
        if (fee > 0) GivePlayerCash(playerid, -fee);
        SendClientMessage(playerid, COLOR_WHITE, "Изписан си от болницата. Сметка: {FF4040}$%d", fee);
    }

    // Следващите спауни (след смърт) са в болницата.
    SetSpawnInfo(playerid, NO_TEAM, PlayerData[playerid][pSkin], HOSPITAL_X, HOSPITAL_Y, HOSPITAL_Z, HOSPITAL_A);
    return 1;
}

stock Accounts_OnPlayerDeath(playerid)
{
    if (PlayerData[playerid][pLogged]) {
        PlayerData[playerid][pDied] = true;
    }
}
