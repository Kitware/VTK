#!/usr/bin/env python
import sys
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()


contourValues = [
    84.0,
    105.0,
    126.0,
    148.0,
    169.0,
    191.0,
    212.0,
    233.0,
    255.0,
    276.0
]

wavelet = vtk.vtkRTAnalyticSource()

plane = vtk.vtkPlane()
plane.SetOrigin(0.0, 0.0, 0.0)
plane.SetNormal(0.0, 0.0, 1.0)

planeCut = vtk.vtkCutter()
planeCut.SetInputConnection(wavelet.GetOutputPort())
planeCut.SetCutFunction(plane)

contours = vtk.vtkContourFilter()
contours.SetInputConnection(planeCut.GetOutputPort())
contours.SetNumberOfContours(len(contourValues))
contours.SetComputeScalars(True)

for idx in range(len(contourValues)):
    contours.SetValue(idx, contourValues[idx])

stripper = vtk.vtkStripper()
stripper.SetInputConnection(contours.GetOutputPort())

stripper.Update()
pd = stripper.GetOutput()

pdbounds = pd.GetBounds()

lut = vtk.vtkLookupTable()
lut.SetNumberOfColors(len(contourValues))
lut.SetRange(84.0, 277.0)
lut.Build()

tprops = vtk.vtkTextPropertyCollection()

for i in range(len(contourValues)):
    textProp = vtk.vtkTextProperty()

    col = [0.0, 0.0, 0.0]
    lut.GetColor(contourValues[i], col)

    textProp.SetColor(*col)
    textProp.SetBackgroundColor((1.0, 1.0, 1.0))
    textProp.SetFrame(0)
    textProp.SetFontSize(16)
    textProp.SetBold(0)
    textProp.SetItalic(0)
    textProp.SetShadow(0)
    textProp.SetJustification(0)
    textProp.SetBackgroundOpacity(1.0)
    textProp.SetVerticalJustification(1)
    textProp.SetUseTightBoundingBox(0)
    textProp.SetOrientation(0.0)
    textProp.SetLineSpacing(1.1)
    textProp.SetLineOffset(0.0)

    tprops.AddItem(textProp)

tpropMap = vtk.vtkDoubleArray()
tpropMap.SetNumberOfComponents(1)

for i in range(len(contourValues)):
    tpropMap.InsertNextTypedTuple([contourValues[i]])

mappedColors = vtk.vtkUnsignedCharArray()
mappedColors.SetNumberOfComponents(4)

for i in range(pd.GetNumberOfPoints()):
    mappedColors.InsertNextTypedTuple([0, 0, 0, 255])

item = vtk.vtkLabeledContourPolyDataItem()
item.SetPolyData(pd)
item.SetTextProperties(tprops)
item.SetTextPropertyMapping(tpropMap)
item.SetLabelVisibility(1)
item.SetSkipDistance(20.0)
item.SetScalarMode(vtk.VTK_SCALAR_MODE_USE_POINT_DATA)
item.SetMappedColors(mappedColors)

width = 600
height = 600

view = vtk.vtkContextView()
renWin = view.GetRenderWindow()
renWin.SetSize(width, height)

area = vtk.vtkInteractiveArea()
view.GetScene().AddItem(area)

xmin = pdbounds[0]
ymin = pdbounds[2]
dataWidth = pdbounds[1] - pdbounds[0]
dataHeight = pdbounds[3] - pdbounds[2]
drawAreaBounds = vtk.vtkRectd(xmin, ymin, dataWidth, dataHeight)

vp = [0.0, 1.0, 0.0, 1.0]
screenGeometry = vtk.vtkRecti(int(vp[0] * width),
                              int(vp[2] * height),
                              int((vp[1] - vp[0]) * width),
                              int((vp[3] - vp[2]) * height))

area.SetDrawAreaBounds(drawAreaBounds)
area.SetGeometry(screenGeometry)
area.SetFillViewport(False)
area.SetShowGrid(False)

axisLeft = area.GetAxis(vtk.vtkAxis.LEFT)
axisRight = area.GetAxis(vtk.vtkAxis.RIGHT)
axisBottom = area.GetAxis(vtk.vtkAxis.BOTTOM)
axisTop = area.GetAxis(vtk.vtkAxis.TOP)
axisTop.SetVisible(False)
axisRight.SetVisible(False)
axisLeft.SetVisible(False)
axisBottom.SetVisible(False)
axisTop.SetMargins(0, 0)
axisRight.SetMargins(0, 0)
axisLeft.SetMargins(0, 0)
axisBottom.SetMargins(0, 0)

renWin.Render()

area.GetDrawAreaItem().AddItem(item)

renWin.Render()
