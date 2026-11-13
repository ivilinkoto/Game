// Уникални ID-та на диалозите
enum {
    DIALOG_NONE,

    // Вход
    DIALOG_LOGIN,

    // Регистрация (в този ред)
    DIALOG_REG_WELCOME,
    DIALOG_REG_PASSWORD,
    DIALOG_REG_PASSWORD_CONFIRM,
    DIALOG_REG_EMAIL,
    DIALOG_REG_GENDER,
    DIALOG_REG_AGE,
    DIALOG_REG_SOURCE,
    DIALOG_REG_REFERRAL,
    DIALOG_REG_PROMO,
    DIALOG_REG_SUMMARY,

    // Главно меню
    DIALOG_MENU,
    DIALOG_MENU_BACK,   // информационен прозорец с бутон "Назад" към менюто

    // Информационни
    DIALOG_INFO,
    DIALOG_INFO_KICK    // след затваряне играчът се изгонва
}
