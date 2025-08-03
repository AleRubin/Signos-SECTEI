# Signos-SECTEI
Dispositivos para tomar signos vitales

Prototipo Presión Arterial

                           ┌─────────────────┐
                           │     main.py     │
                           │  (punto de arr.)│
                           └───────┬─────────┘
                                   │
         ┌─────────────────────────┼─────────────────────────┐
         │                         │                         │
         ▼                         ▼                         ▼
 ┌─────────────┐           ┌─────────────┐           ┌───────────────┐
 │  Monitor    │──────────▶│  Servidor   │           │   MemoriaSD    │
 │ Presion     │           │   Web       │           │ (memoria.py)   │
 │ (monitor.py)│           │ (servidor.py)           └───────┬───────┘
 └─────┬───────┘           └───────┬─────┘                   │
       │                           │                         │
       │ usa                       │ lee datos de            │ guarda CSV
       │                           │                         │
 ┌─────▼──────┐     ┌──────────────▼─────────────┐
 │  Sensor    │     │           LEDs             │
 │ Presion    │     │         (leds.py)          │
 │ (sensor.py)│     └────────────────────────────┘
 └─────┬──────┘
       │
       │ mide presión
       │
 ┌─────▼──────┐       ┌───────────────┐
 │   Bomba    │       │   Válvula      │
 │ (bomba.py) │       │  (valvula.py)  │
 └────────────┘       └───────────────┘

