from machine import I2C, Pin

class Touch:
    def __init__(self, scl=7, sda=6, addr=0x15):
        self.i2c = I2C(0, scl=Pin(scl), sda=Pin(sda), freq=400000)
        self.addr = addr

    def hay_toque(self):
        try:
            data = self.i2c.readfrom_mem(self.addr, 0x01, 6)
            if data[1] == 0x1:
                print("Toque detectado:", [hex(b) for b in data])
                return True
            return False
        except:
            return False
