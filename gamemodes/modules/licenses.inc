// Шофьорски категории: B (бусове), C (камиони), CE (влекачи с ремарке), ADR (опасни товари - гориво).
//
// - B се дава при регистрация. Останалите се взимат в автошколата в San Fierro (виж school.inc):
//   рецепционистът дава курсовете, теорията е в стаята за теория.
// - Изпит: такса, теория (5 въпроса, поне 4 верни) и за C/CE - практика с изпитен камион:
//   до случайна компания и обратно до автошколата, навреме, без много щети и без превишена скорост.
//   ADR е само теория.
// - Без нужната категория не можеш да караш (сядаш зад волана -> сваля те), да купиш в автокъщата
//   или да вземеш курс с гориво (ADR).

enum {
    LIC_B,
    LIC_C,
    LIC_CE,
    LIC_ADR,
    MAX_LICENSES
}

#define LIC_BIT(%0)             (1 << (%0))
#define LIC_DEFAULT             LIC_BIT(LIC_B)

#define LIC_THEORY_QUESTIONS    5
#define LIC_THEORY_PASS         4
#define LIC_SPEED_LIMIT         90          // км/ч на практическия изпит
#define LIC_SPEED_STRIKES       3
#define LIC_MAX_DAMAGE          200.0       // загуба на "здраве" на камиона = провал
#define LIC_OUT_SECONDS         30          // толкова секунди извън камиона = провал
#define LIC_CP_SIZE             8.0

stock const LicenseNames[MAX_LICENSES][] = {"B", "C", "CE", "ADR"};
stock const LicenseDesc[MAX_LICENSES][] = {"бусове до 3.5 т", "камиони", "влекачи с ремарке", "опасни товари (гориво)"};
static const LicenseLevel[MAX_LICENSES] = {1, 2, 4, 4};
static const LicensePrice[MAX_LICENSES] = {0, 4000, 9000, 6000};
static const LicenseNeeds[MAX_LICENSES] = {-1, LIC_B, LIC_C, LIC_CE};
static const LicenseExamModel[MAX_LICENSES] = {0, 414, 403, 0};    // Mule, Linerunner (+ ремарке)

// Въпроси: категория, въпрос, верен отговор, два грешни
enum E_LIC_Q {
    lqCat,
    lqText[96],
    lqRight[64],
    lqWrong1[64],
    lqWrong2[64]
}
static const LicQuestions[][E_LIC_Q] = {
    {LIC_C, "Какво правиш с товар, когато стигнеш кантар?", "Спирам на платформата за 10 секунди", "Минавам бързо покрай него", "Обръщам и го заобикалям"},
    {LIC_C, "Колко най-много можеш да натовариш (с риск от глоба)?", "До 130% от товароносимостта", "Колкото поиска клиентът", "Точно 100%, нито кг повече"},
    {LIC_C, "Какво става при силен удар с чуплив товар?", "Товарът се чупи и курсът се проваля", "Нищо, застрахован е", "Получавам бонус за бързина"},
    {LIC_C, "С кой клавиш палиш двигателя?", "CTRL", "N", "Y"},
    {LIC_C, "Какво правиш, когато умората стигне високо ниво?", "Почивам на паркинг или пия кафе", "Карам по-бързо, за да свърша", "Пускам радиото по-силно"},
    {LIC_C, "Какво е особеното при хладилния товар?", "По-кратък срок, закъснението го разваля", "Може да се кара без камион", "Плаща се двойно винаги"},
    {LIC_C, "Къде виждаш къде един товар е най-търсен?", "Меню > Борса на товарите", "В автокъщата", "На бензиностанцията"},
    {LIC_CE, "Какво правиш, ако ремаркето се откачи?", "Връщам се и го закачам отново (H)", "Продължавам без него", "Обаждам се на админ"},
    {LIC_CE, "Какви товари карат влекачите?", "Само тежки товари в ремарке", "Само пратки", "Каквото и да е, дори 100 кг"},
    {LIC_CE, "Как се завива с дълго ремарке?", "Широко и бавно", "Рязко и бързо", "С ръчната спирачка"},
    {LIC_CE, "Къде се товари и разтоварва?", "В червения маркер на компанията", "Където спра", "Само в гаража"},
    {LIC_CE, "Какво става, ако си претоварен на кантар?", "Глоба според тоновете отгоре", "Нищо", "Камионът се конфискува"},
    {LIC_CE, "Защо ремаркето удължава спирането?", "Заради по-голямата маса", "Защото е по-ниско", "Не го удължава"},
    {LIC_ADR, "Какво означава ADR?", "Превоз на опасни товари", "Автоматична дълга ремарка", "Административна регистрация"},
    {LIC_ADR, "Кой товар в играта изисква ADR?", "Гориво", "Пясък", "Мебели"},
    {LIC_ADR, "Какво правиш при авария с цистерна?", "Спирам, отдалечавам се, не паля огън", "Паля цигара и чакам", "Продължавам, все едно нищо"},
    {LIC_ADR, "С какво се маркира камион с опасен товар?", "Оранжеви табели", "Зелени светлини", "Нищо не се маркира"},
    {LIC_ADR, "Кога цистерната е най-опасна?", "При резки завои и спирания", "На паркинг", "При слизане от камиона"},
    {LIC_ADR, "Може ли да се пуши при товарене на гориво?", "Не", "Да, ако е навън", "Да, ако бързам"}
};

enum {
    LEX_NONE,
    LEX_THEORY,
    LEX_TO_POINT,
    LEX_BACK
}

static PlayerLic[MAX_PLAYERS];
static LexStage[MAX_PLAYERS];
static LexCat[MAX_PLAYERS];
static LexQ[MAX_PLAYERS][LIC_THEORY_QUESTIONS];
static LexQIndex[MAX_PLAYERS];
static LexQRight[MAX_PLAYERS];          // на кой ред (1-3) е верният отговор
static LexCorrect[MAX_PLAYERS];
static LexVeh[MAX_PLAYERS] = {INVALID_VEHICLE_ID, ...};
static LexTrailer[MAX_PLAYERS] = {INVALID_VEHICLE_ID, ...};
static Float:LexSchool[MAX_PLAYERS][4];
static Float:LexPoint[MAX_PLAYERS][3];
static LexDeadline[MAX_PLAYERS];
static Float:LexStartHealth[MAX_PLAYERS];
static LexStrikes[MAX_PLAYERS];
static LexLastStrike[MAX_PLAYERS];
static LexOut[MAX_PLAYERS];
static LexSchoolMap[MAX_PLAYERS][MAX_LICENSES];

// ---------- Данни ----------

stock Licenses_Init()
{
    Database_AddColumn("accounts", "licenses", "INTEGER NOT NULL DEFAULT 1");
    // Еднократно: играчите, които вече имат камиони, получават нужните категории
    if (Database_GetMeta("licenses_migrated") == 0) {
        Database_Exec("UPDATE accounts SET licenses = licenses | 2 WHERE id IN (SELECT owner_id FROM player_vehicles WHERE model IN (414, 499, 456, 455, 403, 514, 515))");
        Database_Exec("UPDATE accounts SET licenses = licenses | 4 WHERE id IN (SELECT owner_id FROM player_vehicles WHERE model IN (403, 514, 515))");
        Database_SetMeta("licenses_migrated", 1);
    }
}

stock License_OnPlayerConnect(playerid)
{
    PlayerLic[playerid] = LIC_DEFAULT;
    LexStage[playerid] = LEX_NONE;
    LexVeh[playerid] = INVALID_VEHICLE_ID;
    LexTrailer[playerid] = INVALID_VEHICLE_ID;
}

stock License_OnLogin(playerid)
{
    new q[96];
    format(q, sizeof (q), "SELECT licenses FROM accounts WHERE id = %d", PlayerData[playerid][pID]);
    new DBResult:r = Database_Query(q);
    PlayerLic[playerid] = LIC_DEFAULT;
    if (r != DBResult:0) {
        if (DB_GetRowCount(r) > 0) PlayerLic[playerid] = DB_GetFieldIntByName(r, "licenses") | LIC_DEFAULT;
        DB_FreeResultSet(r);
    }
}

stock License_OnPlayerDisconnect(playerid)
{
    License_EndExam(playerid);
}

static stock License_Save(playerid)
{
    new q[96];
    format(q, sizeof (q), "UPDATE accounts SET licenses = %d WHERE id = %d", PlayerLic[playerid], PlayerData[playerid][pID]);
    Database_Exec(q);
}

stock bool:License_Has(playerid, cat)
{
    return (PlayerLic[playerid] & LIC_BIT(cat)) != 0;
}

stock License_GetBits(playerid)
{
    return PlayerLic[playerid];
}

stock License_SetBits(playerid, bits)
{
    PlayerLic[playerid] = bits | LIC_DEFAULT;
    License_Save(playerid);
}

// Нужна категория за модела: влекач = CE, над 3.5 т = C, иначе B (-1 = не е от каталога)
stock License_ForModel(model)
{
    new i = Catalog_Find(model);
    if (i == -1) return -1;
    if (VehCatalog[i][vcTrailer]) return LIC_CE;
    if (VehCatalog[i][vcPayload] > 3500) return LIC_C;
    return LIC_B;
}

stock bool:License_CanDrive(playerid, model)
{
    new cat = License_ForModel(model);
    return (cat == -1 || License_Has(playerid, cat));
}

// "B, C, CE"
stock License_Format(bits, dest[], size = sizeof (dest))
{
    dest[0] = EOS;
    for (new c = 0; c < MAX_LICENSES; c++) {
        if (!(bits & LIC_BIT(c))) continue;
        if (dest[0] != EOS) strcat(dest, ", ", size);
        strcat(dest, LicenseNames[c], size);
    }
    if (dest[0] == EOS) format(dest, size, "-");
}

// Сяда зад волана: без категория - вън (админите са свободни)
stock License_OnDriverEnter(playerid)
{
    new vehicleid = GetPlayerVehicleID(playerid);
    // Изпитният камион е само за изпитвания
    for (new i = 0; i < MAX_PLAYERS; i++) {
        if (i != playerid && LexVeh[i] == vehicleid && LexStage[i] >= LEX_TO_POINT) {
            RemovePlayerFromVehicle(playerid);
            SendError(playerid, "Това е изпитен камион на автошколата.");
            return;
        }
    }
    if (vehicleid == LexVeh[playerid]) return;
    if (PlayerData[playerid][pAdmin] >= 1 && PlayerData[playerid][pAdminAuthed]) return;
    new model = GetVehicleModel(vehicleid);
    if (License_CanDrive(playerid, model)) return;
    RemovePlayerFromVehicle(playerid);
    new cat = License_ForModel(model);
    SendClientMessage(playerid, COLOR_RED, "[!] " C_WHITE "Нямаш категория " C_BRAND "%s" C_WHITE " (%s). Вземи я в автошкола (GPS > Автошкола).",
        LicenseNames[cat], LicenseDesc[cat]);
}

// ---------- Автошкола ----------

stock License_ShowSchool(playerid)
{
    if (LexStage[playerid] != LEX_NONE) {
        SendError(playerid, "Вече си на изпит.");
        return;
    }
    // Изпитният камион тръгва от (и се връща на) мястото пред автошколата
    School_GetVehicleSpawn(LexSchool[playerid][0], LexSchool[playerid][1], LexSchool[playerid][2], LexSchool[playerid][3]);

    new body[640], line[160], n = 0, money[16];
    format(body, sizeof (body), "Категория\tЗа какво\tСтатус\n");
    for (new c = LIC_C; c < MAX_LICENSES; c++) {
        FormatMoney(LicensePrice[c], money);
        if (License_Has(playerid, c)) {
            format(line, sizeof (line), C_BRAND "%s\t" C_WHITE "%s\t" C_GREEN "имаш я\n", LicenseNames[c], LicenseDesc[c]);
        } else if (LicenseNeeds[c] != -1 && !License_Has(playerid, LicenseNeeds[c])) {
            format(line, sizeof (line), C_BRAND "%s\t" C_WHITE "%s\t" C_GREY "първо %s\n", LicenseNames[c], LicenseDesc[c], LicenseNames[LicenseNeeds[c]]);
        } else if (PlayerData[playerid][pLevel] < LicenseLevel[c]) {
            format(line, sizeof (line), C_BRAND "%s\t" C_WHITE "%s\t" C_RED "ниво %d\n", LicenseNames[c], LicenseDesc[c], LicenseLevel[c]);
        } else {
            format(line, sizeof (line), C_BRAND "%s\t" C_WHITE "%s\t" C_GREEN "$%s\n", LicenseNames[c], LicenseDesc[c], money);
        }
        strcat(body, line);
        LexSchoolMap[playerid][n++] = c;
    }
    ShowPlayerDialog(playerid, DIALOG_LIC_SCHOOL, DIALOG_STYLE_TABLIST_HEADERS, C_BRAND "Рецепция" C_WHITE " | Кой курс избираш?", "%s", "Запиши ме", "Затвори", body);
}

static stock License_AskConfirm(playerid, cat)
{
    new money[16], title[64];
    FormatMoney(LicensePrice[cat], money);
    format(title, sizeof (title), C_BRAND "Изпит за категория %s", LicenseNames[cat]);
    LexCat[playerid] = cat;
    if (LicenseExamModel[cat] != 0) {
        ShowPlayerDialog(playerid, DIALOG_LIC_CONFIRM, DIALOG_STYLE_MSGBOX, title,
            C_WHITE "Такса: " C_GREEN "$%s\n\n" \
            C_WHITE "1. Теория: %d въпроса, нужни са поне %d верни.\n" \
            C_WHITE "2. Практика: караш изпитния камион до компания и обратно.\n" \
            C_GREY "   Без превишаване над %d км/ч, без сериозни удари, навреме.\n\n" \
            C_WHITE "При провал таксата не се връща.",
            "Започни", "Отказ", money, LIC_THEORY_QUESTIONS, LIC_THEORY_PASS, LIC_SPEED_LIMIT);
    } else {
        ShowPlayerDialog(playerid, DIALOG_LIC_CONFIRM, DIALOG_STYLE_MSGBOX, title,
            C_WHITE "Такса: " C_GREEN "$%s\n\n" \
            C_WHITE "Теория: %d въпроса, нужни са поне %d верни.\n\n" \
            C_WHITE "При провал таксата не се връща.",
            "Започни", "Отказ", money, LIC_THEORY_QUESTIONS, LIC_THEORY_PASS);
    }
}

static stock License_StartTheory(playerid)
{
    new cat = LexCat[playerid];
    if (Delivery_HasJob(playerid)) return SendError(playerid, "Първо завърши или откажи курса си.");
    if (PlayerData[playerid][pLevel] < LicenseLevel[cat]) return SendError(playerid, "Нивото ти не стига.");
    if (LicenseNeeds[cat] != -1 && !License_Has(playerid, LicenseNeeds[cat])) return SendError(playerid, "Първо вземи по-ниската категория.");
    if (!Money_Take(playerid, LicensePrice[cat])) return SendError(playerid, "Нямаш достатъчно пари.");

    // Случайни въпроси от категорията
    new pool[sizeof (LicQuestions)], count = 0;
    for (new i = 0; i < sizeof (LicQuestions); i++) {
        if (LicQuestions[i][lqCat] == cat) pool[count++] = i;
    }
    for (new i = 0; i < LIC_THEORY_QUESTIONS; i++) {
        new j = i + random(count - i);
        new tmp = pool[i];
        pool[i] = pool[j];
        pool[j] = tmp;
        LexQ[playerid][i] = pool[i];
    }
    LexStage[playerid] = LEX_THEORY;
    LexQIndex[playerid] = 0;
    LexCorrect[playerid] = 0;
    LexQRight[playerid] = 1 + random(3);
    School_SeatForTheory(playerid);
    SendClientMessage(playerid, COLOR_BRAND, "[АВТОШКОЛА] " C_WHITE "Седни и се съсредоточи - теорията започва.");
    License_ShowQuestion(playerid);
    return 1;
}

static stock License_ShowQuestion(playerid)
{
    new q = LexQ[playerid][LexQIndex[playerid]], body[400], title[64];
    new right = LexQRight[playerid];
    format(title, sizeof (title), C_BRAND "Категория %s" C_WHITE " | въпрос %d/%d", LicenseNames[LexCat[playerid]], LexQIndex[playerid] + 1, LIC_THEORY_QUESTIONS);
    // Ред 0 е въпросът, редове 1-3 - отговорите (верният е на ред right)
    if (right == 1) format(body, sizeof (body), C_BRAND "%s\n" C_WHITE "1) %s\n2) %s\n3) %s", LicQuestions[q][lqText], LicQuestions[q][lqRight], LicQuestions[q][lqWrong1], LicQuestions[q][lqWrong2]);
    else if (right == 2) format(body, sizeof (body), C_BRAND "%s\n" C_WHITE "1) %s\n2) %s\n3) %s", LicQuestions[q][lqText], LicQuestions[q][lqWrong1], LicQuestions[q][lqRight], LicQuestions[q][lqWrong2]);
    else format(body, sizeof (body), C_BRAND "%s\n" C_WHITE "1) %s\n2) %s\n3) %s", LicQuestions[q][lqText], LicQuestions[q][lqWrong2], LicQuestions[q][lqWrong1], LicQuestions[q][lqRight]);
    ShowPlayerDialog(playerid, DIALOG_LIC_QUESTION, DIALOG_STYLE_LIST, title, "%s", "Отговори", "Откажи", body);
}

static stock License_TheoryAnswer(playerid, listitem)
{
    if (listitem == LexQRight[playerid]) LexCorrect[playerid]++;
    LexQIndex[playerid]++;
    LexQRight[playerid] = 1 + random(3);
    if (LexQIndex[playerid] < LIC_THEORY_QUESTIONS) {
        License_ShowQuestion(playerid);
        return;
    }
    new cat = LexCat[playerid];
    if (LexCorrect[playerid] < LIC_THEORY_PASS) {
        LexStage[playerid] = LEX_NONE;
        School_StandUp(playerid);
        ShowPlayerDialog(playerid, DIALOG_LIC_RESULT, DIALOG_STYLE_MSGBOX, C_RED "Скъсан",
            C_WHITE "Верни отговори: " C_RED "%d" C_WHITE " от %d (нужни са %d).\n\nОпитай пак, когато си готов.",
            "Добре", "", LexCorrect[playerid], LIC_THEORY_QUESTIONS, LIC_THEORY_PASS);
        return;
    }
    if (LicenseExamModel[cat] == 0) {
        License_Grant(playerid, cat);
        return;
    }
    License_StartDrive(playerid);
}

// ---------- Практика ----------

static stock License_PickPoint(playerid)
{
    new best = -1, Float:bestScore = 999999.0;
    for (new c = 0; c < MAX_COMPANIES; c++) {
        if (!Company_IsValid(c)) continue;
        new Float:x, Float:y, Float:z;
        Company_GetPos(c, x, y, z);
        new Float:d = VectorSize(x - LexSchool[playerid][0], y - LexSchool[playerid][1], 0.0);
        // Предпочита 600-1800 м; иначе най-близкото до 1200 м
        new Float:score = floatabs(d - 1200.0);
        if (d >= 600.0 && d <= 1800.0) score = float(random(500));
        if (score < bestScore) {
            bestScore = score;
            best = c;
        }
    }
    if (best == -1) return 0;
    Company_GetPos(best, LexPoint[playerid][0], LexPoint[playerid][1], LexPoint[playerid][2]);
    return 1;
}

static stock License_StartDrive(playerid)
{
    if (!License_PickPoint(playerid)) {
        License_Grant(playerid, LexCat[playerid]);      // няма компании - практиката се пропуска
        return;
    }
    new Float:a = LexSchool[playerid][3];
    new Float:x = LexSchool[playerid][0];
    new Float:y = LexSchool[playerid][1];
    new Float:z = LexSchool[playerid][2];
    // От стаята за теория - навън, при камиона
    School_LeaveForExam(playerid, x, y, z, a);
    new veh = CreateVehicle(LicenseExamModel[LexCat[playerid]], x, y, z + 1.0, a, 1, 3, -1);
    if (veh == INVALID_VEHICLE_ID) {
        License_Grant(playerid, LexCat[playerid]);
        return;
    }
    LexVeh[playerid] = veh;
    SetVehicleVirtualWorld(veh, 0);
    PutPlayerInVehicle(playerid, veh, 0);
    Engine_Set(veh, true);
    if (LexCat[playerid] == LIC_CE) {
        new trailer = CreateVehicle(435, x - 10.0 * floatsin(-a, degrees), y - 10.0 * floatcos(-a, degrees), z + 1.0, a, 1, 1, -1);
        if (trailer != INVALID_VEHICLE_ID) {
            LexTrailer[playerid] = trailer;
            SetTimerEx("License_AttachTrailer", 500, false, "iii", playerid, PlayerData[playerid][pConnSeq], trailer);
        }
    }
    new Float:dist = VectorSize(LexPoint[playerid][0] - x, LexPoint[playerid][1] - y, 0.0);
    new limit = floatround(dist * 2.0 * 1.4 / 11.0) + 120;
    LexDeadline[playerid] = gettime() + limit;
    GetVehicleHealth(veh, LexStartHealth[playerid]);
    LexStrikes[playerid] = 0;
    LexLastStrike[playerid] = 0;
    LexOut[playerid] = 0;
    LexStage[playerid] = LEX_TO_POINT;
    License_SetCP(playerid, LexPoint[playerid][0], LexPoint[playerid][1], LexPoint[playerid][2], false);

    GameTextForPlayer(playerid, "~y~DRIVING TEST~n~~w~FOLLOW THE ROUTE", 4000, 1);
    SendClientMessage(playerid, COLOR_BRAND, "[АВТОШКОЛА] " C_WHITE "Теорията е взета (%d/%d)! Сега практика: карай до маркера и се върни.",
        LexCorrect[playerid], LIC_THEORY_QUESTIONS);
    SendClientMessage(playerid, COLOR_BRAND, "[АВТОШКОЛА] " C_WHITE "Време: %d мин. Не карай над %d км/ч и пази камиона%s.",
        limit / 60 + 1, LIC_SPEED_LIMIT, (LexCat[playerid] == LIC_CE) ? (" и ремаркето") : (""));
}

forward License_AttachTrailer(playerid, seq, trailer);
public License_AttachTrailer(playerid, seq, trailer)
{
    if (!IsPlayerConnected(playerid) || PlayerData[playerid][pConnSeq] != seq) return 0;
    if (LexTrailer[playerid] != trailer || LexVeh[playerid] == INVALID_VEHICLE_ID) return 0;
    AttachTrailerToVehicle(trailer, LexVeh[playerid]);
    return 1;
}

static stock License_SetCP(playerid, Float:x, Float:y, Float:z, bool:finish)
{
    DisablePlayerRaceCheckpoint(playerid);
    SetPlayerRaceCheckpoint(playerid, finish ? CP_TYPE_GROUND_FINISH : CP_TYPE_GROUND_NORMAL, x, y, z, x, y, z, LIC_CP_SIZE);
    Route_Set(playerid, x, y, z, ROUTE_SRC_JOB);
}

static stock bool:License_TrailerOk(playerid)
{
    if (LexCat[playerid] != LIC_CE) return true;
    return (LexTrailer[playerid] != INVALID_VEHICLE_ID && GetVehicleTrailer(LexVeh[playerid]) == LexTrailer[playerid]);
}

// Вика се от OnPlayerEnterRaceCheckpoint; 1 = изпитът го обработи
stock License_OnEnterRaceCP(playerid)
{
    if (LexStage[playerid] != LEX_TO_POINT && LexStage[playerid] != LEX_BACK) return 0;
    if (GetPlayerVehicleID(playerid) != LexVeh[playerid]) return 1;
    if (!License_TrailerOk(playerid)) {
        SendError(playerid, "Закачи ремаркето (H), преди да продължиш.");
        return 1;
    }
    if (LexStage[playerid] == LEX_TO_POINT) {
        LexStage[playerid] = LEX_BACK;
        PlayerPlaySound(playerid, 1056, 0.0, 0.0, 0.0);
        License_SetCP(playerid, LexSchool[playerid][0], LexSchool[playerid][1], LexSchool[playerid][2], true);
        SendClientMessage(playerid, COLOR_BRAND, "[АВТОШКОЛА] " C_WHITE "Добре! Сега се върни в автошколата.");
        return 1;
    }
    License_Grant(playerid, LexCat[playerid]);
    return 1;
}

static stock License_Fail(playerid, const reason[])
{
    new cat = LexCat[playerid];
    License_EndExam(playerid);
    GameTextForPlayer(playerid, "~r~TEST FAILED", 4000, 1);
    SendClientMessage(playerid, COLOR_RED, "[АВТОШКОЛА] " C_WHITE "Скъсан на изпита за категория %s: %s", LicenseNames[cat], reason);
}

static stock License_Grant(playerid, cat)
{
    License_EndExam(playerid);
    PlayerLic[playerid] |= LIC_BIT(cat);
    License_Save(playerid);
    PlayerPlaySound(playerid, 1058, 0.0, 0.0, 0.0);
    GameTextForPlayer(playerid, "~g~LICENSE %s~n~~w~PASSED!", 5000, 1, LicenseNames[cat]);
    SendClientMessage(playerid, COLOR_GREEN, "[АВТОШКОЛА] " C_WHITE "Поздравления! Вече имаш категория " C_BRAND "%s" C_WHITE " (%s).",
        LicenseNames[cat], LicenseDesc[cat]);
}

// Чисти изпита (без съобщения)
stock License_EndExam(playerid)
{
    if (LexStage[playerid] == LEX_THEORY) School_StandUp(playerid);
    if (LexStage[playerid] == LEX_TO_POINT || LexStage[playerid] == LEX_BACK) {
        DisablePlayerRaceCheckpoint(playerid);
        Route_Clear(playerid, ROUTE_SRC_JOB);
    }
    if (LexTrailer[playerid] != INVALID_VEHICLE_ID) DestroyVehicle(LexTrailer[playerid]);
    if (LexVeh[playerid] != INVALID_VEHICLE_ID) DestroyVehicle(LexVeh[playerid]);
    LexTrailer[playerid] = INVALID_VEHICLE_ID;
    LexVeh[playerid] = INVALID_VEHICLE_ID;
    LexStage[playerid] = LEX_NONE;
}

stock bool:License_InExam(playerid)
{
    return (LexStage[playerid] != LEX_NONE);
}

// Всяка секунда
stock License_Tick(playerid)
{
    if (LexStage[playerid] != LEX_TO_POINT && LexStage[playerid] != LEX_BACK) return;
    new veh = LexVeh[playerid];
    new left = LexDeadline[playerid] - gettime();
    if (left <= 0) {
        License_Fail(playerid, "не успя навреме.");
        return;
    }
    if (GetPlayerVehicleID(playerid) != veh) {
        LexOut[playerid]++;
        if (LexOut[playerid] == 1) SendClientMessage(playerid, COLOR_RED, "[АВТОШКОЛА] " C_WHITE "Върни се в изпитния камион до %d секунди!", LIC_OUT_SECONDS);
        if (LexOut[playerid] >= LIC_OUT_SECONDS) License_Fail(playerid, "напусна изпитния камион.");
        return;
    }
    LexOut[playerid] = 0;

    new Float:health;
    GetVehicleHealth(veh, health);
    if (LexStartHealth[playerid] - health > LIC_MAX_DAMAGE) {
        License_Fail(playerid, "твърде много щети по камиона.");
        return;
    }
    new Float:vx, Float:vy, Float:vz;
    GetVehicleVelocity(veh, vx, vy, vz);
    new kmh = floatround(VectorSize(vx, vy, vz) * 180.0);
    if (kmh > LIC_SPEED_LIMIT && gettime() - LexLastStrike[playerid] >= 5) {
        LexLastStrike[playerid] = gettime();
        LexStrikes[playerid]++;
        if (LexStrikes[playerid] >= LIC_SPEED_STRIKES) {
            License_Fail(playerid, "превишена скорост три пъти.");
            return;
        }
        GameTextForPlayer(playerid, "~r~SLOW DOWN!~n~~w~%d KM/H", 2000, 3, kmh);
        SendClientMessage(playerid, COLOR_RED, "[АВТОШКОЛА] " C_WHITE "Превишена скорост (%d км/ч)! Предупреждение %d/%d.", kmh, LexStrikes[playerid], LIC_SPEED_STRIKES - 1);
    }
    if (left == 60) SendClientMessage(playerid, COLOR_BRAND, "[АВТОШКОЛА] " C_WHITE "Остава 1 минута!");
}

stock License_OnVehicleDeath(vehicleid)
{
    for (new i = 0; i < MAX_PLAYERS; i++) {
        if (LexVeh[i] == vehicleid && (LexStage[i] == LEX_TO_POINT || LexStage[i] == LEX_BACK)) {
            License_Fail(i, "изпитният камион е разбит.");
        }
    }
}

// ---------- Диалози ----------

stock License_OnDialogResponse(playerid, dialogid, response, listitem)
{
    switch (dialogid) {
        case DIALOG_LIC_SCHOOL: {
            if (!response || listitem < 0 || listitem >= MAX_LICENSES - 1) return 1;
            new cat = LexSchoolMap[playerid][listitem];
            if (License_Has(playerid, cat)) {
                SendClientMessage(playerid, COLOR_GREEN, "[АВТОШКОЛА] " C_WHITE "Вече имаш категория %s.", LicenseNames[cat]);
                return 1;
            }
            if (LicenseNeeds[cat] != -1 && !License_Has(playerid, LicenseNeeds[cat])) {
                SendClientMessage(playerid, COLOR_RED, "[!] " C_WHITE "Първо вземи категория %s.", LicenseNames[LicenseNeeds[cat]]);
                return 1;
            }
            if (PlayerData[playerid][pLevel] < LicenseLevel[cat]) {
                SendClientMessage(playerid, COLOR_RED, "[!] " C_WHITE "Нужно е ниво %d за тази категория.", LicenseLevel[cat]);
                return 1;
            }
            License_AskConfirm(playerid, cat);
            return 1;
        }
        case DIALOG_LIC_CONFIRM: {
            if (response) License_StartTheory(playerid);
            return 1;
        }
        case DIALOG_LIC_QUESTION: {
            if (LexStage[playerid] != LEX_THEORY) return 1;
            if (!response) {
                LexStage[playerid] = LEX_NONE;
                School_StandUp(playerid);
                SendClientMessage(playerid, COLOR_RED, "[АВТОШКОЛА] " C_WHITE "Отказа се от изпита.");
                return 1;
            }
            if (listitem <= 0) {
                License_ShowQuestion(playerid);
                return 1;
            }
            License_TheoryAnswer(playerid, listitem);
            return 1;
        }
        case DIALOG_LIC_RESULT: return 1;
    }
    return 0;
}
