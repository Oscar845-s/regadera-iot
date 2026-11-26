# Conexiones

Montaje con dos protoboards y el ESP32 (DevKit de 30 pines) como puente entre ellas:
pines izquierdos en la **columna j** de la protoboard izquierda y pines derechos en la **columna A** de la derecha, ambos desde la fila 1.

## Pines del ESP32 usados

| Pin ESP32 | GPIO | Hueco | Conectado a |
|---|---|---|---|
| 3V3 | — | j1 | Riel + izquierdo (3.3 V) |
| GND | — | j2 | Riel − izquierdo |
| D4 | 4 | j5 | DATA del DHT22 |
| VIN | — | A1 | Riel + derecho (5 V del USB) |
| GND | — | A2 | Riel − derecho |
| D26 | 26 | A7 | IN1 del módulo de relés |
| D34 | 34 (ADC1_CH6) | A12 | AO del módulo del sensor de suelo |

## Cableado por componente

### Alimentación
| Desde | Hasta |
|---|---|
| f1 | Riel + izquierdo |
| f2 | Riel − izquierdo |
| E1 | Riel + derecho |
| E2 | Riel − derecho |

### Sensor de suelo (módulo FC-28 en c25–c28)
| Pin del módulo | Hueco | Cable hasta |
|---|---|---|
| VCC | c25 | a25 → riel + izquierdo (3.3 V) |
| GND | c26 | a26 → riel − izquierdo |
| DO | c27 | sin conectar |
| AO | c28 | e28 → E12 |

La sonda YL-69 se conecta a los pines + y − del otro lado del módulo (sin polaridad).

### DHT22 (módulo de 3 pines en c20–c22)
| Pin | Hueco | Cable hasta |
|---|---|---|
| + / VCC | c20 | a20 → riel + izquierdo |
| OUT | c21 | e21 → f5 |
| − / GND | c22 | a22 → riel − izquierdo |

Si es el sensor suelto de 4 pines, agregar una resistencia de 10 kΩ entre VCC y DATA.

### Módulo de 2 relés
| Pin | Hasta |
|---|---|
| VCC | Riel + derecho (5 V) |
| GND | Riel − derecho |
| IN1 | E7 |
| IN2 | sin conectar |

### Bomba (circuito separado)
| Desde | Hasta |
|---|---|
| + del portapilas | COM del relé 1 |
| NO del relé 1 | + de la bomba |
| − de la bomba | − del portapilas |

La bomba **nunca** se alimenta desde el ESP32: el arranque del motor provoca caídas de voltaje que reinician el microcontrolador.
