// Лого на сървъра в горния десен ъгъл: малък 3D камион + "CARGO STATE" + подзаглавие.
// Над парите и иконата на оръжието, за да не ги застъпва (и на PC, и на мобилния клиент).

#define LOGO_RIGHT      556.0
#define LOGO_Y          3.0

static Text:LogoTruck = Text:INVALID_TEXT_DRAW;
static Text:LogoTitle = Text:INVALID_TEXT_DRAW;
static Text:LogoSub   = Text:INVALID_TEXT_DRAW;
static Text:LogoLine  = Text:INVALID_TEXT_DRAW;

stock Logo_Init()
{
    // Надпис "CARGO STATE" (подравнен вдясно)
    LogoTitle = TextDrawCreate(LOGO_RIGHT, LOGO_Y, "~w~CARGO ~y~STATE");
    TextDrawFont(LogoTitle, TEXT_DRAW_FONT_2);
    TextDrawLetterSize(LogoTitle, 0.30, 1.25);
    TextDrawAlignment(LogoTitle, TEXT_DRAW_ALIGN_RIGHT);
    TextDrawSetOutline(LogoTitle, 1);
    TextDrawSetShadow(LogoTitle, 0);
    TextDrawBackgroundColour(LogoTitle, 0x000000AA);
    TextDrawSetProportional(LogoTitle, true);

    // Оранжева линия под надписа
    LogoLine = TextDrawCreate(LOGO_RIGHT, LOGO_Y + 14.5, "_");
    TextDrawUseBox(LogoLine, true);
    TextDrawBoxColour(LogoLine, 0xF5A623FF);
    TextDrawLetterSize(LogoLine, 0.0, -0.35);
    TextDrawTextSize(LogoLine, LOGO_RIGHT - 82.0, 0.0);
    TextDrawAlignment(LogoLine, TEXT_DRAW_ALIGN_RIGHT);

    // Подзаглавие
    LogoSub = TextDrawCreate(LOGO_RIGHT, LOGO_Y + 16.0, "TRUCK SIMULATOR");
    TextDrawFont(LogoSub, TEXT_DRAW_FONT_2);
    TextDrawLetterSize(LogoSub, 0.14, 0.7);
    TextDrawAlignment(LogoSub, TEXT_DRAW_ALIGN_RIGHT);
    TextDrawColour(LogoSub, 0xDDDDDDFF);
    TextDrawSetOutline(LogoSub, 1);
    TextDrawSetShadow(LogoSub, 0);
    TextDrawBackgroundColour(LogoSub, 0x00000099);
    TextDrawSetProportional(LogoSub, true);

    // Малък 3D камион вляво от надписа (прозрачен фон)
    LogoTruck = TextDrawCreate(LOGO_RIGHT - 118.0, LOGO_Y - 4.0, "_");
    TextDrawFont(LogoTruck, TEXT_DRAW_FONT_MODEL_PREVIEW);
    TextDrawUseBox(LogoTruck, true);
    TextDrawBoxColour(LogoTruck, 0x00000000);
    TextDrawBackgroundColour(LogoTruck, 0x00000000);
    TextDrawTextSize(LogoTruck, 34.0, 30.0);
    TextDrawSetPreviewModel(LogoTruck, 515);
    TextDrawSetPreviewRot(LogoTruck, -10.0, 0.0, -35.0, 1.0);
    TextDrawSetPreviewVehCol(LogoTruck, 6, 1);
}

stock Logo_Show(playerid)
{
    TextDrawShowForPlayer(playerid, LogoTruck);
    TextDrawShowForPlayer(playerid, LogoTitle);
    TextDrawShowForPlayer(playerid, LogoLine);
    TextDrawShowForPlayer(playerid, LogoSub);
}
