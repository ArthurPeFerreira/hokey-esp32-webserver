#include <WiFi.h>
#include <WebServer.h>
#include <stdio.h>
#include <math.h>
#include <L298N.h>

// Pinos
#define DO_PWM_M1 16   // Pino do Arduino ligado ao PWM para controle de Velocidade do Motor 1 - Direita
#define DO_DIR_M1 4   // Pino do Arduino ligado ao IN1 para controle de Direção do Motor 1 - Direita
#define DO_PWM_M2 2   // Pino do Arduino ligado ao PWM para controle de Velocidade do Motor 2 - Esquerda
#define DO_DIR_M2 15  // Pino do Arduino ligado ao IN3 para controle de Direção do Motor 2 - Esquerda

// Declaração Motores
L298N M1(DO_PWM_M1, DO_DIR_M1);
L298N M2(DO_PWM_M2, DO_DIR_M2);

const char* ssid = "AP 65 2.4G";
const char* password = "Apf-Irnds24";

WebServer server(80);

// Página HTML com o joystick
const char* html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Controle Hokey</title>
    <style>
        :root {
        --size_joystick: 900px;
        --size_stick: 80px;
        }
        body {
            display: flex;
            justify-content: center;
            align-items: center;
            height: calc(100vh - var(--size_joystick)));
            margin: 0;
            background-color: #f0f0f0;
        }
        #joystick {
            margin-top: 20vh;
            width: var(--size_joystick);
            height: var(--size_joystick);
            background-color: #ddd;
            border-radius: 50%;
            position: relative;
            overflow: hidden;
        }
        #stick {
            width: var(--size_stick);
            height: var(--size_stick);
            background-color: #000;
            border-radius: 50%;
            position: absolute;
            align-items: center;
            top: calc(50% - (var(--size_stick) / 2));
            left: calc(50% - (var(--size_stick) / 2));
            transform: translate(-50%, -50%);
            transition: transform 0s;
            
        } 
    </style>
</head>
<body>
    <div id="joystick">
        <div id="stick"></div>
    </div>

    <script>
        const joystick = document.getElementById('joystick');
        const stick = document.getElementById('stick');
        const maxRange = joystick.clientWidth / 2 - stick.clientWidth / 2;

        let originX = joystick.clientWidth / 2;
        let originY = joystick.clientHeight / 2;

        function getDistance(x1, y1, x2, y2) {
            return Math.sqrt((x2 - x1) ** 2 + (y2 - y1) ** 2);
        }

        function onMove(event) {
            let touch = event.touches ? event.touches[0] : event;
            let x = touch.clientX - joystick.getBoundingClientRect().left;
            let y = touch.clientY - joystick.getBoundingClientRect().top;
            let distance = getDistance(originX, originY, x, y);

            if (distance < maxRange) {
                stick.style.transform = `translate(${x - originX}px, ${y - originY}px)`;
            } else {
                let angle = Math.atan2(y - originY, x - originX);
                stick.style.transform = `translate(${maxRange * Math.cos(angle)}px, ${maxRange * Math.sin(angle)}px)`;
            }

            // Envia as coordenadas para o ESP8266 via requisição GET
            let deltaX = (x - originX) / maxRange;
            let deltaY = (y - originY) / maxRange;
            let xhr = new XMLHttpRequest();
            xhr.open("GET", `/joystick?x=${deltaX.toFixed(2)}&y=${deltaY.toFixed(2)}`, true);
            xhr.send();
        }

        function onEnd() {
            stick.style.transform = 'translate(-50%, -50%)';
            let xhr = new XMLHttpRequest();
            xhr.open("GET", `/joystick?x=0&y=0`, true);
            xhr.send();
        }

        joystick.addEventListener('mousedown', onMove);
        joystick.addEventListener('touchstart', onMove);
        document.addEventListener('mousemove', onMove);
        document.addEventListener('touchmove', onMove);
        document.addEventListener('mouseup', onEnd);
        document.addEventListener('touchend', onEnd);
    </script>
</body>
</html>
)rawliteral";

// Configurações de IP fixo
IPAddress local_IP(192, 168, 1, 100);  // Substitua pelo IP desejado
IPAddress gateway(192, 168, 1, 1);     // Substitua pelo gateway da sua rede
IPAddress subnet(255, 255, 255, 0);    // Substitua pela máscara de sub-rede
IPAddress primaryDNS(8, 8, 8, 8);      // Opcional: Servidor DNS primário
IPAddress secondaryDNS(8, 8, 4, 4);    // Opcional: Servidor DNS secundário

//Variáveis Globais
double x, y;

//Funcões
double AjusteCirculoTrigonometrico(double variavel){
  if (variavel > 1.0){
    variavel = 1.0;
  }

  if (variavel < -1.0){
    variavel = -1.0;
  }

  return variavel;
}

void CalculoVelocidadeMotores(double x, double y , double &velocidade_esquerda, double &velocidade_direita){
  double modulo,angulo_radianos,angulo_graus,velocidade_maxima;

  modulo = AjusteCirculoTrigonometrico(sqrt(x * x + y * y));

  velocidade_maxima = modulo * 255.0;

  angulo_radianos = atan2(y, x); // Calcula o ângulo em radianos entre -π e π

  angulo_graus = angulo_radianos * 180 / M_PI; // Converte para graus

  // Se o ângulo em graus for negativo, podemos ajustá-lo para o intervalo de 0 a 360:
  if (angulo_graus < 0) {
    angulo_graus += 360;
  }

  if(angulo_graus < 90 && angulo_graus >= 0){
    velocidade_esquerda = velocidade_maxima;
    velocidade_direita = velocidade_maxima * (sin(angulo_radianos));
  }

  if (angulo_graus >= 90 && angulo_graus < 180) {
    velocidade_esquerda = velocidade_maxima * (sin(angulo_radianos));
    velocidade_direita = velocidade_maxima;
  }

  if(angulo_graus >= 180 && angulo_graus < 270){
    velocidade_esquerda = velocidade_maxima * (sin(angulo_radianos));
    velocidade_direita = -velocidade_maxima;
  }

  if (angulo_graus >= 270 && angulo_graus < 360) {
    velocidade_esquerda = -velocidade_maxima;
    velocidade_direita = velocidade_maxima * (sin(angulo_radianos));
  }
}

void associa_velocidade_motores() {
  double PwmM1,PwmM2;

  CalculoVelocidadeMotores(x,y,PwmM1,PwmM2);

  // Serial.print("M1: ");
  // Serial.print(PwmM1, 4);  // Imprime o valor de x com 4 casas decimais
  // Serial.print(" | M2: ");
  // Serial.println(PwmM2, 4);  // Imprime o valor de y com 4 casas decimais e adiciona uma nova linha

  // Controle do Motor 1 (Direita)
    if (PwmM1 > 0) {
    M1.setSpeed((int)PwmM1);
    M1.forward();
  } else if (PwmM1 < 0) {
    PwmM1 = PwmM1 * (-1);
    M1.setSpeed((int)PwmM1);
    M1.backward();
  } else{
    M1.stop();
  }

  if (PwmM2 > 0) {
    M2.setSpeed((int)PwmM2);
    M2.forward();
  } else if (PwmM2 < 0) {
    PwmM2 = PwmM2 * (-1);
    M2.setSpeed((int)PwmM2);
    M2.backward();
  } else{
    M2.stop();
  }
}

void setup() {
    //Inicia Serial
    Serial.begin(115200);

    // Configurar a CPU para rodar a 240 MHz
    if (setCpuFrequencyMhz(240)) {
        Serial.println("Frequência da CPU definida");
    } else {
        Serial.println("Falha ao definir a frequência da CPU");
    }

    //Configurar IP
    // if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)){
    //   Serial.println("Erro ao Configurar o IP");
    // }

    // Conectar ao Wi-Fi
    WiFi.begin(ssid, password);
    Serial.println("Conectando-se ao Wi-Fi: ");
    Serial.print(ssid);

    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }

    Serial.println(" Conectado!");
    // digitalWrite(LED_BUILTIN, LOW);

    // Exibir o IP atribuído
    Serial.print("Endereço IP: ");
    Serial.println(WiFi.localIP());

    // Iniciar o servidor
    server.on("/", []() {
        server.send(200, "text/html", html);
    });

    server.on("/joystick", []() {
        String xValue = server.arg("x").c_str();
        String yValue = server.arg("y").c_str();
        x = AjusteCirculoTrigonometrico(xValue.toDouble());
        y = -AjusteCirculoTrigonometrico(yValue.toDouble());
        server.send(204); // Resposta sem conteúdo
    });

    server.begin();
    Serial.println("Servidor HTTP iniciado");
}

void loop() {
  //Servidor HTTP
  server.handleClient();

  //Associação da Velocidade dos Motores
  associa_velocidade_motores();

  // yield(); 
}
