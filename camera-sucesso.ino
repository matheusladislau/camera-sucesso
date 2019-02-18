#include <ESP8266WiFi.h> 
#include <PubSubClient.h> 
#define TOPICO_SUBSCRIBE "x" 
#define TOPICO_SUBSCRIBE2 "y" 
#define TOPICO_SUBSCRIBE3 "infravermelho" 
#define TOPICO_PUBLISH   "message"   
#define ID_MQTT  "HomeAut"     //id mqtt (para identificação de sessão)IMPORTANTE: este deve ser único no broker (ou seja, se um client MQTT tentar entrar com o mesmo id de outro já conectado ao broker, o broker irá fechar a conexão de um deles).                           
//defines - mapeamento de pinos do NodeMCU
#define D0    16
#define D1    5
#define D2    4
#define D3    0
#define D4    2
#define D5    14
#define D6    12
#define D7    13
#define D8    15
#define D9    3
#define D10   1

int message;
#include <Servo.h>

int manual=1;
int pos_y = 0;
int pos_x=0;
Servo myservo_y; 
Servo myservo_x; 
int infravermelho=12;

// WIFI
const char* SSID = ""; // SSID / nome da rede WI-FI que deseja se conectar
const char* PASSWORD = ""; // Senha da rede WI-FI que deseja se conectar
  
// MQTT
const char* BROKER_MQTT = "m15.cloudmqtt.com";
int BROKER_PORT = 12131;

//Variáveis e objetos globais
WiFiClient espClient; // Cria o objeto espClient
PubSubClient MQTT(espClient); // Instancia o Cliente MQTT passando o objeto espClient
char EstadoSaida = '0';  //variável que armazena o estado atual da saída
  
//Prototypes
void initSerial();
void initWiFi();
void initMQTT();
void reconectWiFi(); 
void mqtt_callback(char* topic, byte* payload, unsigned int length);
void VerificaConexoesWiFIEMQTT(void);
void InitOutput(void);
 
void setup() {
  pinMode(infravermelho, OUTPUT);
  myservo_y.attach(15);
  delay(100);
  myservo_x.attach(13);
  myservo_x.write(-180);
  InitOutput();
  initSerial();
  initWiFi();
  initMQTT();
}
  
//Função: inicializa comunicação serial com baudrate 115200 (para fins de monitorar no terminal serial o que está acontecendo.
void initSerial(){
  Serial.begin(115200);
}
 
//Função: inicializa e conecta-se na rede WI-FI desejada
void initWiFi(){
  delay(10);
  Serial.println("------Conexao WI-FI------");
  Serial.print("Conectando-se na rede: ");
  Serial.println(SSID);
  Serial.println("Aguarde");
  
  reconectWiFi();
}
  
//Função: inicializa parâmetros de conexão MQTT(endereço do broker, porta e seta função de callback)
void initMQTT(){
    MQTT.setServer(BROKER_MQTT, BROKER_PORT);   //informa qual broker e porta deve ser conectado
    MQTT.setCallback(mqtt_callback);            //atribui função de callback (função chamada quando qualquer informação de um dos tópicos subescritos chega)
}
  
//Função: função de callback 
//        esta função é chamada toda vez que uma informação de 
//        um dos tópicos subescritos chega)
void mqtt_callback(char* topic, byte* payload, unsigned int length) 
{
    String msg;
 
    //obtem a string do payload recebido
    for(int i = 0; i < length; i++) 
    {
       char c = (char)payload[i];
       msg += c;
    }
    Serial.println(topic);
    Serial.println(msg);
    message=msg.toInt();
    
    if (strcmp(topic,"x")==0){
      moveX();
    }
    if (strcmp(topic,"y")==0){
      moveY();
    }
	if (strcmp(topic,"infravermelho")==0){
		alterar_infravermelho();
    }
        
}
  
//Função: reconecta-se ao broker MQTT (caso ainda não esteja conectado ou em caso de a conexão cair)
//        em caso de sucesso na conexão ou reconexão, o subscribe dos tópicos é refeito.
void reconnectMQTT() 
{
    while (!MQTT.connected()) 
    {
        Serial.print("* Tentando se conectar ao Broker MQTT: ");
        Serial.println(BROKER_MQTT);
        if (MQTT.connect("ifsp", "woittipk", "TTa4plzNwzBE")) 
        {
            Serial.println("Conectado com sucesso ao broker MQTT!");
            MQTT.subscribe(TOPICO_SUBSCRIBE);
            MQTT.subscribe(TOPICO_SUBSCRIBE2); 
            MQTT.subscribe(TOPICO_SUBSCRIBE3); 
        } 
        else
        {
            Serial.println("Falha ao reconectar no broker.");
            Serial.println("Havera nova tentativa de conexao em 2s");
            delay(2000);
        }
    }
}
  
//Função: reconecta-se ao WiFi
void reconectWiFi() 
{
    //se já está conectado a rede WI-FI, nada é feito. 
    //Caso contrário, são efetuadas tentativas de conexão
    if (WiFi.status() == WL_CONNECTED)
        return;
         
    WiFi.begin(SSID, PASSWORD); // Conecta na rede WI-FI
     
    while (WiFi.status() != WL_CONNECTED) 
    {
        delay(100);
        Serial.print(".");
    }
   
    Serial.println();
    Serial.print("Conectado com sucesso na rede ");
    Serial.print(SSID);
    Serial.println("IP obtido: ");
    Serial.println(WiFi.localIP());
}
 
//Função: verifica o estado das conexões WiFI e ao broker MQTT. Em caso de desconexão (qualquer uma das duas), a conexão é refeita.
void VerificaConexoesWiFIEMQTT(void){
  if (!MQTT.connected()) 
    reconnectMQTT(); //se não há conexão com o Broker, a conexão é refeita
  reconectWiFi(); //se não há conexão com o WiFI, a conexão é refeita
}

void recebeOutputMQTT(void){}
//Função: envia ao Broker o estado atual do output 
void EnviaEstadoOutputMQTT(void){
  if (EstadoSaida == '0')
    MQTT.publish(TOPICO_PUBLISH, "D");
  
  if (EstadoSaida == '1')
    MQTT.publish(TOPICO_PUBLISH, "L");
  
  Serial.println("- Estado da saida D0 enviado ao broker!");
  delay(1000);
}
 
//Função: inicializa o output em nível lógico baixo
void InitOutput(void){
  //IMPORTANTE: o Led já contido na placa é acionado com lógica invertida (ou seja,enviar HIGH para o output faz o Led apagar / enviar LOW faz o Led acender)
  pinMode(D0, OUTPUT);
  digitalWrite(D0, HIGH);          
}
 
 
//programa principal
void loop(){ 
  VerificaConexoesWiFIEMQTT();
  MQTT.loop();
}

void moveX(){
  if(message!=0){ 
    if((pos_x+message>=0)and(pos_x+message<=180)){
      pos_x=pos_x+message;              
      myservo_x.write(pos_x);
      message=0;
      Serial.print("atual x:");
      Serial.println(pos_x);
    }
  }
  
}

void moveY(){
  if(message!=0){ 
    if((pos_y+message>=0)and(pos_y+message<=180)){
      pos_y=pos_y+message;              
      myservo_y.write(pos_y);
      message=0;
      Serial.print("atual y:");
      Serial.println(pos_y);
    }
  }
}

void alterar_infravermelho(){
	if(message==0){
		digitalWrite(infravermelho, LOW);
	}
	if(message==1){
		digitalWrite(infravermelho, HIGH);
	}
}







