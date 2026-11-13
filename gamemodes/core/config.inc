// ============================================================
//  HOTFIX ONLINE – основни настройки
// ============================================================

#define SERVER_NAME             "HOTFIX ONLINE"
#define SERVER_DISCORD          "скоро"

// Файлът се създава в Server/scriptfiles/
#define DATABASE_FILE           "hotfix.db"

// --- Акаунти ---
#define MAX_LOGIN_ATTEMPTS      3
#define MIN_PASSWORD_LENGTH     6
#define MAX_PASSWORD_LENGTH     32
#define MIN_PLAYER_AGE          10
#define MAX_PLAYER_AGE          99

// --- Нов играч ---
#define START_MONEY             5000

// Начален спаун – гара Unity, Los Santos
#define SPAWN_X                 1742.9
#define SPAWN_Y                 -1861.0
#define SPAWN_Z                 13.58
#define SPAWN_A                 0.0

// Избор на скин – къде стои героят и откъде го гледа камерата
#define SKIN_SELECT_X           1742.9
#define SKIN_SELECT_Y           -1861.0
#define SKIN_SELECT_Z           13.58
#define SKIN_SELECT_A           0.0
#define SKIN_CAMERA_DISTANCE    3.2
#define SKIN_CAMERA_HEIGHT      0.5

// Болница All Saints – спаун след смърт
#define HOSPITAL_X              1177.5
#define HOSPITAL_Y              -1323.5
#define HOSPITAL_Z              14.08
#define HOSPITAL_A              270.0
#define HOSPITAL_FEE            200

// --- Нива и заплата ---
#define PAYDAY_MINUTES          60      // активни минути до заплата
#define PAYDAY_BASE             500     // базова заплата
#define PAYDAY_PER_LEVEL        100     // + за всяко ниво
#define EXP_PER_LEVEL           2       // нужен опит = ниво * това (1 заплата = 1 опит)
#define AFK_IDLE_MINUTES        10      // след толкова минути без движение не се брои време
#define AFK_PAUSE_MS            15000   // клиентът не праща данни -> играта е на пауза

// --- Чат (разстояния) ---
#define CHAT_RADIUS_NORMAL      20.0
#define CHAT_RADIUS_SHOUT       40.0
#define CHAT_RADIUS_LOW         5.0
#define CHAT_RADIUS_ME          25.0

// --- Свят ---
#define WORLD_WEATHER           1
#define WORLD_HOUR              12

// --- Таймери (секунди) ---
#define AUTOSAVE_SECONDS        300
