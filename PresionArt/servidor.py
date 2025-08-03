import uasyncio as asyncio
import json

class ServidorWeb:
    def __init__(self, monitor):
        self.monitor = monitor

    async def serve_client(self, reader, writer):
        req_line = await reader.readline()
        req = req_line.decode()
        while await reader.readline() != b"\r\n":
            pass

        if "GET /data" in req:
            presiones_suaves = self.monitor.suavizado(self.monitor.presiones, ventana=3)
            data = {
                "sistolica": round(self.monitor.sistolica,1) if self.monitor.sistolica else None,
                "diastolica": round(self.monitor.diastolica,1) if self.monitor.diastolica else None,
                "map": round(self.monitor.map_presion,1) if self.monitor.map_presion else None,
                "presiones": presiones_suaves[-50:],
                "historial": self.monitor.historial,
            }
            response = 'HTTP/1.0 200 OK\r\nContent-Type: application/json\r\n\r\n'
            response += json.dumps(data)
        else:
            response = """HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n
            <html>
            <head>
                <title>Monitor de Presión</title>
                <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
            </head>
            <body style="font-family:Arial; background:#fdfdfd;">
                <h2 style="color:#2c3e50;">Mediciones de Presión</h2>
                <p><b>Sistólica:</b> <span id="sist">--</span> mmHg</p>
                <p><b>Diastólica:</b> <span id="diast">--</span> mmHg</p>
                <p><b>MAP:</b> <span id="map">--</span> mmHg</p>
                <canvas id="chart" width="400" height="200"></canvas>
                <script>
                const ctx = document.getElementById('chart').getContext('2d');
                let chart = new Chart(ctx, {
                    type: 'line',
                    data: { labels: [], datasets: [{ 
                        label: 'Presión',
                        data: [], borderColor:'#e74c3c',
                        fill:false, tension:0.4, pointBackgroundColor:'#2980b9'
                    }] },
                    options: {
                        scales: {
                            y: { title: { display:true, text:'mmHg' } },
                            x: { title: { display:true, text:'Tiempo' } }
                        }
                    }
                });
                async function actualizar(){
                    const resp = await fetch('/data');
                    const data = await resp.json();
                    if(data.sistolica) document.getElementById('sist').innerText = data.sistolica;
                    if(data.diastolica) document.getElementById('diast').innerText = data.diastolica;
                    if(data.map) document.getElementById('map').innerText = data.map;
                    chart.data.labels = data.presiones.map((v,i)=>i);
                    chart.data.datasets[0].data = data.presiones;
                    chart.update();
                }
                setInterval(actualizar, 1000);
                </script>
            </body>
            </html>
            """
        writer.write(response)
        await writer.drain()
        await writer.wait_closed()

    async def iniciar(self):
        server = await asyncio.start_server(self.serve_client, "0.0.0.0", 80)
        print("Servidor web listo en http://0.0.0.0:80")
        async with server:
            await server.serve_forever()
