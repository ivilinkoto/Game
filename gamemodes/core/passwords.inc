// Пароли – BCrypt (плъгин samp-bcrypt).
//
// BCrypt е нарочно бавен, затова плъгинът работи асинхронно: хешира в отделна
// нишка и после извиква Pass_OnHashed / Pass_OnVerified. Сървърът не спира.
//
// Стари акаунти (SHA-256 + сол, salt != "") се проверяват по стария начин и при
// успешен вход паролата веднага се записва наново с BCrypt.

#include <samp_bcrypt>

// За какво е заявката – callback-ът решава какво да прави с резултата.
enum {
    PASS_LOGIN,             // проверка при вход
    PASS_REGISTER,          // нова парола при регистрация
    PASS_REHASH_ACCOUNT,    // стара SHA-256 парола -> BCrypt
    PASS_ADMIN_LOGIN,       // проверка на админ паролата
    PASS_ADMIN_SAME,        // дали новата админ парола е същата като на акаунта
    PASS_ADMIN_CREATE,      // нова админ парола
    PASS_REHASH_ADMIN       // стара SHA-256 админ парола -> BCrypt
}

new gConnectionSeq;

// Всяко свързване получава уникален номер. Ако играчът излезе, докато се хешира,
// и друг влезе със същото ID, резултатът няма да отиде при грешния човек.
stock Pass_OnPlayerConnect(playerid)
{
    PlayerData[playerid][pConnSeq] = ++gConnectionSeq;
}

stock bool:Pass_IsLegacy(const salt[])
{
    return (salt[0] != EOS);
}

// Проверка на стара SHA-256 парола (синхронна).
stock bool:Pass_LegacyCheck(const input[], const salt[], const hash[])
{
    if (input[0] == EOS || hash[0] == EOS) return false;
    new test[65];
    SHA256_Hash(input, salt, test);
    return (strcmp(test, hash, false) == 0);
}

stock Pass_Hash(playerid, purpose, const input[])
{
    bcrypt_hash(playerid, "Pass_OnHashed", input, BCRYPT_COST, "dd", purpose, PlayerData[playerid][pConnSeq]);
}

stock Pass_Verify(playerid, purpose, const input[], const hash[])
{
    if (input[0] == EOS || hash[0] == EOS) {
        // Празен вход – директно "грешно", без да питаме плъгина.
        Pass_Dispatch(playerid, purpose, false, "");
        return;
    }
    bcrypt_verify(playerid, "Pass_OnVerified", input, hash, "dd", purpose, PlayerData[playerid][pConnSeq]);
}

// ВНИМАНИЕ: плъгинът подава само (playerid, нашите аргументи...) – без "hashid",
// въпреки че документацията в samp_bcrypt.inc твърди друго.
forward Pass_OnHashed(playerid, purpose, seq);
public Pass_OnHashed(playerid, purpose, seq)
{
    if (!IsPlayerConnected(playerid) || PlayerData[playerid][pConnSeq] != seq) return 0;

    new hash[BCRYPT_HASH_LENGTH];
    bcrypt_get_hash(hash);
    Pass_Dispatch(playerid, purpose, true, hash);
    return 1;
}

forward Pass_OnVerified(playerid, bool:success, purpose, seq);
public Pass_OnVerified(playerid, bool:success, purpose, seq)
{
    if (!IsPlayerConnected(playerid) || PlayerData[playerid][pConnSeq] != seq) return 0;
    Pass_Dispatch(playerid, purpose, success, "");
    return 1;
}

// Разпределя резултата към модула, който го е поискал.
stock Pass_Dispatch(playerid, purpose, bool:success, const hash[])
{
    switch (purpose) {
        case PASS_LOGIN:          Auth_OnLoginResult(playerid, success);
        case PASS_REGISTER:       Auth_OnRegisterHashed(playerid, hash);
        case PASS_REHASH_ACCOUNT: Auth_OnRehashed(playerid, hash);
        case PASS_ADMIN_LOGIN:    Admin_OnLoginResult(playerid, success);
        case PASS_ADMIN_SAME:     Admin_OnSameCheck(playerid, success);
        case PASS_ADMIN_CREATE:   Admin_OnPassHashed(playerid, hash);
        case PASS_REHASH_ADMIN:   Admin_OnRehashed(playerid, hash);
    }
}
