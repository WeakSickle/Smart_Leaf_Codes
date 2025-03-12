# Packages needed to run teh code 
# PyQt5, PyQtWebEngine, folium, requests


import sys
import io
import folium
from PyQt5 import QtCore, QtWidgets, QtWebEngineWidgets
import requests

#Class for the map 
class WebEngineView(QtWebEngineWidgets.QWebEngineView):
    map_loaded = QtCore.pyqtSignal()

    def __init__(self, parent=None):
        super().__init__(parent)
        self.loaded = False
        self.current_map = None  # Store the Folium map reference
        
    def set_folium_map(self, coordinates):
        # Create and store the Folium map instance
        self.current_map = folium.Map(location=coordinates, zoom_start=12)
        
        # Save to bytes buffer
        data = io.BytesIO()
        self.current_map.save(data, close_file=False)
        self.setHtml(data.getvalue().decode('utf-8'))
        self.loaded = True
        self.map_loaded.emit()
        
    def add_marker(self, coordinates, popup_text='Marker'):
        # Add marker to the stored map instance
        folium.Marker(coordinates, popup=popup_text).add_to(self.current_map)
        
        # Update the web view with modified map
        data = io.BytesIO()
        self.current_map.save(data, close_file=False)
        self.setHtml(data.getvalue().decode('utf-8'))

# Main GUI stuff that would be replaced with other GUI code
class MainWindow(QtWidgets.QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle('Folium Map')
        self.setGeometry(100, 100, 800, 600)
        
        # Initialize web view
        self.web_view = WebEngineView()
        self.setCentralWidget(self.web_view)
        
        # Get location from IP - from your thing
        response = requests.get('https://ipinfo.io')
        data = response.json()
        latitude, longitude = map(float, data['loc'].split(','))
        
        # Load map after making the window
        QtCore.QTimer.singleShot(100, lambda: (self.web_view.set_folium_map((latitude, longitude)),
            # Add marker after map loads this method using a 500ms delay 
            QtCore.QTimer.singleShot(500, lambda: self.web_view.add_marker((latitude, longitude), "Device 1"))
        ))

if __name__ == '__main__':
    QtWidgets.QApplication.setAttribute(QtCore.Qt.AA_EnableHighDpiScaling, True)
    QtWidgets.QApplication.setAttribute(QtCore.Qt.AA_UseHighDpiPixmaps, True)
    
    app = QtWidgets.QApplication(sys.argv)
    window = MainWindow()
    window.show()

    # Adding a marker through main (this waits till map is loaded to add marker)
    window.web_view.map_loaded.connect(lambda: window.web_view.add_marker((-36.89, 174.72), "Device 2"))

    sys.exit(app.exec_())