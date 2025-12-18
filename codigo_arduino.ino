#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>
#include <DallasTemperature.h>
#include <OneWire.h>

#define ONE_WIRE_BUS 2
#define SQW_PIN 3  

LiquidCrystal_I2C lcd(0x27, 16, 2);
RTC_DS3231 rtc;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

int contador = 0;
bool medindo = false;
float t0 = 0;
int rele_1 = 7;
int rele_2 = 8;
bool testeluna = true;
int tempo_1 = 400;
int tempo_2 = 400;

volatile byte pulsos = 0;
volatile bool fazerMedicao = false;

void ISR_RTC() {
  pulsos++;
  if (pulsos >= 5) { 
    pulsos = 0;
    fazerMedicao = true;
  }
}

void medir() {
  sensors.requestTemperatures();
  float temp = sensors.getTempCByIndex(0);
  
  lcd.setCursor(0, 0);
  lcd.print("Tempo:");
  lcd.setCursor(7, 0);
  lcd.print(contador * 5);
  lcd.print("s ");
  
  lcd.setCursor(0, 1);
  lcd.print("Temp:");
  lcd.setCursor(6, 1);
  lcd.print(temp, 1);
  lcd.write(223);
  lcd.print("C ");
  
  Serial.print(temp, 2);
}

void ligadesliga() {
  static unsigned long ultimoTempo = 0;
  static bool estado = HIGH;
  unsigned long agora = micros();
  
  if (estado == HIGH && agora - ultimoTempo >= tempo_2) {
    digitalWrite(rele_1, LOW);
    digitalWrite(rele_2, LOW);
    estado = LOW;
    ultimoTempo = agora;
  } 
  else if (estado == LOW && agora - ultimoTempo >= tempo_1) {
    digitalWrite(rele_1, HIGH);
    digitalWrite(rele_2, HIGH);
    estado = HIGH;
    ultimoTempo = agora;
  }
}

void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("Iniciando RTC...");
  
  sensors.begin();
  
  if (!rtc.begin()) {
    lcd.clear();
    lcd.print("ERRO: RTC!");
    while (1);
  }
  
  rtc.writeSqwPinMode(DS3231_SquareWave1Hz);
  
  pinMode(rele_1, OUTPUT);
  pinMode(rele_2, OUTPUT);
  digitalWrite(rele_1, LOW);
  digitalWrite(rele_2, LOW);
  
  pinMode(SQW_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(SQW_PIN), ISR_RTC, FALLING);
  
  sensors.requestTemperatures();
  delay(1000);
  
  lcd.clear();
  lcd.print("Pronto!");
}

void loop() {
  if (Serial.available()) {
    String comando = Serial.readStringUntil('\n');
    comando.trim();
    
    if (comando == "b") {
      
      noInterrupts();  
      pulsos = 0;
      fazerMedicao = false;
      interrupts();    
      
      contador = 0;
      testeluna = true;
      tempo_1 = 400;
      tempo_2 = 400;
      medindo = true;
      lcd.clear();
      
      sensors.requestTemperatures();
      delay(100);
      t0 = sensors.getTempCByIndex(0);
      
      
    } 
    else if (comando == "d") {
      medindo = false;
      lcd.clear();
      lcd.print("Parado");
    }
  }
  
  if (!medindo) return;
  
  if (fazerMedicao) {
    fazerMedicao = false;  
    
    //Serial.print(contador);
    //Serial.print(" ");
    
    medir();
    
    if (testeluna) ligadesliga();
    
    float tempAtual = sensors.getTempCByIndex(0);
    float tempEsperada = (contador / 60.0) + t0;
    
    if (tempAtual >= tempEsperada + 1) {
      testeluna = false;
    }
    if (tempAtual < tempEsperada - 1) {
      testeluna = true;
    }
    
    if (tempAtual >= 75) {
      tempo_1 = 800;
      tempo_2 = 250;
    }
    
    contador++;
    Serial.println();
  }
}
