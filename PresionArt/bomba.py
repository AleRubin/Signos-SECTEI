from machine import Pin

class Bomba:
    def __init__(self, pin1=26, pin2=14):
        self.p1 = Pin(pin1, Pin.OUT)
        self.p2 = Pin(pin2, Pin.OUT)

    def encender(self):
        self.p1.value(1)
        self.p2.value(0)

    def apagar(self):
        self.p1.value(0)
        self.p2.value(0)
