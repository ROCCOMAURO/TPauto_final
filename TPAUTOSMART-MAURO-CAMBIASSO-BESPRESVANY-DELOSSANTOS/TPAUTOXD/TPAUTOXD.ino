#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_AHTX0.h>
#include <NTPClient.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <Preferences.h>

#include <PubSubClient.h>  

const char* ssid = "AndroidAP";
const char* password = "douu7733";

#define BOTtoken "6831901316:AAFT_fFwLBDhuod6jQc3p6pWZCWHdmzC33I"
#define CHAT_ID "-4182536698"
WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

WiFiClient espClient;

PubSubClient clientMQTT(espClient);  // Cliente MQTT

const char* mqttServer = "test.mosquitto.org";  
const int mqttPort = 1883; 

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", -3 * 3600, 60000);  // GMT-3

#define BUZZER 15
#define xAxis 35
#define yAxis 34
#define SENLDR 32
#define GUINO1 27
#define GUINO2 4
#define LED1 33
#define LED2 25
#define BTNJOY 19
#define BTN1 12
#define BTN2 13
#define PUERTA 14

Preferences preferences;  // creacion de una instancia

enum estadoactual {
  PANTALLA1,
  cambiarscreen1,
  PANTALLA2,
  cambiarscreen2,
  PANTALLA3,
  cambiarscreen3,
  PANTALLA4,
  volver1,
  atras1,
  atras2,
  atras3,
  atras4,
  cambiarscreen4,
  PANTALLA5,  
  atras5
};

Adafruit_AHTX0 aht10;
sensors_event_t aht10Temp, aht10Hum;

float temperature;
float humedad;
float lasttemp = 0;
float lasthumedad = 0;
float lastLuz = 0;

long tiempoUltimoCambioTemp = 0;
int intervaloTemp = 10000;
int umbral = 20;
int intervaloMQTT = 10;
unsigned long lastButtonPress1 = 0;
unsigned long lastButtonPress2 = 0;
unsigned long delayxd = 200;  
unsigned long lastMQTTPublish = 0;

bool mandado;

bool btn1Pressed = false; 

int lecturapuerta;

bool estadoPuerta = false; 
unsigned long tiempo2 = 0;
unsigned long tiempoUltimoMensaje = 0;

estadoactual estado = PANTALLA1;

LiquidCrystal_I2C lcd(0x27, 16, 2);

long offset = -3 * 3600;

void setup() {
  pinMode(BUZZER, OUTPUT);
  pinMode(BTNJOY, INPUT_PULLUP);
  pinMode(BTN1, INPUT_PULLUP);
  pinMode(BTN2, INPUT_PULLUP);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(GUINO1, OUTPUT);
  pinMode(GUINO2, OUTPUT);
  pinMode(PUERTA, INPUT_PULLUP);

  Serial.begin(115200);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("TP AUTOSMART");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  
#ifdef ESP32
client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
#endif

bot.sendMessage(CHAT_ID, "La chispa esta aca", "");

if (!aht10.begin()) {
Serial.println("Advertencia: Sensor AHT10 no inicializado.");
} else {
  Serial.println("Sensor AHT10 inicializado.");
}

  Wire.begin();
  timeClient.begin();

  preferences.begin("guardado", false);
  umbral = preferences.getUInt("valor_umbral", 20);

  intervaloMQTT = preferences.getUInt("valor_mqtt", 10);

 clientMQTT.setServer(mqttServer, mqttPort);
  while (!clientMQTT.connected()) {
    Serial.print("Conectando al broker MQTT...");
    if (clientMQTT.connect("ESP32Client")) {
      Serial.println("Conectado al broker MQTT");
    } else {
      Serial.print("Error de conexión: ");
      Serial.println(clientMQTT.state());
      delay(2000); // Intenta nuevamente después de 5 segundos
    }
  }

   clientMQTT.publish("lospibes/sensores", "Test message");

}

void loop() {
  clientMQTT.loop(); // tamo activo papi
  
  timeClient.update();
  int xValue = analogRead(xAxis);
  maquina(xValue);
  temperatura();
  LDR();  
  puerta();

}

void maquina(int xValue) {
  switch (estado) {

    case PANTALLA1:
      menu1();
      if (xValue >= 2100) {
        lcd.clear();
        estado = cambiarscreen1;
      }
      if (xValue <= 0) {
      lcd.clear();
      estado = atras1;
      }
      break;
    
    case atras1:
    if (xValue > 400) {
    estado = PANTALLA5;
    }
    break;

    case cambiarscreen1:
      if (xValue < 1200) {
        estado = PANTALLA2;
      }
      break;

    case PANTALLA2:
      menu2();
      if (xValue >= 2100) {
        lcd.clear();   
        estado = cambiarscreen2;
      }
      if (xValue <= 0) {
      lcd.clear();
      estado = atras2;
      }
      break;

    case atras2:
    if (xValue > 400) {
    estado = PANTALLA1;
    }
    break;

    case cambiarscreen2:
      if (xValue < 1200) {
        estado = PANTALLA3;
      }
      break;

case PANTALLA3:
  menu3(); 
  if (xValue >= 2100) {
  lcd.clear();
  estado = cambiarscreen3;
  }
  if (xValue <= 0) {
  lcd.clear();
  estado = atras3;
  }

  if (digitalRead(BTN1) == HIGH && (millis() - lastButtonPress1 > delayxd)) {
    lastButtonPress1 = millis();
    offset += 3600;  // +1 hora 
    timeClient.setTimeOffset(offset);
  }
  if (digitalRead(BTN2) == HIGH && (millis() - lastButtonPress2 > delayxd)) {
    lastButtonPress2 = millis();
    offset -= 3600;  // -1 hora 
    timeClient.setTimeOffset(offset);
  }
  break;

    case atras3:
    if (xValue > 400) {
    estado = PANTALLA2;
    }
    break;

    case cambiarscreen3:
      if (xValue < 1200) {
        estado = PANTALLA4;
        lcd.clear();
      }
      break;

    case PANTALLA4:
    menu4();
    preferences.putUInt("valor_umbral", umbral);
    if (xValue >= 2100) {    
    estado = cambiarscreen4;
    }   
    if (xValue <= 0) {
    lcd.clear();
    estado = atras4;
    }  

    if (digitalRead(BTN1) == HIGH && (millis() - lastButtonPress1 > delayxd)) {
    lastButtonPress1 = millis();  
    umbral++;  
  }
    if (digitalRead(BTN2) == HIGH && (millis() - lastButtonPress2 > delayxd)) {
    lastButtonPress2 = millis();  
    umbral--;  
  }
    break;

    case atras4:
    if (xValue > 400) {
    estado = PANTALLA3;
    }
    break;

      case cambiarscreen4:
      if (xValue < 1200) {
        estado = PANTALLA5;
        lcd.clear();
      }
      break;

    case PANTALLA5:
  menu5();
  preferences.putUInt("valor_mqtt", intervaloMQTT);

  if (xValue <= 0) {
    lcd.clear();
    estado = atras5;
  } 
  if (xValue >= 2100) {    
    estado = volver1;
  } 

  if (digitalRead(BTN1) == HIGH && (millis() - lastButtonPress1 > delayxd)) {
    lastButtonPress1 = millis();  
    intervaloMQTT += 10000; 
  }

  if (digitalRead(BTN2) == HIGH && (millis() - lastButtonPress2 > delayxd)) {
    lastButtonPress2 = millis();  
    intervaloMQTT -= 10000;  
    if (intervaloMQTT < 10000) intervaloMQTT = 10000;  // Mínimo 10 segundos
  }

  // Verificar si ha pasado el intervalo para enviar la temperatura
  if (millis() - lastMQTTPublish >= intervaloMQTT) {
    lastMQTTPublish = millis();  // Actualizar el tiempo de la última publicación

    // Obtener los valores de temperatura y humedad
    if (aht10.getEvent(&aht10Hum, &aht10Temp)) {
      temperature = aht10Temp.temperature;
      humedad = aht10Hum.relative_humidity;

      // Crear el mensaje y publicarlo
      String mensaje = "Temp: " + String(temperature, 1) + "C, Humedad: " + String(humedad, 1) + "%";
      bool resultado = clientMQTT.publish("lospibes/sensores", mensaje.c_str());

      if (resultado) {
        Serial.println("Mensaje publicado: " + mensaje);
      } else {
        Serial.println("Error al publicar el mensaje MQTT.");
      }
    }
  }
      break;

    case atras5:
      if (xValue > 400) {
        estado = PANTALLA4;
      }
      break;

    case volver1:
      if (xValue < 1200) {
        estado = PANTALLA1;
      }
      break;

  }
}


void menu1() {

  if (aht10.getEvent(&aht10Hum, &aht10Temp)) {
    temperature = aht10Temp.temperature;
    humedad = aht10Hum.relative_humidity;

    if (temperature != lasttemp) {
      lcd.setCursor(0, 0);
      lcd.print("Temp: ");
      lcd.print(temperature, 1);
      lcd.print(" C   ");
      lasttemp = temperature;
    }

    if (humedad != lasthumedad) {
      lcd.setCursor(0, 1);
      lcd.print("Humedad: ");
      lcd.print(humedad, 1);
      lcd.print("%   ");
      lasthumedad = humedad;
    }
} 
}


void menu2() {

  float lectura = analogRead(SENLDR) / 10.0;

  lcd.setCursor(0, 0);
  lcd.print("Luz: ");
  lcd.print(lectura, 1);
  lcd.print("   ");
  lastLuz = lectura;

    lcd.setCursor(0, 1);
    lcd.print("Corriente: -- A");
}


void menu3() {
  timeClient.update();
  String formattedTime = timeClient.getFormattedTime();
  lcd.setCursor(0, 0);
  lcd.print("Hora: ");
  lcd.setCursor(0, 1);
  lcd.print(formattedTime);
}

void menu4() {
  lcd.setCursor(0, 0);
  lcd.print("Umbral: ");
  lcd.print(umbral);
  lcd.setCursor(0, 1);
  lcd.print("1: SUBE 2: BAJA ");
}

void menu5() {
  lcd.setCursor(0, 0);
  lcd.print("MQTT:");
  lcd.setCursor(0, 1);
  lcd.print(intervaloMQTT / 1000);
  lcd.print(" s ");
}

void temperatura() {
  if (millis() - tiempoUltimoCambioTemp >= intervaloTemp ) {
    tiempoUltimoCambioTemp = millis();
    temperature = aht10Temp.temperature;
    if (temperature >= umbral && mandado == false) {
      bot.sendMessage(CHAT_ID, "Cuidado, la temperatura es mayor al umbral: " + String(temperature) + "°C");
      mandado = true;

    }
    else{
      mandado = false;
    }}
}

void puerta() {
  
  int lecturapuerta = digitalRead(PUERTA);

  if (lecturapuerta == LOW) {
    digitalWrite(GUINO1, HIGH);
    digitalWrite(GUINO2, HIGH);
    tone(BUZZER, 500);

    if (millis() - tiempoUltimoMensaje >= 3000) {
      tiempoUltimoMensaje = millis();
      bot.sendMessage(CHAT_ID, "¡CUIDADO PUERTAS ABIERTAS!");
    }
  }
   else {
    digitalWrite(GUINO1, LOW);
    digitalWrite(GUINO2, LOW);
    noTone(BUZZER);
  }
}

void LDR() {
  float lectura = analogRead(SENLDR) / 10.0;
  if (lectura >= 50) {
    digitalWrite(LED1, LOW);   
    digitalWrite(LED2, LOW);   
  } else  {
    digitalWrite(LED1, HIGH);   
    digitalWrite(LED2, HIGH);
    }
}
