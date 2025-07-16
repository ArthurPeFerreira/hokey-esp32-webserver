# Robô de Hockey com ESP32 e Webserver

Robô de Hockey controlado via webserver.

---

## 🚀 Visão Geral

Este projeto une hardware e software para criar um robô de hockey movido por ESP32 que recebe comandos em tempo real através de um servidor web. Você acessa uma página HTML com um joystick virtual e envia requisições GET para o robô—sem fios, sem complicação.

## 🎯 Funcionalidades

* **Servidor Web Integrado**: O ESP32 serve uma página HTML com joystick virtual.
* **Joystick Virtual**: Interface responsiva que capta movimentos e envia coordenadas normalizadas (x, y) ao robô.
* **Comunicação HTTP GET**: Cada movimento dispara uma requisição GET (`/joystick?x=...&y=...`).
* **Retorno ao Centro**: Quando soltar o stick, o robô recebe `x=0&y=0`, parando o movimento.
* **Código Simples e Elegante**: Boa base para extensões, sensores e estratégias de jogo.
