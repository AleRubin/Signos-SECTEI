import uasyncio as asyncio
from utime import ticks_ms, ticks_diff

class MonitorPresion:
    def __init__(self, bomba, valvula, sensor, leds, memoria):
        self.bomba = bomba
        self.valvula = valvula
        self.sensor = sensor
        self.leds = leds
        self.memoria = memoria

        self.fase = 0
        self.presiones, self.oscilaciones = [], []
        self.sistolica = self.diastolica = self.map_presion = None
        self.historial = []
        self.lectura_start = 0

    def suavizado(self, datos, ventana=3):
        if len(datos) < ventana:
            return datos
        smoothed = []
        for i in range(len(datos)):
            start = max(0, i - ventana + 1)
            ventana_datos = datos[start:i+1]
            smoothed.append(sum(ventana_datos)/len(ventana_datos))
        return smoothed

    async def iniciar(self):
        while True:
            presion = self.sensor.leer()

            if self.fase == 0:  # reposo
                self.bomba.apagar()
                self.valvula.abrir(0)
                self.leds.set([])

            elif self.fase == 1:  # inflado
                self.leds.set([(0,0,50)])
                self.bomba.encender()
                if presion and presion >= 120:
                    self.bomba.apagar()
                    self.valvula.abrir(40)
                    self.fase = 2
                    self.lectura_start = ticks_ms()

            elif self.fase == 2:  # lectura 15s
                self.leds.set([(50,20,0)])
                if presion:
                    self.presiones.append(presion)
                    if len(self.presiones) > 1:
                        self.oscilaciones.append(abs(self.presiones[-1] - self.presiones[-2]))
                if ticks_diff(ticks_ms(), self.lectura_start) >= 15000:
                    self.valvula.abrir(100)
                    self.fase = 3

            elif self.fase == 3:  # liberación final
                if presion and presion <= 50:
                    self.valvula.abrir(0)
                    self.fase = 4

            elif self.fase == 4:  # procesar
                if self.oscilaciones:
                    oscil_suave = self.suavizado(self.oscilaciones, ventana=5)
                    max_osc = max(oscil_suave)
                    idx_max = oscil_suave.index(max_osc)
                    self.map_presion = self.presiones[idx_max]
                    self.sistolica = next((self.presiones[i] for i in range(idx_max, -1, -1)
                                          if oscil_suave[i] <= 0.5*max_osc), self.presiones[0])
                    self.diastolica = next((self.presiones[i] for i in range(idx_max, len(oscil_suave))
                                           if oscil_suave[i] <= 0.7*max_osc), self.presiones[-1])
                    print(f"S: {self.sistolica:.1f}, D: {self.diastolica:.1f}, MAP: {self.map_presion:.1f}")
                    self.memoria.guardar_csv(self.sistolica, self.diastolica, self.map_presion)
                    self.historial.append({
                        "sistolica": self.sistolica,
                        "diastolica": self.diastolica,
                        "map": self.map_presion
                    })
                self.fase = 0  # volver a reposo

            await asyncio.sleep(0.1)
