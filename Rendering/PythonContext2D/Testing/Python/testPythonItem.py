import sys
from vtkmodules.vtkCommonCore import (
    vtkPoints,
    vtkUnsignedCharArray,
)
from vtkmodules.vtkCommonDataModel import (
    vtkCellArray,
    vtkPolyData,
    vtkRectd,
    vtkRecti,
)
from vtkmodules.vtkChartsCore import (
    vtkAxis,
    vtkInteractiveArea,
)
from vtkmodules.vtkPythonContext2D import vtkPythonItem
from vtkmodules.vtkRenderingCore import (
    VTK_SCALAR_MODE_USE_CELL_DATA,
    vtkTextProperty,
)
from vtkmodules.vtkRenderingContext2D import vtkMarkerUtilities
from vtkmodules.vtkTestingRendering import vtkTesting
from vtkmodules.vtkViewsContext2D import vtkContextView
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingContextOpenGL2
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.test import Testing


class CustomPythonItem(object):
    def __init__(self, polydata):
        self.polydata = polydata

    def Initialize(self, vtkSelf):
        return True

    def Paint(self, vtkSelf, context2D):
        context2D.DrawPolyData(0.0,
                               0.0,
                               self.polydata,
                               self.polydata.GetCellData().GetScalars(),
                               VTK_SCALAR_MODE_USE_CELL_DATA)

        pen = context2D.GetPen()

        penColor = [0, 0, 0]
        pen.GetColor(penColor)
        penWidth = pen.GetWidth()

        brush = context2D.GetBrush()
        brushColor = [0, 0, 0, 0]
        brush.GetColor(brushColor)

        pen.SetColor([0, 0, 255])
        brush.SetColor([0, 0, 255])
        context2D.DrawWedge(0.75, 0.25, 0.125, 0.005, 30.0, 60.0)

        pen.SetWidth(20.0)
        pen.SetColor([0, 0, 0])
        brush.SetColor([0, 0, 0])
        context2D.DrawMarkers(vtkMarkerUtilities.CIRCLE, False, [0.1, 0.1, 0.5, 0.5, 0.9, 0.9], 3)
        pen.SetWidth(1.0)

        textProp = vtkTextProperty()
        textProp.BoldOn()
        textProp.ItalicOn()
        textProp.SetFontSize(22)
        textProp.SetColor(0.5, 0.0, 1.0)
        textProp.SetOrientation(45)
        context2D.ApplyTextProp(textProp)
        context2D.DrawString(0.35, 0.4, "Context2D!")

        pen.SetColor([200, 200, 30])
        brush.SetColor([200, 200, 30])
        brush.SetOpacity(128)
        context2D.DrawPolygon([0.5, 0.5, 0.75, 0.0, 1.0, 0.5], 3)

        pen.SetColor([133, 70, 70])
        brush.SetColor([133, 70, 70])
        brush.SetOpacity(255)
        context2D.DrawArc(0.25, 0.75, 0.125, 0.0, 360.0)

        pen.SetWidth(penWidth)
        pen.SetColor(penColor)
        brush.SetColor(brushColor[:3])
        brush.SetOpacity(brushColor[3])

        return True


def buildPolyData():
    pd = vtkPolyData()

    pts = vtkPoints()
    pts.InsertNextPoint([0.1, 0.1, 0.0])
    pts.InsertNextPoint([0.9, 0.9, 0.0])

    pd.SetPoints(pts)

    lines = vtkCellArray()
    lines.InsertNextCell(2)
    lines.InsertCellPoint(0)
    lines.InsertCellPoint(1)

    pd.SetLines(lines)

    colors = vtkUnsignedCharArray()
    colors.SetNumberOfComponents(4)
    colors.InsertNextTypedTuple([27, 128, 89, 255])

    pd.GetCellData().SetScalars(colors)

    return pd


class TestPythonItem(Testing.vtkTest):
    def testPythonItem(self):
        width = 400
        height = 400

        view = vtkContextView()
        renWin = view.GetRenderWindow()
        renWin.SetSize(width, height)

        area = vtkInteractiveArea()
        view.GetScene().AddItem(area)

        drawAreaBounds = vtkRectd(0.0, 0.0, 1.0, 1.0)

        vp = [0.05, 0.95, 0.05, 0.95]
        screenGeometry = vtkRecti(int(vp[0] * width),
                                      int(vp[2] * height),
                                      int((vp[1] - vp[0]) * width),
                                      int((vp[3] - vp[2]) * height))

        item = vtkPythonItem()
        item.SetPythonObject(CustomPythonItem(buildPolyData()))
        item.SetVisible(True)
        area.GetDrawAreaItem().AddItem(item)

        area.SetDrawAreaBounds(drawAreaBounds)
        area.SetGeometry(screenGeometry)
        area.SetFillViewport(False)
        area.SetShowGrid(False)

        axisLeft = area.GetAxis(vtkAxis.LEFT)
        axisRight = area.GetAxis(vtkAxis.RIGHT)
        axisBottom = area.GetAxis(vtkAxis.BOTTOM)
        axisTop = area.GetAxis(vtkAxis.TOP)
        axisTop.SetVisible(False)
        axisRight.SetVisible(False)
        axisLeft.SetVisible(False)
        axisBottom.SetVisible(False)
        axisTop.SetMargins(0, 0)
        axisRight.SetMargins(0, 0)
        axisLeft.SetMargins(0, 0)
        axisBottom.SetMargins(0, 0)

        renWin.Render()

        # Create a vtkTesting object
        rtTester = vtkTesting()
        rtTester.SetRenderWindow(renWin)

        for arg in sys.argv[1:]:
            rtTester.AddArgument(arg)

        # Perform the image comparison test and print out the result.
        result = rtTester.RegressionTest(0.0)

        if result == 0:
            raise Exception("TestPythonItem failed.")

if __name__ == "__main__":
    Testing.main([(TestPythonItem, 'test')])
