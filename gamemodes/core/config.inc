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

// Болница All Saints – спаун след смърт (без такса), в отделението на 2-рия етаж.
// Интериорът е карта (maps/hospital.inc), построена вътре в истинската сграда.
#define HOSPITAL_X              1161.3
#define HOSPITAL_Y              -1346.0
#define HOSPITAL_Z              21.6
#define HOSPITAL_A              270.0
#define HOSPITAL_INTERIOR       0
#define HOSPITAL_WORLD          0

// Вход/изход на болницата (пикапи до главния вход)
#define HOSPITAL_DOOR_OUT_X     1172.0795   // отвън, пред стъклените врати
#define HOSPITAL_DOOR_OUT_Y     -1323.3483
#define HOSPITAL_DOOR_OUT_Z     15.4014
#define HOSPITAL_DOOR_IN_X      1171.0427   // отвътре, до стъклените врати
#define HOSPITAL_DOOR_IN_Y      -1322.4243
#define HOSPITAL_DOOR_IN_Z      15.4723
#define HOSPITAL_DOOR_STEP      2.5         // с колко метра навътре/навън пренася (да не стъпиш на другия пикап)

// --- PayDay и нива ---
#define PAYDAY_MINUTES          60      // активни минути до PayDay
#define PAYDAY_EXP              1       // опит за един PayDay
#define EXP_PER_LEVEL           2       // нужен опит = ниво * това

// --- AFK ---
#define AFK_PAUSE_MS            5000    // клиентът не праща данни толкова време -> играта е на пауза
#define AFK_IDLE_MINUTES        5       // без движение толкова минути -> AFK
#define AFK_KICK_MINUTES        0       // кик след толкова минути AFK (0 = изключено, админите не се кикват)

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
