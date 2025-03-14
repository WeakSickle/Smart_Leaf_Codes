import sys
import numpy as np
from PyQt5.QtWidgets import (QApplication, QMainWindow, QWidget, 
                            QVBoxLayout, QHBoxLayout, QComboBox,
                            QStyleFactory)
from PyQt5.QtGui import QColor
from PyQt5.QtCore import Qt
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.backends.backend_qt5agg import NavigationToolbar2QT as NavigationToolbar
from matplotlib.figure import Figure

class PlotWidget(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.figure = Figure(figsize=(5, 4), dpi=100)
        self.canvas = FigureCanvas(self.figure)
        self.toolbar = NavigationToolbar(self.canvas, self)
        
        layout = QVBoxLayout()
        layout.addWidget(self.toolbar)
        layout.addWidget(self.canvas)
        self.setLayout(layout)
        
        self.axes = self.figure.add_subplot(111)
        
        self.plot_types = {
            'Sine Wave': self.plot_sin,
            'Cosine Wave': self.plot_cos,
            'Random Walk': self.plot_random,
            'Quadratic': self.plot_quadratic
        }

    def update_plot(self, plot_name):
        if plot_name in self.plot_types:
            self.plot_types[plot_name]()
            
    def clear_plot(self):
        self.axes.clear()
        self.canvas.draw()

    def plot_sin(self):
        self.clear_plot()
        x = np.linspace(0, 2 * np.pi, 100)
        y = np.sin(x)
        self.axes.plot(x, y, 'cyan', label='Sine Wave')
        self.axes.set_xlabel('Time')
        self.axes.set_ylabel('Moisture')
        self.axes.legend()
        self.canvas.draw()

    def plot_cos(self):
        self.clear_plot()
        x = np.linspace(0, 2 * np.pi, 100)
        y = np.cos(x)
        self.axes.plot(x, y, 'magenta', label='Cosine Wave')
        self.axes.set_xlabel('Time')
        self.axes.set_ylabel('Moisture')
        self.axes.legend()
        self.canvas.draw()

    def plot_random(self):
        self.clear_plot()
        y = np.random.randn(100).cumsum()
        self.axes.plot(y, 'lime', label='Random')
        self.axes.set_xlabel('Time')
        self.axes.set_ylabel('Moisture')
        self.axes.legend()
        self.canvas.draw()

    def plot_quadratic(self):
        self.clear_plot()
        x = np.linspace(-5, 5, 100)
        y = x**2
        self.axes.plot(x, y, 'yellow', label='Quadratic')
        self.axes.set_xlabel('Time')
        self.axes.set_ylabel('Moisture')
        self.axes.legend()
        self.canvas.draw()

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle('Modern Data Plots')
        self.setGeometry(100, 100, 800, 600)
        
        main_widget = QWidget()
        self.setCentralWidget(main_widget)
        layout = QVBoxLayout(main_widget)
        
        self.plot_widget = PlotWidget()
        self.plot_selector = QComboBox()
        self.plot_selector.addItems(self.plot_widget.plot_types.keys())
        
        layout.addWidget(self.plot_selector)
        layout.addWidget(self.plot_widget)
        
        self.plot_selector.currentTextChanged.connect(self.plot_widget.update_plot)
        self.plot_selector.setCurrentIndex(0)
        self.plot_widget.update_plot(self.plot_selector.currentText())

if __name__ == '__main__':
    app = QApplication(sys.argv)
    
    # Apply Fusion style
    app.setStyle(QStyleFactory.create('Fusion'))
    
    window = MainWindow()
    window.show()
    sys.exit(app.exec_())