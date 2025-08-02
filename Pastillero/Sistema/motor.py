from machine import Pin, PWM
import time

class Motor:
    def __init__(self, motor_pin=15, freq=1000, fin_carrera_pin=16):
        self.motor_pwm = PWM(Pin(motor_pin))
        self.motor_pwm.freq(freq)
        self.motor_pwm.duty_u16(0)
        self.fin_carrera = Pin(fin_carrera_pin, Pin.IN, Pin.PULL_UP)
        self.activo = False
        self.inicio = 0

    def activar(self, duty=32768): # aprox 50%
        self.motor_pwm.duty_u16(duty)
        self.activo = True
        self.inicio = time.time()
        print("Motor activado")

    def detener(self):
        self.motor_pwm.duty_u16(0)
        self.activo = False
        print("Motor detenido")

    def verificar(self, duracion=10):
        if self.activo:
            if self.fin_carrera.value() == 0:
                print("Fin de carrera -> deteniendo motor")
                self.detener()
                return True
            if time.time() - self.inicio >= duracion:
                print("Tiempo cumplido -> deteniendo motor")
                self.detener()
                return True
        return False
