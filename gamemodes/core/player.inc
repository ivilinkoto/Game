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
    PlayerText:pSkinCounterTD
}

new PlayerData[MAX_PLAYERS][E_PLAYER_DATA];

stock ResetPlayerData(playerid)
{
    static const empty[E_PLAYER_DATA];
    PlayerData[playerid] = empty;
    PlayerData[playerid][pLevel] = 1;
    PlayerData[playerid][pSkinCounterTD] = INVALID_PLAYER_TEXT_DRAW;
}
