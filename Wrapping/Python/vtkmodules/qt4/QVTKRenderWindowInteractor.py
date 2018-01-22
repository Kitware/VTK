import vtkmodules.qt

try:
    import PyQt4
    vtkmodules.qt.PyQtImpl = "PyQt4"
except ImportError:
    try:
        import PySide
        vtkmodules.qt.PyQtImpl = "PySide"
    except ImportError:
        raise ImportError("Cannot load either PyQt or PySide")

from vtkmodules.qt.QVTKRenderWindowInteractor import *

if __name__ == "__main__":
    print(PyQtImpl)
    QVTKRenderWidgetConeExample()
