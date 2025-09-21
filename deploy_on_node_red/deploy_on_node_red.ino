#include <WiFi.h>
#include <PubSubClient.h>

// Wi-Fi credentials
const char* ssid = "ZTE_Hello";
const char* password = "al0952371687";

// MQTT broker IP
const char* mqtt_server = "192.168.0.84";

WiFiClient espClient;
PubSubClient client(espClient);

const char *topic_status = "working_status";
const char *topic_info = "information";

//pins
#define stepPin1 13
#define dirPin1 12
#define stepPin2 27
#define dirPin2 26
#define limit_sw1 2
#define limit_sw2 4

#define ldr 32

// Positioning
float x_axis;
float y_axis;
int limit_x;
int limit_y;
int i;
int ldrsensor;
int box_num = 0;
volatile int receivedBox = -1;
String boxType = "";  // "recive" or "sent"

int box_position[6][2] = {
  {92, 35}, {211, 35}, {313, 35},
  {92, 150}, {211, 150}, {313, 150}
};

int box_existence[6] = {0,0,0,0,0,0};

// =================== WiFi and MQTT ====================
void connectToWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected");
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect("ESP32Client")) {
      Serial.println(" connected");
      client.subscribe("box_number");  // Subscribe to receive box number
      client.subscribe("type");    // Subscribe to receive action type
    } else {
      Serial.print(" failed, rc=");
      Serial.print(client.state());
      delay(2000);
    }
  }
}

// MQTT callback function
void callback(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }

  if (String(topic) == "box_number") {
    receivedBox = msg.toInt();
    Serial.print("MQTT Received box number: ");
    Serial.println(receivedBox);
  }

  if (String(topic) == "type") {
    boxType = msg;
    Serial.print("MQTT Received box type: ");
    Serial.println(boxType);
  }
}

// =================== Motion Functions ====================
void home_position() {
  digitalWrite(dirPin1, LOW);
  digitalWrite(dirPin2, LOW);
  while (true) {
    limit_x = digitalRead(limit_sw1);
    limit_y = digitalRead(limit_sw2);


    if (limit_y == 0) digitalWrite(stepPin1, LOW);
    if (limit_x == 0) digitalWrite(stepPin2, LOW);

    delayMicroseconds(700);
    digitalWrite(stepPin1, HIGH);
    digitalWrite(stepPin2, HIGH);
    delayMicroseconds(700);

    if (limit_x == 1 && limit_y == 1) {
      x_axis = 0;
      y_axis = 0;
      break;
    }
  }
}

void move_to() {
  Serial.print("x_axis: ");
  Serial.println(x_axis);
  while (x_axis != box_position[box_num][0] || y_axis != box_position[box_num][1]) {
    
    if (y_axis < box_position[box_num][1]) {
      digitalWrite(dirPin1, HIGH); y_axis += 1;
    } else if (y_axis > box_position[box_num][1]) {
      digitalWrite(dirPin1, LOW); y_axis -= 1;
    }

    if (x_axis < box_position[box_num][0]) {
      digitalWrite(dirPin2, HIGH); x_axis += 1;
    } else if (x_axis > box_position[box_num][0]) {
      digitalWrite(dirPin2, LOW); x_axis -= 1;
    }

    for (i = 0; i <= 25; i++) {
      if (y_axis != box_position[box_num][1]) digitalWrite(stepPin1, HIGH);
      else if (y_axis == box_position[box_num][1]) digitalWrite(stepPin1, LOW);
      if (x_axis != box_position[box_num][0]) digitalWrite(stepPin2, HIGH);
      else if (x_axis == box_position[box_num][0]) digitalWrite(stepPin2, LOW);
      delayMicroseconds(700);
      digitalWrite(stepPin1, LOW);
      digitalWrite(stepPin2, LOW);
      delayMicroseconds(700);
    }

    if (x_axis == box_position[box_num][0] && y_axis == box_position[box_num][1]) {
      delay(1000);
      break;
    }
  }
}

void recive(int box) {
  move_to();

  // Pick/drop simulation
  digitalWrite(dirPin1, HIGH);
  for (i = 0; i <= 25 * 25; i++) {
    digitalWrite(stepPin1, HIGH); delayMicroseconds(700);
    digitalWrite(stepPin1, LOW); delayMicroseconds(700);
  }

  digitalWrite(dirPin1, LOW);
  for (i = 0; i <= 25 * 25; i++) {
    digitalWrite(stepPin1, HIGH); delayMicroseconds(700);
    digitalWrite(stepPin1, LOW); delayMicroseconds(700);
  }

  home_position();
}

void sent(int box) {

  home_position();
  delay(2000);

  for(int k=0 ; k<=5 ; k++){
    ldrsensor = digitalRead(ldr); 
    if (ldrsensor == 0) {
      delay(1000);
      move_to();

      // Pick/drop simulation
      digitalWrite(dirPin1, HIGH);
      for (i = 0; i <= 25 * 25; i++) {
        digitalWrite(stepPin1, HIGH); delayMicroseconds(700);
        digitalWrite(stepPin1, LOW); delayMicroseconds(700);
      }

      digitalWrite(dirPin1, LOW);
      for (i = 0; i <= 25 * 25; i++) {
        digitalWrite(stepPin1, HIGH); delayMicroseconds(700);
        digitalWrite(stepPin1, LOW); delayMicroseconds(700);
      }
      client.publish(topic_status,(String("sentding box number ") + String(box_num +1) + String(" is done")).c_str());
      break;
    }
    Serial.print("sent k: ");
    Serial.println(k);
    delay(1000);

    if(k == 5){
      client.publish(topic_status,(String("Error: can't detect the box")).c_str());
    }
  }
}

// =================== Setup and Loop ====================
void setup() {
  Serial.begin(9600);
  pinMode(stepPin1, OUTPUT); pinMode(dirPin1, OUTPUT);
  pinMode(stepPin2, OUTPUT); pinMode(dirPin2, OUTPUT);
  pinMode(limit_sw1, INPUT); pinMode(limit_sw2, INPUT);
  pinMode(ldr, INPUT);

  connectToWiFi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  reconnectMQTT();
  client.publish(topic_status,String("setup position").c_str());
  home_position();
}

void loop() {
  if (!client.connected()) reconnectMQTT();
  client.loop();

  ldrsensor = digitalRead(ldr);
  Serial.println(ldrsensor);

  if (receivedBox >= 0 && receivedBox <= 6 && boxType == "recive") {
    box_num = receivedBox-1;
    Serial.print("Moving to box (recive): ");
    Serial.println(box_num);
    client.publish(topic_status,(String("reciving box number ") + String(box_num +1)).c_str());
    recive(box_num);
    box_existence[box_num] = 0;
    client.publish(topic_status,(String("reciving box number ") + String(box_num +1) + String(" is done")).c_str());
    receivedBox = -1;
    //boxType = ""; // reset
  }

  if (receivedBox >= 0 && receivedBox <= 6 && boxType == "sent") {
    box_num = receivedBox-1;
    Serial.print("Moving to box (sent): ");
    Serial.println(box_num);
    client.publish(topic_status,(String("sentding box number ") + String(box_num +1)).c_str());
    sent(box_num);
    box_existence[box_num] = 1;
    // client.publish(topic_status,(String("sentding box number ") + String(box_num +1) + String(" is done")).c_str());
    receivedBox = -1;
    //boxType = ""; // reset
  }

  String infoArray = "[";  // Start JSON array

  for (int i = 0; i < 6; i++) {
    infoArray += "{";
    infoArray += "\"box_number\": \"" + String(i + 1) + "\",";
    infoArray += "\"status\": " + String(box_existence[i]);
    infoArray += "}";

    if (i < 5) infoArray += ",";  // Add comma between items, not after last
  }

  infoArray += "]";  // End JSON array

  client.publish(topic_info, infoArray.c_str());

  delay(100);
}
