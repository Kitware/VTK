"""Qt module for VTK/Python.

Example usage:

    import sys
    import PyQt5
    from PyQt5.QtWidgets import QApplication
    from vtkmodules.qt.QVTKRenderWindowInteractor import QVTKRenderWindowInteractor

    app = QApplication(sys.argv)

    widget = QVTKRenderWindowInteractor()
    widget.Initialize()
    widget.Start()

    renwin = widget.GetRenderWindow()

For more information, see QVTKRenderWidgetConeExample() in the file
QVTKRenderWindowInteractor.py.
"""

import importlib
import sys

# PyQtImpl can be set by the user
PyQtImpl = None

# Has an implementation has been imported yet?
for impl in ["PySide6", "PyQt6", "PyQt5", "PySide2", "PyQt4", "PySide"]:
    if impl in sys.modules:
        # Sometimes an attempted import can be crufty (e.g., unclean
        # uninstalls of PyQt5), so let's try to import the actual functionality
        try:
            importlib.import_module(impl + '.QtCore')
        except Exception:
            pass
        else:
            PyQtImpl = impl
            break

# QVTKRWIBase, base class for QVTKRenderWindowInteractor,
# can be altered by the user to "QGLWidget" or "QOpenGLWidget"
# in case of rendering errors (e.g. depth check problems,
# readGLBuffer warnings...)
QVTKRWIBase = "QWidget"

__all__ = ['QVTKRenderWindowInteractor']
