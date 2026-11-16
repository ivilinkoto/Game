// Мапинг: интериор на личния гараж.
//
// Една хала високо над картата. Обектите са във всички виртуални светове (-1),
// а всеки играч влиза в свой свят – така всеки има "свой" гараж с един мапинг.
//
// Размер отвътре: 21 м (X) x 31.5 м (Y), височина 7 м. Вратата е на южната стена (-Y).
// Подът е на височина GARAGE_Z.
//
// Стените, подът и таванът са големи бетонни плоскости (19538, 62.5 x 125 м), а не парчета:
// така няма процепи. Плоскостта се вижда само от едната страна, затова всяка стена е от две
// плоскости, обърнати една срещу друга. Излишното стърчи извън халата и не се вижда отвътре.
// Цветът се сменя с оцветяване на текстурата (SetDynamicObjectMaterial с модел -1).

#define GARAGE_X        0.0
#define GARAGE_Y        0.0
#define GARAGE_Z        1200.0

#define GARAGE_HALF_W   10.5        // половин ширина (X)
#define GARAGE_HALF_L   15.75       // половин дължина (Y)
#define GARAGE_HEIGHT   7.0

#define GARAGE_PLANE    19538       // бетонна плоскост 62.5 (X) x 125 (Y) м, лицето нагоре

// Цветове (ARGB) – умножават се по текстурата на бетона
#define GARAGE_TINT_WALL    0xFF6F7C88      // тъмно сиво-синьо
#define GARAGE_TINT_BAND    0xFFF5A623      // оранжева ивица (цветът на сървъра)
#define GARAGE_TINT_FLOOR   0xFFB8B8B0      // светъл бетон
#define GARAGE_TINT_CEIL    0xFF4A525A      // тъмен таван

static stock Garage_Obj(model, Float:x, Float:y, Float:z, Float:rx, Float:ry, Float:rz)
{
    return CreateDynamicObject(model, GARAGE_X + x, GARAGE_Y + y, GARAGE_Z + z, rx, ry, rz, -1, -1, -1, 250.0, 250.0);
}

static stock Garage_Plane(Float:x, Float:y, Float:z, Float:rx, Float:ry, color)
{
    new obj = Garage_Obj(GARAGE_PLANE, x, y, z, rx, ry, 0.0);
    SetDynamicObjectMaterial(obj, 0, -1, "none", "none", color);
}

// Вертикална плоскост, видима и от двете страни.
// alongX = true: стената е по оста X (северна/южна), иначе по Y (западна/източна).
// top = височина на горния ръб над пода (за ивици); 0 = цяла стена.
static stock Garage_Wall(bool:alongX, Float:x, Float:y, Float:top, color)
{
    if (alongX) {
        // Завъртане около X: плоскостта е 62.5 по X и 125 по Z
        new Float:z = (top > 0.0) ? (top - 62.5) : 0.0;
        Garage_Plane(x, y, z, 90.0, 0.0, color);
        Garage_Plane(x, y, z, -90.0, 0.0, color);
    } else {
        // Завъртане около Y: плоскостта е 62.5 по Z и 125 по Y
        new Float:z = (top > 0.0) ? (top - 31.25) : 0.0;
        Garage_Plane(x, y, z, 0.0, 90.0, color);
        Garage_Plane(x, y, z, 0.0, -90.0, color);
    }
}

stock Garage_CreateMap()
{
    // Под и таван (таванът е обърнат надолу, за да се вижда отвътре)
    Garage_Plane(0.0, 0.0, 0.0, 0.0, 0.0, GARAGE_TINT_FLOOR);
    Garage_Plane(0.0, 0.0, GARAGE_HEIGHT, 180.0, 0.0, GARAGE_TINT_CEIL);

    // Стени
    Garage_Wall(true,  0.0,  GARAGE_HALF_L, 0.0, GARAGE_TINT_WALL);     // север
    Garage_Wall(true,  0.0, -GARAGE_HALF_L, 0.0, GARAGE_TINT_WALL);     // юг (с вратата)
    Garage_Wall(false, -GARAGE_HALF_W, 0.0, 0.0, GARAGE_TINT_WALL);     // запад
    Garage_Wall(false,  GARAGE_HALF_W, 0.0, 0.0, GARAGE_TINT_WALL);     // изток

    // Оранжева ивица долу по стените (1.2 м), малко пред тях
    Garage_Wall(true,  0.0,  GARAGE_HALF_L - 0.05, 1.2, GARAGE_TINT_BAND);
    Garage_Wall(true,  0.0, -GARAGE_HALF_L + 0.05, 1.2, GARAGE_TINT_BAND);
    Garage_Wall(false, -GARAGE_HALF_W + 0.05, 0.0, 1.2, GARAGE_TINT_BAND);
    Garage_Wall(false,  GARAGE_HALF_W - 0.05, 0.0, 1.2, GARAGE_TINT_BAND);

    // Голяма метална врата на южната стена (отвътре)
    Garage_Obj(980, 0.0, -GARAGE_HALF_L + 0.4, 2.7, 0.0, 0.0, 0.0);

    // Лампи на тавана – два реда
    for (new i = -1; i <= 1; i++) {
        Garage_Obj(1893, -5.0, i * 9.0, GARAGE_HEIGHT - 0.1, 0.0, 0.0, 0.0);
        Garage_Obj(1893,  5.0, i * 9.0, GARAGE_HEIGHT - 0.1, 0.0, 0.0, 0.0);
    }

    // Работен кът – северозапад
    Garage_Obj(19815, -6.0, GARAGE_HALF_L - 0.2, 2.0, 0.0, 0.0, 0.0);      // табло с инструменти на стената
    Garage_Obj(19899, -9.6, GARAGE_HALF_L - 1.0, 0.0, 0.0, 0.0, 0.0);      // шкаф с инструменти
    Garage_Obj(19900, -8.4, GARAGE_HALF_L - 1.0, 0.0, 0.0, 0.0, 0.0);      // количка с инструменти
    Garage_Obj(19903, -3.5, GARAGE_HALF_L - 1.0, 0.0, 0.0, 0.0, 0.0);      // диагностичен компютър

    // Гуми и сандъци – североизток
    Garage_Obj(1327,  9.3, GARAGE_HALF_L - 1.2, 0.6, 0.0, 0.0, 0.0);
    Garage_Obj(1327,  8.0, GARAGE_HALF_L - 1.2, 0.6, 0.0, 0.0, 30.0);
    Garage_Obj(1327,  9.4, GARAGE_HALF_L - 2.6, 0.6, 0.0, 0.0, 75.0);
    Garage_Obj(2912,  9.4, GARAGE_HALF_L - 4.2, 0.0, 0.0, 0.0, 0.0);
    Garage_Obj(2912,  9.4, GARAGE_HALF_L - 5.2, 0.0, 0.0, 0.0, 0.0);
    Garage_Obj(2912,  9.4, GARAGE_HALF_L - 4.7, 1.0, 0.0, 0.0, 15.0);

    // Рафтове и кашони – до вратата
    Garage_Obj(939,  -9.6, -GARAGE_HALF_L + 3.5, 0.0, 0.0, 0.0, 90.0);
    Garage_Obj(944,   9.0, -GARAGE_HALF_L + 2.5, 0.0, 0.0, 0.0, 0.0);

    // Надписи
    CreateDynamic3DTextLabel(C_BRAND "МЯСТО 1", COLOR_WHITE, GARAGE_X - 5.0, GARAGE_Y + 10.5, GARAGE_Z + 0.5, 15.0, INVALID_PLAYER_ID, INVALID_VEHICLE_ID, 0, -1, -1);
    CreateDynamic3DTextLabel(C_BRAND "МЯСТО 2", COLOR_WHITE, GARAGE_X + 5.0, GARAGE_Y + 10.5, GARAGE_Z + 0.5, 15.0, INVALID_PLAYER_ID, INVALID_VEHICLE_ID, 0, -1, -1);
    CreateDynamic3DTextLabel(C_WHITE "ИЗХОД\n" C_BRAND "ALT" C_WHITE " пеша / " C_BRAND "H" C_WHITE " с камион", COLOR_WHITE, GARAGE_X, GARAGE_Y - 13.0, GARAGE_Z + 1.5, 10.0, INVALID_PLAYER_ID, INVALID_VEHICLE_ID, 0, -1, -1);
}

// Точки в гаража (спрямо центъра)
#define GARAGE_FOOT_X       -6.0        // къде се появява пеша
#define GARAGE_FOOT_Y       -12.5
#define GARAGE_DOOR_Y       -13.0       // зона на вратата отвътре (Y), X = 0
#define GARAGE_DOOR_RANGE_FOOT  3.5
#define GARAGE_DOOR_RANGE_VEH   7.0

// Места за паркиране – с лице към вратата
stock const Float:GarageSlotPos[][3] = {
    {-5.0, 4.0, 180.0},
    { 5.0, 4.0, 180.0}
};
