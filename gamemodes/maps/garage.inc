// Мапинг: интериор на личния гараж.
//
// Една хала високо над картата. Обектите са във всички виртуални светове (-1),
// а всеки играч влиза в свой свят – така всеки има "свой" гараж с един мапинг.
//
// Размер отвътре: 21 м (X) x 31.5 м (Y), височина 7 м. Вратата е на южната стена (-Y).
// Подът е на височина GARAGE_Z.

#define GARAGE_X        0.0
#define GARAGE_Y        0.0
#define GARAGE_Z        1200.0

#define GARAGE_HALF_W   10.5        // половин ширина (X)
#define GARAGE_HALF_L   15.75       // половин дължина (Y)

// Стена 19447: 10.5 м дълга, 3.5 м висока, по дължина на Y (без завъртане).
#define GARAGE_WALL     19447
#define GARAGE_FLOOR    19538       // бетонна плоскост 62.5 x 125 м

static stock Garage_Obj(model, Float:x, Float:y, Float:z, Float:rx, Float:ry, Float:rz)
{
    CreateDynamicObject(model, GARAGE_X + x, GARAGE_Y + y, GARAGE_Z + z, rx, ry, rz, -1, -1, -1, 200.0, 200.0);
}

stock Garage_CreateMap()
{
    // Под и таван (таванът е обърнат надолу, за да се вижда отвътре)
    Garage_Obj(GARAGE_FLOOR, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    Garage_Obj(GARAGE_FLOOR, 0.0, 0.0, 7.0, 180.0, 0.0, 0.0);

    // Стени – два реда по 3.5 м
    for (new row = 0; row < 2; row++) {
        new Float:z = 1.75 + row * 3.5;

        // Запад и изток (по Y): 3 парчета по 10.5 м
        for (new i = -1; i <= 1; i++) {
            Garage_Obj(GARAGE_WALL, -GARAGE_HALF_W, i * 10.5, z, 0.0, 0.0, 0.0);
            Garage_Obj(GARAGE_WALL,  GARAGE_HALF_W, i * 10.5, z, 0.0, 0.0, 0.0);
        }
        // Север и юг (по X): 2 парчета по 10.5 м
        Garage_Obj(GARAGE_WALL, -5.25,  GARAGE_HALF_L, z, 0.0, 0.0, 90.0);
        Garage_Obj(GARAGE_WALL,  5.25,  GARAGE_HALF_L, z, 0.0, 0.0, 90.0);
        Garage_Obj(GARAGE_WALL, -5.25, -GARAGE_HALF_L, z, 0.0, 0.0, 90.0);
        Garage_Obj(GARAGE_WALL,  5.25, -GARAGE_HALF_L, z, 0.0, 0.0, 90.0);
    }

    // Голяма метална врата на южната стена (отвътре)
    Garage_Obj(980, 0.0, -GARAGE_HALF_L + 0.4, 2.7, 0.0, 0.0, 0.0);

    // Работен кът – северозапад
    Garage_Obj(19815, -6.0, GARAGE_HALF_L - 0.2, 2.0, 0.0, 0.0, 0.0);      // табло с инструменти на стената
    Garage_Obj(19899, -9.6, GARAGE_HALF_L - 1.0, 0.0, 0.0, 0.0, 0.0);      // шкаф с инструменти
    Garage_Obj(19900, -8.4, GARAGE_HALF_L - 1.0, 0.0, 0.0, 0.0, 0.0);      // количка с инструменти
    Garage_Obj(19903, -3.5, GARAGE_HALF_L - 1.0, 0.0, 0.0, 0.0, 0.0);      // диагностичен компютър

    // Гуми и сандъци – североизток
    Garage_Obj(1327,  9.3, GARAGE_HALF_L - 1.2, 0.6, 0.0, 0.0, 0.0);
    Garage_Obj(1327,  8.0, GARAGE_HALF_L - 1.2, 0.6, 0.0, 0.0, 30.0);
    Garage_Obj(2912,  9.4, GARAGE_HALF_L - 3.2, 0.0, 0.0, 0.0, 0.0);
    Garage_Obj(2912,  9.4, GARAGE_HALF_L - 4.2, 0.0, 0.0, 0.0, 0.0);
    Garage_Obj(2912,  9.4, GARAGE_HALF_L - 3.7, 1.0, 0.0, 0.0, 15.0);

    // Рафтове и кашони – до вратата
    Garage_Obj(939,  -9.6, -GARAGE_HALF_L + 3.5, 0.0, 0.0, 0.0, 90.0);
    Garage_Obj(944,   9.0, -GARAGE_HALF_L + 2.5, 0.0, 0.0, 0.0, 0.0);
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
