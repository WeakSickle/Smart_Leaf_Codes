import sys
import numpy as np
from PyQt5.QtWidgets import (QApplication, QMainWindow, QWidget,QVBoxLayout, QHBoxLayout, QComboBox)
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.backends.backend_qt5agg import NavigationToolbar2QT as NavigationToolbar
from matplotlib.figure import Figure

# this is teh main important class that would be needed for the main ui of the graphing page 
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
        
        # Dictionary of available plot functions
        self.plot_types = {
            'Sine Wave': self.plot_sin,
            'Cosine Wave': self.plot_cos,
            'Random Walk': self.plot_random,
            'Quadratic': self.plot_quadratic
        }

    def update_plot(self, plot_name):
        """Execute the selected plot function"""
        if plot_name in self.plot_types:
            self.plot_types[plot_name]()
            
    def clear_plot(self):
        """Clear the axes"""
        self.axes.clear()
        self.canvas.draw()

    # Functions to plot different graphs (this would be replaced with just accessing teh db with given id)
    def plot_sin(self):
        self.clear_plot()
        x = np.linspace(0, 2 * np.pi, 100)
        y = np.sin(x)
        self.axes.plot(x, y, 'b-', label='Sine Wave')
        self.axes.legend()
        self.canvas.draw()

    def plot_cos(self):
        self.clear_plot()
        x = np.linspace(0, 2 * np.pi, 100)
        y = np.cos(x)
        self.axes.plot(x, y, 'r-', label='Cosine Wave')
        self.axes.legend()
        self.canvas.draw()

    def plot_random(self):
        self.clear_plot()
        y = np.random.randn(100).cumsum()
        self.axes.plot(y, 'g-', label='Random')
        self.axes.legend()
        self.canvas.draw()

    def plot_quadratic(self):
        self.clear_plot()
        x = np.linspace(-5, 5, 100)
        y = x**2
        self.axes.plot(x, y, 'm-', label='Quadratic')
        self.axes.legend()
        self.canvas.draw()

# Making a window to show it
class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle('Data Plots')
        self.setGeometry(100, 100, 800, 600)
        
        main_widget = QWidget()
        self.setCentralWidget(main_widget)
        layout = QVBoxLayout(main_widget)
        
        # Create plot widget
        self.plot_widget = PlotWidget()

        # Create dropdown selector
        self.plot_selector = QComboBox()
        self.plot_selector.addItems(self.plot_widget.plot_types.keys())
        
        
        # Add widgets to layout
        layout.addWidget(self.plot_selector)
        layout.addWidget(self.plot_widget)
        
        # Connect signals to update the current plot presented
        self.plot_selector.currentTextChanged.connect(self.plot_widget.update_plot)
        self.plot_selector.setCurrentIndex(0)  # Show first plot by default
        self.plot_widget.update_plot(self.plot_selector.currentText())

# starts the application 
if __name__ == '__main__':
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()
    sys.exit(app.exec_())