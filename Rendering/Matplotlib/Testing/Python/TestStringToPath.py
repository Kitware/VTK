#!/usr/bin/env python

from vtkmodules.vtkCommonCore import (
    vtkIntArray,
    vtkPoints,
)

from vtkmodules.vtkCommonDataModel import (
    vtkPath,
)

from vtkmodules.vtkViewsContext2D import (
    vtkContextView,
)

from vtkmodules.vtkRenderingContext2D import (
    vtkContext2D,
    vtkContextItem,
    vtkContextScene,
    vtkPen,
)

from vtkmodules.vtkRenderingCore import (
    vtkRenderer,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkTextProperty,
)

from vtkmodules.vtkRenderingFreeType import (
    vtkMathTextUtilities,
)

from vtkmodules.vtkPythonContext2D import (
    vtkPythonItem,
)

import vtkmodules.vtkRenderingContextOpenGL2
import vtkmodules.vtkRenderingMatplotlib
import vtkmodules.vtkRenderingOpenGL2
import vtkmodules.vtkRenderingUI

#------------------------------------------------------------------------------
class StringToPathContextTest(object):
    def __init__(self):
        self.Path = None

    def SetPath(self, path):
        self.Path = path

    def Initialize(self, vtkSelf):
        return True

    def Paint(self, vtkSelf, painter):
        # Paint event for the chart, called whenever the chart needs to be drawn
        # This function aims to test the primitives provided by the 2D API.
        # RGB color lookup table by path point code:
        color = [(), (), (), ()]
        color[vtkPath.MOVE_TO] = (1.0, 0.0, 0.0)
        color[vtkPath.LINE_TO] = (0.0, 1.0, 0.0)
        color[vtkPath.CONIC_CURVE] = (0.0, 0.0, 1.0)
        color[vtkPath.CUBIC_CURVE] = (1.0, 0.0, 1.0)

        points = self.Path.GetPoints()
        codes = self.Path.GetCodes()

        if points.GetNumberOfPoints() != codes.GetNumberOfTuples():
            return False

        # scaling factor and offset to ensure that the points will fit the view:
        scale = 5.16591
        offset = 20.0

        # Draw the control points, colored by codes:
        point = [0.0, 0.0, 0.0]
        painter.GetPen().SetWidth(2)
        for i in range(points.GetNumberOfPoints()):
            points.GetPoint(i, point)
            code = codes.GetValue(i)

            painter.GetPen().SetColorF(color[code])
            painter.DrawPoint(point[0] * scale + offset, point[1] * scale + offset)

        return True

#------------------------------------------------------------------------------
# Set up a 2D context view, context test object and add it to the scene
view = vtkContextView()
ren = view.GetRenderer()
ren.SetBackground(1.0, 1.0, 1.0)
renWin = view.GetRenderWindow()
renWin.SetSize(325, 150)
test = vtkPythonItem()
testObject = StringToPathContextTest()
test.SetPythonObject(testObject)
view.GetScene().AddItem(test)

path = vtkPath()
tprop = vtkTextProperty()

vtkMathTextUtilities.GetInstance().StringToPath(
    "$\\frac{-b\\pm\\sqrt{b^2-4ac}}{2a}$", path, tprop, view.GetRenderWindow().GetDPI())

testObject.SetPath(path)

renWin.SetMultiSamples(0)
iren = view.GetInteractor()
iren.Initialize()
if 'rtTester' not in locals() or rtTester.IsInteractiveModeSpecified():
    iren.Start()
