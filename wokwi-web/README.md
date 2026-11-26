# Simulación en wokwi.com (navegador)

Versión Arduino del proyecto para probar el circuito completo en la web, montado en **dos protoboards** con las mismas coordenadas del montaje real (ver `docs/conexiones.md`).

1. En wokwi.com crea un proyecto **ESP32** (Arduino).
2. Reemplaza `sketch.ino` y `diagram.json` con los de esta carpeta.
3. Agrega una pestaña nueva llamada `libraries.txt` con el contenido de este `libraries.txt`.
4. Opcional: pon tu usuario y AIO Key de Adafruit IO en `sketch.ino` para enviar datos a la nube.
5. Presiona ▶.

- Potenciómetro = sensor de humedad de suelo.
- LED azul = bomba.
- Clic en el DHT22 para cambiar temperatura y humedad.
