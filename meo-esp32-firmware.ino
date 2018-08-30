// MEO cho esp 32
// Các thư viện có số 2 ở cuối đều là các thư viện đã chỉnh sửa để chạy được trên esp32, em đã đổi tên thư viện để tránh gây conflict
// nguồn libary : https://github.com/zhouhan0126/WIFIMANAGER-ESP32 
#include <WiFi.h>
#include <PubSubClient.h>
#include <WebServer2.h>
#include <WiFiManager2.h>
#include <ESPmDNS.h>
#include <DNSServer2.h>
#include "DHT.h"
#include <esp_int_wdt.h>
#include <esp_task_wdt.h>

#define analogWrite ledcWrite
/*
#define D0 IO12 12
#define D! IO13 13
#define D2 IO14 14
#define D3 IO15 15
#define D4 IO16 16
#define D5 IO17 17
#define D6 IO18 18
#define D7 IO19 19
#define D8 IO21 21
#define A0 27*/

// ======= Thay đổi các thông tin ở đây =========

const char* default_mqtt_server = "broker.hivemq.com";
const char* default_mqtt_port = "1883";
const char* device_id = "meo_esp32";
/*const char* ssid = "FPT Telecom-453E";
const char* password = "123456788";*/
// ==============================================

char mqtt_server[255];
char mqtt_port[6];
const char* channel_in_postfix = "/in";
const char* channel_out_postfix = "/out";
String channel_in = "esp/", channel_out = "esp/";
String hotspot_name_prefix = "ESP_8266_";

// Initialize DHT sensor
const int DHTPIN = 27;       //Đọc dữ liệu từ DHT11 ở chân A0 trên ESP
const int DHTTYPE = DHT11;  //Khai báo loại cảm biến, có 2 loại là DHT11 và DHT22
DHT dht(DHTPIN, DHTTYPE); // 11 works fine for ESP8266

float humidity, temp_c;   // Values read from sensor
// Generally, you should use "unsigned long" for variables that hold time
unsigned long previousMillis = 0;        // will store last temp was read
const long cnt_sensor = 2000;              // interval at which to read sensor - See more at: http://www.esp8266.com/viewtopic.php?f=29&t=8746#sthash.IJ0JNSIx.dpuf
unsigned long preTimer = 0;
const long interval = 5000;             // interval to publish data each time.

int val = 0;
int pirState = LOW; // Start with no caution

WiFiClient espClient;
PubSubClient client(espClient);
char hostString[16] = {0};
char IP_Server_char[20];
int Port_Server;
IPAddress ip;


void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0'; // Null terminator used to terminate the char array
  String message = (char*)payload;
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  for (int i = 0; i < length; i++) {  // print message converted from byte to char
    Serial.print((char)payload[i]);
  }
  Serial.println(" ");
  int A[10][2];
  int index1 = 0, index2 = 0;
  int temp = 0;
  int prep = -1;
  for (int i = 0; i < length; i++)
  {
    char c = payload[i];
    if (c >= '0' && c <= '9') {
      temp = temp * 10 + (c - '0');
    }
    else if (c == '.') {
      A[index1][index2] = temp;
      temp = 0;
      index2 = (index2 + 1 ) % 2;
    }
    else if (c == ';')
    {
      if (i == prep + 1)
      {
        A[index1][0] = -1;
        A[index1][1] = -1;
      }
      else
      {
        A[index1][index2] = temp;

      }
      temp = 0;
      index2 = 0;
      index1 ++;
      prep = i;
    }

  }
  if (prep + 1 == length) {
    A[index1][0] = -1;
    A[index1][1] = -1;
  } else {
    A[index1][index2] = temp;
  }


  for (int i = 0; i < 10 ; i++)
  {
    if (A[i][0] == 0)
    {
      switch (i)
      {
        case 0:
          if (A[i][1] == 0)
            digitalWrite(12, LOW);
          else
            digitalWrite(12, HIGH);
          break;
        case 1:
          if (A[i][1] == 0)
            digitalWrite(13, LOW);
          else
            digitalWrite(13, HIGH); break;
        case 2:
          if (A[i][1] == 0)
            digitalWrite(14, LOW);
          else
            digitalWrite(14, HIGH); break;
        case 3:
          if (A[i][1] == 0)
            digitalWrite(15, LOW);
          else
            digitalWrite(15, HIGH); break;
        case 4:
          if (A[i][1] == 0)
            digitalWrite(16, LOW);
          else
            digitalWrite(16, HIGH); break;
      }
    }
    else if (A[i][0] == 1)
    {
      switch (i)
      {
        case 0: analogWrite(12, A[i][1]); break;
        case 1: analogWrite(13, A[i][1]); break;
        case 2: analogWrite(14, A[i][1]); break;
        case 3: analogWrite(15, A[i][1]); break;
        case 4: analogWrite(16, A[i][1]); break;
        case 5: Function_F1(); break;
        case 6: Function_F2(); break;
        case 7: Function_F3(); break;
        case 8: Function_F4(); break;
        case 9: Function_F5(); break;

      }
    }
  }

}

//////////////////////////////PUB/////////////////
void send_data() {
  int IO17 = digitalRead(17);
  int IO18 = digitalRead(18);
  int IO19 = digitalRead(19);
  int IO21 = digitalRead(21);
  int A0 = analogRead(27);
  int U1 = Virtual_U1();
  int U2 = Virtual_U2();
  int U3 = Virtual_U3();
  int U4 = Virtual_U4();
  int U5 = Virtual_U5();
  String pubString = String(IO17) + ';' + String(IO18) + ';' + String(IO19) + ';' + String(IO21) + ';' + String(A0) + ';' + String(U1) + ';' + String(U2) + ';' + String(U3) + ';' + String(U4) + ';' + String(U5) ;
  
  unsigned long curTimer = millis();
  if (curTimer - preTimer >= interval)  // set time cycle to publish data: each 5s
  {
    preTimer = curTimer;
    client.publish(channel_out.c_str(), pubString.c_str());
    Serial.print("Message published [");
    Serial.print(channel_out);
    Serial.print("]: ");
    Serial.println(pubString);
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    //    if (client.connect("ESP8266Client")) {
    if (client.connect(device_id)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      //      client.publish("ESP8266/connection status", "Connected!");
      //      client.publish(channel_out.c_str(), device_id);
      // ... and resubscribe
      //      client.subscribe("ESP8266/LED status");
      client.subscribe(channel_in.c_str());
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void gettemperature() {
  // Wait at least 2 seconds seconds between measurements.
  // if the difference between the current time and last time you read
  // the sensor is bigger than the interval you set, read the sensor
  // Works better than delay for things happening elsewhere also
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= cnt_sensor) {
    // save the last time you read the sensor
    previousMillis = currentMillis;

    // Reading temperature for humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (it's a very slow sensor)
    humidity = dht.readHumidity();          // Read humidity (percent)
    temp_c = dht.readTemperature();     // Read temperature as Celcius
    // Check if any reads failed and exit early (to try again).
    /*    if (isnan(humidity) || isnan(temp_c)) {
          Serial.println("Failed to read from DHT sensor!");
          return;
        }*/
  }
}


//////////////////////// Virtual output//////////////////
inline unsigned char Virtual_U1() {
  gettemperature();
  return temp_c;
}

inline unsigned char Virtual_U2() {
  gettemperature();
  return humidity;
}

inline unsigned char Virtual_U3() {
  return 1;
}

inline unsigned char Virtual_U4() {
  return 1;
}

inline unsigned char Virtual_U5() {
  return 1;
}

/////////////////////////////// Function input/////////////////////
inline void Function_F1() {
  Serial.println("F1");
}

inline void Function_F2() {
  Serial.println("F2");
}

inline void Function_F3() {
  Serial.println("F3");
}
inline void Function_F4() {
  Serial.println("F4");
}

inline void Function_F5() {
  Serial.println("F5");
}

/*==================== Setup ============================*/
void setup() {
  hotspot_name_prefix += device_id;
  
  pinMode(12, OUTPUT);    // Initialize the BUILTIN_LED pin as an output
  pinMode(13, OUTPUT);
  pinMode(14, OUTPUT);
  pinMode(15, OUTPUT);
  pinMode(16, OUTPUT);

  pinMode(17, INPUT);
  pinMode(18, INPUT);
  pinMode(19, INPUT);
  pinMode(21, INPUT);
  pinMode(27, INPUT);
  Serial.begin(115200);
  WiFiManager wifiManager;
  WiFiManagerParameter custom_text("<br/><p>Enter MQTT Server/IP and Port Number</p>");
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", default_mqtt_server, 255);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", default_mqtt_port, 6);
  wifiManager.addParameter(&custom_text);
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.resetSettings(); //uncomnent khi muốn để default mqtt broker
  if (!wifiManager.autoConnect(hotspot_name_prefix.c_str())) {
    Serial.println("failed to connect, we should reset as see if it connects");
    delay(3000);
    hard_restart(); // ESP.reset();
    delay(5000);
  }
 

  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());

  Serial.print("mqtt server: ");
  Serial.println(mqtt_server);
  Serial.print("mqtt port: ");
  Serial.println(atoi(mqtt_port));
  
  Serial.println("connected...yeey :)");
  Serial.println("local ip");
  Serial.println(WiFi.localIP());

  // ============== This section is for mDNS =================
  // setup_wifi();
  /*
  Serial.println("\r\nsetup()");
  sprintf(hostString, "ESP_%06X", ESP.getChipId());
  Serial.print("Hostname: ");
  Serial.println(hostString);
  WiFi.hostname(hostString);
  if (!MDNS.begin(hostString)) {
    Serial.println("Error setting up MDNS responder!");
  }
  Serial.println("mDNS responder started");
  //MDNS.addService("esp", "tcp", 8080); // Announce esp tcp service on port 8080

  Serial.println("Sending mDNS query");
  int n = MDNS.queryService("mqtt", "tcp"); // Send out query for esp tcp services
  Serial.println("mDNS query done");
  if (n == 0) {
    Serial.println("no services found");
  }
  else {
    Serial.print(n);
    Serial.println(" service(s) found");
    //    for (int i = 0; i < n; ++i) {
    // Print details for each service found
    //      Serial.print(i + 1);
    //      Serial.print(": ");
    Serial.print(MDNS.hostname(0));
    Serial.print(" (");
    Serial.print(MDNS.IP(0));
    Serial.print(":");
    Serial.print(MDNS.port(0));
    Serial.println(")");
    }
    
  ///////////////// Get IP Adress and Port Adress/////////////
  IPAddress ip = MDNS.IP(0); // Get IPAddress of Server
  String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]); // convert IP Adress to String
  ipStr.toCharArray(IP_Server_char, 20);  // convert IP to char
  Serial.println(IP_Server_char);
  Port_Server = MDNS.port(0);
  Serial.println(Port_Server);
  //  client.setServer(IP_Server_char, Port_Server);
  */
  // ========================================================
  client.setServer(mqtt_server, atoi(mqtt_port));
  Serial.println();

  channel_in += device_id;          // "esp/NodeMcuEsp01"
  channel_in += channel_in_postfix; // "esp/NodeMcuEsp01/in"
  Serial.println("Subscribe channel: ");
  Serial.println(channel_in);

  channel_out += device_id;           // "esp/NodeMcuEsp01"
  channel_out += channel_out_postfix; // "esp/NodeMcuEsp01/out"
  Serial.println("Channel out: ");
  Serial.println(channel_out);

  Serial.println("loop() next");
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  send_data();
}

void hard_restart() { // hàm reset cho esp 32
  esp_task_wdt_init(1,true);
  esp_task_wdt_add(NULL);
  while(true);
}

