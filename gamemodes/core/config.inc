// Основни настройки на сървъра

#define SERVER_NAME         "CARGO STATE"
#define DATABASE_FILE       "cargo.db"

// Начален капитал на нов играч
#define START_MONEY         3000

// Място за спаун (Los Santos, пред Unity Station).
// В етап 2 спаунът ще е в депото, където играчът е бил последно.
#define SPAWN_X             1743.0
#define SPAWN_Y             -1862.0
#define SPAWN_Z             13.58
#define SPAWN_A             0.0

// Камера при вход/регистрация – изглед над Los Santos
#define AUTH_CAM_X          1500.0
#define AUTH_CAM_Y          -1700.0
#define AUTH_CAM_Z          120.0
#define AUTH_LOOK_X         1750.0
#define AUTH_LOOK_Y         -1850.0
#define AUTH_LOOK_Z         30.0

#define MAX_LOGIN_ATTEMPTS  3
#define PASSWORD_MIN        6
#define PASSWORD_MAX        32

// Автоматичен запис на всички играчи (секунди)
#define AUTOSAVE_SECONDS    300

// CB радио
#define CB_DEFAULT_CHANNEL  19
#define CB_MAX_CHANNEL      40

// Нива: нужен опит (общо) за ниво 1..5
#define MAX_LEVEL           5
stock const LevelExp[MAX_LEVEL] = {0, 150, 500, 1200, 2500};
