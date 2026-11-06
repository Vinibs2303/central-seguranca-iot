# Respostas da atividade

## Parte III – Sistema inteligente
- **Desarmado + movimento:** nada acontece. O PIR pode detectar, mas o sistema ignora: sem alarme e sem notificação.
- **Movimentos seguidos:** cada ocorrência é registrada, mas só 1 notificação é enviada a cada 10 s (anti-spam).
- **Uma mensagem por movimento?** Não. Excesso de mensagens faz o usuário ignorar os alertas e pode estourar limites da API do Telegram.

## Parte IV – Controle do sistema
**Opção escolhida: A – Alternância** (cada clique alterna DESARMADO ↔ ARMADO).

## Reflexão final
1. **O que torna isso IoT?** Dispositivo com sensores e atuadores conectado à internet, enviando dados e notificações remotamente.
2. **Função do ESP32:** ler os sensores, aplicar a lógica de decisão, acionar LEDs e buzzer e se comunicar via Wi-Fi.
3. **Movimento × intrusão:** movimento é só uma leitura do sensor; intrusão é movimento com o sistema armado, dentro das regras de decisão.
4. **Por que lógica de decisão?** Sem ela o sistema dispararia a qualquer movimento, gerando alarmes falsos e mensagens em excesso.
5. **Problemas de usar só um PIR:** falsos positivos (animais, calor, correntes de ar), não identifica quem é, cobertura limitada e pode ser burlado.
