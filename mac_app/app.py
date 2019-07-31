import rumps
import firebase

app=rumps.App("")

def start_app():
    app.run()
    app.icon='status.png'

start_app()
