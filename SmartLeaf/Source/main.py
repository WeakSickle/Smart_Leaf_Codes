from PyQt5 import QtCore, QtGui, QtWidgets
import sys
from HomeUI import UI_Home
#from MapUI3 import UI_Map
from map_interface import FoliumMap

if __name__ == "__main__":
    import sys
    app = QtWidgets.QApplication(sys.argv)
    window = UI_Home()
    window.show()
    sys.exit(app.exec_())