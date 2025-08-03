from machine import Pin, I2C
import time

class SensorPresion:
    def __init__(self, scl=22, sda=21, freq=100000, sensor_id=0x18):
        self.i2c = I2C(0, scl=Pin(scl), sda=Pin(sda), freq=freq)
        self.sensor_id = sensor_id
        self.cmd = bytearray([0xAA, 0x00, 0x00])
        self.counts_80mmHg = 1350819.0
        self.counts_100mmHg = 1542201.0

    def map_float(self, x, in_min, in_max, out_min, out_max):
        return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min

    def leer(self):
        try:
            self.i2c.writeto(self.sensor_id, self.cmd)
            time.sleep_ms(10)
            data = self.i2c.readfrom(self.sensor_id, 4)
            if len(data) == 4:
                counts = int(data[3]) + int(data[2]) * 256 + int(data[1]) * 65536
                return self.map_float(counts, self.counts_80mmHg, self.counts_100mmHg, 80.0, 100.0)
        except:
            return None
        return None
