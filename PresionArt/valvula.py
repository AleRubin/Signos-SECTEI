from machine import Pin, PWM

class Valvula:
    def __init__(self, pin=4):
        self.pwm = PWM(Pin(pin))
        self.pwm.freq(1000)
        self.abrir(0)

    def abrir(self, duty_percent):
        duty = int(duty_percent * 1023 / 100)
        self.pwm.duty(duty)
