// Обща рамка за работите: видове, скилове и точки на картата.
//
// Начални работи (JOB_TYPE_STARTER): без ниво, започват се на място, 3 нива на скила.
// Стандартни работи (JOB_TYPE_STANDARD): изискват ниво и назначаване в кметството, 5 нива на скила.
//
// Скилът се пази завинаги за всяка работа поотделно (таблица job_skills).
// Заплащането зависи от скила в работата, а не от нивото на героя.

#define JOB_TYPE_STARTER    0
#define JOB_TYPE_STANDARD   1

// ID-та на работите. Добавяй нови САМО в края – ID-то се пази в базата.
enum {
    JOB_NONE,
    JOB_FARM,
    MAX_JOBS
}

enum E_JOB_INFO {
    jiName[32],
    jiType,
    jiMinLevel      // минимално ниво на героя (за стандартните)
}

static const JobInfo[MAX_JOBS][E_JOB_INFO] = {
    {"Няма",    JOB_TYPE_STARTER, 0},
    {"Фермер",  JOB_TYPE_STARTER, 0}
};

// Нужен EXP (общо) за всяко ниво на скила
static const StarterSkillExp[]  = {0, 600, 2000};
static const StandardSkillExp[] = {0, 800, 2500, 6000, 12000};

// Множител на заплащането според скила (в проценти)
static const StarterSkillPay[]  = {100, 130, 160};
static const StandardSkillPay[] = {100, 125, 155, 190, 230};

// EXP във всяка работа за играча (зарежда се при вход)
new PlayerJobExp[MAX_PLAYERS][MAX_JOBS];

stock Job_GetName(job, dest[], size = sizeof (dest))
{
    if (job < 0 || job >= MAX_JOBS) job = JOB_NONE;
    format(dest, size, "%s", JobInfo[job][jiName]);
}

stock Job_GetType(job)
{
    return JobInfo[job][jiType];
}

stock Job_GetMaxSkill(job)
{
    return (JobInfo[job][jiType] == JOB_TYPE_STARTER) ? sizeof (StarterSkillExp) : sizeof (StandardSkillExp);
}

// Нужен EXP за ниво на скила (1..max)
stock Job_GetSkillExp(job, skill)
{
    if (skill < 1) skill = 1;
    if (JobInfo[job][jiType] == JOB_TYPE_STARTER) {
        if (skill > sizeof (StarterSkillExp)) skill = sizeof (StarterSkillExp);
        return StarterSkillExp[skill - 1];
    }
    if (skill > sizeof (StandardSkillExp)) skill = sizeof (StandardSkillExp);
    return StandardSkillExp[skill - 1];
}

stock Job_GetSkill(playerid, job)
{
    new exp = PlayerJobExp[playerid][job], max = Job_GetMaxSkill(job), skill = 1;
    for (new s = 2; s <= max; s++) {
        if (exp >= Job_GetSkillExp(job, s)) skill = s;
    }
    return skill;
}

// Множител на заплащането в проценти (100 = x1.0)
stock Job_GetPayPercent(playerid, job)
{
    new skill = Job_GetSkill(playerid, job);
    if (JobInfo[job][jiType] == JOB_TYPE_STARTER) return StarterSkillPay[skill - 1];
    return StandardSkillPay[skill - 1];
}

stock Job_ResetPlayer(playerid)
{
    for (new j = 0; j < MAX_JOBS; j++) PlayerJobExp[playerid][j] = 0;
}

stock Job_LoadSkills(playerid)
{
    Job_ResetPlayer(playerid);
    new q[128];
    format(q, sizeof (q), "SELECT job, exp FROM job_skills WHERE account_id = %d", PlayerData[playerid][pID]);
    new DBResult:result = Database_Query(q);
    if (result == DBResult:0) return;
    if (DB_GetRowCount(result) > 0) {
        new job;
        do {
            job = DB_GetFieldIntByName(result, "job");
            if (0 < job < MAX_JOBS) PlayerJobExp[playerid][job] = DB_GetFieldIntByName(result, "exp");
        } while (DB_SelectNextRow(result));
    }
    DB_FreeResultSet(result);
}

// Добавя EXP в работата и съобщава при ново ниво на скила.
stock Job_AddExp(playerid, job, amount)
{
    if (amount <= 0 || job <= JOB_NONE || job >= MAX_JOBS) return;

    new before = Job_GetSkill(playerid, job);
    PlayerJobExp[playerid][job] += amount;
    new after = Job_GetSkill(playerid, job);

    new q[192];
    format(q, sizeof (q), "INSERT OR REPLACE INTO job_skills (account_id, job, exp) VALUES (%d, %d, %d)",
        PlayerData[playerid][pID], job, PlayerJobExp[playerid][job]);
    Database_Exec(q);

    new name[32];
    Job_GetName(job, name);
    if (after > before) {
        SendClientMessage(playerid, COLOR_GOLD, "[Скил] Поздравления! Скилът ти като %s е вече %d/%d. Заплащането се увеличава.",
            name, after, Job_GetMaxSkill(job));
        GameTextForPlayer(playerid, "~y~SKILL UP!", 4000, 1);
    } else if (after < Job_GetMaxSkill(job)) {
        SendClientMessage(playerid, COLOR_GREY, "+%d EXP (%s): %d / %d до скил %d.",
            amount, name, PlayerJobExp[playerid][job], Job_GetSkillExp(job, after + 1), after + 1);
    }
}

// Ред за статистиката: "Фермер: скил 2/3 (750/2000 EXP)"
stock Job_AppendSkillsText(playerid, dest[], size)
{
    new line[128], name[32], count = 0;
    for (new j = 1; j < MAX_JOBS; j++) {
        if (PlayerJobExp[playerid][j] <= 0) continue;
        new skill = Job_GetSkill(playerid, j), max = Job_GetMaxSkill(j);
        Job_GetName(j, name);
        if (skill < max) {
            format(line, sizeof (line), C_GREY "%s: " C_GOLD "скил %d/%d" C_GREY " (%d / %d EXP)\n",
                name, skill, max, PlayerJobExp[playerid][j], Job_GetSkillExp(j, skill + 1));
        } else {
            format(line, sizeof (line), C_GREY "%s: " C_GOLD "скил %d/%d" C_GREEN " (максимален)\n", name, skill, max);
        }
        strcat(dest, line, size);
        count++;
    }
    if (count == 0) strcat(dest, C_GREY "Още нямаш опит в нито една работа.\n", size);
}

// ------------------------------------------------------------
//  Точки на работите (поставят се от админите в играта)
// ------------------------------------------------------------

stock Job_CreateTables()
{
    Database_Exec("CREATE TABLE IF NOT EXISTS job_skills (account_id INTEGER NOT NULL, job INTEGER NOT NULL, exp INTEGER NOT NULL DEFAULT 0, PRIMARY KEY (account_id, job))");
    Database_Exec("CREATE TABLE IF NOT EXISTS job_points (job INTEGER NOT NULL, name TEXT NOT NULL, x REAL NOT NULL, y REAL NOT NULL, z REAL NOT NULL, a REAL NOT NULL DEFAULT 0, PRIMARY KEY (job, name))");
}

// Записва точка (вход, изкупвач, кладенец...) на мястото на играча.
stock Job_SetPoint(job, const name[], Float:x, Float:y, Float:z, Float:a)
{
    new q[256];
    format(q, sizeof (q), "INSERT OR REPLACE INTO job_points (job, name, x, y, z, a) VALUES (%d, '%s', %f, %f, %f, %f)",
        job, name, x, y, z, a);
    Database_Exec(q);
}

// Връща true и координатите, ако точката е поставена.
stock bool:Job_GetPoint(job, const name[], &Float:x, &Float:y, &Float:z, &Float:a)
{
    new q[160], bool:found = false;
    format(q, sizeof (q), "SELECT x, y, z, a FROM job_points WHERE job = %d AND name = '%s' LIMIT 1", job, name);
    new DBResult:result = Database_Query(q);
    if (result != DBResult:0) {
        if (DB_GetRowCount(result) > 0) {
            x = DB_GetFieldFloatByName(result, "x");
            y = DB_GetFieldFloatByName(result, "y");
            z = DB_GetFieldFloatByName(result, "z");
            a = DB_GetFieldFloatByName(result, "a");
            found = true;
        }
        DB_FreeResultSet(result);
    }
    return found;
}
