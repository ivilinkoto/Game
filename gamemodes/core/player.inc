// Данни за всеки играч в паметта.

#define GENDER_MALE     0
#define GENDER_FEMALE   1

enum E_PLAYER_DATA {
    // --- Записват се в базата ---
    pID,
    pName[MAX_PLAYER_NAME + 1],
    pPassHash[65],
    pSalt[17],
    pEmail[65],
    pGender,
    pAge,
    pSource[32],
    pReferrerID,
    pPromoCode[33],
    pPromoMoney,
    pSkin,
    pMoney,
    pLevel,
    pExp,
    pAdmin,
    pPlayedMinutes,     // общо активно време
    pPaydayMinutes,     // активни минути от последната заплата
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
    pLoginAttempts,
    bool:pInSkinSelect,
    pSkinIndex,
    PlayerText:pSkinCounterTD,
    bool:pBanned,
    pBanInfo[256],
    bool:pDied,
    pLastUpdateTick,    // последен пакет от клиента (за AFK)
    pIdleMinutes,       // поредни минути без движение
    Float:pIdleX,
    Float:pIdleY,
    Float:pIdleZ,
    pLastPM,
    pAdminVehicle
}

new PlayerData[MAX_PLAYERS][E_PLAYER_DATA];

stock ResetPlayerData(playerid)
{
    static const empty[E_PLAYER_DATA];
    PlayerData[playerid] = empty;
    PlayerData[playerid][pLevel] = 1;
    PlayerData[playerid][pSkinCounterTD] = INVALID_PLAYER_TEXT_DRAW;
    PlayerData[playerid][pLastPM] = INVALID_PLAYER_ID;
    PlayerData[playerid][pAdminVehicle] = INVALID_VEHICLE_ID;
    PlayerData[playerid][pLastUpdateTick] = GetTickCount();
}
