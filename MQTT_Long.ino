// JSON-C - Version: Latest 
#include <ArduinoJsonC.h>
#include <json-c/json.h>

// Eclipse Paho MQTT - Version: Latest 
#include <ArduinoPahoASync.h>
#include <MQTTAsync.h>
#include <MQTTClient.h>
#include <MQTTClientPersistence.h>

//#include <time.h>

#include "th02.hpp"
#include "upm_utilities.h"
//#include "jhd1313m1.h"

//#define ADDRESS     "tcp://localhost:1883"
//#define ADDRESS "test.mosquitto.org:1883"
#define ADDRESS "iot.eclipse.org:1883"
//#define ADDRESS "q.emqtt.com:1883"

#define CLIENTID    "MQTTExample"
#define TOPIC_ONE   "user99/sensors/temperature/data"
#define TOPIC_TWO   "user99/sensors/humidity/data"
#define QOS         1
#define TIMEOUT     10000L

volatile int finished = 0;
volatile int server_connect = 0;
volatile int msg_counter=0;
volatile int reconnect_flag=0;
 
void connLost(void *context, char *cause) {
  MQTTAsync client = (MQTTAsync)context;
  MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
  int rc;

  printf("\nConnection lost\n");
  printf("     cause: %s\n", cause);
  
  #if 1
  printf("Reconnecting\n");
  conn_opts.keepAliveInterval = 20;
  conn_opts.cleansession = 1;
  conn_opts.onSuccess = onConnect;
  conn_opts.onFailure = onConnectFailure;
  conn_opts.context = client;
  if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS) {
    printf("Failed to start connect, return code %d\n", rc);
    finished = 1;
  }
  #endif

}
void onConnectFailure(void* context, MQTTAsync_failureData* response) {
  printf("Connect failed, rc %d\n", response ? response->code : 0);
  finished = 1;
}
void onConnect(void* context, MQTTAsync_successData* response) {
  printf("Successful connection\n");
  server_connect = 1;
}

void onDisconnect(void* context, MQTTAsync_successData* response) {
  printf("Successful disconnection\n");
  server_connect = 0;
}
void onDisconnectFailure(void* context, MQTTAsync_failureData* response) {
  printf("disconnect failed, rc %d\n", response ? response->code : 0);
  server_connect = 0;
  finished = 1;
}

void onSend(void* context, MQTTAsync_successData* response) {
  msg_counter ++;  
//  printf("%d: Message with token value %d delivery confirmed, %s\n", msg_counter,response->token,ADDRESS);
  printf("%d: Message with token value %d delivery confirmed\n", msg_counter,response->token);
    
  finished = 1;
  return;
}

#if 1
void onUnSuccessSend(void* context, MQTTAsync_failureData* response) {
  msg_counter ++;  
//  printf("%d: Message with token value %d delivery confirmed, %s\n", msg_counter,response->token,ADDRESS);
  printf("%d: Message with token value %d delivery failed\n", msg_counter,response->token);
  printf("%d: code = %d\n", msg_counter,response->code);
  printf("%d: Message =%s\n", msg_counter,response->message);
  
  finished = 1;
  //connection lost, set flag to reconnect to server
  reconnect_flag = 1;
  printf("%d: finished =%d, reconnect_flag=%d\n", msg_counter,finished,reconnect_flag);
  return;
}
#endif

void setup() {
  
  // Set the subplatform for the shield
  mraa_add_subplatform(MRAA_GROVEPI, "0");

  // Create the temperature & humidity sensor object 
  upm::TH02 sensor;
  

  msg_counter = 0;
  reconnect_flag = 0;

#if 0
  // initialize the LCD and check for initialization
  jhd1313m1_context lcd = jhd1313m1_init(0, 0x3e, 0x62);

  if (!lcd) {
    printf("jhd1313m1_i2c_init() failed\n");
    return 1;
  }
  
  // set the LCD parameters
  char string1[20];
  char string2[20];
  uint8_t rgb[7][3] = {
            {0xd1, 0x00, 0x00},
            {0xff, 0x66, 0x22},
            {0xff, 0xda, 0x21},
            {0x33, 0xdd, 0x00},
            {0x11, 0x33, 0xcc},
            {0x22, 0x00, 0x66},
            {0x33, 0x00, 0x44}};
#endif

  MQTTAsync client;
  MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
  int rc;

  MQTTAsync_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);


  server_connect = 0;
  
  MQTTAsync_setCallbacks(client, NULL, connLost, NULL, NULL);

  conn_opts.keepAliveInterval = 20;
  conn_opts.cleansession = 1;
  conn_opts.onSuccess = onConnect;
  conn_opts.onFailure = onConnectFailure;
  conn_opts.context = client;
  if ((rc = MQTTAsync_connect(client, &conn_opts)) !=  MQTTASYNC_SUCCESS) {
    printf("Failed to start connect, return code %d\n", rc);
    exit(EXIT_FAILURE);
  }
  
  while(server_connect == 0)
  {
    delay(500);
  }
    
  // Read the temperature and humidity printing both the Celsius and
  // equivalent Fahrenheit temperature and Relative Humidity, waiting two seconds between readings
  while (1) {
      float celsius = sensor.getTemperature();
      float fahrenheit = (celsius * 9.0 / 5.0 + 32.0);
      float humidity = sensor.getHumidity();
      //printf("%2.3f Celsius, or %2.3f Fahrenheit\n", celsius, fahrenheit);
      //printf("%2.3f%% Relative Humidity\n", humidity);
      
#if 0
      snprintf(string1, sizeof(string1), "Temperature:");
      snprintf(string2, sizeof(string2), "%2.1f%cF %2.1f%cC", fahrenheit, 223, celsius, 223);
      // Alternate rows on the LCD
      jhd1313m1_set_cursor(lcd, 0, 0);
      jhd1313m1_write(lcd, string1, strlen(string1));
      jhd1313m1_set_cursor(lcd, 1, 0);
      jhd1313m1_write(lcd, string2, strlen(string2));
      // Change the color
      uint8_t r = rgb[(int)fahrenheit%7][0];
      uint8_t g = rgb[(int)fahrenheit%7][1];
      uint8_t b = rgb[(int)fahrenheit%7][2];
      jhd1313m1_set_color(lcd, r, g, b);
      upm_delay(2);
      jhd1313m1_clear(lcd);
      
      snprintf(string1, sizeof(string1), "Humidity:");
      snprintf(string2, sizeof(string2), "%2.1f%%", humidity);
      // Alternate rows on the LCD
      jhd1313m1_set_cursor(lcd, 0, 0);
      jhd1313m1_write(lcd, string1, strlen(string1));
      jhd1313m1_set_cursor(lcd, 1, 0);
      jhd1313m1_write(lcd, string2, strlen(string2));
      upm_delay(2);
      jhd1313m1_clear(lcd);
#endif     


      // Create JSON Objects for MQTT
      char cnum[13];
	    sprintf(cnum, "%3.7f", celsius);
	    char hnum[13];
	    sprintf(hnum, "%3.7f", humidity);
	    struct timeval time;
	    gettimeofday(&time,NULL);
	    char snum[13];
	    sprintf(snum, "%d", (int) time.tv_sec);
	    char *sensor = "sensor_id";
	    char *temper = "temperature";
	    char *humid = "humidity";
	    char *value  = "value";
	    char *timet   = "timestamp";
	    //Create the json object for TOPIC_ONE i.e temperature
	    struct json_object *jobj_1;
	    jobj_1 = json_object_new_object();
	    json_object_object_add(jobj_1, sensor, json_object_new_string(temper));
	    json_object_object_add(jobj_1, value, json_object_new_string(cnum));
	    json_object_object_add(jobj_1, timet, json_object_new_string(snum));
	    
	    //Create the json object for TOPIC_TWO i.e humidity
	    struct json_object *jobj_2;
	    jobj_2 = json_object_new_object();
	    json_object_object_add(jobj_2, sensor, json_object_new_string(humid));
	    json_object_object_add(jobj_2, value, json_object_new_string(hnum));
	    json_object_object_add(jobj_2, timet, json_object_new_string(snum));
	    
	    char *str_mqtt;

	    str_mqtt = (char *) json_object_to_json_string(jobj_1);
	    
	    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
      MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
      opts.onSuccess = onSend;
      opts.onFailure = onUnSuccessSend;
      opts.context = client;
	    // Send JSON Object via MQTT
      pubmsg.payload = str_mqtt;
      pubmsg.payloadlen = strlen(str_mqtt);
      pubmsg.qos = QOS;
      pubmsg.retained = 0;

      finished = 0;
      // Publish TOPIC_ONE
      if ((rc = MQTTAsync_sendMessage(client, TOPIC_ONE, &pubmsg, &opts)) != MQTTASYNC_SUCCESS) {
        printf("Failed to start sendMessage1, return code %d, finished = %d, reconnect_flag=%d\n", rc,finished,reconnect_flag);
        finished = 1;
      }
      
      while (!finished) {
        usleep(10000L);
      }
      
      str_mqtt = (char *) json_object_to_json_string(jobj_2);
	    
	    MQTTAsync_responseOptions opts2 = MQTTAsync_responseOptions_initializer;
      MQTTAsync_message pubmsg2 = MQTTAsync_message_initializer;
      opts2.onSuccess = onSend;
      opts2.onFailure = onUnSuccessSend;
      opts2.context = client;
      // Send JSON Object via MQTT
      pubmsg2.payload = str_mqtt;
      pubmsg2.payloadlen = strlen(str_mqtt);
      pubmsg2.qos = QOS;
      pubmsg2.retained = 0;
      
      finished = 0;
      // Publish TOPIC_ONE
      if ((rc = MQTTAsync_sendMessage(client, TOPIC_TWO, &pubmsg2, &opts2)) != MQTTASYNC_SUCCESS) {
        printf("Failed to start sendMessage2, return code %d， finished = %d, reconnect_flag=%d\n", rc, finished,reconnect_flag);
        finished = 1;
      }
    
      // Wait until message is sent
      while (!finished) {
        usleep(10000L);
      }
      
      if(reconnect_flag == 1)
      {
        reconnect_flag = 0;
        printf("disconnect here\n");
#if 0
        MQTTAsync_disconnectOptions opts1 = MQTTAsync_disconnectOptions_initializer;
        opts1.onSuccess = onDisconnect;
        opts1.onFailure = onDisconnectFailure;
  	    opts1.context = client;
  	    rc = MQTTAsync_disconnect(client, &opts1);
        // Wait until message is sent
        while (server_connect==1) {
          printf("wait\n");
          usleep(10000L);
        } 
#endif
        //MQTTClient_disconnect(client, 2000);

        printf("reconnect here\n");
        server_connect = 0;
        
        MQTTAsync_setCallbacks(client, NULL, connLost, NULL, NULL);
      
        conn_opts.keepAliveInterval = 20;
        conn_opts.cleansession = 1;
        conn_opts.onSuccess = onConnect;
        conn_opts.onFailure = onConnectFailure;
        conn_opts.context = client;
        if ((rc = MQTTAsync_connect(client, &conn_opts)) !=  MQTTASYNC_SUCCESS) {
          printf("Failed to start connect, return code %d\n", rc);
          exit(EXIT_FAILURE);
        }
        
        while(server_connect == 0)
        {
          printf("wait establish connection\n");
          delay(500);
        }
        printf("done\n");
          
      }

      
      delay(1000);
  }

  //return 0;
}

void loop()
{
  delay(1000);
}

