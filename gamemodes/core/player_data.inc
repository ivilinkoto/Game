// Всички данни за играча в паметта. Записват се в базата от Accounts_Save.

enum E_PLAYER_DATA {
    // --- Записват се в базата ---
    pID,
    pName[MAX_PLAYER_NAME + 1],
    pPassHash[65],
    pSalt[17],
    pGender,                // 0 = мъж, 1 = жена
    pMoney,
    pBank,
    pLevel,
    pExp,
    pSkin,
    pAdmin,
    pVip,
    pVipExpire,             // unix timestamp
    pCoins,                 // дарителска валута
    pPlayedMinutes,
    pPaydayMinutes,
    Float:pPosX,
    Float:pPosY,
    Float:pPosZ,
    Float:pPosA,
    pInterior,
    pVirtualWorld,

    // --- Само в паметта ---
    bool:pLogged,
    bool:pHasAccount,
    bool:pAuthShown,
    bool:pFirstSpawnDone,
    bool:pDied,
    pLoginAttempts,
    pAdminVehicle,
    pShopItem,
    pLastPM
}

new PlayerData[MAX_PLAYERS][E_PLAYER_DATA];

stock ResetPlayerData(playerid)
{
    static const empty[E_PLAYER_DATA];
    PlayerData[playerid] = empty;
    PlayerData[playerid][pLevel] = 1;
    PlayerData[playerid][pAdminVehicle] = INVALID_VEHICLE_ID;
    PlayerData[playerid][pLastPM] = INVALID_PLAYER_ID;
    PlayerData[playerid][pShopItem] = -1;
}
