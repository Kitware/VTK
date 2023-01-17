"""this is python equivalent of ./Wrapping/Tcl/vtktesting/backdrop.tcl
This script is used while running python tests translated from Tcl."""

from vtkmodules.vtkFiltersSources import vtkCubeSource
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPolyDataMapper,
)
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2

basePlane = None
baseMapper = None
base = None

backPlane = None
backMapper = None
back = None

leftPlane = None
leftMapper = None
left = None

def BuildBackdrop (minX, maxX, minY, maxY, minZ, maxZ, thickness):
    global basePlane
    global baseMapper
    global base
    global backPlane
    global backMapper
    global back
    global left
    global leftPlane
    global leftMapper

    if not basePlane:
        basePlane = vtkCubeSource()
    basePlane.SetCenter( (maxX + minX)/2.0, minY, (maxZ + minZ)/2.0)
    basePlane.SetXLength(maxX-minX)
    basePlane.SetYLength(thickness)
    basePlane.SetZLength(maxZ - minZ)

    if not baseMapper:
        baseMapper = vtkPolyDataMapper()
    baseMapper.SetInputConnection(basePlane.GetOutputPort())

    if not base:
        base = vtkActor()
    base.SetMapper(baseMapper)

    if not backPlane:
        backPlane = vtkCubeSource()
    backPlane.SetCenter( (maxX + minX)/2.0, (maxY + minY)/2.0, minZ)
    backPlane.SetXLength(maxX-minX)
    backPlane.SetYLength(maxY - minY)
    backPlane.SetZLength(thickness)

    if not backMapper:
        backMapper = vtkPolyDataMapper()
    backMapper.SetInputConnection(backPlane.GetOutputPort())

    if not back:
        back = vtkActor()
    back.SetMapper(backMapper)

    if not leftPlane:
        leftPlane = vtkCubeSource()
    leftPlane.SetCenter( minX, (maxY+minY)/2.0, (maxZ+minZ)/2.0)
    leftPlane.SetXLength(thickness)
    leftPlane.SetYLength(maxY-minY)
    leftPlane.SetZLength(maxZ-minZ)

    if not leftMapper:
        leftMapper = vtkPolyDataMapper()
    leftMapper.SetInputConnection(leftPlane.GetOutputPort())

    if not left:
        left = vtkActor()
    left.SetMapper(leftMapper)

    return [base, back, left]
