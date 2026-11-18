// Мапинг: интериор на автошколата (San Fierro).
//
// Сграда 20 м (X) x 16 м (Y), височина 3.5 м, високо над картата в собствен виртуален свят.
// Вход/изход на южната стена (-Y). Разпределение:
//
//      север (+Y)
//   +----------------------+-----------+
//   |   СТАЯ ЗА ТЕОРИЯ      | КАБИНЕТ   |
//   |  (бяла дъска, TV,     | инструктор|
//   |   9 работни места)    |           |
//   +----вр.------[ стена с акцент ]--вр.+   y = 0
//   |  чакалня    РЕЦЕПЦИЯ (NPC)  автомати|
//   |                                     |
//   +---------------[ вход ]--------------+  y = -8
//
// Черупката (под, таван, външни стени) е от големите плоскости 19538 като в гаража.
// Вътрешните стени са 19447 (~9.6 x 3.5 м), плотът на рецепцията - от няколко 19353,
// наредени плътно един до друг. Цветовете са плътни (текстура "white" от matcolours, оцветена).

#define SCHOOL_WORLD        5000
#define SCHOOL_X            600.0
#define SCHOOL_Y            600.0
#define SCHOOL_Z            1400.0

#define SCHOOL_HALF_W       10.0
#define SCHOOL_HALF_L       8.0
#define SCHOOL_HEIGHT       3.5

#define SCHOOL_PLANE        19538       // 62.5 x 125 м, лицето нагоре
#define SCHOOL_WALL_LONG    19447       // ~9.6 м дълга, 3.5 м висока (по Y при rz = 0)
#define SCHOOL_WALL_SMALL   19353       // ~3.2-3.5 м широка, 3.5 м висока

// Цветове (ARGB)
#define SCH_FLOOR           0xFFB9B4AC      // светъл камък
#define SCH_CEIL            0xFFF4F4F2
#define SCH_WALL            0xFFE9E5DE      // топло бяло
#define SCH_ACCENT          0xFFE0832F      // оранжево - цветът на сървъра
#define SCH_ANTHRACITE      0xFF2E3640
#define SCH_WALNUT          0xFF5A3D2B
#define SCH_CARPET          0xFF2E4B6E      // синьо - стая за теория
#define SCH_CARPET_OFFICE   0xFF6B4A3A
#define SCH_GLASS           0xFF3B4C5C
#define SCH_MAT             0xFF3A3A3A

// Точки (спрямо центъра на сградата)
#define SCHOOL_DOOR_X           0.0
#define SCHOOL_DOOR_Y           -6.9        // тук се появяваш при влизане / тук е изходът
#define SCHOOL_DESK_X           0.0
#define SCHOOL_DESK_Y           -3.9        // пред плота на рецепцията (тук се натиска ALT)
#define SCHOOL_NPC_Y            -1.2        // рецепционистът (зад плота)
#define SCHOOL_NPC_SKIN         141

// Врати: модел 1491 (вътрешна дървена врата ~1 м), пантата е в началото на модела.
// Вътрешните отвори са с двойни врати, които се отварят сами (виж school.inc).
#define SCHOOL_DOOR_MODEL       1491
#define SCHOOL_DOOR_TOP         2.2         // височина на вратата - над нея е преградата
stock const Float:SchoolDoors[4][3] = {
    // x на пантата, y, завъртане (затворена)
    {-4.2, 0.0, 0.0}, {-2.2, 0.0, 180.0},      // към стаята за теория
    { 7.4, 0.0, 0.0}, { 9.4, 0.0, 180.0}       // към кабинета
};

// Работни места в стаята за теория: 3 реда по 3 бюра (бюрото е на +0.75 м пред стола)
stock const Float:SchoolSeats[9][2] = {
    {-8.0, 4.05}, {-5.0, 4.05}, {-2.0, 4.05},
    {-8.0, 2.45}, {-5.0, 2.45}, {-2.0, 2.45},
    {-8.0, 0.85}, {-5.0, 0.85}, {-2.0, 0.85}
};

static stock School_Obj(model, Float:x, Float:y, Float:z, Float:rx, Float:ry, Float:rz)
{
    return CreateDynamicObject(model, SCHOOL_X + x, SCHOOL_Y + y, SCHOOL_Z + z, rx, ry, rz, SCHOOL_WORLD, -1, -1, 200.0, 200.0);
}

static stock School_Paint(obj, color)
{
    SetDynamicObjectMaterial(obj, 0, 18646, "matcolours", "white", color);
    return obj;
}

static stock School_Plane(Float:x, Float:y, Float:z, Float:rx, Float:ry, color)
{
    School_Paint(School_Obj(SCHOOL_PLANE, x, y, z, rx, ry, 0.0), color);
}

// Външна стена от две плоскости (видима отвътре и отвън)
static stock School_OuterWall(bool:alongX, Float:x, Float:y, color)
{
    if (alongX) {
        School_Plane(x, y, 0.0, 90.0, 0.0, color);
        School_Plane(x, y, 0.0, -90.0, 0.0, color);
    } else {
        School_Plane(x, y, 0.0, 0.0, 90.0, color);
        School_Plane(x, y, 0.0, 0.0, -90.0, color);
    }
}

// Вътрешна стена 9.6 м от пода до тавана; alongX = по оста X
static stock School_Partition(bool:alongX, Float:x, Float:y, color)
{
    School_Paint(School_Obj(SCHOOL_WALL_LONG, x, y, SCHOOL_HEIGHT / 2.0, 0.0, 0.0, alongX ? 90.0 : 0.0), color);
}

// Плоча на пода (килим, изтривалка): легнала стена, горната страна на 2 см над пода
static stock School_FloorPanel(model, Float:x, Float:y, bool:alongX, color)
{
    School_Paint(School_Obj(model, x, y, -0.13, 0.0, 90.0, alongX ? 90.0 : 0.0), color);
}

stock School_CreateMap()
{
    // ---------- Черупка ----------
    School_Plane(0.0, 0.0, 0.0, 0.0, 0.0, SCH_FLOOR);
    School_Plane(0.0, 0.0, SCHOOL_HEIGHT, 180.0, 0.0, SCH_CEIL);
    School_OuterWall(true,  0.0,  SCHOOL_HALF_L, SCH_WALL);
    School_OuterWall(true,  0.0, -SCHOOL_HALF_L, SCH_WALL);
    School_OuterWall(false, -SCHOOL_HALF_W, 0.0, SCH_WALL);
    School_OuterWall(false,  SCHOOL_HALF_W, 0.0, SCH_WALL);

    // ---------- Вътрешни стени ----------
    // По y = 0 с два отвора по 2 м: към теорията (x -4.2..-2.2) и към кабинета (x 7.4..9.4).
    // Излишното от парчетата влиза в страничните стени и не се вижда.
    School_Partition(true, -9.0, 0.0, SCH_WALL);
    School_Partition(true,  2.6, 0.0, SCH_ACCENT);         // зад рецепцията - акцентна стена
    School_Partition(true, 14.2, 0.0, SCH_WALL);           // отворът към кабинета е x 7.4..9.4
    // Между теорията и кабинета (x = 2), от y = 0 на север
    School_Partition(false, 2.0, 4.8, SCH_WALL);

    // ---------- Подове ----------
    School_FloorPanel(SCHOOL_WALL_LONG, -4.0, 2.25, true, SCH_CARPET);     // килим в стаята за теория
    School_FloorPanel(SCHOOL_WALL_LONG, -4.0, 5.75, true, SCH_CARPET);
    School_FloorPanel(SCHOOL_WALL_SMALL, 6.0, 4.0, false, SCH_CARPET_OFFICE); // килим в кабинета
    School_FloorPanel(SCHOOL_WALL_SMALL, 0.0, -6.6, true, SCH_MAT);         // изтривалка на входа

    // ---------- Входна врата (отвътре): двойна врата плътно до стената ----------
    School_Obj(SCHOOL_DOOR_MODEL, -1.0, -SCHOOL_HALF_L + 0.06, 0.0, 0.0, 0.0, 0.0);
    School_Obj(SCHOOL_DOOR_MODEL,  1.0, -SCHOOL_HALF_L + 0.06, 0.0, 0.0, 0.0, 180.0);

    // ---------- Прегради над вътрешните врати (от горния ръб на вратата до тавана) ----------
    School_Paint(School_Obj(SCHOOL_WALL_SMALL, -3.2, 0.0, SCHOOL_DOOR_TOP + 1.75, 0.0, 0.0, 90.0), SCH_WALL);
    School_Paint(School_Obj(SCHOOL_WALL_SMALL,  8.4, 0.0, SCHOOL_DOOR_TOP + 1.75, 0.0, 0.0, 90.0), SCH_WALL);

    // ---------- Рецепция ----------
    // Плот 1.1 м висок: 5 стени една зад друга, видими само горните им ръбове -> плътен плот.
    for (new i = 0; i < 5; i++) {
        new color = (i == 0) ? SCH_ANTHRACITE : SCH_WALNUT;
        for (new side = -1; side <= 1; side += 2) {
            School_Paint(School_Obj(SCHOOL_WALL_SMALL, side * 1.6, -3.0 + i * 0.2, 1.1 - 1.75, 0.0, 0.0, 90.0), color);
        }
    }
    School_Obj(2190, 1.3, -2.5, 1.1, 0.0, 0.0, 0.0);           // компютър на плота
    School_Obj(1714, -0.8, -1.6, 0.0, 0.0, 0.0, 180.0);        // стол зад плота
    School_Obj(2186, 3.8, -0.8, 0.0, 0.0, 0.0, 180.0);         // копирна машина
    School_Obj(2001, -3.9, -2.6, 0.0, 0.0, 0.0, 0.0);          // саксии отстрани на рецепцията
    School_Obj(2001,  3.9, -2.6, 0.0, 0.0, 0.0, 0.0);

    // ---------- Чакалня (запад) ----------
    School_Obj(1702, -9.3, -5.0, 0.0, 0.0, 0.0, 90.0);         // диван до стената, към изток
    School_Obj(1704, -6.2, -4.3, 0.0, 0.0, 0.0, 270.0);        // два фотьойла срещу него
    School_Obj(1704, -6.2, -5.9, 0.0, 0.0, 0.0, 270.0);
    School_Obj(1433, -7.7, -5.0, 0.0, 0.0, 0.0, 0.0);          // масичка
    School_Obj(948,  -9.4, -7.4, 0.0, 0.0, 0.0, 0.0);          // растения в ъглите
    School_Obj(948,  -9.4, -0.7, 0.0, 0.0, 0.0, 0.0);
    School_Obj(948,   9.4, -7.4, 0.0, 0.0, 0.0, 0.0);

    // ---------- Автомати (изток) ----------
    School_Obj(2002, 9.5, -2.0, 0.0, 0.0, 0.0, 270.0);         // диспенсър за вода
    School_Obj(1775, 9.4, -4.2, 1.1, 0.0, 0.0, 270.0);         // автомат за напитки
    School_Obj(1776, 9.4, -5.8, 1.1, 0.0, 0.0, 270.0);         // автомат за закуски
    School_Obj(1704, 6.6, -6.8, 0.0, 0.0, 0.0, 0.0);           // фотьойли до входа
    School_Obj(1704, 5.2, -6.8, 0.0, 0.0, 0.0, 0.0);

    // ---------- Стая за теория (северозапад) ----------
    School_Obj(19805, -4.5, SCHOOL_HALF_L - 0.12, 1.7, 0.0, 0.0, 0.0);    // бяла дъска
    School_Obj(19786, -8.4, SCHOOL_HALF_L - 0.12, 1.8, 0.0, 0.0, 0.0);    // голям екран
    School_Obj(2165, -4.5, 6.4, 0.0, 0.0, 0.0, 180.0);                    // бюро на инструктора
    School_Obj(1714, -4.5, 7.1, 0.0, 0.0, 0.0, 0.0);
    for (new i = 0; i < sizeof (SchoolSeats); i++) {
        School_Obj(2165, SchoolSeats[i][0], SchoolSeats[i][1] + 0.75, 0.0, 0.0, 0.0, 0.0);   // бюро с компютър
        School_Obj(1714, SchoolSeats[i][0], SchoolSeats[i][1], 0.0, 0.0, 0.0, 180.0);        // стол към дъската
    }
    School_Obj(948, -9.4, 7.4, 0.0, 0.0, 0.0, 0.0);
    School_Obj(948,  1.4, 7.4, 0.0, 0.0, 0.0, 0.0);

    // ---------- Кабинет на инструктора (североизток) ----------
    School_Obj(2165, 6.0, 5.6, 0.0, 0.0, 0.0, 180.0);
    School_Obj(1714, 6.0, 6.3, 0.0, 0.0, 0.0, 0.0);
    School_Obj(1704, 5.2, 4.1, 0.0, 0.0, 0.0, 180.0);         // столове за посетители
    School_Obj(1704, 6.8, 4.1, 0.0, 0.0, 0.0, 180.0);
    School_Obj(1742, 9.6, 6.2, 0.0, 0.0, 0.0, 270.0);         // библиотека
    School_Obj(1742, 9.6, 4.8, 0.0, 0.0, 0.0, 270.0);
    School_Obj(2007, 2.6, 7.4, 0.0, 0.0, 0.0, 0.0);           // шкаф с папки
    School_Obj(948,  9.4, 0.7, 0.0, 0.0, 0.0, 0.0);

    // ---------- Осветление ----------
    School_Obj(1893, -5.0, -4.0, SCHOOL_HEIGHT - 0.1, 0.0, 0.0, 0.0);
    School_Obj(1893,  0.0, -4.0, SCHOOL_HEIGHT - 0.1, 0.0, 0.0, 0.0);
    School_Obj(1893,  5.0, -4.0, SCHOOL_HEIGHT - 0.1, 0.0, 0.0, 0.0);
    School_Obj(1893, -7.0,  2.5, SCHOOL_HEIGHT - 0.1, 0.0, 0.0, 0.0);
    School_Obj(1893, -2.0,  2.5, SCHOOL_HEIGHT - 0.1, 0.0, 0.0, 0.0);
    School_Obj(1893, -7.0,  6.0, SCHOOL_HEIGHT - 0.1, 0.0, 0.0, 0.0);
    School_Obj(1893, -2.0,  6.0, SCHOOL_HEIGHT - 0.1, 0.0, 0.0, 0.0);
    School_Obj(1893,  6.0,  4.0, SCHOOL_HEIGHT - 0.1, 0.0, 0.0, 0.0);

    // ---------- Надписи ----------
    CreateDynamic3DTextLabel(C_BRAND "CARGO STATE\n" C_WHITE "АВТОШКОЛА", COLOR_WHITE, SCHOOL_X, SCHOOL_Y - 0.4, SCHOOL_Z + 2.7, 20.0, INVALID_PLAYER_ID, INVALID_VEHICLE_ID, 0, SCHOOL_WORLD, -1);
    CreateDynamic3DTextLabel(C_BRAND "РЕЦЕПЦИЯ\n" C_WHITE "ALT - избери курс", COLOR_WHITE, SCHOOL_X + SCHOOL_DESK_X, SCHOOL_Y + SCHOOL_DESK_Y + 0.6, SCHOOL_Z + 1.5, 8.0, INVALID_PLAYER_ID, INVALID_VEHICLE_ID, 0, SCHOOL_WORLD, -1);
    CreateDynamic3DTextLabel(C_WHITE "СТАЯ ЗА ТЕОРИЯ", COLOR_WHITE, SCHOOL_X - 3.2, SCHOOL_Y - 0.3, SCHOOL_Z + 2.6, 10.0, INVALID_PLAYER_ID, INVALID_VEHICLE_ID, 0, SCHOOL_WORLD, -1);
    CreateDynamic3DTextLabel(C_WHITE "КАБИНЕТ НА ИНСТРУКТОРА", COLOR_WHITE, SCHOOL_X + 8.2, SCHOOL_Y - 0.3, SCHOOL_Z + 2.6, 10.0, INVALID_PLAYER_ID, INVALID_VEHICLE_ID, 0, SCHOOL_WORLD, -1);
    CreateDynamic3DTextLabel(C_WHITE "ИЗХОД\n" C_BRAND "ALT", COLOR_WHITE, SCHOOL_X + SCHOOL_DOOR_X, SCHOOL_Y + SCHOOL_DOOR_Y - 0.5, SCHOOL_Z + 1.4, 8.0, INVALID_PLAYER_ID, INVALID_VEHICLE_ID, 0, SCHOOL_WORLD, -1);
}
