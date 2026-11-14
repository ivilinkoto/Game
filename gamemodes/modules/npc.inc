// NPC система (актьори).
//
// NPC-тата стоят на определени места, имат име, роля и анимация.
// Отиваш до NPC и натискаш ALT (или Y) – отваря се менюто на ролята му.
//
// Админ (ниво 3+):
//   /npcadd   – поставя NPC там, където стоиш (име -> роля -> скин -> анимация)
//   /npcdel   – премахва най-близкото NPC (до 3 м)
//   /npclist  – списък с NPC-тата
//
// Нова роля:
//   1. добави константа NPC_ROLE_... и име в NpcRoleName
//   2. добави case в Npc_OnInteract, който извиква менюто на системата ти

#define MAX_NPCS            100
#define NPC_INTERACT_RANGE  2.2
#define NPC_NAME_LEN        32

// Роли – добавяй само в края (пазят се в базата)
enum {
    NPC_ROLE_DECOR,     // само за украса / атмосфера
    NPC_ROLE_FARM,      // фермер – работата във фермата
    MAX_NPC_ROLES
}

static const NpcRoleName[MAX_NPC_ROLES][] = {
    "Украса (без меню)",
    "Фермер (работа)"
};

// Анимации: библиотека, име
static const NpcAnimName[][] = {
    "Стои",
    "Пуши",
    "Говори",
    "Скръстени ръце",
    "Облегнат",
    "Работи (копае)",
    "Пие",
    "Чете / пише"
};
static const NpcAnimLib[][] = {
    "",
    "SMOKING",
    "PED",
    "COP_AMBIENT",
    "GANGS",
    "BOMBER",
    "BAR",
    "INT_OFFICE"
};
static const NpcAnimFunc[][] = {
    "",
    "M_smklean_loop",
    "IDLE_CHAT",
    "Coplook_loop",
    "leanIDLE",
    "BOM_Plant_Loop",
    "dnk_stndM_loop",
    "OFF_Sit_Type_Loop"
};

enum E_NPC {
    bool:npcExists,
    npcDBID,
    npcActor,
    npcName[NPC_NAME_LEN],
    npcRole,
    npcSkin,
    Float:npcX,
    Float:npcY,
    Float:npcZ,
    Float:npcA,
    npcWorld,
    npcAnim,
    Text3D:npcLabel
}
static NpcData[MAX_NPCS][E_NPC];

// Временни данни за /npcadd
static NpcEditName[MAX_PLAYERS][NPC_NAME_LEN],
    NpcEditRole[MAX_PLAYERS],
    NpcEditSkin[MAX_PLAYERS],
    NpcInteractTick[MAX_PLAYERS];

// ============================================================
//  Зареждане
// ============================================================

stock Npc_Init()
{
    Database_Exec("CREATE TABLE IF NOT EXISTS npcs (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL, role INTEGER NOT NULL DEFAULT 0, skin INTEGER NOT NULL, x REAL NOT NULL, y REAL NOT NULL, z REAL NOT NULL, a REAL NOT NULL, world INTEGER NOT NULL DEFAULT 0, anim INTEGER NOT NULL DEFAULT 0)");

    for (new i = 0; i < MAX_NPCS; i++) NpcData[i][npcExists] = false;

    new DBResult:result = Database_Query("SELECT * FROM npcs ORDER BY id");
    new count = 0;
    if (result != DBResult:0) {
        if (DB_GetRowCount(result) > 0) {
            new name[NPC_NAME_LEN];
            do {
                DB_GetFieldStringByName(result, "name", name);
                Npc_Spawn(DB_GetFieldIntByName(result, "id"), name,
                    DB_GetFieldIntByName(result, "role"), DB_GetFieldIntByName(result, "skin"),
                    DB_GetFieldFloatByName(result, "x"), DB_GetFieldFloatByName(result, "y"),
                    DB_GetFieldFloatByName(result, "z"), DB_GetFieldFloatByName(result, "a"),
                    DB_GetFieldIntByName(result, "world"), DB_GetFieldIntByName(result, "anim"));
                count++;
            } while (DB_SelectNextRow(result));
        }
        DB_FreeResultSet(result);
    }

    if (count == 0) Npc_SeedDefaults();
    printf("[NPC] Loaded %d NPC(s).", count);
}

// Първо пускане: фермерът стои там, където е входът на фермата.
stock Npc_SeedDefaults()
{
    new Float:x, Float:y, Float:z, Float:a;
    if (Job_GetPoint(JOB_FARM, "job", x, y, z, a)) {
        Npc_Create("Иван - Фермер", NPC_ROLE_FARM, 158, x, y, z, 180.0, 0, 3);
    }
}

// Записва ново NPC в базата и го създава.
stock Npc_Create(const name[], role, skin, Float:x, Float:y, Float:z, Float:a, world, anim)
{
    new eName[80], q[384];
    SQL_Escape(name, eName);
    format(q, sizeof (q), "INSERT INTO npcs (name, role, skin, x, y, z, a, world, anim) VALUES ('%s', %d, %d, %f, %f, %f, %f, %d, %d)",
        eName, role, skin, x, y, z, a, world, anim);
    Database_Exec(q);

    new dbid = 0;
    new DBResult:result = Database_Query("SELECT MAX(id) AS id FROM npcs");
    if (result != DBResult:0) {
        dbid = DB_GetFieldIntByName(result, "id");
        DB_FreeResultSet(result);
    }
    return Npc_Spawn(dbid, name, role, skin, x, y, z, a, world, anim);
}

// Създава NPC в играта (без да пише в базата). Връща индекса или -1.
stock Npc_Spawn(dbid, const name[], role, skin, Float:x, Float:y, Float:z, Float:a, world, anim)
{
    new i = -1;
    for (new n = 0; n < MAX_NPCS; n++) {
        if (!NpcData[n][npcExists]) {
            i = n;
            break;
        }
    }
    if (i == -1) return -1;

    if (role < 0 || role >= MAX_NPC_ROLES) role = NPC_ROLE_DECOR;
    if (anim < 0 || anim >= sizeof (NpcAnimName)) anim = 0;

    NpcData[i][npcExists] = true;
    NpcData[i][npcDBID] = dbid;
    format(NpcData[i][npcName], NPC_NAME_LEN, "%s", name);
    NpcData[i][npcRole] = role;
    NpcData[i][npcSkin] = skin;
    NpcData[i][npcX] = x;
    NpcData[i][npcY] = y;
    NpcData[i][npcZ] = z;
    NpcData[i][npcA] = a;
    NpcData[i][npcWorld] = world;
    NpcData[i][npcAnim] = anim;

    new actor = CreateActor(skin, x, y, z, a);
    NpcData[i][npcActor] = actor;
    if (actor != INVALID_ACTOR_ID) {
        SetActorInvulnerable(actor, true);
        SetActorVirtualWorld(actor, world);
        Npc_ApplyAnim(i);
    }

    new text[128];
    if (role == NPC_ROLE_DECOR) {
        format(text, sizeof (text), "{FF6A00}%s", name);
    } else {
        format(text, sizeof (text), "{FF6A00}%s\n{A0A0A0}Натисни ALT", name);
    }
    NpcData[i][npcLabel] = Create3DTextLabel("%s", COLOR_WHITE, x, y, z + 1.15, 12.0, world, true, text);
    return i;
}

stock Npc_ApplyAnim(i)
{
    new anim = NpcData[i][npcAnim], actor = NpcData[i][npcActor];
    if (actor == INVALID_ACTOR_ID) return;
    if (anim == 0) {
        ClearActorAnimations(actor);
        return;
    }
    ApplyActorAnimation(actor, NpcAnimLib[anim], NpcAnimFunc[anim], 4.1, true, false, false, false, 0);
}

stock Npc_Destroy(i)
{
    if (!NpcData[i][npcExists]) return;
    if (NpcData[i][npcActor] != INVALID_ACTOR_ID) DestroyActor(NpcData[i][npcActor]);
    Delete3DTextLabel(NpcData[i][npcLabel]);
    NpcData[i][npcExists] = false;
}

// Мести първото NPC с дадена роля (напр. когато фермата се премести).
stock Npc_MoveRole(role, Float:x, Float:y, Float:z, Float:a)
{
    for (new i = 0; i < MAX_NPCS; i++) {
        if (!NpcData[i][npcExists] || NpcData[i][npcRole] != role) continue;

        new q[256], name[NPC_NAME_LEN], dbid = NpcData[i][npcDBID], skin = NpcData[i][npcSkin];
        new world = NpcData[i][npcWorld], anim = NpcData[i][npcAnim];
        format(name, sizeof (name), "%s", NpcData[i][npcName]);
        format(q, sizeof (q), "UPDATE npcs SET x = %f, y = %f, z = %f, a = %f WHERE id = %d", x, y, z, a, dbid);
        Database_Exec(q);

        Npc_Destroy(i);
        Npc_Spawn(dbid, name, role, skin, x, y, z, a, world, anim);
        return 1;
    }
    return 0;
}

// Някои анимации се „изключват“, когато играч влезе в обхват – подновяваме ги.
stock Npc_OnTick(tick)
{
    if (tick % 30 != 0) return;
    for (new i = 0; i < MAX_NPCS; i++) {
        if (NpcData[i][npcExists] && NpcData[i][npcAnim] != 0) Npc_ApplyAnim(i);
    }
}

// ============================================================
//  Взаимодействие
// ============================================================

// Най-близкото NPC в обхват (-1 ако няма)
stock Npc_GetNearest(playerid, Float:range)
{
    new best = -1, Float:bestDist = range, world = GetPlayerVirtualWorld(playerid);
    for (new i = 0; i < MAX_NPCS; i++) {
        if (!NpcData[i][npcExists] || NpcData[i][npcWorld] != world) continue;
        new Float:d = GetPlayerDistanceFromPoint(playerid, NpcData[i][npcX], NpcData[i][npcY], NpcData[i][npcZ]);
        if (d <= bestDist) {
            bestDist = d;
            best = i;
        }
    }
    return best;
}

// Връща true, ако бутонът е използван за NPC.
stock bool:Npc_OnKeyStateChange(playerid, KEY:newkeys, KEY:oldkeys)
{
    if (!PlayerData[playerid][pLogged] || IsPlayerInAnyVehicle(playerid)) return false;

    new bool:alt = ((newkeys & KEY_WALK) && !(oldkeys & KEY_WALK)),
        bool:yes = ((newkeys & KEY_YES) && !(oldkeys & KEY_YES));
    if (!alt && !yes) return false;

    new npc = Npc_GetNearest(playerid, NPC_INTERACT_RANGE);
    if (npc == -1 || NpcData[npc][npcRole] == NPC_ROLE_DECOR) return false;

    new now = GetTickCount();
    if (now - NpcInteractTick[playerid] < 1000) return true;
    NpcInteractTick[playerid] = now;

    Npc_OnInteract(playerid, npc);
    return true;
}

stock Npc_OnInteract(playerid, npc)
{
    switch (NpcData[npc][npcRole]) {
        case NPC_ROLE_FARM: Farm_OnNpcInteract(playerid);
    }
}

// ============================================================
//  Админ редактор
// ============================================================

CMD:npcadd(playerid, params[])
{
    #pragma unused params
    if (!Admin_Check(playerid, ADMIN_ADMIN)) return 1;
    ShowPlayerDialog(playerid, DIALOG_NPC_NAME, DIALOG_STYLE_INPUT, C_RED "Ново NPC " C_WHITE "| Име",
        C_WHITE "Въведи името, което ще се вижда над NPC-то.\n" \
        C_GREY "Пример: Иван - Фермер", "Напред", "Отказ");
    return 1;
}

CMD:npcdel(playerid, params[])
{
    #pragma unused params
    if (!Admin_Check(playerid, ADMIN_ADMIN)) return 1;
    new npc = Npc_GetNearest(playerid, 3.0);
    if (npc == -1) return SendError(playerid, "Няма NPC в радиус 3 м.");

    new q[96];
    format(q, sizeof (q), "DELETE FROM npcs WHERE id = %d", NpcData[npc][npcDBID]);
    Database_Exec(q);
    SendClientMessage(playerid, COLOR_ADMIN, "[NPC] \"%s\" е премахнато.", NpcData[npc][npcName]);
    Log_Add("npcdel", PlayerData[playerid][pName], NpcData[npc][npcName], "");
    Npc_Destroy(npc);
    return 1;
}

CMD:npclist(playerid, params[])
{
    #pragma unused params
    if (!Admin_Check(playerid, ADMIN_ADMIN)) return 1;
    new count = 0;
    SendClientMessage(playerid, COLOR_ADMIN, "______ NPC ______");
    for (new i = 0; i < MAX_NPCS; i++) {
        if (!NpcData[i][npcExists]) continue;
        new Float:d = GetPlayerDistanceFromPoint(playerid, NpcData[i][npcX], NpcData[i][npcY], NpcData[i][npcZ]);
        SendClientMessage(playerid, COLOR_WHITE, "%s {A0A0A0}- %s, скин %d, %.0f м от теб",
            NpcData[i][npcName], NpcRoleName[NpcData[i][npcRole]], NpcData[i][npcSkin], d);
        count++;
    }
    if (count == 0) SendClientMessage(playerid, COLOR_GREY, "Няма NPC. Добави с /npcadd.");
    return 1;
}

stock Npc_OnDialogResponse(playerid, dialogid, response, listitem, const inputtext[])
{
    switch (dialogid) {
        case DIALOG_NPC_NAME: {
            if (!response) return 1;
            if (strlen(inputtext) < 2 || strlen(inputtext) >= NPC_NAME_LEN) {
                SendError(playerid, "Името трябва да е между 2 и 31 символа.");
                return 1;
            }
            format(NpcEditName[playerid], NPC_NAME_LEN, "%s", inputtext);

            new body[512];
            body[0] = EOS;
            for (new r = 0; r < MAX_NPC_ROLES; r++) {
                strcat(body, C_BRAND "» " C_WHITE);
                strcat(body, NpcRoleName[r]);
                strcat(body, "\n");
            }
            ShowPlayerDialog(playerid, DIALOG_NPC_ROLE, DIALOG_STYLE_LIST, C_RED "Ново NPC " C_WHITE "| Роля", "%s", "Напред", "Отказ", body);
            return 1;
        }
        case DIALOG_NPC_ROLE: {
            if (!response || listitem < 0 || listitem >= MAX_NPC_ROLES) return 1;
            NpcEditRole[playerid] = listitem;
            ShowPlayerDialog(playerid, DIALOG_NPC_SKIN, DIALOG_STYLE_INPUT, C_RED "Ново NPC " C_WHITE "| Скин",
                C_WHITE "Въведи номер на скин (0-311).\n" C_GREY "Пример: 158 (фермер), 70 (лекар), 147 (бизнесмен)", "Напред", "Отказ");
            return 1;
        }
        case DIALOG_NPC_SKIN: {
            if (!response) return 1;
            new skin = IsNumeric(inputtext) ? strval(inputtext) : -1;
            if (skin < 0 || skin > 311 || skin == 74) {
                SendError(playerid, "Невалиден скин.");
                return 1;
            }
            NpcEditSkin[playerid] = skin;

            new body[512];
            body[0] = EOS;
            for (new a = 0; a < sizeof (NpcAnimName); a++) {
                strcat(body, C_BRAND "» " C_WHITE);
                strcat(body, NpcAnimName[a]);
                strcat(body, "\n");
            }
            ShowPlayerDialog(playerid, DIALOG_NPC_ANIM, DIALOG_STYLE_LIST, C_RED "Ново NPC " C_WHITE "| Анимация", "%s", "Създай", "Отказ", body);
            return 1;
        }
        case DIALOG_NPC_ANIM: {
            if (!response || listitem < 0 || listitem >= sizeof (NpcAnimName)) return 1;

            new Float:x, Float:y, Float:z, Float:a;
            GetPlayerPos(playerid, x, y, z);
            GetPlayerFacingAngle(playerid, a);

            // NPC-то застава на твоето място и гледа напред; ти минаваш крачка пред него и се обръщаш към него.
            new npc = Npc_Create(NpcEditName[playerid], NpcEditRole[playerid], NpcEditSkin[playerid],
                x, y, z, a, GetPlayerVirtualWorld(playerid), listitem);
            if (npc == -1) return SendError(playerid, "Достигнат е максималният брой NPC.");

            SetPlayerPos(playerid, x - 1.5 * floatsin(a, degrees), y + 1.5 * floatcos(a, degrees), z);
            SetPlayerFacingAngle(playerid, a + 180.0);
            SendClientMessage(playerid, COLOR_ADMIN, "[NPC] Създадено: \"%s\" (%s).", NpcEditName[playerid], NpcRoleName[NpcEditRole[playerid]]);
            Log_Add("npcadd", PlayerData[playerid][pName], NpcEditName[playerid], "");
            return 1;
        }
    }
    return 0;
}
