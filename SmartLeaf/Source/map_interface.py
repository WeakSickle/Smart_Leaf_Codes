import io
import folium
from PyQt5 import QtCore, QtWebEngineWidgets

class FoliumMap(QtWebEngineWidgets.QWebEngineView):
    map_loaded = QtCore.pyqtSignal()
    # Default for coords for centre
    def __init__(self, parent=None, coordinates=(-36.89, 174.72), zoom=12):
        super().__init__(parent)
        self.current_map = None
        # Setup the map with initial settings
        self.set_folium_map(coordinates, zoom)

    def set_folium_map(self, coordinates, zoom_level):
        '''Set the map to the given central coordinates, this is where inital config can be set'''
        # Check what zoom level is being set
        print(zoom_level)
        print("Setting map")
        self.current_map = folium.Map(location=coordinates, zoom_start=zoom_level)
        
        # Add the central marker to the map for home
        self.add_marker(coordinates, "Home")
        data = io.BytesIO()
        self.current_map.save(data, close_file=False)
        self.setHtml(data.getvalue().decode('utf-8'))
        
    
    def add_marker(self, coordinates, popup_text='Marker'):
        '''Add a marker to the current map for given coordinates and name'''
        folium.Marker(coordinates, popup=popup_text,tooltip=popup_text, icon=folium.Icon(color='darkblue', icon_color='white',icon='tint')).add_to(self.current_map)
        data = io.BytesIO()
        self.current_map.save(data, close_file=False)
        self.setHtml(data.getvalue().decode('utf-8'))
