# Alarma-TFG

Proyecto de mi TFG (Grado en Informática Industrial y Robótica, ETSINF-UPV). Es la parte de middleware: los flujos de Node-RED que hacen de puente entre el ESP32-S3 y la app móvil.

## Qué hace

El ESP32-S3 detecta intrusiones con un sensor HC-SR04 y expone su estado por Modbus TCP. Node-RED se conecta a él como cliente Modbus, gestiona la lógica de armado/desarmado, guarda el historial de eventos en un CSV, y avisa por Telegram cuando salta la alarma (y también se puede armar/desarmar mandando un mensaje al bot).

También expone una API REST para que la app móvil pueda hablar con el sistema sin tener que saber nada de Modbus:

- `POST /armar`
- `POST /desarmar`
- `GET /estado-sensores`
- `GET /historial` (pide un PIN)

## Cómo montarlo

1. Instala Node-RED (`npm install -g node-red`).
2. Instala el paquete `node-red-contrib-modbus` desde el gestor de paletas.
3. Importa el `flows.json` de este repo.
4. Antes de darle a Deploy, configura estas variables de entorno desde los ajustes de Node-RED:
   - `ALARM_PIN` — el PIN para descargar el historial
   - `TELEGRAM_CHAT_ID` — el chat de Telegram donde quieres recibir las alertas
5. El token del bot de Telegram se configura directamente en el nodo de Telegram (no va en el código, va como credencial).

## Estado

De momento el sensor físico es solo uno (Puerta Principal), aunque el flujo tiene margen para ampliarlo a varias zonas más adelante. La cerradura de la app es virtual, no hay ningún actuador físico detrás todavía.

## Repos relacionados

- App móvil: [App-Alarma-TFG](https://github.com/Luciapp20/App-Alarma-TFG)
