import time
import gc9a01
from bitmap import vga1_bold_16x32 as font


class Pantalla:
    def __init__(self, tft, rtc, pastillas, cantidades):
        self.tft = tft
        self.rtc = rtc
        self.pastillas = pastillas
        self.cantidades = cantidades
        self.ultimo_hora = None

    def mostrar_hora(self):
        now = self.rtc.datetime()
        hora = "{:02d}:{:02d}:{:02d}".format(now[4], now[5], now[6])
        if self.ultimo_hora:
            x = (self.tft.width() - font.WIDTH * len(self.ultimo_hora)) // 2
            y = (self.tft.height() - font.HEIGHT) // 2
            self.tft.text(font, self.ultimo_hora, x, y, gc9a01.BLACK, gc9a01.BLACK)

        x = (self.tft.width() - font.WIDTH * len(hora)) // 2
        y = (self.tft.height() - font.HEIGHT) // 2
        self.tft.text(font, hora, x, y, gc9a01.CYAN, gc9a01.BLACK)
        self.ultimo_hora = hora

    def mostrar_cantidades(self):
        self.tft.fill(gc9a01.BLACK)
        y = (self.tft.height() - (len(self.pastillas) * (font.HEIGHT + 10))) // 2
        for i, med in enumerate(self.pastillas):
            texto = "{}: {}".format(med, self.cantidades[i])
            x = (self.tft.width() - font.WIDTH * len(texto)) // 2
            self.tft.text(font, texto, x, y, gc9a01.GREEN, gc9a01.BLACK)
            y += font.HEIGHT + 10

    def mostrar_qr(self):
        self.tft.fill(gc9a01.BLACK)
        try:
            qr_width, qr_height = 165, 165
            x = (self.tft.width() - qr_width) // 2
            y = (self.tft.height() - qr_height) // 2
            self.tft.jpg("qr.jpg", x, y, gc9a01.SLOW)
        except Exception as e:
            error_msg = "Error QR"
            x = (self.tft.width() - font.WIDTH * len(error_msg)) // 2
            y = (self.tft.height() - font.HEIGHT) // 2
            self.tft.text(font, error_msg, x, y, gc9a01.RED, gc9a01.BLACK)
            print("Error cargando QR:", e)
