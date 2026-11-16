// Пароли – BCrypt (плъгин samp-bcrypt).
//
// BCrypt е нарочно бавен, затова плъгинът работи асинхронно: хешира в отделна
// нишка и после извиква Pass_OnHashed / Pass_OnVerified. Сървърът не спира.

enum {
    PASS_LOGIN,             // проверка при вход
    PASS_REGISTER,          // нова парола при регистрация
    PASS_ADMIN_LOGIN,       // проверка на админ паролата
    PASS_ADMIN_CREATE       // нова админ парола
}

new gConnectionSeq;

// Всяко свързване получава уникален номер. Ако играчът излезе, докато се хешира,
// и друг влезе със същото ID, резултатът няма да отиде при грешния човек.
stock Pass_OnPlayerConnect(playerid)
{
    PlayerData[playerid][pConnSeq] = ++gConnectionSeq;
}

stock Pass_Hash(playerid, purpose, const input[])
{
    bcrypt_hash(playerid, "Pass_OnHashed", input, BCRYPT_COST, "dd", purpose, PlayerData[playerid][pConnSeq]);
}

stock Pass_Verify(playerid, purpose, const input[], const hash[])
{
    if (input[0] == EOS || hash[0] == EOS) {
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

stock Pass_Dispatch(playerid, purpose, bool:success, const hash[])
{
    switch (purpose) {
        case PASS_LOGIN:        Auth_OnLoginResult(playerid, success);
        case PASS_REGISTER:     Auth_OnRegisterHashed(playerid, hash);
        case PASS_ADMIN_LOGIN:  Admin_OnLoginResult(playerid, success);
        case PASS_ADMIN_CREATE: Admin_OnPassHashed(playerid, hash);
    }
}
