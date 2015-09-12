import vtk.qt

try:
    import PyQt5
    vtk.qt.PyQtImpl = "PyQt5"
except ImportError:
    raise ImportError("Cannot load PyQt5")

from vtk.qt.QVTKRenderWindowInteractor import *

if __name__ == "__main__":
    print PyQtImpl
    QVTKRenderWidgetConeExample()
