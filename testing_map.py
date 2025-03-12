import sys
from PyQt5 import QtCore, QtWidgets
import requests
from map_interface import FoliumMap


class MainWindow(QtWidgets.QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle('Folium Map test')
        self.setGeometry(100, 100, 800, 600)
        
        # Get the current location and set the map to that location
        response = requests.get('https://ipinfo.io')
        data = response.json()
        latitude, longitude = map(float, data['loc'].split(','))

        # This should be all you need to start the map instance
        self.map = FoliumMap()

        # Set the map as the central widget
        self.setCentralWidget(self.map)
        
        # QtCore.QTimer.singleShot(100, lambda: (
        #     self.map.set_folium_map((latitude, longitude)),
        
        # ))
    
    def Update_markers(self, coordinates, popup_text):
        self.map.add_marker(coordinates, popup_text)

if __name__ == '__main__':
    QtWidgets.QApplication.setAttribute(QtCore.Qt.AA_EnableHighDpiScaling, True)
    QtWidgets.QApplication.setAttribute(QtCore.Qt.AA_UseHighDpiPixmaps, True)
    
    app = QtWidgets.QApplication(sys.argv)
    window = MainWindow()
    window.show()

    # window.map.map_loaded.connect(lambda: window.map.add_marker((-36.89, 174.72), "Device 2"))
    # window.map.add_marker((-36.89, 174.72), "Device 2")
    # Handle 'U' key press
    window.keyPressEvent = lambda event: window.Update_markers((-35, 174), "Device 5") if event.key() == 85 else None

    sys.exit(app.exec_())