#include <SPI.h>
#include <MFRC522.h>

// ---------- Pines ----------
#define SS_PIN    10
#define RST_PIN   9
#define LED_VERDE 6
#define LED_ROJO  7

// ---------- Configuración ----------
const unsigned long TIEMPO_LED = 3000;   // 3 segundos

// UID autorizado (tu tarjeta)
byte uidAutorizado[] = {0xC7, 0xB4, 0x27, 0x3C};
const byte TAM_UID = sizeof(uidAutorizado);

MFRC522 rfid(SS_PIN, RST_PIN);

// ---------- Control de LEDs con millis() ----------
unsigned long inicioLed = 0;
bool ledActivo = false;
int  pinActivo = -1;

void encenderLed(int pin) {
  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_ROJO, LOW);
  digitalWrite(pin, HIGH);
  pinActivo = pin;
  inicioLed = millis();
  ledActivo = true;
}

void actualizarLed() {
  if (ledActivo && (millis() - inicioLed >= TIEMPO_LED)) {
    digitalWrite(pinActivo, LOW);
    ledActivo = false;
    pinActivo = -1;
  }
}

// ---------- Utilidades ----------
void imprimirUID(byte *uid, byte tam) {
  Serial.print("UID:");
  for (byte i = 0; i < tam; i++) {
    Serial.print(' ');
    if (uid[i] < 0x10) Serial.print('0');
    Serial.print(uid[i], HEX);
  }
  Serial.println();
}

bool uidCoincide(byte *uid, byte tam) {
  if (tam != TAM_UID) return false;
  for (byte i = 0; i < tam; i++) {
    if (uid[i] != uidAutorizado[i]) return false;
  }
  return true;
}

bool lectorResponde() {
  byte version = rfid.PCD_ReadRegister(MFRC522::VersionReg);
  Serial.print("Version del chip: 0x");
  Serial.println(version, HEX);
  // 0x00 o 0xFF = no hay comunicacion SPI
  return (version != 0x00 && version != 0xFF);
}

// ---------- Setup ----------
void setup() {
  Serial.begin(9600);
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_ROJO, OUTPUT);
  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_ROJO, LOW);

  SPI.begin();
  rfid.PCD_Init();
  delay(50);

  if (lectorResponde()) {
    Serial.println("Lector RC522 detectado: comunicacion SPI OK");
    Serial.println("Acerque una tarjeta...");
  } else {
    Serial.println("ERROR: no hay comunicacion con el RC522.");
    Serial.println("Revise cableado (MISO, MOSI, SCK, SDA, RST, 3.3V, GND).");
  }
}

// ---------- Loop ----------
void loop() {
  actualizarLed();   // apaga el LED a los 3 s sin bloquear

  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial())   return;

  imprimirUID(rfid.uid.uidByte, rfid.uid.size);

  if (uidCoincide(rfid.uid.uidByte, rfid.uid.size)) {
    Serial.println("ACCESO PERMITIDO");
    encenderLed(LED_VERDE);
  } else {
    Serial.println("ACCESO DENEGADO");
    encenderLed(LED_ROJO);
  }

  rfid.PICC_HaltA();        // detiene la tarjeta
  rfid.PCD_StopCrypto1();   // termina la comunicacion
}