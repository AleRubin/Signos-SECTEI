import uasyncio as asyncio
import network
import time
from proyecto_bp import Bomba, Valvula, SensorPresion, Leds, MemoriaSD, MonitorPresion, ServidorWeb

SSID = "INFINITUM87F5"
PASSWORD = "Mj9Ff6Qm3w"

async def main():
    # --- Conectar WiFi ---
    wlan = network.WLAN(network.STA_IF)
    wlan.active(True)
    wlan.connect(SSID, PASSWORD)
    print("Conectando a WiFi...")
    while not wlan.isconnected():
        time.sleep(1)
    print("Conectado a WiFi:", wlan.ifconfig())

    # --- Inicializar módulos ---
    bomba = Bomba()
    valvula = Valvula()
    sensor = SensorPresion()
    leds = Leds()
    memoria = MemoriaSD()
    monitor = MonitorPresion(bomba, valvula, sensor, leds, memoria)
    servidor = ServidorWeb(monitor)

    # --- Ejecutar monitor y servidor ---
    asyncio.create_task(monitor.iniciar())
    await servidor.iniciar()

asyncio.run(main())
