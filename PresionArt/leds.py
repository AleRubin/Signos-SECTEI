from machine import Pin
import neopixel

class Leds:
    def __init__(self, pin=27, n=4):
        self.np = neopixel.NeoPixel(Pin(pin), n)

    def set(self, colores):
        self.np.fill((0,0,0))
        for i, col in enumerate(colores):
            if i < self.np.n:
                self.np[i] = col
        self.np.write()
