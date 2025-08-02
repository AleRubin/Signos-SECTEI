import urequests

class TelegramBot:
    def __init__(self, token, chat_id):
        self.token = token
        self.chat_id = chat_id

    def enviar_mensaje(self, mensaje):
        try:
            url = "https://api.telegram.org/bot{}/sendMessage".format(self.token)
            payload = {"chat_id": self.chat_id, "text": mensaje}
            urequests.post(url, json=payload)
            print("Mensaje enviado:", mensaje)
        except Exception as e:
            print("Error enviando mensaje Telegram:", e)
