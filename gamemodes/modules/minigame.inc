// Мини-игра "натисни навреме" – за работите (ферма и бъдещи).
//
// На екрана има лента с зелена зона и червен маркер, който се движи.
// Играчът натиска бутона TAP, когато маркерът е в зелената зона.
//   MG_TYPE_BOUNCE – маркерът ходи напред-назад
//   MG_TYPE_FILL   – маркерът се пълни отляво надясно; ако стигне края – провал (преляване)
//
// Резултатът се връща в Minigame_Dispatch -> модула, който е пуснал играта.
// Приема се и позицията от предишната стъпка – прощава малко закъснение на мрежата.

#define MG_TYPE_BOUNCE      0
#define MG_TYPE_FILL        1

#define MG_RESULT_FAIL      0
#define MG_RESULT_GOOD      1
#define MG_RESULT_PERFECT   2

#define MG_BAR_LENGTH       20
#define MG_ZONE_LENGTH      5
#define MG_STEP_MS          150
#define MG_TIMEOUT_MS       15000

// Кой модул е пуснал играта
#define MG_OWNER_FARM       1

static Text:gMgBox = INVALID_TEXT_DRAW, Text:gMgButton = INVALID_TEXT_DRAW;

static bool:MgActive[MAX_PLAYERS],
    MgType[MAX_PLAYERS],
    MgPos[MAX_PLAYERS],
    MgPrevPos[MAX_PLAYERS],
    MgDir[MAX_PLAYERS],
    MgZone[MAX_PLAYERS],
    MgTimer[MAX_PLAYERS],
    MgStartTick[MAX_PLAYERS],
    MgOwner[MAX_PLAYERS],
    MgPurpose[MAX_PLAYERS],
    MgData[MAX_PLAYERS],
    PlayerText:MgTitleTD[MAX_PLAYERS],
    PlayerText:MgBarTD[MAX_PLAYERS];

stock Minigame_Init()
{
    gMgBox = TextDrawCreate(320.0, 292.0, "_");
    TextDrawFont(gMgBox, TEXT_DRAW_FONT_1);
    TextDrawLetterSize(gMgBox, 0.0, 7.2);
    TextDrawAlignment(gMgBox, TEXT_DRAW_ALIGN_CENTRE);
    TextDrawUseBox(gMgBox, true);
    TextDrawBoxColour(gMgBox, 0x000000BB);
    TextDrawTextSize(gMgBox, 0.0, 260.0);

    gMgButton = TextDrawCreate(320.0, 342.0, "TAP");
    TextDrawFont(gMgButton, TEXT_DRAW_FONT_2);
    TextDrawLetterSize(gMgButton, 0.45, 1.8);
    TextDrawAlignment(gMgButton, TEXT_DRAW_ALIGN_CENTRE);
    TextDrawColour(gMgButton, COLOR_WHITE);
    TextDrawSetOutline(gMgButton, 0);
    TextDrawSetShadow(gMgButton, 0);
    TextDrawUseBox(gMgButton, true);
    TextDrawBoxColour(gMgButton, COLOR_BRAND);
    TextDrawTextSize(gMgButton, 20.0, 140.0);
    TextDrawSetSelectable(gMgButton, true);

    for (new i = 0; i < MAX_PLAYERS; i++) {
        MgTitleTD[i] = INVALID_PLAYER_TEXT_DRAW;
        MgBarTD[i] = INVALID_PLAYER_TEXT_DRAW;
    }
}

stock bool:Minigame_IsActive(playerid)
{
    return MgActive[playerid];
}

// title е на латиница (шрифтовете на GTA нямат кирилица).
stock Minigame_Start(playerid, type, const title[], owner, purpose, data)
{
    if (MgActive[playerid]) Minigame_Stop(playerid);

    MgActive[playerid] = true;
    MgType[playerid] = type;
    MgOwner[playerid] = owner;
    MgPurpose[playerid] = purpose;
    MgData[playerid] = data;
    MgPos[playerid] = 0;
    MgPrevPos[playerid] = 0;
    MgDir[playerid] = 1;
    MgStartTick[playerid] = GetTickCount();

    if (type == MG_TYPE_FILL) {
        // Зоната е към края – трябва да спреш преди да прелее
        MgZone[playerid] = 9 + random(MG_BAR_LENGTH - MG_ZONE_LENGTH - 9);
    } else {
        MgZone[playerid] = 2 + random(MG_BAR_LENGTH - MG_ZONE_LENGTH - 2);
    }

    new PlayerText:t = CreatePlayerTextDraw(playerid, 320.0, 297.0, "_");
    PlayerTextDrawFont(playerid, t, TEXT_DRAW_FONT_2);
    PlayerTextDrawLetterSize(playerid, t, 0.30, 1.20);
    PlayerTextDrawAlignment(playerid, t, TEXT_DRAW_ALIGN_CENTRE);
    PlayerTextDrawColour(playerid, t, COLOR_BRAND);
    PlayerTextDrawSetOutline(playerid, t, 1);
    PlayerTextDrawSetShadow(playerid, t, 0);
    PlayerTextDrawBackgroundColour(playerid, t, 0x000000FF);
    PlayerTextDrawSetString(playerid, t, "%s", title);
    MgTitleTD[playerid] = t;

    new PlayerText:b = CreatePlayerTextDraw(playerid, 320.0, 315.0, "_");
    PlayerTextDrawFont(playerid, b, TEXT_DRAW_FONT_1);
    PlayerTextDrawLetterSize(playerid, b, 0.45, 2.0);
    PlayerTextDrawAlignment(playerid, b, TEXT_DRAW_ALIGN_CENTRE);
    PlayerTextDrawColour(playerid, b, COLOR_WHITE);
    PlayerTextDrawSetOutline(playerid, b, 1);
    PlayerTextDrawSetShadow(playerid, b, 0);
    PlayerTextDrawBackgroundColour(playerid, b, 0x000000FF);
    MgBarTD[playerid] = b;

    Minigame_Draw(playerid);
    TextDrawShowForPlayer(playerid, gMgBox);
    TextDrawShowForPlayer(playerid, gMgButton);
    PlayerTextDrawShow(playerid, t);
    PlayerTextDrawShow(playerid, b);
    SelectTextDraw(playerid, COLOR_GOLD);

    MgTimer[playerid] = SetTimerEx("Minigame_Step", MG_STEP_MS, true, "i", playerid);
}

stock Minigame_Draw(playerid)
{
    new str[256], lastColour = -1, colour;
    str[0] = EOS;
    for (new i = 0; i < MG_BAR_LENGTH; i++) {
        if (i == MgPos[playerid]) colour = 2;                                            // маркер
        else if (i >= MgZone[playerid] && i < MgZone[playerid] + MG_ZONE_LENGTH) colour = 1; // зелена зона
        else colour = 0;

        if (colour != lastColour) {
            switch (colour) {
                case 0: strcat(str, "~w~");
                case 1: strcat(str, "~g~");
                case 2: strcat(str, "~r~");
            }
            lastColour = colour;
        }
        if (colour == 2) strcat(str, "O");
        else strcat(str, "I");
    }
    PlayerTextDrawSetString(playerid, MgBarTD[playerid], "%s", str);
}

forward Minigame_Step(playerid);
public Minigame_Step(playerid)
{
    if (!MgActive[playerid] || !IsPlayerConnected(playerid)) return 0;

    if (GetTickCount() - MgStartTick[playerid] > MG_TIMEOUT_MS) {
        Minigame_Finish(playerid, MG_RESULT_FAIL);
        return 0;
    }

    MgPrevPos[playerid] = MgPos[playerid];
    if (MgType[playerid] == MG_TYPE_FILL) {
        MgPos[playerid]++;
        if (MgPos[playerid] >= MG_BAR_LENGTH) {
            Minigame_Finish(playerid, MG_RESULT_FAIL); // преля
            return 0;
        }
    } else {
        MgPos[playerid] += MgDir[playerid];
        if (MgPos[playerid] >= MG_BAR_LENGTH - 1) MgDir[playerid] = -1;
        else if (MgPos[playerid] <= 0) MgDir[playerid] = 1;
    }
    Minigame_Draw(playerid);
    return 1;
}

static stock bool:Minigame_InZone(playerid, pos)
{
    return (pos >= MgZone[playerid] && pos < MgZone[playerid] + MG_ZONE_LENGTH);
}

// Връща true, ако натискането е обработено тук.
stock bool:Minigame_OnClickTextDraw(playerid, Text:clickedid)
{
    if (!MgActive[playerid]) return false;

    if (clickedid == INVALID_TEXT_DRAW) {
        // ESC (на PC) – отказ
        Minigame_Finish(playerid, MG_RESULT_FAIL);
        return true;
    }
    if (clickedid != gMgButton) return false;

    new center = MgZone[playerid] + MG_ZONE_LENGTH / 2, result = MG_RESULT_FAIL;
    if (MgPos[playerid] == center || MgPrevPos[playerid] == center) {
        result = MG_RESULT_PERFECT;
    } else if (Minigame_InZone(playerid, MgPos[playerid]) || Minigame_InZone(playerid, MgPrevPos[playerid])) {
        result = MG_RESULT_GOOD;
    }
    Minigame_Finish(playerid, result);
    return true;
}

// Спира без резултат (напр. при изход)
stock Minigame_Stop(playerid)
{
    if (!MgActive[playerid]) return;
    MgActive[playerid] = false;
    KillTimer(MgTimer[playerid]);
    TextDrawHideForPlayer(playerid, gMgBox);
    TextDrawHideForPlayer(playerid, gMgButton);
    if (MgTitleTD[playerid] != INVALID_PLAYER_TEXT_DRAW) {
        PlayerTextDrawDestroy(playerid, MgTitleTD[playerid]);
        MgTitleTD[playerid] = INVALID_PLAYER_TEXT_DRAW;
    }
    if (MgBarTD[playerid] != INVALID_PLAYER_TEXT_DRAW) {
        PlayerTextDrawDestroy(playerid, MgBarTD[playerid]);
        MgBarTD[playerid] = INVALID_PLAYER_TEXT_DRAW;
    }
    // Сваляме флага ПРЕДИ CancelSelectTextDraw – той извиква OnPlayerClickTextDraw с INVALID_TEXT_DRAW.
    CancelSelectTextDraw(playerid);
}

stock Minigame_Finish(playerid, result)
{
    if (!MgActive[playerid]) return;
    new owner = MgOwner[playerid], purpose = MgPurpose[playerid], data = MgData[playerid];
    Minigame_Stop(playerid);

    switch (result) {
        case MG_RESULT_PERFECT: GameTextForPlayer(playerid, "~g~PERFECT!", 1500, 3);
        case MG_RESULT_GOOD:    GameTextForPlayer(playerid, "~y~GOOD", 1500, 3);
        default:                GameTextForPlayer(playerid, "~r~MISS", 1500, 3);
    }

    switch (owner) {
        case MG_OWNER_FARM: Farm_OnMinigameResult(playerid, purpose, data, result);
    }
}

stock Minigame_OnPlayerDisconnect(playerid)
{
    if (MgActive[playerid]) {
        MgActive[playerid] = false;
        KillTimer(MgTimer[playerid]);
    }
    MgTitleTD[playerid] = INVALID_PLAYER_TEXT_DRAW;
    MgBarTD[playerid] = INVALID_PLAYER_TEXT_DRAW;
}
