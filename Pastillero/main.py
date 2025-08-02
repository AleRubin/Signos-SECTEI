import time, network, ntptime
import gc9a01
from machine import Pin, SPI, RTC
from pantalla import Pantalla
from motor import Motor
from touch import Touch
from telegram_boot import TelegramBot

# ---------------- Configuración WiFi ----------------
SSID = "RED"       # <-- Cambia por tu red
PASSWORD = "Contraseña"  # <-- Cambia por tu contraseña
        
#---------------Conexión WiFi----------------------------
def conectar_wifi():
    wlan = network.WLAN(network.STA_IF)
    wlan.active(True)
    if not wlan.isconnected():
        print("Conectando a WiFi...")
        wlan.connect(SSID, PASSWORD)
        while not wlan.isconnected():
            time.sleep(0.5)
    print("WiFi conectado:", wlan.ifconfig())
    return wlan

# ---------------- Sincronización NTP ----------------
def sincronizar_hora(rtc, offset=-6):
    try:
        ntptime.settime()  # Ajusta la hora UTC
        now = rtc.datetime()
        hora_local = (now[4] + offset) % 24
        rtc.datetime((now[0], now[1], now[2], now[3], hora_local, now[5], now[6], now[7]))
        print("Hora sincronizada con NTP:", rtc.datetime())
    except Exception as e:
        print("No se pudo sincronizar la hora:", e)


# ---------------- Configuración pantalla ----------------
tft = gc9a01.GC9A01(
    SPI(2, baudrate=80000000, polarity=0, sck=Pin(10), mosi=Pin(11)),
    240,
    240,
    reset=Pin(14, Pin.OUT),
    cs=Pin(9, Pin.OUT),
    dc=Pin(8, Pin.OUT),
    backlight=Pin(2, Pin.OUT),
    rotation=2)

tft.init()
rtc = RTC()

pantalla = Pantalla(tft, rtc, ["MED 1", "MED 2", "MED 3", "MED 4"], [5, 1, 0, 4])
motor = Motor()
touch = Touch()
telegram = TelegramBot("8048494037:AAFN-xQkU0VniW6eRkBYMgtSvf4sMFop9dU", "<AQUI_TU_CHAT_ID>")


conectar_wifi()
sincronizar_hora()  # ajusta tu zona horaria (ej: -6 para México)

vista = 0
ultimo_toque = time.time()
ultimo_cambio_auto = time.time()
ultimo_motor = time.time()

tft.fill(gc9a01.BLACK)
pantalla.mostrar_hora()

while True:
    now = time.time()

    if vista == 0:
        pantalla.mostrar_hora()

    if touch.hay_toque():
        vista = (vista + 1) % 3
        if vista == 0: pantalla.mostrar_hora()
        elif vista == 1: pantalla.mostrar_cantidades()
        elif vista == 2: pantalla.mostrar_qr()
        ultimo_toque = now
        while touch.hay_toque(): time.sleep(0.05)
        time.sleep(0.2)

    if now - ultimo_toque >= 10 and now - ultimo_cambio_auto >= 5:
        vista = (vista + 1) % 3
        if vista == 0: pantalla.mostrar_hora()
        elif vista == 1: pantalla.mostrar_cantidades()
        elif vista == 2: pantalla.mostrar_qr()
        ultimo_cambio_auto = now

    if now - ultimo_motor >= 20:
        motor.activar()
        telegram.enviar_mensaje("Motor activado")
        ultimo_motor = now

    motor.verificar()
    time.sleep(0.1)
