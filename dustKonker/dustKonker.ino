// Conecte o pino 3 do sensor DSM501A no pino 5V  do Arduino
// Conecte o pino 5 do sensor DSM501A no pino GND do Arduino
// Conecte o pino 2 do sensor DSM501A no pino D8  do Arduino

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#define DEBUG

int pin = D8;
unsigned long duration;
unsigned long starttime;
unsigned long sampletime_ms = 30000;
unsigned long lowpulseoccupancy = 0;
float ratio = 0;
float concentration = 0;
char mensagemSTR[10];
char lowpulseSTR[10];
char concentrationSTR[10];
char mensagem[50];

 //informações da rede WIFI

const char* ssid = "Lucas_2Ghz";        //SSID da rede WIFI
const char* pass =  "lucas123";    //senha da rede wifi

//Informações do MQTT
const char* mqttServer = "mqtt.prod.konkerlabs.net";   //server
const char* userMqtt = "4h69dhjqljl8";    //user
const char* passMqtt = "GuOHfVNGnicf";  //password
const int mqttPort = 1883;          //port
const char* mqttTopicSub ="data/4h69dhjqljl8/pub/dust";
const char* mqttTopicPub = "data/4h69dhjqljl8/pub/dust";//tópico que sera assinado

//Variáveis para criação da mensagem em JSON
char* msg;
char bufferJ[256];

/* Variáveis do cálculo da massa de uma partícula */
double massa = 5.8874 * pow(10,(-13));
double massaconst = massa * 3531.5;
double particulas = 0;

int ultimoEnvioMQTT = 0;


//Vamos criar uma funcao para formatar os dados no formato JSON
char *jsonMQTTmsgDATA(const char *device_id, unsigned long lowpulseoccupancy, float concentration, double qntParticulas) {
  const int capacity = JSON_OBJECT_SIZE(4);
  StaticJsonDocument<capacity> jsonMSG;
  jsonMSG["deviceId"] = device_id;
  jsonMSG["Low Pulse Occupancy"] = lowpulseoccupancy;
  jsonMSG["Concentration"] = concentration;
  jsonMSG["Particules"] = qntParticulas;
  serializeJson(jsonMSG, bufferJ);
  return bufferJ;
}

//Função de callback para ler a mensagem
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}


//Criando um cliente wifi e Publicador/Subscritor
WiFiClient espClient;
PubSubClient client(espClient);
 
void setup() {
  //Speed do upload, tem que estar de acordo com o speed que foi setado na IDE
  Serial.begin(115200);

  //Iniciando conexão wifi
  WiFi.begin(ssid, pass);

 //Fica tentando conectar até conseguir
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: \n");
    delay(500);
  }
  Serial.println("\nConnected.\n");

  
  //Seta o server a porta no cliente MQTT e tenta fazer a conexão passando usuário e senha 
  
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

  
  while (!client.connected()) {
    if (client.connect("DSM501A",  userMqtt, passMqtt )) {

    } else {
      Serial.print("Attempting to connect to MQTT Server\n");
      delay(2000);
    }
  }

  //Após conectar o cliente se inscreve no topico/canal indicado
  Serial.println("\nMQTT Server Connected.\n");
  client.subscribe(mqttTopicSub);

  
  pinMode(D8,INPUT);
  starttime = millis();
}

void reconect() {
  
  //Enquanto estiver desconectado tenta fazer a conexão MQTT novamente
  
  while (!client.connected()) {
 
    if(client.connect("DSM501A",  userMqtt, passMqtt )) {   
      client.subscribe(mqttTopicSub, 1); //nivel de qualidade: QoS 1
    } else {
      delay(5000);
    }
  }
}
 
 
void loop() {

  //Caso a conexão wifi se desconecte ele tenta fazer a conexão
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println("Lucas_2Ghz");
    while (WiFi.status() != WL_CONNECTED) {
      WiFi.begin(ssid, pass); // Connect to WPA/WPA2 network. Change this line if using open or WEP network
      Serial.print(".");
      delay(5000);
    }
    Serial.println("\nConnected.");
  }

  //Chamada da reconexão de MQTT caso tenha caído
  if (!client.connected()) {
    reconect();
  }
  
  duration = pulseIn(pin, LOW);
  
  lowpulseoccupancy = lowpulseoccupancy+duration;
  
 
  if ((millis()-starttime) > sampletime_ms)
  {
    ratio = lowpulseoccupancy/(sampletime_ms*10.0);  // Integer percentage 0=>100 Taxa de ocupação seria o quanto de poeira o sensor capturou durante o tempo setado na visão da lente em modo geral
    concentration = 1.1*pow(ratio,3)-3.8*pow(ratio,2)+520*ratio+0.62; // using spec sheet curve Concentração de partículas bruta
    //Serial.print(lowpulseoccupancy);
    // Serial.print(",");
    Serial.print("Proporcao: ");
    Serial.print(ratio);
    Serial.print(" , Concentracao: ");
    Serial.println(concentration);
    

    
//char lowpulseSTR[10];
//char concentrationSTR[10];
//    strcpy(mensagem,"{Node 1, ");
//    sprintf(mensagemSTR,"%f",ratio);
//    strcat(mensagem,mensagemSTR);
//    strcat(mensagem,", ");
//    
//    sprintf(lowpulseSTR,"%f",lowpulseoccupancy);
//    strcat(lowpulseSTR, ", ");
//    strcat(mensagem,lowpulseSTR);
//    
//    sprintf(concentrationSTR,"%f",concentration);
//    strcat(mensagem,concentrationSTR);

    //Quantidade de partículas em ug/m3
    particulas = massaconst * concentration * pow(10,6);
    Serial.println("Particulças: \n");
    Serial.println(particulas);

    //Cria a mensagem no farmato JSON utilizando a função criada no começo
    msg = jsonMQTTmsgDATA("Casa", lowpulseoccupancy, concentration, particulas);


    //Envia a mensagem para o tópico/canal escolhido
    if(client.publish(mqttTopicPub, msg)){
      Serial.println("\nMensagem enviada");
    }
    client.loop();

//    delay(15000);
    
    lowpulseoccupancy = 0;
    
    
    starttime = millis();
  }
}
