import uos, sdcard
from machine import Pin, SPI
from utime import localtime

class MemoriaSD:
    def __init__(self, sck=18, mosi=23, miso=19, cs=5):
        try:
            spi = SPI(2, baudrate=1000000, polarity=0, phase=0,
                      sck=Pin(sck), mosi=Pin(mosi), miso=Pin(miso))
            self.sd = sdcard.SDCard(spi, Pin(cs, Pin.OUT))
            vfs = uos.VfsFat(self.sd)
            uos.mount(vfs, "/sd")
            print("SD montada en /sd")
        except Exception as e:
            print("Error montando SD:", e)

    def guardar_csv(self, sistolica, diastolica, map_presion):
        try:
            with open("/sd/mediciones.csv", "a") as f:
                t = localtime()
                fecha = "{:04}-{:02}-{:02} {:02}:{:02}:{:02}".format(*t[:6])
                f.write("{},{},{},{}\n".format(
                    fecha, round(sistolica,1), round(diastolica,1), round(map_presion,1)))
        except:
            print("No se pudo guardar en SD")
