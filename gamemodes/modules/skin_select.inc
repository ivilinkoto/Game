// Визуален избор на скин при регистрация: решетка с 3D модели на героите.
// Играчът докосва/цъква героя, който иска. ESC = назад към избора на пол.
// Текстът е на латиница - шрифтовете на SA не показват кирилица.

#define SKIN_MAX_CARDS      8
#define SKIN_COLS           4
#define SKIN_CARD_W         90.0
#define SKIN_CARD_H         115.0
#define SKIN_GAP            10.0
#define SKIN_TOP            135.0

static const SkinsMale[]   = {133, 202, 261, 206, 34, 128, 158, 161};
static const SkinsFemale[] = {201, 157, 131, 198, 41, 93};

static PlayerText:SkinPanel[MAX_PLAYERS] = {INVALID_PLAYER_TEXT_DRAW, ...};
static PlayerText:SkinTitle[MAX_PLAYERS] = {INVALID_PLAYER_TEXT_DRAW, ...};
static PlayerText:SkinHint[MAX_PLAYERS] = {INVALID_PLAYER_TEXT_DRAW, ...};
static PlayerText:SkinCards[MAX_PLAYERS][SKIN_MAX_CARDS];
static SkinCardSkin[MAX_PLAYERS][SKIN_MAX_CARDS];
static SkinCardCount[MAX_PLAYERS];
static bool:SkinActive[MAX_PLAYERS];

static stock PlayerText:SkinSelect_Label(playerid, Float:y, const text[], Float:size)
{
    new PlayerText:t = CreatePlayerTextDraw(playerid, 320.0, y, text);
    PlayerTextDrawFont(playerid, t, TEXT_DRAW_FONT_2);
    PlayerTextDrawLetterSize(playerid, t, size, size * 4.0);
    PlayerTextDrawAlignment(playerid, t, TEXT_DRAW_ALIGN_CENTRE);
    PlayerTextDrawColour(playerid, t, COLOR_WHITE);
    PlayerTextDrawSetOutline(playerid, t, 1);
    PlayerTextDrawSetShadow(playerid, t, 0);
    PlayerTextDrawBackgroundColour(playerid, t, 0x000000FF);
    PlayerTextDrawSetProportional(playerid, t, true);
    PlayerTextDrawShow(playerid, t);
    return t;
}

stock SkinSelect_Show(playerid, gender)
{
    SkinSelect_Destroy(playerid);

    new count = (gender == 0) ? sizeof (SkinsMale) : sizeof (SkinsFemale);
    if (count > SKIN_MAX_CARDS) count = SKIN_MAX_CARDS;
    new rows = (count + SKIN_COLS - 1) / SKIN_COLS;

    // Тъмен фон зад решетката
    new Float:panelBottom = SKIN_TOP + rows * (SKIN_CARD_H + SKIN_GAP) + 20.0;
    SkinPanel[playerid] = CreatePlayerTextDraw(playerid, 320.0, SKIN_TOP - 45.0, "_");
    PlayerTextDrawUseBox(playerid, SkinPanel[playerid], true);
    PlayerTextDrawBoxColour(playerid, SkinPanel[playerid], 0x000000B0);
    PlayerTextDrawAlignment(playerid, SkinPanel[playerid], TEXT_DRAW_ALIGN_CENTRE);
    PlayerTextDrawLetterSize(playerid, SkinPanel[playerid], 0.0, (panelBottom - (SKIN_TOP - 45.0)) / 9.0);
    PlayerTextDrawTextSize(playerid, SkinPanel[playerid], 0.0, SKIN_COLS * (SKIN_CARD_W + SKIN_GAP) + 20.0);
    PlayerTextDrawShow(playerid, SkinPanel[playerid]);

    SkinTitle[playerid] = SkinSelect_Label(playerid, SKIN_TOP - 38.0, "~y~CHOOSE YOUR DRIVER", 0.40);
    SkinHint[playerid]  = SkinSelect_Label(playerid, panelBottom - 18.0, "TAP A DRIVER TO SELECT  -  ESC: BACK", 0.22);

    // Карти с 3D модел. Последният ред се центрира, ако не е пълен.
    for (new i = 0; i < count; i++) {
        new row = i / SKIN_COLS, col = i % SKIN_COLS;
        new inRow = (row == rows - 1) ? (count - row * SKIN_COLS) : SKIN_COLS;
        new Float:rowWidth = inRow * SKIN_CARD_W + (inRow - 1) * SKIN_GAP;
        new Float:x = 320.0 - rowWidth / 2.0 + col * (SKIN_CARD_W + SKIN_GAP);
        new Float:y = SKIN_TOP + row * (SKIN_CARD_H + SKIN_GAP);
        new skin = (gender == 0) ? SkinsMale[i] : SkinsFemale[i];

        new PlayerText:c = CreatePlayerTextDraw(playerid, x, y, "_");
        PlayerTextDrawFont(playerid, c, TEXT_DRAW_FONT_MODEL_PREVIEW);
        PlayerTextDrawUseBox(playerid, c, true);
        PlayerTextDrawBoxColour(playerid, c, 0x00000000);
        PlayerTextDrawBackgroundColour(playerid, c, 0x2B2B2BFF);
        PlayerTextDrawTextSize(playerid, c, SKIN_CARD_W, SKIN_CARD_H);
        PlayerTextDrawSetPreviewModel(playerid, c, skin);
        PlayerTextDrawSetPreviewRot(playerid, c, -10.0, 0.0, -20.0, 1.0);
        PlayerTextDrawSetSelectable(playerid, c, true);
        PlayerTextDrawShow(playerid, c);

        SkinCards[playerid][i] = c;
        SkinCardSkin[playerid][i] = skin;
    }
    SkinCardCount[playerid] = count;
    SkinActive[playerid] = true;
    SelectTextDraw(playerid, COLOR_BRAND);
}

// Маха всичко. Не вика нищо обратно.
stock SkinSelect_Destroy(playerid)
{
    new bool:wasActive = SkinActive[playerid];
    SkinActive[playerid] = false;   // преди CancelSelectTextDraw - той извиква OnPlayerClickTextDraw

    if (SkinPanel[playerid] != INVALID_PLAYER_TEXT_DRAW) PlayerTextDrawDestroy(playerid, SkinPanel[playerid]);
    if (SkinTitle[playerid] != INVALID_PLAYER_TEXT_DRAW) PlayerTextDrawDestroy(playerid, SkinTitle[playerid]);
    if (SkinHint[playerid] != INVALID_PLAYER_TEXT_DRAW) PlayerTextDrawDestroy(playerid, SkinHint[playerid]);
    SkinPanel[playerid] = INVALID_PLAYER_TEXT_DRAW;
    SkinTitle[playerid] = INVALID_PLAYER_TEXT_DRAW;
    SkinHint[playerid] = INVALID_PLAYER_TEXT_DRAW;

    for (new i = 0; i < SkinCardCount[playerid]; i++) {
        PlayerTextDrawDestroy(playerid, SkinCards[playerid][i]);
    }
    SkinCardCount[playerid] = 0;

    if (wasActive && IsPlayerConnected(playerid)) CancelSelectTextDraw(playerid);
}

stock SkinSelect_OnPlayerConnect(playerid)
{
    SkinActive[playerid] = false;
    SkinCardCount[playerid] = 0;
    SkinPanel[playerid] = INVALID_PLAYER_TEXT_DRAW;
    SkinTitle[playerid] = INVALID_PLAYER_TEXT_DRAW;
    SkinHint[playerid] = INVALID_PLAYER_TEXT_DRAW;
}

stock SkinSelect_OnClickPlayerTD(playerid, PlayerText:td)
{
    if (!SkinActive[playerid]) return 0;
    for (new i = 0; i < SkinCardCount[playerid]; i++) {
        if (SkinCards[playerid][i] == td) {
            new skin = SkinCardSkin[playerid][i];
            SkinSelect_Destroy(playerid);
            Auth_OnSkinChosen(playerid, skin);
            return 1;
        }
    }
    return 0;
}

// ESC затваря избора на textdraw-ове -> назад към пола.
stock SkinSelect_OnClickTD(playerid, Text:clickedid)
{
    if (!SkinActive[playerid] || clickedid != INVALID_TEXT_DRAW) return 0;
    SkinSelect_Destroy(playerid);
    Auth_OnSkinCancelled(playerid);
    return 1;
}
