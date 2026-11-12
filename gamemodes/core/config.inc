// ============================================================
//  Основни настройки на сървъра – промени ги тук.
// ============================================================

#define SERVER_NAME             "BG RolePlay"
#define SERVER_SHORT            "BG-RP"
#define SERVER_VERSION          "0.1.0"
#define SERVER_WEBSITE          "www.example.com"

// Файлът се създава в папка scriptfiles/
#define DATABASE_FILE           "rp.db"

// --- Акаунти ---
#define MAX_LOGIN_ATTEMPTS      3
#define MIN_PASSWORD_LENGTH     6
#define MAX_PASSWORD_LENGTH     32

// --- Начални стойности ---
#define START_MONEY             5000
#define DEFAULT_SKIN_MALE       26
#define DEFAULT_SKIN_FEMALE     12

// Начален спаун (гара Unity, Los Santos)
#define SPAWN_X                 1742.9
#define SPAWN_Y                 -1861.0
#define SPAWN_Z                 13.58
#define SPAWN_A                 0.0

// Болница (All Saints, Los Santos) – след смърт
#define HOSPITAL_X              1177.5
#define HOSPITAL_Y              -1323.5
#define HOSPITAL_Z              14.08
#define HOSPITAL_A              270.0
#define HOSPITAL_FEE            200

// --- Икономика ---
#define PAYDAY_MINUTES          60      // изиграни минути до заплата
#define PAYDAY_BASE             500     // базова заплата
#define PAYDAY_PER_LEVEL        50      // + за всяко ниво
#define EXP_PER_LEVEL           4       // нужни часове = ниво * това

// --- Чат разстояния ---
#define CHAT_RADIUS_NORMAL      20.0
#define CHAT_RADIUS_SHOUT       40.0
#define CHAT_RADIUS_LOW         5.0
#define CHAT_RADIUS_ME          25.0

// --- Таймери (в секунди) ---
#define AUTOSAVE_SECONDS        300
#define DONATION_CHECK_SECONDS  60
