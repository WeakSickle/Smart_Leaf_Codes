import sys
import io
import folium
from PyQt5 import QtCore, QtWidgets, QtWebEngineWidgets
import requests

# Class for interacting with the map with markers and stuff 
class FoliumMap(QtWebEngineWidgets.QWebEngineView):
    map_loaded = QtCore.pyqtSignal()

    def __init__(self, parent=None):
        super().__init__(parent)
        self.loaded = False
        self.current_map = None  # Store the Folium map reference
        
    def set_folium_map(self, coordinates):
        self.current_map = folium.Map(location=coordinates, zoom_start=12)
        data = io.BytesIO()
        self.current_map.save(data, close_file=False)
        self.setHtml(data.getvalue().decode('utf-8'))
        self.loaded = True
        self.map_loaded.emit()
        
    def add_marker(self, coordinates, popup_text='Marker'):
        folium.Marker(coordinates, popup=popup_text,tooltip=popup_text, icon=folium.Icon(color='darkblue', icon_color='white',icon='tint')).add_to(self.current_map)
        data = io.BytesIO()
        self.current_map.save(data, close_file=False)
        self.setHtml(data.getvalue().decode('utf-8'))

class MainWindow(QtWidgets.QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle('Folium Map')
        self.setGeometry(100, 100, 800, 600)
        
        self.map = FoliumMap()
        self.setCentralWidget(self.map)
        
        response = requests.get('https://ipinfo.io')
        data = response.json()
        latitude, longitude = map(float, data['loc'].split(','))
        
        QtCore.QTimer.singleShot(100, lambda: (
            self.map.set_folium_map((latitude, longitude)),
            QtCore.QTimer.singleShot(500, lambda: self.map.add_marker((latitude, longitude), "Device 1"))
        ))
    
    def Update_markers(self, coordinates, popup_text):
        self.map.add_marker(coordinates, popup_text)

if __name__ == '__main__':
    QtWidgets.QApplication.setAttribute(QtCore.Qt.AA_EnableHighDpiScaling, True)
    QtWidgets.QApplication.setAttribute(QtCore.Qt.AA_UseHighDpiPixmaps, True)
    
    app = QtWidgets.QApplication(sys.argv)
    window = MainWindow()
    window.show()

    window.map.map_loaded.connect(lambda: window.map.add_marker((-36.89, 174.72), "Device 2"))

    # Handle 'U' key press
    window.keyPressEvent = lambda event: window.Update_markers((-35, 174), "Device 5") if event.key() == 85 else None

    # Add a marker after 5 seconds (simulated delayed serial data)
    QtCore.QTimer.singleShot(5000, lambda: window.Update_markers((-36, 174), "Device 6"))


    sys.exit(app.exec_())