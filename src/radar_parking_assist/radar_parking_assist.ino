/*
  Radar Parking Assist - Robotics Homework 1
  Vilniaus universitetas, MIF
  Platforma: Arduino Uno (Tinkercad)

  Ultragarsinis jutiklis ant besisukancio servo, RGB LED + pjezo garsiakalbis,
  16x2 I2C LCD (nuosavas tvarkykle per Wire, be isorines LCD bibliotekos),
  mygtukas rezimui SWEEP (radaras) / PARK (fiksuotas) perjungti ir
  potenciometras A0 kontakte, kuris nustato jautruma.

  Pastaba del kontaktu: Servo biblioteka isjungia PWM kontaktuose D9/D10,
  o tone() isjungia PWM kontaktuose D3/D11. Todel raudonai ir zaliai spalvai
  lieka D6/D5 (PWM), o melyna veikia tik ijungta/isjungta (D3).

  Autorius: Danielius Starkutis
  Licencija: MIT
*/

#include <Servo.h>
#include <Wire.h>

// ---------- Integruota I2C LCD tvarkykle (isorines bibliotekos nereikia) ----------
// Nustatykite pagal LCD "Type" reiksme Tinkercad'e (spustelekite LCD elementa).
#define LCD_PCF8574   1
#define LCD_MCP23008  2
const byte LCD_TYPE = LCD_PCF8574;
// 0 = ieskoti automatiskai. Kitaip ivesti LCD adresa, pvz. 0x20 arba 0x27
const byte LCD_ADDR = 0;

class SimpleLcd {
 public:
  byte addr = 0x20;

  void begin() {
    Wire.begin();
    if (LCD_ADDR) addr = LCD_ADDR; else findAddress();
    Serial.print("LCD I2C address: 0x");
    Serial.println(addr, HEX);
    if (LCD_TYPE == LCD_MCP23008) writeReg(0x00, 0x00);   // visi kontaktai kaip isejimai
    expanderWrite(0);
    delay(50);
    write4(0x03, false); delay(5);        // HD44780 4 bitu inicializacija
    write4(0x03, false); delay(1);
    write4(0x03, false); delay(1);
    write4(0x02, false);
    command(0x28);                        // 4 bitai, 2 eilutes, 5x8 sriftas
    command(0x0C);                        // ekranas ijungtas, zymeklis isjungtas
    command(0x06);                        // rasymas is kaires i desine
    clear();
  }

  void clear()                       { command(0x01); delay(2); }
  void setCursor(byte col, byte row) { command(0x80 | (col + (row ? 0x40 : 0))); }
  void print(const char* s)          { while (*s) send(*s++, true); }

 private:
  // Suranda, kuriuo adresu atsiliepia I2C plestuvas
  void findAddress() {
    const byte candidates[] = {0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,
                               0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F};
    for (byte i = 0; i < sizeof(candidates); i++) {
      Wire.beginTransmission(candidates[i]);
      if (Wire.endTransmission() == 0) { addr = candidates[i]; break; }
    }
  }

  void writeReg(byte reg, byte val) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.write(val);
    Wire.endTransmission();
  }

  void expanderWrite(byte v) {
    if (LCD_TYPE == LCD_PCF8574) {
      Wire.beginTransmission(addr);
      Wire.write(v | 0x08);               // P3 = apsvietimas ijungtas
      Wire.endTransmission();
    } else {
      writeReg(0x09, v | 0x80);           // GPIO registras, GP7 = apsvietimas
    }
  }

  // Priskiria 4 bitu puselei, RS ir E bitams plestuvo kontaktus
  byte pins(byte nibble, bool rs, bool en) {
    if (LCD_TYPE == LCD_PCF8574)          // P0=RS P2=E P4..P7=D4..D7
      return (nibble << 4) | (en ? 0x04 : 0) | (rs ? 0x01 : 0);
    else                                  // GP1=RS GP2=E GP3..GP6=D4..D7
      return (nibble << 3) | (en ? 0x04 : 0) | (rs ? 0x02 : 0);
  }

  // E impulsas liepia ekranui nuskaityti duomenis
  void write4(byte nibble, bool rs) {
    expanderWrite(pins(nibble & 0x0F, rs, true));
    delayMicroseconds(1);
    expanderWrite(pins(nibble & 0x0F, rs, false));
    delayMicroseconds(50);
  }

  // Baitas siunciamas dviem puselemis (4 bitu rezimas)
  void send(byte value, bool rs) {
    write4(value >> 4, rs);
    write4(value & 0x0F, rs);
  }

  void command(byte c) { send(c, false); }
};

// ---------- Kontaktai ----------
const byte PIN_TRIG   = 12;
const byte PIN_ECHO   = 11;
const byte PIN_SERVO  = 9;
const byte PIN_BUZZER = 8;
const byte PIN_RED    = 6;   // PWM
const byte PIN_GREEN  = 5;   // PWM
const byte PIN_BLUE   = 3;   // tik skaitmeninis
const byte PIN_BUTTON = 2;   // mygtukas i GND, vidinis pull-up
const byte PIN_POT    = A0;  // jautrumo reguliatorius (vidurinis kontaktas)

// Potenciometras nera butinas. Jei jo grandineje NERA, palikite false:
// neprijungtas A0 kontaktas grazina atsitiktines reiksmes ir zonu ribos
// imtu nenuspejamai sokineti.
const bool USE_POT = false;
const int  FIXED_SENS = 100;   // jautrumas %, kai USE_POT = false

// ---------- Atstumo zonos (cm) ----------
const int DIST_SAFE   = 100;
const int DIST_WARN   = 50;
const int DIST_DANGER = 20;
const int DIST_MAX    = 300;   // laikoma, kad aido negauta

// ---------- Garsas ----------
const bool SOUND_ON = true;              // false isjungia garsa testavimo metu
const unsigned int TONE_CAUTION = 440;   // zemesni tonai maziau rezia ausi
const unsigned int TONE_WARNING = 523;
const unsigned int TONE_STOP    = 659;
const unsigned long BEEP_MS     = 40;    // vieno pyptelejimo trukme

// ---------- Skenavimo nustatymai ----------
const int SWEEP_MIN  = 15;
const int SWEEP_MAX  = 165;
const int SWEEP_STEP = 5;
const unsigned long STEP_MS = 60;

Servo radar;
SimpleLcd lcd;

enum Mode { MODE_SWEEP, MODE_PARK };
Mode mode = MODE_SWEEP;

int angle = 90;
int dir = 1;                   // sukimosi kryptis: +1 arba -1
int currentDist = DIST_MAX;

// Paskutinio pilno pravaziavimo rezultatas
int closestDist = DIST_MAX;
int closestAngle = 90;
// Maziausia reiksme, rasta siuo metu vykstanciame pravaziavime
int scanMinDist = DIST_MAX;
int scanMinAngle = 90;

unsigned long lastStep = 0, lastBeep = 0, lastLcd = 0, lastButtonChange = 0;
bool lastButtonState = HIGH;

// ---------- Jutiklis ----------
long readOnceCm() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);        // 10 us impulsas paleidzia ultragarso siuntima
  digitalWrite(PIN_TRIG, LOW);

  unsigned long us = pulseIn(PIN_ECHO, HIGH, 25000UL);  // laukiama iki ~4 m
  if (us == 0) return DIST_MAX;                          // aidas negrizo
  long cm = us / 58;            // garsas 1 cm ir atgal nukeliauja per ~58 us
  return (cm > DIST_MAX) ? DIST_MAX : cm;
}

// Triju matavimu mediana atmeta pavienius klaidingus rezultatus
int readDistance() {
  long a = readOnceCm();
  long b = readOnceCm();
  long c = readOnceCm();
  long lo = min(a, b);
  long hi = max(a, b);
  return (int)max(lo, min(hi, c));
}

// ---------- Griztamasis rysys ----------
void setColor(byte r, byte g, bool b) {
  analogWrite(PIN_RED, r);
  analogWrite(PIN_GREEN, g);
  digitalWrite(PIN_BLUE, b ? HIGH : LOW);
}

// Pyptelima leidzia tik praejus nurodytam laiko tarpui (be delay)
void beepEvery(unsigned long now, unsigned long interval, unsigned int freq) {
  if (now - lastBeep >= interval) {
    if (SOUND_ON) tone(PIN_BUZZER, freq, BEEP_MS);
    lastBeep = now;
  }
}

void updateAlert(int d) {
  unsigned long now = millis();

  if (d >= DIST_MAX) {                 // nieko nerasta
    setColor(0, 0, true);              // melyna
  } else if (d >= DIST_SAFE) {         // saugu
    setColor(0, 255, false);           // zalia
  } else if (d >= DIST_WARN) {         // atsargiai
    setColor(255, 150, false);         // geltona
    beepEvery(now, 700, TONE_CAUTION);
  } else if (d >= DIST_DANGER) {       // pavojinga: kuo arciau, tuo dazniau
    setColor(255, 40, false);          // oranzine
    unsigned long interval = map(d, DIST_DANGER, DIST_WARN, 120, 450);
    beepEvery(now, interval, TONE_WARNING);
  } else {                             // stok!
    setColor(255, 0, false);           // raudona
    beepEvery(now, 100, TONE_STOP);
  }
}

const char* statusText(int d) {
  if (d >= DIST_MAX)    return "CLEAR";
  if (d >= DIST_SAFE)   return "SAFE";
  if (d >= DIST_WARN)   return "CAUTION";
  if (d >= DIST_DANGER) return "WARNING";
  return "STOP!";
}

// ---------- LCD ----------
void updateLcd(int d) {
  char line[17];                       // 16 simboliu + baigiamasis nulis

  lcd.setCursor(0, 0);
  if (d >= DIST_MAX) {
    snprintf(line, sizeof(line), "%s ---cm     ",
             mode == MODE_SWEEP ? "Near:" : "Dist:");
  } else if (mode == MODE_SWEEP) {
    int a = (currentDist <= closestDist) ? angle : closestAngle;
    snprintf(line, sizeof(line), "Near:%3dcm @%3d ", d, a);
  } else {
    snprintf(line, sizeof(line), "Dist:%3dcm      ", d);
  }
  lcd.print(line);

  lcd.setCursor(0, 1);
  snprintf(line, sizeof(line), "%-6s %-9s",
           mode == MODE_SWEEP ? "SWEEP" : "PARK", statusText(d));
  lcd.print(line);
}

// ---------- Mygtukas (su debounce) ----------
void handleButton() {
  bool s = digitalRead(PIN_BUTTON);
  if (s != lastButtonState && millis() - lastButtonChange > 50) {
    lastButtonChange = millis();
    lastButtonState = s;
    if (s == LOW) {                    // paspausta
      if (mode == MODE_SWEEP) {
        mode = MODE_PARK;
        angle = 90;                    // PARK: ziuri tiesiai
        radar.write(angle);
      } else {
        mode = MODE_SWEEP;
      }
      closestDist = scanMinDist = DIST_MAX;
      lcd.clear();
    }
  }
}

// ---------- Setup / Loop ----------
void setup() {
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_RED, OUTPUT);
  pinMode(PIN_GREEN, OUTPUT);
  pinMode(PIN_BLUE, OUTPUT);
  pinMode(PIN_BUTTON, INPUT_PULLUP);   // isorinio rezistoriaus nereikia

  Serial.begin(9600);
  radar.attach(PIN_SERVO);
  radar.write(angle);

  lcd.begin();
  lcd.print("Radar Parking");
  lcd.setCursor(0, 1);
  lcd.print("Assist  v1.0");
  delay(1200);
  lcd.clear();
}

void loop() {
  handleButton();
  unsigned long now = millis();

  if (now - lastStep >= STEP_MS) {
    lastStep = now;

    // 1) Matuojamas atstumas esamu kampu
    currentDist = readDistance();

    if (mode == MODE_SWEEP) {
      if (currentDist < scanMinDist) {
        scanMinDist = currentDist;
        scanMinAngle = angle;
      }

      // 2) Servo pasukamas i kita kampa; pasiekus krasta paskelbiamas rezultatas
      angle += dir * SWEEP_STEP;
      if (angle >= SWEEP_MAX || angle <= SWEEP_MIN) {
        angle = constrain(angle, SWEEP_MIN, SWEEP_MAX);
        dir = -dir;                    // pasiekus krasta kryptis apsivercia
        closestDist = scanMinDist;
        closestAngle = scanMinAngle;
        scanMinDist = DIST_MAX;
      }
      radar.write(angle);
    }

    // "kampas,atstumas" - tinka Serial Plotter arba radaro vizualizacijai
    Serial.print(angle);
    Serial.print(',');
    Serial.println(currentDist);
  }

  int alertDist = (mode == MODE_SWEEP) ? min(currentDist, closestDist)
                                       : currentDist;
  // Jautrumo reguliatorius: 50-200 % keicia zonu ribas, o ne rodoma atstuma
  int sens = USE_POT ? map(analogRead(PIN_POT), 0, 1023, 50, 200) : FIXED_SENS;
  int scaled = (alertDist >= DIST_MAX) ? DIST_MAX
                                       : (int)((long)alertDist * 100 / sens);
  updateAlert(scaled);

  if (now - lastLcd >= 250) {          // ekranas atnaujinamas reciau, kad nemirgetu
    lastLcd = now;
    updateLcd(alertDist);
  }
}
