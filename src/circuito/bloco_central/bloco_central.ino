// Importing all required libraries to WiFi network and MQTT.
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>

// Importing all required libraries to use string methods.
#include <String.h>

// Importing all required libraries to RFID reader.
#include <SPI.h>
#include <MFRC522.h>

// Defining the RFID ports...
#define RFID_SS_SDA 21
#define RFID_RST 14

// Defining the RFID base...
MFRC522 rfid = MFRC522(RFID_SS_SDA, RFID_RST);

// Defining other devices pin ports...
#define OUTPUT_BUZZER 20
#define OUTPUT_LED_1_R 36
#define OUTPUT_LED_1_G 0
#define OUTPUT_LED_1_B 45
#define OUTPUT_LED_2_R 42
#define OUTPUT_LED_2_G 40
#define OUTPUT_LED_2_B 39

// Defining the hardware stages...
#define ENTRY__WAITING_VALET_LINK_RFID_TO_SEARCH_ORDER_OF_SERVICE 0
#define ENTRY__WAITING_SERVER_RESPONSE 1
#define ENTRY__WAITING_CAR_PARKING 2
#define ENTRY__CAR_PARKED 3
#define EXIT__WAITING_VALET_LINK_RFID_TO_SEARCH_ORDER_OF_SERVICE 4
#define EXIT__WAITING_SERVER_RESPONSE 5
#define EXIT__WAITING_CAR_DELIVERY 6
#define EXIT__CAR_DELIVERED 7


// Instancing the primary WiFi credentials.
const char* WIFI_SSID = "iPhone";
const char* WIFI_PASSWORD = "12345678";

// Instancing the access point WiFi credentials.
const char * AP_FTM_SSID = "HelloWorld";
const char * AP_FTM_PASS = "HelloWorld";

// Instancing the primary MQTT credentials.
const char *BROKER_MQTT_HOST = "broker.hivemq.com";
const char *BROKER_MQTT_CLIENT_ID = "7b6e3af9-fee4-4e9e-94ac-662e9020a17d";
const int   BROKER_MQTT_PORT = 1883;

// Instancing the WiFi client.
WiFiClient espClient;

// Instancing the MQTT client.
PubSubClient client(espClient);

// Instancing the current step variable to store at which stage of the flow we are in.
int hardware_stage = ENTRY__WAITING_VALET_LINK_RFID_TO_SEARCH_ORDER_OF_SERVICE;

// Intancing buffers to store the ids of orders of service to be linked to this device. 
String entry_order_of_service_id = "";
String exit_order_of_service_id = "";

// Instancing a memory space to store the RFID read.
String strRFID = "";

// -------------------------- Warning --------------------------
//
// Initial configurations for distance measurement - <<<FTM>>>.
//
// -------------------------------------------------------------

// Setting up the periods.
const uint8_t FTM_FRAME_COUNT = 16;
const uint16_t FTM_BURST_PERIOD = 2;

// Instancing a semaphore to signal when a FTM report is received.
xSemaphoreHandle sinalizadorFTM;

// Instancing a variable to store the status of the FTM report to be received.
bool status_do_relatorio_FTM = true;

// Instancing a variable to store the calculated distance.
float distancia_do_roteador_em_metros = 0.0;

/*
 * Measure the estimated distance when a ftm report is received.
 */
void ao_receber_um_relatorio_FTM(arduino_event_t *event) {

	// Instancing the possible response status.
  const char * status_str[5] = {"SUCCESS", "UNSUPPORTED", "CONF_REJECTED", "NO_RESPONSE", "FAIL"};

	// Instancing a pointer to the report.
  wifi_event_ftm_report_t * report = &event -> event_info.wifi_ftm_report;

  // Definindo o status global do relatório.
  status_do_relatorio_FTM = report -> status == FTM_STATUS_SUCCESS;

	// Controlling the flow through the current status of the report.
  if (status_do_relatorio_FTM) {

		// Storing the calculated distance.
    distancia_do_roteador_em_metros = (report -> dist_est / 100.0) - 40.0;

		// Freeing the memory after the use of the variables.
    free(report->ftm_report_data);

  } else {

		// Printing the error.
    Serial.println("FTM report denied: ");
    Serial.println(status_str[report->status]);

  }

	// Informing that a semaphore was received.
  xSemaphoreGive(sinalizadorFTM);

}

/*
 * Initializing a FTM session and requesting a FTM report.
 */
bool solicitar_um_relatorio_FTM(){

	// Controlling possible errors.
  if(!WiFi.initiateFTM(FTM_FRAME_COUNT, FTM_BURST_PERIOD)){

		// Informing the failure to start the session.
		Serial.println("FTM Error: Failed to start session.");

    // Returning a false response.
    return false;

  }

	// Awaiting the signal that the report was received successfully and returning true.
  return xSemaphoreTake(sinalizadorFTM, portMAX_DELAY) == pdPASS && status_do_relatorio_FTM;

}

// -------------------------- Warning --------------------------
//
// Ending configurations for distance measurement - <<<FTM>>>.
//
// -------------------------------------------------------------

/*
 * Settuping the circuit.
 */
void setup() {

  // Setting up the frequency of the serial port.
  Serial.begin(115200);

  // Printing a blank line.
  Serial.println("");

  // Settuping leds...
  setup_leds();

  // Setupping the WiFi network...
  setup_wifi();

  // Settuping the MQTT client...
  setup_mqtt();

  // Settuping the RFID...
  setup_rfid();

  // Setting the pin mode of buzzer as output.
  pinMode(OUTPUT_BUZZER, OUTPUT);

}

/*
 * Setupping leds.
 */
void setup_leds() {

	// Setting the pin mode of leds as output.
	pinMode(OUTPUT_LED_1_R, OUTPUT);
	pinMode(OUTPUT_LED_1_G, OUTPUT);
	pinMode(OUTPUT_LED_1_B, OUTPUT);
	pinMode(OUTPUT_LED_2_R, OUTPUT);
	pinMode(OUTPUT_LED_2_G, OUTPUT);
	pinMode(OUTPUT_LED_2_B, OUTPUT);

	// Turning on the leds.
	digitalWrite(OUTPUT_LED_1_R, LOW);
	digitalWrite(OUTPUT_LED_1_G, HIGH);
	digitalWrite(OUTPUT_LED_1_B, LOW);
	digitalWrite(OUTPUT_LED_2_R, LOW);
	digitalWrite(OUTPUT_LED_2_G, LOW);
	digitalWrite(OUTPUT_LED_2_B, LOW);

}

/*
 * Setupping wifi.
 */
void setup_wifi() {

  // Waiting 10 miliseconds to avoid bugs...
  delay(10);

  // Printing the current system status...
  Serial.println();
  Serial.print("Connecting to ");
  Serial.print(WIFI_SSID);

  // Connecting to the primary WiFi network...
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // Printing the current system status...
  while (WiFi.status() != WL_CONNECTED) {

    delay(500);
    Serial.print(".");

  }

  // Printing the current system status...
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.print(WiFi.localIP());
  Serial.println("");

  // Printing a blank line.
  Serial.println("");

}

/*
 * Setupping mqtt.
 */
void setup_mqtt() 
{

  // Setting up the MQTT server.
  client.setServer(BROKER_MQTT_HOST, BROKER_MQTT_PORT);

  // Setting up the MQTT client callback.
  client.setCallback(on_message);

}

/*
 * Setupping rfid.
 */
void setup_rfid() 
{

  // Initializing the SPI barrament.
	SPI.begin();

	// Initializing the MFRC522.
	rfid.PCD_Init();

}

/*
 * Setupping acess point.
 */
void setup_ap() {

  // Waiting 10 miliseconds to avoid bugs...
  delay(10);

  // Printing the current system status...
  Serial.println();
  Serial.print("Connecting to ");
  Serial.print(AP_FTM_SSID);

  // Connecting to the access point WiFi network...
  WiFi.begin(AP_FTM_SSID, AP_FTM_PASS);

  // Printing the current system status...
  while (WiFi.status() != WL_CONNECTED) {

    delay(500);
    Serial.print(".");

  }

  // Printing the current system status...
  Serial.println("");
  Serial.println("AP connected");
  Serial.print("IP address: ");
  Serial.print(WiFi.localIP());
  Serial.println("");

  // Printing a blank line.
  Serial.println("");

}

/*
 * Instancing a function to be called when the circuit needs to emit an error,
 * pulsing the led one as red and the buzzer three times.
 */
void emit_error() {

  // Looping three times...
  for(int i = 0; i < 3; i++) {

      // Playing buzzer...
      play_buzzer();

      // Turning on LED 1 as red.
      digitalWrite(OUTPUT_LED_1_R, HIGH);
      digitalWrite(OUTPUT_LED_1_G, LOW);
      digitalWrite(OUTPUT_LED_1_B, LOW);

      // Waiting one second...
      delay(1000);

      // Turning off LED 1.
      digitalWrite(OUTPUT_LED_1_R, LOW);
      digitalWrite(OUTPUT_LED_1_G, LOW);
      digitalWrite(OUTPUT_LED_1_B, LOW);

      // Stopping buzzer...
      stop_buzzer();

      // Waiting one second...
      delay(1000);

    }

}

/*
 * Instancing a callback to be called when a mqtt message arrives.
 */
void on_message(char* topic, byte* message, unsigned int length) 
{

  // Printing the arrived message topic...
  Serial.print("\nMessage arrived on topic: ");
  Serial.print(topic);
  Serial.print(".");

  // Warning the message print...
  Serial.print("\nMessage: ");


  // Instancing a variable to store the temporary message.
  String messageTemp;
  
  // Constructing the message...
  for (int i = 0; i < length; i++) {
    messageTemp += (char)message[i];
  }

  // Printing the message.
  Serial.print(messageTemp);

  // Printing a blank line.
  Serial.println("");

  // If the system find an entry order of service associated to the read rfid...
  if(hardware_stage == ENTRY__WAITING_SERVER_RESPONSE &&  strcmp(topic, "Estapar/OrdemDeServicoDeEntradaEncontrada") == 0) {

    // Storing the id of the entry order of service.
    entry_order_of_service_id = messageTemp;

    // Turning on LED 1 as blue.
    digitalWrite(OUTPUT_LED_1_R, LOW);
    digitalWrite(OUTPUT_LED_1_G, LOW);
    digitalWrite(OUTPUT_LED_1_B, HIGH);

    // Turning on LED 2 as yellow.
    digitalWrite(OUTPUT_LED_2_R, HIGH);
    digitalWrite(OUTPUT_LED_2_G, HIGH);
    digitalWrite(OUTPUT_LED_2_B, LOW);

    // Updating the hardware stage to the next step.
    hardware_stage = ENTRY__WAITING_CAR_PARKING;

  }

  // If the system doesn't find an entry order of service associated to the read rfid...
  if(hardware_stage == ENTRY__WAITING_SERVER_RESPONSE && strcmp(topic, "Estapar/OrdemDeServicoDeEntradaNaoEncontrada") == 0) {

    // Emitting an error and pulsing three times...
    emit_error();

    // Restarting device.
    ESP.restart();
  
  }

  // If the system find an exit order of service associated to the read rfid...
  if(hardware_stage == EXIT__WAITING_SERVER_RESPONSE && strcmp(topic, "Estapar/OrdemDeServicoDeSaidaEncontrada") == 0) {

    // Storing the id of the exit order of service.
    exit_order_of_service_id = messageTemp;

    // Keeping LED 1 as blue.
    digitalWrite(OUTPUT_LED_1_R, LOW);
    digitalWrite(OUTPUT_LED_1_G, LOW);
    digitalWrite(OUTPUT_LED_1_B, HIGH);

    // Setting LED 2 as cian.
    digitalWrite(OUTPUT_LED_2_R, LOW);
    digitalWrite(OUTPUT_LED_2_G, HIGH);
    digitalWrite(OUTPUT_LED_2_B, HIGH);

    // Updating the hardware stage to the next step.
    hardware_stage = EXIT__WAITING_CAR_DELIVERY;

  }

  // If the system doesn't find an exit order of service associated to the read rfid...
  if(hardware_stage == EXIT__WAITING_SERVER_RESPONSE && strcmp(topic, "Estapar/OrdemDeServicoDeSaidaNaoEncontrada") == 0) {

    // Emitting an error and pulsing three times...
    emit_error();

    // Setting the hardware stage as... (backing to the last step).
    hardware_stage = EXIT__WAITING_VALET_LINK_RFID_TO_SEARCH_ORDER_OF_SERVICE;

    // Keeping LED 1 as blue.
    digitalWrite(OUTPUT_LED_1_R, LOW);
    digitalWrite(OUTPUT_LED_1_G, LOW);
    digitalWrite(OUTPUT_LED_1_B, HIGH);

  }

  // If the system receives a message on "Estapar/LiberarDispositivo"...
  if(hardware_stage == EXIT__CAR_DELIVERED && strcmp(topic, "Estapar/LiberarDispositivo") == 0) {

    // Playing buzzer...
    play_buzzer();

    // Waiting five secods...
    delay(5000);

    // Restarting the device.
    ESP.restart();

  }

  
  
}


/*
 * Instancing a function to be called when mqtt client is disconnected.
 */
void reconnect() 
{

  // Loop until we're reconnected.
  while (!client.connected()) {

    // Printing the current system status.
    Serial.println("Attempting MQTT connection...");

    // Attempting to connect.
    if (client.connect(BROKER_MQTT_CLIENT_ID)) {

      // Printing the current system status.
      Serial.println("MQTT Connected!");

      // Subscribing to entry related mqtt topics...
      client.subscribe("Estapar/OrdemDeServicoDeEntradaEncontrada");
      client.subscribe("Estapar/OrdemDeServicoDeEntradaNaoEncontrada");

      // Subscribing to exit related mqtt topics...
			client.subscribe("Estapar/OrdemDeServicoDeSaidaEncontrada");
			client.subscribe("Estapar/OrdemDeServicoDeSaidaNaoEncontrada");

      // Printing a blank line.
      Serial.println("");
      
    } else {

	  	// Printing the current system status.
      Serial.println("Trying again in 5 seconds...");

      // Wait 5 seconds before retrying
      delay(5000);

    }
  }

}

/*
 * Instancing a function to play a song on buzzer.
 */
void play_buzzer()
{

	// Playing a song of 300Hz...
	tone(OUTPUT_BUZZER, 3000);

}

/*
 * Instancing a function to stop the song on buzzer.
 */
void stop_buzzer()
{

	// Interrupting the song...
	noTone(OUTPUT_BUZZER);

}

/*
 * Instancing a function to capture the RFID tag.
 */
void rfid_captor()
{

	// Cleaning up the RFID tag...
	strRFID = "";

	// Generating the number of RFID tag....
	for (byte i = 0; i < 4; i++)
	{
		strRFID +=
			(rfid.uid.uidByte[i] < 0x10 ? "0" : "") +
			String(rfid.uid.uidByte[i], HEX) +
			(i != 3 ? ":" : "");
	}

  // Transforming the text to uppercase...
	strRFID.toUpperCase();

  // Printing the RFID read.
	Serial.print("RFID Tag: ");
	Serial.println(strRFID);

	// Stopping the reading...
	rfid.PICC_HaltA();

  // Stopping PCD cryptografy...
	rfid.PCD_StopCrypto1();

  // Playing buzzer...
  play_buzzer();

  // Waiting 1 second...
  delay(1000);

  // Stopping buzzer.
  stop_buzzer();

}

/*
 * Instancing the loop function.
 */
void loop() {

	// If client is not connected.
	if (!client.connected()) {

		// Reconnecting...
		reconnect();

	}

	// Listen MQTT broker.
	client.loop();

	// If has not RFID card nearby, or the RFID stay nearby the sensor, ignoring a new read...
	if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial())
	{

		return;
    
	}
	else 
	{

		// If a new card is detected...
		// Capturing a new RFID...
		rfid_captor();

		// Transforming the string to char array.
		char *nStr = const_cast<char*>(strRFID.c_str());

		// If the hardware stage is waiting for a valet link the RFID to search the order of service...
		if(hardware_stage == ENTRY__WAITING_VALET_LINK_RFID_TO_SEARCH_ORDER_OF_SERVICE) {

			// Publishing the RFID to search the order of service.
			client.publish("Estapar/VincularOrdemDeServicoDeEntradaComBlocoCentral", nStr);

			// Setting the current step as 1.
			hardware_stage = ENTRY__WAITING_SERVER_RESPONSE;

		} else if (hardware_stage == ENTRY__WAITING_CAR_PARKING) {

			// Keeping LED 1 as blue.
			digitalWrite(OUTPUT_LED_1_R, LOW);
			digitalWrite(OUTPUT_LED_1_G, LOW);
			digitalWrite(OUTPUT_LED_1_B, HIGH);

			// Turning on LED 2 as yellow.
			digitalWrite(OUTPUT_LED_2_R, HIGH);
			digitalWrite(OUTPUT_LED_2_G, HIGH);
			digitalWrite(OUTPUT_LED_2_B, LOW);

			// Converting the string to a array of chars...
			char *orderOfServiceIdStr = const_cast<char*>(entry_order_of_service_id.c_str());

			// Publishing that car is parked...
			client.publish("Estapar/CarroEstacionado", orderOfServiceIdStr);

			// Setting the hardware stage as car parked.
			hardware_stage = ENTRY__CAR_PARKED;

      delay(2000);

			// -------------------------- Warning --------------------------
			//
			// Initial configurations for distance measurement - <<<FTM>>>.
			//
			// -------------------------------------------------------------

			// Disconnecting from the current connected WiFi network.
			WiFi.disconnect();

			// Delaying 200ms.
			delay(200);

			// Setupping the WiFi in AP mode.
			setup_ap();

			// Creating a binary semaphore to control the FTM report.
			sinalizadorFTM = xSemaphoreCreateBinary();

			// Listening FTM reports events.
			WiFi.onEvent(ao_receber_um_relatorio_FTM, ARDUINO_EVENT_WIFI_FTM_REPORT);

			// Printing the current status of the system.
			Serial.println("Initializing FTM session...");

			// Requesting a FTM report.
			solicitar_um_relatorio_FTM();

			// Printing the current status of the system.
  		Serial.printf("Car distance from access point: %.2f m\n", (float) distancia_do_roteador_em_metros);

			// Disconnecting from the WiFi AP network.
			WiFi.disconnect();

			// Connecting to the WiFi primary network.
			setup_wifi();

			// -------------------------- Warning --------------------------
			//
			// Ending configurations for distance measurement - <<<FTM>>>.
			//
			// -------------------------------------------------------------

			// Delaying 200ms.
			delay(200);

			// Setting LED 2 as magenta.
			digitalWrite(OUTPUT_LED_2_R, HIGH);
			digitalWrite(OUTPUT_LED_2_G, LOW);
			digitalWrite(OUTPUT_LED_2_B, HIGH);

			// Updating the hardware stage.
			hardware_stage = EXIT__WAITING_VALET_LINK_RFID_TO_SEARCH_ORDER_OF_SERVICE;

      if(hardware_stage == EXIT__WAITING_VALET_LINK_RFID_TO_SEARCH_ORDER_OF_SERVICE && distancia_do_roteador_em_metros) {

        // Converting the distance to string.
        String distance_in_meters_str = String(distancia_do_roteador_em_metros);

        // Converting the string to a array of chars...
        char *distance_in_meters_str_c = const_cast<char*>(distance_in_meters_str.c_str());

        // Publishing that car distance...
        client.publish("Estapar/CarroEstacionado/distancia", distance_in_meters_str_c);

        // Printing the system current status.
        Serial.println("Measured distance transmitted successfully!\n");

      }

		} else if (hardware_stage == EXIT__WAITING_VALET_LINK_RFID_TO_SEARCH_ORDER_OF_SERVICE) {

			// Keeping LED 1 as blue.
			digitalWrite(OUTPUT_LED_1_R, LOW);
			digitalWrite(OUTPUT_LED_1_G, LOW);
			digitalWrite(OUTPUT_LED_1_B, HIGH);

			// Publishing the RFID to search the order of service.
			client.publish("Estapar/VincularOrdemDeServicoDeSaidaComBlocoCentral", nStr);

			// Updating the hardware stage.
			hardware_stage = EXIT__WAITING_SERVER_RESPONSE;

		} else if (hardware_stage == EXIT__WAITING_CAR_DELIVERY) {

			// Keeping LED 1 as blue.
			digitalWrite(OUTPUT_LED_1_R, LOW);
			digitalWrite(OUTPUT_LED_1_G, LOW);
			digitalWrite(OUTPUT_LED_1_B, HIGH);

			// Setting LED 2 as white.
			digitalWrite(OUTPUT_LED_2_R, HIGH);
			digitalWrite(OUTPUT_LED_2_G, HIGH);
			digitalWrite(OUTPUT_LED_2_B, HIGH);

			// Updating the hardware stage.
			hardware_stage = EXIT__CAR_DELIVERED;

			// Subscribing to the...
			client.subscribe("Estapar/LiberarDispositivo");

			// Converting the string to a array of chars...
			char *exitOrderOfServiceIdStr = const_cast<char*>(exit_order_of_service_id.c_str());

			// Publishing car delivered.
			client.publish("Estapar/CarroEntregue", exitOrderOfServiceIdStr);

		}
    

  }
    

}