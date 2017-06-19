#include <SPI.h>
#include <Ethernet2.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Servo.h>
#include <Adafruit_NeoPixel.h>
#include <String.h>

/*
 *  Servo
 */
Servo myservo;

/*
 *  Neo Pixel
 */
#define PIN            6
#define NUMPIXELS      8
int delayval = 100;
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);


#define DHT_pin 4
#define DHTTYPE DHT11

#define SW1 12  //door
#define SW2 13  //led
#define SW3 9   //rain

// DHT11
DHT dht(DHT_pin, DHTTYPE);

// Update these with values suitable for your network.
const char* mqtt_server = "192.168.0.196";
// Update these with values suitable for your network.
byte mac[]    = {  0x00, 0x08, 0xDC, 0x1D, 0x6A, 0x62 };
EthernetClient ethClient;
PubSubClient client(ethClient);
long lastMsg = 0;
char msg[50];
int value = 0;

void setup() {
  pixels.begin(); // This initializes the NeoPixel library.
  
  SerialUSB.begin(115200);
  // this check is only needed on the Leonardo:
  while (!SerialUSB) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }

  if (Ethernet.begin(mac) == 0) {
    SerialUSB.println("Failed to configure Ethernet using DHCP");
    // no point in carrying on, so do nothing forevermore:
    for (;;)
      ;
  }
  
  // print your local IP address:
  SerialUSB.print("My IP address: ");
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    // print the value of each byte of the IP address:
    SerialUSB.print(Ethernet.localIP()[thisByte], DEC);
    SerialUSB.print(".");
  }
  SerialUSB.println();
  
  dht.begin();
  pinMode(SW1, OUTPUT);
  pinMode(SW2, OUTPUT);
  pinMode(SW3, OUTPUT);

  myservo.attach(8);
  myservo.write(90);
  
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

/*
 *  LED ON/OFF
 */
void LED_ON(){
    for(int i=0;i<NUMPIXELS;i++){
      pixels.setPixelColor(i, pixels.Color(200,200,200));
      delay(delayval);
    }
    pixels.show();
}
void LED_OFF(){
    for(int i=0;i<NUMPIXELS;i++){
      pixels.setPixelColor(i, pixels.Color(0,0,0));
      delay(delayval);
    }
    pixels.show();
}

void callback(char* topic, byte* payload, unsigned int length) {
  SerialUSB.print("Message arrived [");
  SerialUSB.print(topic);
  SerialUSB.print("] ");

  if( strcmp(topic,"/wiznet/sw1") == 0 )
  {
    if( (char)payload[0] == '1' )
    {
      SerialUSB.print("On");  myservo.write(0);
    }
    else if( (char)payload[0] == '0' )
    {
      SerialUSB.print("Off"); myservo.write(90);
    }
  }
  else if( strcmp(topic,"/wiznet/sw2") == 0 )
  {
    if( (char)payload[0] == '1' )
    {
      SerialUSB.print("On"); LED_ON();
    }
    else if( (char)payload[0] == '0' )
    {
      SerialUSB.print("Off"); LED_OFF();
    }
  }
  else if( strcmp(topic,"/wiznet/sw3") == 0 )
  {
    if( (char)payload[0] == '1' )
    {
      SerialUSB.print("On");  digitalWrite(SW3, HIGH);
    }
    else if( (char)payload[0] == '0' )
    {
      SerialUSB.print("Off"); digitalWrite(SW3, LOW);
    }
  }

  SerialUSB.println();
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    SerialUSB.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("WizArduino-2")) {
      SerialUSB.println("connected");
      // ... and resubscribe
      client.subscribe("/wiznet/sw1");
      client.subscribe("/wiznet/sw2");
      client.subscribe("/wiznet/sw3");
    } else {
      SerialUSB.print("failed, rc=");
      SerialUSB.print(client.state());
      SerialUSB.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    String strHum = String(h);
    String strTemp = String(t);
    char hum[10]; 
    char temp[10];
    strHum.toCharArray(hum,10);
    strTemp.toCharArray(temp,10);

    SerialUSB.print("Temp : ");
    SerialUSB.print(temp);
    SerialUSB.print(" Hum : ");
    SerialUSB.println(hum);

    int rain = analogRead(A1);
    SerialUSB.print(" rain : ");
    SerialUSB.println(rain);

    // Once connected, publish an announcement...
    client.publish("/wiznet/humidity", hum );
    client.publish("/wiznet/temperature", temp );
    if(rain < 1000)
     {
        client.publish("/wiznet/rainy", "rainy" );
        SerialUSB.print(" rain ");
     }
    else
    {
        client.publish("/wiznet/rainy", "not rainy" );
        SerialUSB.print(" no rain ");
    }
  }
}
