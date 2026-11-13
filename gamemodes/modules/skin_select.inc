// Избор на начален скин след регистрация.
//
// Героят стои пред камерата в собствен виртуален свят (другите не го виждат),
// а играчът сменя скиновете със стрелките и потвърждава със SELECT.
//
// Textdraw шрифтовете на GTA нямат кирилица, затова надписите са на латиница.

static const MaleSkins[] = {
    7, 14, 15, 17, 19, 20, 21, 22, 23, 26, 29, 46,
    47, 48, 60, 72, 98, 101, 170, 180, 184, 185, 188, 250
};

static const FemaleSkins[] = {
    9, 11, 12, 13, 40, 41, 55, 56, 69, 76, 91, 93,
    141, 148, 150, 169, 190, 191, 192, 193, 211, 216, 226, 233
};

enum {
    SKIN_TD_TITLE,
    SKIN_TD_SUBTITLE,
    SKIN_TD_PANEL,
    SKIN_TD_LEFT,
    SKIN_TD_SELECT,
    SKIN_TD_RIGHT,
    SKIN_TD_COUNT
}

static Text:gSkinTD[SKIN_TD_COUNT];

// ------------------------------------------------------------
//  Textdraw-ове (създават се веднъж при старт на сървъра)
// ------------------------------------------------------------

stock Text:Skin_CreateButton(Float:x, Float:y, const text[], boxColour, Float:width)
{
    new Text:td = TextDrawCreate(x, y, text);
    TextDrawFont(td, TEXT_DRAW_FONT_2);
    TextDrawLetterSize(td, 0.40, 1.70);
    TextDrawAlignment(td, TEXT_DRAW_ALIGN_CENTRE);
    TextDrawColour(td, COLOR_WHITE);
    TextDrawSetOutline(td, 0);
    TextDrawSetShadow(td, 0);
    TextDrawUseBox(td, true);
    TextDrawBoxColour(td, boxColour);
    // При центриран textdraw: (височина на зоната за натискане, ширина)
    TextDrawTextSize(td, 18.0, width);
    TextDrawSetSelectable(td, true);
    return td;
}

stock Skin_CreateTextDraws()
{
    // Заглавие
    gSkinTD[SKIN_TD_TITLE] = TextDrawCreate(320.0, 20.0, "HOTFIX ONLINE");
    TextDrawFont(gSkinTD[SKIN_TD_TITLE], TEXT_DRAW_FONT_3);
    TextDrawLetterSize(gSkinTD[SKIN_TD_TITLE], 0.70, 2.80);
    TextDrawAlignment(gSkinTD[SKIN_TD_TITLE], TEXT_DRAW_ALIGN_CENTRE);
    TextDrawColour(gSkinTD[SKIN_TD_TITLE], COLOR_BRAND);
    TextDrawSetOutline(gSkinTD[SKIN_TD_TITLE], 1);
    TextDrawSetShadow(gSkinTD[SKIN_TD_TITLE], 0);
    TextDrawBackgroundColour(gSkinTD[SKIN_TD_TITLE], 0x000000FF);

    gSkinTD[SKIN_TD_SUBTITLE] = TextDrawCreate(320.0, 50.0, "CHOOSE YOUR CHARACTER");
    TextDrawFont(gSkinTD[SKIN_TD_SUBTITLE], TEXT_DRAW_FONT_2);
    TextDrawLetterSize(gSkinTD[SKIN_TD_SUBTITLE], 0.25, 1.10);
    TextDrawAlignment(gSkinTD[SKIN_TD_SUBTITLE], TEXT_DRAW_ALIGN_CENTRE);
    TextDrawColour(gSkinTD[SKIN_TD_SUBTITLE], COLOR_WHITE);
    TextDrawSetOutline(gSkinTD[SKIN_TD_SUBTITLE], 1);
    TextDrawSetShadow(gSkinTD[SKIN_TD_SUBTITLE], 0);
    TextDrawBackgroundColour(gSkinTD[SKIN_TD_SUBTITLE], 0x000000FF);

    // Тъмен панел отдолу
    gSkinTD[SKIN_TD_PANEL] = TextDrawCreate(320.0, 350.0, "_");
    TextDrawFont(gSkinTD[SKIN_TD_PANEL], TEXT_DRAW_FONT_1);
    TextDrawLetterSize(gSkinTD[SKIN_TD_PANEL], 0.0, 7.0);
    TextDrawAlignment(gSkinTD[SKIN_TD_PANEL], TEXT_DRAW_ALIGN_CENTRE);
    TextDrawUseBox(gSkinTD[SKIN_TD_PANEL], true);
    TextDrawBoxColour(gSkinTD[SKIN_TD_PANEL], 0x000000AA);
    TextDrawTextSize(gSkinTD[SKIN_TD_PANEL], 0.0, 300.0);

    // Бутони
    gSkinTD[SKIN_TD_LEFT] = Skin_CreateButton(225.0, 382.0, "~<~", 0x2A2A2AFF, 55.0);
    gSkinTD[SKIN_TD_SELECT] = Skin_CreateButton(320.0, 382.0, "SELECT", COLOR_BRAND, 110.0);
    gSkinTD[SKIN_TD_RIGHT] = Skin_CreateButton(415.0, 382.0, "~>~", 0x2A2A2AFF, 55.0);
}

// ------------------------------------------------------------
//  Логика
// ------------------------------------------------------------

stock Skin_GetCount(playerid)
{
    return (PlayerData[playerid][pGender] == GENDER_FEMALE) ? sizeof (FemaleSkins) : sizeof (MaleSkins);
}

stock Skin_GetModel(playerid, index)
{
    if (PlayerData[playerid][pGender] == GENDER_FEMALE) return FemaleSkins[index];
    return MaleSkins[index];
}

// Извиква се след обобщението на регистрацията.
stock Skin_Start(playerid)
{
    PlayerData[playerid][pInSkinSelect] = true;
    PlayerData[playerid][pSkinIndex] = 0;
    PlayerData[playerid][pSkin] = Skin_GetModel(playerid, 0);

    // Брояч "SKIN 1 / 24" – личен за всеки играч
    new PlayerText:ptd = CreatePlayerTextDraw(playerid, 320.0, 358.0, "_");
    PlayerTextDrawFont(playerid, ptd, TEXT_DRAW_FONT_2);
    PlayerTextDrawLetterSize(playerid, ptd, 0.22, 1.00);
    PlayerTextDrawAlignment(playerid, ptd, TEXT_DRAW_ALIGN_CENTRE);
    PlayerTextDrawColour(playerid, ptd, 0xA0A0A0FF);
    PlayerTextDrawSetOutline(playerid, ptd, 0);
    PlayerTextDrawSetShadow(playerid, ptd, 0);
    PlayerData[playerid][pSkinCounterTD] = ptd;

    // Спаунваме героя – останалото е в Skin_OnPlayerSpawn
    SetSpawnInfo(playerid, NO_TEAM, PlayerData[playerid][pSkin],
        SKIN_SELECT_X, SKIN_SELECT_Y, SKIN_SELECT_Z, SKIN_SELECT_A);
    TogglePlayerSpectating(playerid, false);
}

stock Skin_OnPlayerSpawn(playerid)
{
    SetPlayerVirtualWorld(playerid, 10000 + playerid);
    SetPlayerInterior(playerid, 0);
    SetPlayerPos(playerid, SKIN_SELECT_X, SKIN_SELECT_Y, SKIN_SELECT_Z);
    SetPlayerFacingAngle(playerid, SKIN_SELECT_A);
    TogglePlayerControllable(playerid, false);

    // Камерата е пред героя и гледа към него
    new Float:camX = SKIN_SELECT_X - SKIN_CAMERA_DISTANCE * floatsin(SKIN_SELECT_A, degrees),
        Float:camY = SKIN_SELECT_Y + SKIN_CAMERA_DISTANCE * floatcos(SKIN_SELECT_A, degrees);
    SetPlayerCameraPos(playerid, camX, camY, SKIN_SELECT_Z + SKIN_CAMERA_HEIGHT);
    SetPlayerCameraLookAt(playerid, SKIN_SELECT_X, SKIN_SELECT_Y, SKIN_SELECT_Z + 0.1);

    for (new i = 0; i < SKIN_TD_COUNT; i++) {
        TextDrawShowForPlayer(playerid, gSkinTD[i]);
    }
    PlayerTextDrawShow(playerid, PlayerData[playerid][pSkinCounterTD]);
    Skin_Update(playerid);

    SelectTextDraw(playerid, COLOR_GOLD);
    SendInfo(playerid, "Използвай стрелките, за да разгледаш скиновете, и натисни SELECT.");
}

stock Skin_Update(playerid)
{
    new model = Skin_GetModel(playerid, PlayerData[playerid][pSkinIndex]);
    PlayerData[playerid][pSkin] = model;
    SetPlayerSkin(playerid, model);
    PlayerTextDrawSetString(playerid, PlayerData[playerid][pSkinCounterTD], "SKIN %d / %d",
        PlayerData[playerid][pSkinIndex] + 1, Skin_GetCount(playerid));
}

stock Skin_HideAll(playerid)
{
    for (new i = 0; i < SKIN_TD_COUNT; i++) {
        TextDrawHideForPlayer(playerid, gSkinTD[i]);
    }
    if (PlayerData[playerid][pSkinCounterTD] != INVALID_PLAYER_TEXT_DRAW) {
        PlayerTextDrawDestroy(playerid, PlayerData[playerid][pSkinCounterTD]);
        PlayerData[playerid][pSkinCounterTD] = INVALID_PLAYER_TEXT_DRAW;
    }
}

// Връща true, ако натискането е обработено тук.
stock bool:Skin_OnClickTextDraw(playerid, Text:clickedid)
{
    if (!PlayerData[playerid][pInSkinSelect]) return false;

    // ESC (на PC) – не позволяваме да излезе от избора
    if (clickedid == INVALID_TEXT_DRAW) {
        SelectTextDraw(playerid, COLOR_GOLD);
        return true;
    }

    new count = Skin_GetCount(playerid);

    if (clickedid == gSkinTD[SKIN_TD_LEFT]) {
        PlayerData[playerid][pSkinIndex]--;
        if (PlayerData[playerid][pSkinIndex] < 0) PlayerData[playerid][pSkinIndex] = count - 1;
        Skin_Update(playerid);
        return true;
    }
    if (clickedid == gSkinTD[SKIN_TD_RIGHT]) {
        PlayerData[playerid][pSkinIndex]++;
        if (PlayerData[playerid][pSkinIndex] >= count) PlayerData[playerid][pSkinIndex] = 0;
        Skin_Update(playerid);
        return true;
    }
    if (clickedid == gSkinTD[SKIN_TD_SELECT]) {
        // Първо сваляме флага – CancelSelectTextDraw извиква този callback с INVALID_TEXT_DRAW.
        PlayerData[playerid][pInSkinSelect] = false;
        CancelSelectTextDraw(playerid);
        Skin_HideAll(playerid);
        Auth_FinishRegistration(playerid);
        return true;
    }
    return false;
}

stock Skin_OnPlayerDisconnect(playerid)
{
    if (PlayerData[playerid][pSkinCounterTD] != INVALID_PLAYER_TEXT_DRAW) {
        PlayerTextDrawDestroy(playerid, PlayerData[playerid][pSkinCounterTD]);
        PlayerData[playerid][pSkinCounterTD] = INVALID_PLAYER_TEXT_DRAW;
    }
}
