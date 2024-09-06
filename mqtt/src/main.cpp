#include <WiFi.h>  // Librería para ESP32 WiFi
#include <PubSubClient.h>
#include <ArduinoJson.h>

// Configuraciones para la red WiFi y el servidor MQTT
const char* ssid = "HUAWEI-IoT";
const char* password = "ORTWiFiIoT";
const char* mqtt_server = "demo.thingsboard.io";
const char* token = "qvZ204btlqydKSwKs2kd";

// Objetos de conexión
WiFiClient espClient;
PubSubClient client(espClient);

// Timestamp de la última actualización de telemetría
unsigned long lastMsg = 0;

// Tamaños de buffer y mensajes
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];

// Documento JSON para almacenar los mensajes entrantes
DynamicJsonDocument incoming_message(256);

// Telemetría falsa
int value = 0;

// Estado del LED
boolean estado = false;
boolean status = false;

// Función para iniciar la conexión WiFi
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando a ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi conectado");
  Serial.println("Dirección IP: ");
  Serial.println(WiFi.localIP());
}

// Función que se ejecuta cuando llega un mensaje MQTT
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensaje recibido [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  String _topic = String(topic);

  if (_topic.startsWith("v1/devices/me/rpc/request/")) {
    String _number = _topic.substring(26);

    // Leer el mensaje JSON
    deserializeJson(incoming_message, payload);
    String metodo = incoming_message["method"];

    if (metodo == "getState") {  // Verificar estado del dispositivo
      char outTopic[128];
      ("v1/devices/me/rpc/response/" + _number).toCharArray(outTopic, 128);

      DynamicJsonDocument resp(256);
      resp["status"] = status;
      char buffer[256];
      serializeJson(resp, buffer);
      client.publish(outTopic, buffer);

    } else if (metodo == "setState") {  // Encender/apagar LED
      boolean estado = incoming_message["params"];
      if (estado) {
        digitalWrite(LED_BUILTIN, HIGH);  // Encender LED
        status = true;

      } else {
        digitalWrite(LED_BUILTIN, LOW);  // Apagar LED
        status = !status;
      }
      // Actualización de atributos
      DynamicJsonDocument resp(256);
      resp["estado"] = estado;
      char buffer[256];
      serializeJson(resp, buffer);
      client.publish("v1/devices/me/attributes", buffer);
      Serial.print("Publicar mensaje [atributo]: ");
      Serial.println(buffer);

    } else if(metodo == "getState"){
      DynamicJsonDocument stateBuffer(256);
      stateBuffer["estado"] = status;
      char buffer[256];
      serializeJson(stateBuffer, buffer);
      client.publish("v1/devices/me/rpc/response/...", buffer);
      Serial.print("Publicar mensaje [atributo]: ");
      Serial.println(buffer);
    }
  }
}

// Función para reconectar al servidor MQTT
void reconnect() {
  while (!client.connected()) {
    Serial.print("Intentando conexión MQTT...");
    if (client.connect("ESP32Device", token, token)) {
      Serial.println("conectado");
      client.subscribe("v1/devices/me/rpc/request/+");
    } else {
      Serial.print("fallido, rc=");
      Serial.print(client.state());
      Serial.println(" intentando de nuevo en 5 segundos");
      delay(5000);
    }
  }
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);  // Inicializar el pin LED_BUILTIN como salida
  Serial.begin(921600);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    value = random(100);

    // Enviar valor como telemetría
    DynamicJsonDocument resp(256);
    resp["value"] = value;
    char buffer[256];
    serializeJson(resp, buffer);

    client.publish("v1/devices/me/telemetry", buffer);

    Serial.print("Publicar mensaje [telemetría]: ");
    Serial.println(buffer);
  }
}
