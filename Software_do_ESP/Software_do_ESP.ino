/*
   =====================================================================================================
   Projeto: Função de medir temperatura
   Autores: Elismar Silva Pereira, Bruna Cavalcanti
   Professor: Moacy
   =====================================================================================================


   Logs do programa:

    Versão 0.1b (24 de maio de 2019):
      -Parte do código adaptado para o ESP32;

     Versão 0.8a (12 dejunho de 2019):
      -Série de funções adiionadas
        -Sistema WiFi implantado
        -
      RESOLVER ERRO DO FREERTOS TA REINICIANDO O MCU
*/
#include <WiFi.h>

#define temperatura_interna 39
#define temperatura_externa 36
#define ldr 34
#define janela1 35
#define janela2 32
//============= Pre-definicões do ESP =================
#define Fator_Tempo_uS 1000000      //1000000uS. = 1seg Utilizado no timer 
#define SleepTime                   //5 * 1000000uS = 5seg

unsigned int tensao_ref_ADC = 5;    //5 volts
unsigned int resolucao_ADC = 2047;  //2048 bits, 2^9

const char* ID = "Controlador_01";
const char* ssid     = "Latomia";
const char* password = "prototipo";

const char* host = "10.1.24.67";
boolean troca = false;
//======================================================


//=============== Funções auxiliares ==================
void verificacaoPresenca();                        //Função para interrupção do programa quando for detectado presenca no ambiente

void sensores_E_Modulos();                        //Função para ler os módulos
void Conexao_Rede();

boolean SensoresDigitais(int GPIO);               //Função para ler sensores digitais
float Temperaturas(int porta);                    //Cálculo das temperaturas
float CalculoCorrente(int porta_sensor_corrente); //Cálculo para a corrente do ar condicionado
float FiltroADC(float valor_da_porta);

//======================================================


//=================== Variáveis ========================
float temperaturaint = 0.00;
float temperaturaext = 0.00;
float corrente = 0.00;

int intensidade_luz = 0;

boolean servidor = false;
boolean presenca = false;
boolean flag_Sensor = false;
boolean LuzAmbiente = false;
String Luz_do_ambiente_BancoD = "Apagada";
boolean janelas = false;

//=======================================================


void setup() {
  Serial.begin(115200);

  //================Verifica Redes disponíveis===============
  if (!Serial.available())
  {
    // Seta o Wifi para modo estação e disconecta do AP que foi previamente conectada
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    Serial.println("Setup feito!");
    Serial.println("");
    Serial.println("Analisando redes disponíveis");
    delay(100);
    Serial.print('.');
    delay(100);
    Serial.print('.');
    delay(100);
    Serial.println('.');

    // WiFi.scanNetworks will return the number of networks found
    int n = WiFi.scanNetworks();
    Serial.println("Redes verificadas: ");
    if (n == 0) {
      Serial.println("Nenhuma rede foi encontrada");
    } else {
      Serial.print("Rede(s) encontrada(s): ");
      Serial.println(n);
      // Print SSID e a RSSI para cada rede encontrada
      for (int i = 0; i < n; ++i) {
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.print(WiFi.SSID(i));
        Serial.print(" (");
        Serial.print(WiFi.RSSI(i));
        Serial.print(")");
        Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
        delay(10);
      }
    }
    Serial.println("");
    //============Fim Verifica Redes disponíveis===============
    /*
        Serial.println();
        Serial.println("Digite o nome (ssid) da rede: ");
        ssid = String(Serial.read());
        Serial.println("Digite a senha (password) da rede: ");
        password = String(Serial.read());
    */

  }

  // Conectando à rede WiFi
  Serial.println();
  Serial.println();
  Serial.print("Conectando com ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.println("Endereço de IP: ");
  Serial.println(WiFi.localIP());

  pinMode(temperatura_interna, INPUT);
  pinMode(temperatura_externa, INPUT);
  pinMode(ldr, INPUT);

  //================Configurações dos Núcleos===============
  Serial.printf("\nsetup() em core: %d", xPortGetCoreID());//Mostra no monitor em qual core o setup() foi chamado
  //xTaskCreatePinnedToCore(pxTaskCode, pcName, usStackDepth, pvParameters, uxPriority, pxCreatedTask, xCoreID)
  xTaskCreatePinnedToCore(loop2, "loop2", 8192, NULL, 1, NULL, 0);//Cria a tarefa "loop2()" com prioridade 1, atribuída ao core 0
  delay(1);
}

void loop() {                                              //O loop() sempre será atribuído ao core 1 automaticamente pelo sistema, com prioridade 1
  Serial.print("connecting to ");
  Serial.println(host);

  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("Falha na Conexão");
    return;
  }

  // We now create a URI for the request
  String url = "/nodemcu/salvar.php?";
  url += "Aparelho=";
  url += ID;
  url += "&IP=";
  url += WiFi.localIP();
  url += "&Temperatura_Interna=";
  url += temperaturaint;
  url += "&Temperatura_Externa=";
  url += temperaturaext;
  url += "&Luz_Ambiente=";
  url += Luz_do_ambiente_BancoD;
  url += "&Corrente=";
  url += 30;

  Serial.print("Requisitando URL: ");
  Serial.println(url);

  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return;
    }
  }

  // Read all the lines of the reply from server and print them to Serial
  while (client.available()) {
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }

  Serial.println();
  Serial.println("Conexao Fechada");

  for (int i = 0; i < 30; i++)
  {
    delay(60000);
  }

}

void loop2(void*z)                                         //o loop2() será atribuído ao core 0, com prioridade 1
{
  while (1)
  {
    sensores_E_Modulos();
    delay(1);
  }
  //precisa de ao menos um delay de 1ms para evitar o acionamento do timer0
}



//===================== Funções =========================

//Função para medir temperaturas
float Temperaturas(int porta_sensor_temperatura)
{
  unsigned int media = 0;
  unsigned int valor = 0;
  for (int i = 0; i < 30; i++)
  {
    valor = analogRead(porta_sensor_temperatura);
    media += valor;
    delay(1);
  }
  float mediafinal = media / 30;
  float temperatura = (float(mediafinal * 3.3 / 2047)) / 0.01; //Calculo da temperatura

  return temperatura;
}

//Calculo de corrente
float CalculoCorrente(int porta_sensor_corrente)
{
  float sensibilidade = 0.185;
  float ValorADC = FiltroADC(porta_sensor_corrente);
  float Corrente = ((ValorADC * tensao_ref_ADC / resolucao_ADC) - 2.5) / sensibilidade;
  return Corrente;
}


//Função para leitura dos sensores
void sensores_E_Modulos()
{
  temperaturaint = Temperaturas(temperatura_interna);
  temperaturaext = Temperaturas(temperatura_externa);
  if (analogRead(ldr) < 2047)
  {
    Luz_do_ambiente_BancoD = "Acesa";
    LuzAmbiente = true;
  }
  else
  {
    LuzAmbiente = false;
    Luz_do_ambiente_BancoD = "Apagada";
  }
  if ((SensoresDigitais(janela1) || (SensoresDigitais(janela2)))
{
  janelas = true;
}
else {
  janelas = false;
}
delay(500);
}

//Função para ler sensores digitais
boolean SensoresDigitais(int GPIO)
{
  bool sensorMedido = false;
  if (GPIO == HIGH)
  {
    sensorMedido = true;
  }
  else {
    sensorMedido = false;
  }
  return sensorMedido;
}

//=======================================================

/*
  void startTimer()
  float FiltroADC(int porta)
  {
  float media;
  Serial.println("Valor da porta: ");
  for(int
  i = 0; i < 20; i++)
  {
    float momento = (float(analogRead(porta)));
    Serial.println(momento);
    media += momento;
    delayMicroseconds(5000);
  }
  Serial.println(media/20);
  return media/20;
  }
*/
