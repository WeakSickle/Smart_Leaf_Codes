from PyQt5 import QtCore, QtGui, QtWidgets
import sys
from HomeUI import UI_Home
from MapUI3 import UI_Map
from map_interface import FoliumMap

class MainWindow(QtWidgets.QMainWindow):
    def __init__(self):
        super().__init__()

        # Set up the MainWindow
        self.setWindowTitle("Main Window")
        self.setGeometry(100, 100, 1000, 800)  # Set window size

        # Set UI_Home as the central widget
        self.MainWindow = UI_Home(self)
        self.setCentralWidget(self.MainWindow)

        # Connect button in home layout to switch to second layout
        self.MainWindow.Start.clicked.connect(self.switch_to_map)

        self.show()

    def switch_to_map(self):
        # Switch to second layout
        self.ui_map = FoliumMap()
        self.setGeometry(100,100,800,600)
        self.setCentralWidget(self.ui_map)

if __name__ == "__main__":
    
    
    app = QtWidgets.QApplication(sys.argv)
    window = MainWindow()  # Instantiate MainWindow

    sys.exit(app.exec_())  # Run the application