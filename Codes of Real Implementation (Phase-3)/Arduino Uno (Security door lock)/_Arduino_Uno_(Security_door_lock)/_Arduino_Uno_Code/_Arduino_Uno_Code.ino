 // ═══════════════════════════════════════════════════════════════
// SMART LOCK SYSTEM — UNO FINAL VERSION
// RFID + PIN + LCD + SERVO + EEPROM + PANIC + ADMIN/USER/GUEST
// ═══════════════════════════════════════════════════════════════

#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <EEPROM.h>
#include <string.h>

// ───────────────────────────────────────────────────────────────
// PINS
// ───────────────────────────────────────────────────────────────

#define SS_PIN        10
#define RST_PIN        9
#define SERVO_PIN      6
#define BUZZER_PIN     7
#define GREEN_LED      4
#define RED_LED        5

// ───────────────────────────────────────────────────────────────
// CONSTANTS
// ───────────────────────────────────────────────────────────────

#define PIN_MAX_LENGTH       8
#define MAX_USERS            4
#define MAX_FAILED_ATTEMPTS  6
#define LOCKOUT_TIME         30000UL

#define LOCK_POS             90
#define UNLOCK_POS           0

#define DOOR_OPEN_TIME       2000UL
#define PANIC_PIN            "911"

// EEPROM ADDRESSES
#define EEPROM_ATTEMPTS      0
#define EEPROM_LOCKED        1
#define EEPROM_SOUND         2
#define EEPROM_SIG_1         3
#define EEPROM_SIG_2         4
#define EEPROM_USERS_START   10

// Change these signatures when you want EEPROM user data to reset once
#define EEPROM_SIGNATURE_1   0x72
#define EEPROM_SIGNATURE_2   0x91

// ───────────────────────────────────────────────────────────────
// HARDWARE OBJECTS
// ───────────────────────────────────────────────────────────────

MFRC522 mfrc522(SS_PIN, RST_PIN);
Servo doorServo;
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ───────────────────────────────────────────────────────────────
// KEYPAD
// ───────────────────────────────────────────────────────────────

const byte ROWS = 4;
const byte COLS = 3;

char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};

byte rowPins[ROWS] = {2, 3, 8, A0};
byte colPins[COLS] = {A1, A2, A3};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ───────────────────────────────────────────────────────────────
// LCD ICONS
// ───────────────────────────────────────────────────────────────

byte lockIcon[8] = {
  B01110,
  B10001,
  B10001,
  B11111,
  B11011,
  B11011,
  B11111,
  B00000
};

byte unlockIcon[8] = {
  B01110,
  B10000,
  B10000,
  B11111,
  B11011,
  B11011,
  B11111,
  B00000
};

// ───────────────────────────────────────────────────────────────
// USER MODEL
// ───────────────────────────────────────────────────────────────

enum Role {
  ROLE_ADMIN,
  ROLE_USER,
  ROLE_GUEST
};

struct User {
  byte uid[4];
  const char* name;
  char pin[PIN_MAX_LENGTH + 1];
  Role role;
  bool active;
};

// Default users:
// 0 = Admin
// 1 = User Ahmed
// 2 = User Karim
// 3 = Guest
User users[MAX_USERS] = {
  {{0xFF, 0x8D, 0x82, 0xC6}, "Youssef", "0000", ROLE_ADMIN, true},
  {{0x54, 0xE0, 0x34, 0x1D}, "Ahmed",   "1234", ROLE_USER,  true},
  {{0x33, 0x3A, 0x24, 0xAD}, "Karim",   "5678", ROLE_USER,  true},
  {{0xF3, 0xFF, 0xBC, 0x1B}, "Guest",   "9999", ROLE_GUEST, true}
};

// ───────────────────────────────────────────────────────────────
// SYSTEM STATE
// ───────────────────────────────────────────────────────────────

enum SystemState {
  SYS_IDLE,
  SYS_LOCKED,

  SYS_ADMIN_LOGIN,
  SYS_ADMIN_MENU,

  SYS_ROLE_SELECT,
  SYS_USER_SELECT,
  SYS_USER_EDIT,

  SYS_CHANGE_PIN,
  SYS_CARD_MENU,
  SYS_SET_CARD
};

SystemState currentState = SYS_IDLE;

// ───────────────────────────────────────────────────────────────
// VARIABLES
// ───────────────────────────────────────────────────────────────

char pinBuffer[PIN_MAX_LENGTH + 1];
byte pinIndex = 0;

byte failedAttempts = 0;
bool lockedOut = false;
bool silentMode = false;

bool adminLoginFromLockout = false;

byte selectedRole = 255;
byte selectedUser = 255;

unsigned long lockoutEnd = 0;
unsigned long lastLockoutDisplay = 0;

// ───────────────────────────────────────────────────────────────
// LCD HELPERS — LCD SLEEP REMOVED
// ───────────────────────────────────────────────────────────────

void wakeLCD() {
  lcd.backlight();
}

void clearLCD() {
  lcd.setCursor(0, 0);
  lcd.print(F("                "));
  lcd.setCursor(0, 1);
  lcd.print(F("                "));
}

void show(const __FlashStringHelper* l1, const __FlashStringHelper* l2) {
  wakeLCD();
  clearLCD();
  lcd.setCursor(0, 0);
  lcd.print(l1);
  lcd.setCursor(0, 1);
  lcd.print(l2);
}

void show(const __FlashStringHelper* l1, const char* l2) {
  wakeLCD();
  clearLCD();
  lcd.setCursor(0, 0);
  lcd.print(l1);
  lcd.setCursor(0, 1);
  lcd.print(l2);
}

void show(const char* l1, const __FlashStringHelper* l2) {
  wakeLCD();
  clearLCD();
  lcd.setCursor(0, 0);
  lcd.print(l1);
  lcd.setCursor(0, 1);
  lcd.print(l2);
}

void showPinStars() {
  lcd.setCursor(0, 1);
  lcd.print(F("                "));
  lcd.setCursor(0, 1);

  for (byte i = 0; i < pinIndex; i++) {
    lcd.print('*');
  }
}

void printUserName(byte index) {
  lcd.print(users[index].name);
}

void showSelectedUser(const __FlashStringHelper* bottom) {
  wakeLCD();
  clearLCD();

  lcd.setCursor(0, 0);
  printUserName(selectedUser);

  if (!users[selectedUser].active) {
    lcd.print(F(" OFF"));
  }

  lcd.setCursor(0, 1);
  lcd.print(bottom);
}

// ───────────────────────────────────────────────────────────────
// PIN BUFFER
// ───────────────────────────────────────────────────────────────

void resetPinBuffer() {
  pinIndex = 0;
  pinBuffer[0] = '\0';
}

void addKeyToPin(char k) {
  if (pinIndex < PIN_MAX_LENGTH) {
    pinBuffer[pinIndex] = k;
    pinIndex++;
    pinBuffer[pinIndex] = '\0';
  }
}

// ───────────────────────────────────────────────────────────────
// SOUND
// ───────────────────────────────────────────────────────────────

void beep(int frequency, int durationMs) {
  if (silentMode) return;

  tone(BUZZER_PIN, frequency, durationMs);
  delay(durationMs + 20);
  noTone(BUZZER_PIN);
}

void forceBeep(int frequency, int durationMs) {
  tone(BUZZER_PIN, frequency, durationMs);
  delay(durationMs + 20);
  noTone(BUZZER_PIN);
}

void errorSound() {
  beep(500, 220);
}

void menuSound() {
  beep(1500, 50);
}

void adminLoginSound() {
  beep(1200, 80);
  beep(1600, 80);
  beep(2200, 120);
}

void userOneLoginSound() {
  beep(1000, 80);
  beep(1400, 80);
}

void userTwoLoginSound() {
  beep(900, 90);
  beep(1300, 90);
  beep(1700, 90);
}

void guestLoginSound() {
  beep(700, 120);
  beep(1000, 120);
}

void overrideSound() {
  forceBeep(2200, 100);
  forceBeep(1600, 100);
  forceBeep(2200, 100);
}

void loginSound(byte userIndex) {
  if (userIndex == 0) {
    adminLoginSound();
  } else if (userIndex == 1) {
    userOneLoginSound();
  } else if (userIndex == 2) {
    userTwoLoginSound();
  } else if (userIndex == 3) {
    guestLoginSound();
  } else {
    beep(1200, 100);
  }
}

// Alarm stays the same
void newAlarmSound() {
  for (byte i = 0; i < 8; i++) {
    forceBeep(1800, 120);
    forceBeep(700, 120);
  }

  for (byte i = 0; i < 4; i++) {
    forceBeep(2200, 80);
    forceBeep(1200, 80);
    forceBeep(500, 120);
  }
}

// ───────────────────────────────────────────────────────────────
// LED HELPERS
// ───────────────────────────────────────────────────────────────

void allLEDsOff() {
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);
}

void flashGreen(byte times) {
  for (byte i = 0; i < times; i++) {
    digitalWrite(GREEN_LED, HIGH);
    delay(120);
    digitalWrite(GREEN_LED, LOW);
    delay(120);
  }
}

void flashRed(byte times) {
  for (byte i = 0; i < times; i++) {
    digitalWrite(RED_LED, HIGH);
    delay(120);
    digitalWrite(RED_LED, LOW);
    delay(120);
  }
}

// ───────────────────────────────────────────────────────────────
// EEPROM USER STORAGE
// ───────────────────────────────────────────────────────────────

int userEEPROMBase(byte index) {
  return EEPROM_USERS_START + index * 16;
}

void saveUserToEEPROM(byte index) {
  int base = userEEPROMBase(index);

  for (byte i = 0; i < 4; i++) {
    EEPROM.update(base + i, users[index].uid[i]);
  }

  for (byte i = 0; i < PIN_MAX_LENGTH + 1; i++) {
    EEPROM.update(base + 4 + i, users[index].pin[i]);
  }

  EEPROM.update(base + 13, (byte)users[index].role);
  EEPROM.update(base + 14, users[index].active ? 1 : 0);
}

void saveAllUsersToEEPROM() {
  EEPROM.update(EEPROM_SIG_1, EEPROM_SIGNATURE_1);
  EEPROM.update(EEPROM_SIG_2, EEPROM_SIGNATURE_2);

  for (byte i = 0; i < MAX_USERS; i++) {
    saveUserToEEPROM(i);
  }
}

void loadUsersFromEEPROM() {
  byte sig1 = EEPROM.read(EEPROM_SIG_1);
  byte sig2 = EEPROM.read(EEPROM_SIG_2);

  if (sig1 != EEPROM_SIGNATURE_1 || sig2 != EEPROM_SIGNATURE_2) {
    saveAllUsersToEEPROM();
    return;
  }

  for (byte index = 0; index < MAX_USERS; index++) {
    int base = userEEPROMBase(index);

    for (byte i = 0; i < 4; i++) {
      users[index].uid[i] = EEPROM.read(base + i);
    }

    for (byte i = 0; i < PIN_MAX_LENGTH + 1; i++) {
      users[index].pin[i] = EEPROM.read(base + 4 + i);
    }

    users[index].pin[PIN_MAX_LENGTH] = '\0';

    if (users[index].pin[0] == 0xFF) {
      users[index].pin[0] = '\0';
    }

    byte savedRole = EEPROM.read(base + 13);
    if (savedRole <= ROLE_GUEST) {
      users[index].role = (Role)savedRole;
    }

    users[index].active = EEPROM.read(base + 14) == 1;
  }

  // Safety: main admin can never disappear
  users[0].role = ROLE_ADMIN;
  users[0].active = true;

  if (users[0].pin[0] == '\0') {
    strcpy(users[0].pin, "0000");
  }

  saveUserToEEPROM(0);
}

// ───────────────────────────────────────────────────────────────
// EEPROM SYSTEM STORAGE
// ───────────────────────────────────────────────────────────────

void saveEEPROM() {
  EEPROM.update(EEPROM_ATTEMPTS, failedAttempts);
  EEPROM.update(EEPROM_LOCKED, lockedOut ? 1 : 0);
  EEPROM.update(EEPROM_SOUND, silentMode ? 1 : 0);
}

void loadEEPROM() {
  failedAttempts = EEPROM.read(EEPROM_ATTEMPTS);

  if (failedAttempts > MAX_FAILED_ATTEMPTS) {
    failedAttempts = 0;
  }

  lockedOut = EEPROM.read(EEPROM_LOCKED) == 1;
  silentMode = EEPROM.read(EEPROM_SOUND) == 1;

  if (lockedOut) {
    lockoutEnd = millis() + LOCKOUT_TIME;
    currentState = SYS_LOCKED;
  }
}

void clearLockout() {
  failedAttempts = 0;
  lockedOut = false;
  currentState = SYS_IDLE;
  saveEEPROM();
}

// ───────────────────────────────────────────────────────────────
// SERVO
// ───────────────────────────────────────────────────────────────

void openDoor() {
  doorServo.attach(SERVO_PIN);

  for (int p = LOCK_POS; p >= UNLOCK_POS; p -= 2) {
    doorServo.write(p);
    delay(10);
  }

  delay(200);
  doorServo.detach();
}

void closeDoor() {
  doorServo.attach(SERVO_PIN);

  for (int p = UNLOCK_POS; p <= LOCK_POS; p += 2) {
    doorServo.write(p);
    delay(10);
  }

  delay(200);
  doorServo.detach();
}

// ───────────────────────────────────────────────────────────────
// USER SEARCH
// ───────────────────────────────────────────────────────────────

bool uidIsEmpty(byte* uid) {
  return uid[0] == 0 && uid[1] == 0 && uid[2] == 0 && uid[3] == 0;
}

int findUserIndexByUID(byte* uid, byte size) {
  if (size != 4) return -1;

  for (byte i = 0; i < MAX_USERS; i++) {
    if (!users[i].active) continue;
    if (uidIsEmpty(users[i].uid)) continue;

    bool match = true;

    for (byte j = 0; j < 4; j++) {
      if (users[i].uid[j] != uid[j]) {
        match = false;
        break;
      }
    }

    if (match) return i;
  }

  return -1;
}

int findUserIndexByPIN(const char* pin) {
  if (pin[0] == '\0') return -1;

  for (byte i = 0; i < MAX_USERS; i++) {
    if (!users[i].active) continue;

    if (strcmp(users[i].pin, pin) == 0) {
      return i;
    }
  }

  return -1;
}

bool pinBelongsToAdmin(const char* pin) {
  int index = findUserIndexByPIN(pin);

  if (index >= 0 && users[index].role == ROLE_ADMIN) {
    return true;
  }

  return false;
}

bool uidBelongsToAdmin(byte* uid, byte size) {
  int index = findUserIndexByUID(uid, size);

  if (index >= 0 && users[index].role == ROLE_ADMIN) {
    return true;
  }

  return false;
}

// ───────────────────────────────────────────────────────────────
// ACCESS CONTROL
// ───────────────────────────────────────────────────────────────

void grantAccess(byte userIndex) {
  allLEDsOff();

  wakeLCD();
  clearLCD();

  lcd.setCursor(0, 0);
  lcd.print(F("ACCESS GRANTED"));

  lcd.setCursor(0, 1);
  printUserName(userIndex);

  loginSound(userIndex);
  flashGreen(2);

  lcd.setCursor(15, 0);
  lcd.write(byte(1));

  openDoor();

  wakeLCD();
  clearLCD();

  lcd.setCursor(0, 0);
  lcd.print(F("DOOR OPEN"));

  lcd.setCursor(0, 1);
  printUserName(userIndex);

  digitalWrite(GREEN_LED, HIGH);

  delay(DOOR_OPEN_TIME);

  show(F("DOOR CLOSING"), F("PLEASE WAIT"));
  closeDoor();

  digitalWrite(GREEN_LED, LOW);

  failedAttempts = 0;
  lockedOut = false;
  saveEEPROM();

  show(F("SCAN CARD"), F("OR ENTER PIN"));
}

void triggerLockout() {
  lockedOut = true;
  lockoutEnd = millis() + LOCKOUT_TIME;
  currentState = SYS_LOCKED;

  saveEEPROM();

  show(F("SYSTEM LOCKED"), F("TOO MANY TRIES"));
  newAlarmSound();
  flashRed(5);
}

void denyAccess(const __FlashStringHelper* reason) {
  allLEDsOff();

  failedAttempts++;
  saveEEPROM();

  show(F("ACCESS DENIED"), reason);
  errorSound();
  flashRed(3);

  if (failedAttempts >= MAX_FAILED_ATTEMPTS) {
    triggerLockout();
  } else {
    delay(1000);
    show(F("SCAN CARD"), F("OR ENTER PIN"));
  }
}

void panicAlarm() {
  allLEDsOff();

  show(F("!!! PANIC !!!"), F("ALARM ACTIVE"));

  for (byte i = 0; i < 6; i++) {
    digitalWrite(RED_LED, HIGH);
    newAlarmSound();
    digitalWrite(RED_LED, LOW);
    delay(150);
  }

  allLEDsOff();
  show(F("SCAN CARD"), F("OR ENTER PIN"));
}

// ───────────────────────────────────────────────────────────────
// ADMIN MENU DISPLAYS
// ───────────────────────────────────────────────────────────────

void showAdminMenu() {
  show(F("1RESET 2USERS"), F("3SOUND *EXIT"));
}

void showRoleSelect() {
  show(F("1ADMIN 2USER"), F("3GUEST *BACK"));
}

void showUserSelectByRole() {
  wakeLCD();
  clearLCD();

  lcd.setCursor(0, 0);

  if (selectedRole == ROLE_ADMIN) {
    lcd.print(F("ADMIN: 1"));
  } else if (selectedRole == ROLE_USER) {
    lcd.print(F("USER: 1AH 2KA"));
  } else if (selectedRole == ROLE_GUEST) {
    lcd.print(F("GUEST: 1"));
  }

  lcd.setCursor(0, 1);
  lcd.print(F("* TO BACK"));
}

void showUserEdit() {
  showSelectedUser(F("1PIN 2CARD"));
  delay(800);
  show(F("3ON/OFF"), F("* BACK"));
}

void showCardMenu() {
  show(F("1SET CARD"), F("2REMOVE *BACK"));
}

// ───────────────────────────────────────────────────────────────
// ADMIN MENU HANDLERS
// ───────────────────────────────────────────────────────────────

void handleAdminMenu() {
  char k = keypad.getKey();

  if (!k) return;

  wakeLCD();
  menuSound();

  if (k == '1') {
    clearLockout();
    show(F("RESET DONE"), F("LOCK CLEARED"));
    delay(1000);
    showAdminMenu();
  }

  else if (k == '2') {
    currentState = SYS_ROLE_SELECT;
    selectedRole = 255;
    selectedUser = 255;
    showRoleSelect();
  }

  else if (k == '3') {
    silentMode = !silentMode;
    saveEEPROM();

    if (silentMode) {
      show(F("SOUND MODE"), F("MUTED"));
    } else {
      show(F("SOUND MODE"), F("UNMUTED"));
      forceBeep(1500, 80);
    }

    delay(1000);
    showAdminMenu();
  }

  else if (k == '*') {
    currentState = SYS_IDLE;
    resetPinBuffer();
    selectedRole = 255;
    selectedUser = 255;
    show(F("SCAN CARD"), F("OR ENTER PIN"));
  }
}

void handleRoleSelect() {
  char k = keypad.getKey();

  if (!k) return;

  wakeLCD();
  menuSound();

  if (k == '*') {
    currentState = SYS_ADMIN_MENU;
    showAdminMenu();
    return;
  }

  if (k == '1') {
    selectedRole = ROLE_ADMIN;
    currentState = SYS_USER_SELECT;
    showUserSelectByRole();
  }

  else if (k == '2') {
    selectedRole = ROLE_USER;
    currentState = SYS_USER_SELECT;
    showUserSelectByRole();
  }

  else if (k == '3') {
    selectedRole = ROLE_GUEST;
    currentState = SYS_USER_SELECT;
    showUserSelectByRole();
  }
}

void handleUserSelect() {
  char k = keypad.getKey();

  if (!k) return;

  wakeLCD();
  menuSound();

  if (k == '*') {
    currentState = SYS_ROLE_SELECT;
    showRoleSelect();
    return;
  }

  if (selectedRole == ROLE_ADMIN) {
    if (k == '1') selectedUser = 0;
    else return;
  }

  else if (selectedRole == ROLE_USER) {
    if (k == '1') selectedUser = 1;
    else if (k == '2') selectedUser = 2;
    else return;
  }

  else if (selectedRole == ROLE_GUEST) {
    if (k == '1') selectedUser = 3;
    else return;
  }

  currentState = SYS_USER_EDIT;
  showUserEdit();
}

void handleUserEdit() {
  char k = keypad.getKey();

  if (!k) return;

  wakeLCD();
  menuSound();

  if (k == '*') {
    selectedUser = 255;
    currentState = SYS_USER_SELECT;
    showUserSelectByRole();
    return;
  }

  if (k == '1') {
    resetPinBuffer();
    currentState = SYS_CHANGE_PIN;
    show(F("NEW PIN"), F("TYPE THEN #"));
    return;
  }

  if (k == '2') {
    currentState = SYS_CARD_MENU;
    showCardMenu();
    return;
  }

  if (k == '3') {
    if (selectedUser == 0) {
      show(F("MAIN ADMIN"), F("CANT DISABLE"));
      delay(1200);
      showUserEdit();
      return;
    }

    users[selectedUser].active = !users[selectedUser].active;
    saveUserToEEPROM(selectedUser);

    if (users[selectedUser].active) {
      show(F("USER ENABLED"), users[selectedUser].name);
    } else {
      show(F("USER DISABLED"), users[selectedUser].name);
    }

    delay(1200);
    showUserEdit();
  }
}

void handleChangePin() {
  char k = keypad.getKey();

  if (!k) return;

  wakeLCD();

  if (k == '*') {
    resetPinBuffer();
    currentState = SYS_USER_EDIT;
    showUserEdit();
    return;
  }

  if (k == '#') {
    if (pinIndex == 0) {
      show(F("PIN EMPTY"), F("TRY AGAIN"));
      delay(1000);
      show(F("NEW PIN"), F("TYPE THEN #"));
      return;
    }

    if (strcmp(pinBuffer, PANIC_PIN) == 0) {
      show(F("911 RESERVED"), F("USE OTHER PIN"));
      resetPinBuffer();
      delay(1200);
      show(F("NEW PIN"), F("TYPE THEN #"));
      return;
    }

    int existing = findUserIndexByPIN(pinBuffer);

    if (existing >= 0 && existing != selectedUser) {
      show(F("PIN EXISTS"), F("USE ANOTHER"));
      resetPinBuffer();
      delay(1200);
      show(F("NEW PIN"), F("TYPE THEN #"));
      return;
    }

    strcpy(users[selectedUser].pin, pinBuffer);
    saveUserToEEPROM(selectedUser);

    resetPinBuffer();

    show(F("PIN UPDATED"), users[selectedUser].name);
    loginSound(selectedUser);
    delay(1200);

    currentState = SYS_USER_EDIT;
    showUserEdit();
    return;
  }

  addKeyToPin(k);
  showPinStars();
}

void handleCardMenu() {
  char k = keypad.getKey();

  if (!k) return;

  wakeLCD();
  menuSound();

  if (k == '*') {
    currentState = SYS_USER_EDIT;
    showUserEdit();
    return;
  }

  if (k == '1') {
    currentState = SYS_SET_CARD;
    show(F("SCAN NEW CARD"), F("* TO CANCEL"));
    return;
  }

  if (k == '2') {
    for (byte i = 0; i < 4; i++) {
      users[selectedUser].uid[i] = 0;
    }

    saveUserToEEPROM(selectedUser);

    show(F("CARD REMOVED"), users[selectedUser].name);
    flashRed(2);
    delay(1200);

    currentState = SYS_USER_EDIT;
    showUserEdit();
  }
}

void handleSetCard() {
  char k = keypad.getKey();

  if (k == '*') {
    currentState = SYS_CARD_MENU;
    showCardMenu();
    return;
  }

  if (!mfrc522.PICC_IsNewCardPresent()) return;
  if (!mfrc522.PICC_ReadCardSerial()) return;

  int existing = findUserIndexByUID(mfrc522.uid.uidByte, mfrc522.uid.size);

  if (existing >= 0 && existing != selectedUser) {
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();

    show(F("CARD EXISTS"), F("USE ANOTHER"));
    delay(1200);
    show(F("SCAN NEW CARD"), F("* TO CANCEL"));
    return;
  }

  for (byte i = 0; i < 4; i++) {
    users[selectedUser].uid[i] = mfrc522.uid.uidByte[i];
  }

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();

  saveUserToEEPROM(selectedUser);

  show(F("CARD UPDATED"), users[selectedUser].name);
  loginSound(selectedUser);
  flashGreen(2);
  delay(1200);

  currentState = SYS_USER_EDIT;
  showUserEdit();
}

// ───────────────────────────────────────────────────────────────
// ADMIN LOGIN + OVERRIDE
// ───────────────────────────────────────────────────────────────

void handleAdminLoginKey(char k) {
  wakeLCD();

  if (k == '*') {
    resetPinBuffer();

    if (adminLoginFromLockout) {
      adminLoginFromLockout = false;
      currentState = SYS_LOCKED;
      show(F("SYSTEM LOCKED"), F("* ADMIN LOGIN"));
    } else {
      currentState = SYS_IDLE;
      show(F("SCAN CARD"), F("OR ENTER PIN"));
    }

    return;
  }

  if (k == '#') {
    if (pinBelongsToAdmin(pinBuffer)) {
      resetPinBuffer();

      if (adminLoginFromLockout) {
        clearLockout();
        adminLoginFromLockout = false;

        show(F("OVERRIDE OK"), F("LOCK CLEARED"));
        overrideSound();
        flashGreen(3);
        delay(1000);
      }

      currentState = SYS_ADMIN_MENU;
      showAdminMenu();
    } else {
      resetPinBuffer();

      if (adminLoginFromLockout) {
        show(F("BAD ADMIN PIN"), F("STILL LOCKED"));
        errorSound();
        delay(1000);

        adminLoginFromLockout = false;
        currentState = SYS_LOCKED;
        show(F("SYSTEM LOCKED"), F("* ADMIN LOGIN"));
      } else {
        currentState = SYS_IDLE;
        denyAccess(F("BAD ADMIN PIN"));
      }
    }

    return;
  }

  addKeyToPin(k);
  showPinStars();
}

// ───────────────────────────────────────────────────────────────
// LOCKOUT
// ───────────────────────────────────────────────────────────────

void handleLockout() {
  unsigned long now = millis();

  if (now >= lockoutEnd) {
    clearLockout();

    show(F("LOCKOUT ENDED"), F("TRY AGAIN"));
    delay(1000);
    show(F("SCAN CARD"), F("OR ENTER PIN"));
    return;
  }

  if (now - lastLockoutDisplay >= 1000) {
    lastLockoutDisplay = now;

    unsigned long remaining = (lockoutEnd - now) / 1000;

    wakeLCD();
    clearLCD();

    lcd.setCursor(0, 0);
    lcd.print(F("LOCKED "));
    lcd.print(remaining);
    lcd.print(F(" SEC"));

    lcd.setCursor(0, 1);
    lcd.print(F("* ADMIN LOGIN"));

    digitalWrite(RED_LED, !digitalRead(RED_LED));
  }

  char k = keypad.getKey();

  if (k == '*') {
    resetPinBuffer();
    adminLoginFromLockout = true;
    currentState = SYS_ADMIN_LOGIN;
    show(F("ADMIN OVERRIDE"), F("PIN THEN #"));
    return;
  }

  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    bool adminCard = uidBelongsToAdmin(mfrc522.uid.uidByte, mfrc522.uid.size);

    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();

    if (adminCard) {
      clearLockout();

      show(F("RFID OVERRIDE"), F("LOCK CLEARED"));
      overrideSound();
      flashGreen(3);

      delay(1000);
      currentState = SYS_ADMIN_MENU;
      showAdminMenu();
    } else {
      show(F("NOT ADMIN"), F("STILL LOCKED"));
      errorSound();
      delay(1000);
      currentState = SYS_LOCKED;
    }
  }
}

// ───────────────────────────────────────────────────────────────
// NORMAL PIN
// ───────────────────────────────────────────────────────────────

void handleNormalKey(char k) {
  wakeLCD();

  if (k == '*') {
    if (pinIndex == 0) {
      resetPinBuffer();
      adminLoginFromLockout = false;
      currentState = SYS_ADMIN_LOGIN;
      show(F("ADMIN LOGIN"), F("PIN THEN #"));
    } else {
      resetPinBuffer();
      show(F("PIN CLEARED"), F(""));
      delay(600);
      show(F("SCAN CARD"), F("OR ENTER PIN"));
    }

    return;
  }

  if (k == '#') {
    if (strcmp(pinBuffer, PANIC_PIN) == 0) {
      resetPinBuffer();
      panicAlarm();
      return;
    }

    int userIndex = findUserIndexByPIN(pinBuffer);

    if (userIndex >= 0) {
      resetPinBuffer();
      grantAccess(userIndex);
    } else {
      resetPinBuffer();
      denyAccess(F("WRONG PIN"));
    }

    return;
  }

  if (pinIndex == 0) {
    show(F("ENTER PIN"), F("# CONFIRM"));
  }

  addKeyToPin(k);
  showPinStars();
}

// ───────────────────────────────────────────────────────────────
// RFID
// ───────────────────────────────────────────────────────────────

void checkRFID() {
  if (!mfrc522.PICC_IsNewCardPresent()) return;
  if (!mfrc522.PICC_ReadCardSerial()) return;

  int userIndex = findUserIndexByUID(mfrc522.uid.uidByte, mfrc522.uid.size);

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();

  if (userIndex >= 0) {
    grantAccess(userIndex);
  } else {
    denyAccess(F("BAD CARD"));
  }
}

// ───────────────────────────────────────────────────────────────
// SETUP
// ───────────────────────────────────────────────────────────────

void setup() {
  Serial.begin(9600);

  SPI.begin();
  mfrc522.PCD_Init();

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);

  allLEDsOff();

  lcd.init();
  lcd.backlight();

  lcd.createChar(0, lockIcon);
  lcd.createChar(1, unlockIcon);

  resetPinBuffer();

  loadUsersFromEEPROM();
  loadEEPROM();

  closeDoor();

  if (lockedOut) {
    currentState = SYS_LOCKED;
    show(F("SYSTEM LOCKED"), F("* ADMIN LOGIN"));
  } else {
    currentState = SYS_IDLE;
    show(F("SMART LOCK"), F("READY"));
    delay(1500);
    show(F("SCAN CARD"), F("OR ENTER PIN"));
  }
}

// ───────────────────────────────────────────────────────────────
// LOOP
// ───────────────────────────────────────────────────────────────

void loop() {
  // Important: admin login is checked before lockout.
  // This fixes override after pressing * during lockout.
  if (currentState == SYS_ADMIN_LOGIN) {
    char k = keypad.getKey();

    if (k) {
      handleAdminLoginKey(k);
    }

    return;
  }

  if (lockedOut || currentState == SYS_LOCKED) {
    currentState = SYS_LOCKED;
    handleLockout();
    return;
  }

  if (currentState == SYS_ADMIN_MENU) {
    handleAdminMenu();
    return;
  }

  if (currentState == SYS_ROLE_SELECT) {
    handleRoleSelect();
    return;
  }

  if (currentState == SYS_USER_SELECT) {
    handleUserSelect();
    return;
  }

  if (currentState == SYS_USER_EDIT) {
    handleUserEdit();
    return;
  }

  if (currentState == SYS_CHANGE_PIN) {
    handleChangePin();
    return;
  }

  if (currentState == SYS_CARD_MENU) {
    handleCardMenu();
    return;
  }

  if (currentState == SYS_SET_CARD) {
    handleSetCard();
    return;
  }

  char k = keypad.getKey();

  if (k) {
    handleNormalKey(k);
  }

  checkRFID();
}