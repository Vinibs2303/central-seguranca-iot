# Central de Segurança IoT – Detecção de Intrusos

Sistema embarcado de detecção de intrusos simulado no **Wokwi** com **ESP32**, sensor **PIR**, LEDs, buzzer e botão. Ao detectar movimento com o sistema armado, aciona o alarme local, envia notificação ao **Telegram** e registra o evento no **Firebase Realtime Database**. Uma página HTML simples mostra os eventos em tempo real.

## Como funciona
- **Botão:** alterna entre DESARMADO (LED verde) e ARMADO (LED vermelho).
- **Desarmado:** movimento é ignorado, sem alarme nem notificação.
- **Armado + movimento:** buzzer toca (BEEP-BEEP por 5 s), mensagem no Telegram e evento gravado no Firebase.
- **Anti-spam:** no máximo 1 notificação a cada 10 s; movimentos extras são registrados como "Bloqueada".

## Componentes e pinos
| Componente | Pino ESP32 |
|---|---|
| PIR (OUT) | GPIO 27 |
| LED verde | GPIO 26 |
| LED vermelho | GPIO 25 |
| Buzzer | GPIO 33 |
| Botão | GPIO 14 |

## Estrutura
```
wokwi/   sketch.ino, diagram.json, libraries.txt  (copiar para o projeto no Wokwi)
web/     index.html                                (página que lê o Firebase)
docs/    respostas da atividade e prints
```

## Como rodar
1. **Wokwi:** crie um projeto ESP32 e copie os 3 arquivos da pasta `wokwi/`.
2. **Telegram:** crie um bot no @BotFather, pegue o token e o Chat ID (via `getUpdates`).
3. **Firebase:** crie um Realtime Database em modo de teste e copie o endereço.
4. **Configure** no `sketch.ino`: `BOT_TOKEN`, `CHAT_ID` e `FIREBASE_HOST` (sem `https://`).
5. **Configure** no `web/index.html`: `FIREBASE_URL_RAW`.
6. Rode a simulação, arme com o botão azul e clique no PIR em *Simulate motion*.
7. Abra `web/index.html` no navegador para ver os eventos.

## Segurança
O repositório **não contém credenciais**. Nunca suba o token do bot: quem o tem controla o bot. Se ele vazar, gere outro com `/revoke` no @BotFather.
