// Данни на играча (в паметта). Записват се в таблица accounts.

enum E_PLAYER_DATA {
    pID,                                // ID на акаунта в базата (0 = няма)
    pName[MAX_PLAYER_NAME + 1],
    pPassHash[BCRYPT_HASH_LENGTH],
    bool:pHasAccount,
    bool:pLogged,
    bool:pAuthShown,
    bool:pSpawned,
    pLoginAttempts,
    pConnSeq,                           // уникален номер на връзката (за асинхронния BCrypt)

    // Регистрация (временно)
    pRegPass[PASSWORD_MAX + 1],
    pRegGender,

    // Акаунт
    pGender,                            // 0 мъж, 1 жена
    pSkin,
    pMoney,
    pExp,
    pLevel,
    pPlayTime,                          // секунди
    pAdmin,
    pAdminHash[BCRYPT_HASH_LENGTH],
    bool:pAdminAuthed,
    bool:pAdminCreating,

    Float:pPosX,
    Float:pPosY,
    Float:pPosZ,
    Float:pPosA,
    bool:pHasPos,

    // Град и гараж
    pGarageCity,
    pRegCity,
    bool:pStarterGiven,
    bool:pInGarage,

    // Настройки
    pCBChannel,
    bool:pCBOn,
    bool:pHudOn,

    // Чат
    pLastMsgTick,
    pLastPM,                            // ID на последния, който ни е писал (за /r)

    // Админ
    pAdminVehicle                       // кола, извикана с /veh
}

new PlayerData[MAX_PLAYERS][E_PLAYER_DATA];

stock ResetPlayerData(playerid)
{
    static const empty[E_PLAYER_DATA];
    PlayerData[playerid] = empty;
    GetPlayerName(playerid, PlayerData[playerid][pName], MAX_PLAYER_NAME + 1);
    PlayerData[playerid][pLevel] = 1;
    PlayerData[playerid][pCBChannel] = CB_DEFAULT_CHANNEL;
    PlayerData[playerid][pCBOn] = true;
    PlayerData[playerid][pHudOn] = true;
    PlayerData[playerid][pLastPM] = INVALID_PLAYER_ID;
    PlayerData[playerid][pAdminVehicle] = INVALID_VEHICLE_ID;
}

stock bool:IsPlayerLogged(playerid)
{
    return (IsPlayerConnected(playerid) && PlayerData[playerid][pLogged]);
}
