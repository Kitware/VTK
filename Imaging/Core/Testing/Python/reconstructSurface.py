#!/usr/bin/env python
# -*- coding: utf-8 -*-



from vtkmodules.vtkCommonCore import vtkPoints
from vtkmodules.vtkFiltersCore import (
    vtkContourFilter,
    vtkReverseSense,
)
from vtkmodules.vtkFiltersSources import vtkProgrammableSource
from vtkmodules.vtkImagingHybrid import vtkSurfaceReconstructionFilter
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
import vtkmodules.test.Testing
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

class reconstructSurface(vtkmodules.test.Testing.vtkTest):

    def testReconstructSurface(self):

        # Read some points. Use a programmable filter to read them.
        #
        pointSource = vtkProgrammableSource()

        def readPoints():

            fp = open(VTK_DATA_ROOT + "/Data/cactus.3337.pts", "r")

            points = vtkPoints()
            while True:
                line = fp.readline().split()
                if len(line) == 0:
                    break
                if line[0] == "p":
                    points.InsertNextPoint(float(line[1]), float(line[2]), float(line[3]))
            pointSource.GetPolyDataOutput().SetPoints(points)

        pointSource.SetExecuteMethod(readPoints)

        # Construct the surface and create isosurface
        #
        surf = vtkSurfaceReconstructionFilter()
        surf.SetInputConnection(pointSource.GetOutputPort())

        cf = vtkContourFilter()
        cf.SetInputConnection(surf.GetOutputPort())
        cf.SetValue(0, 0.0)

        reverse = vtkReverseSense()
        reverse.SetInputConnection(cf.GetOutputPort())
        reverse.ReverseCellsOn()
        reverse.ReverseNormalsOn()

        map = vtkPolyDataMapper()
        map.SetInputConnection(reverse.GetOutputPort())
        map.ScalarVisibilityOff()

        surfaceActor = vtkActor()
        surfaceActor.SetMapper(map)
        surfaceActor.GetProperty().SetDiffuseColor(1.0000, 0.3882, 0.2784)
        surfaceActor.GetProperty().SetSpecularColor(1, 1, 1)
        surfaceActor.GetProperty().SetSpecular(.4)
        surfaceActor.GetProperty().SetSpecularPower(50)

        # Create the RenderWindow, Renderer and both Actors
        #
        ren = vtkRenderer()
        renWin = vtkRenderWindow()
        renWin.AddRenderer(ren)

        # Add the actors to the renderer, set the background and size
        #
        ren.AddActor(surfaceActor)
        ren.SetBackground(1, 1, 1)
        renWin.SetSize(300, 300)
        ren.GetActiveCamera().SetFocalPoint(0, 0, 0)
        ren.GetActiveCamera().SetPosition(1, 0, 0)
        ren.GetActiveCamera().SetViewUp(0, 0, 1)
        ren.ResetCamera()
        ren.GetActiveCamera().Azimuth(20)
        ren.GetActiveCamera().Elevation(30)
        ren.GetActiveCamera().Dolly(1.2)
        ren.ResetCameraClippingRange()

        # render and interact with data

        iRen = vtkRenderWindowInteractor()
        iRen.SetRenderWindow(renWin);
        renWin.Render()

        img_file = "reconstructSurface.png"
        vtkmodules.test.Testing.compareImage(iRen.GetRenderWindow(), vtkmodules.test.Testing.getAbsImagePath(img_file))
        vtkmodules.test.Testing.interact()

if __name__ == "__main__":
     vtkmodules.test.Testing.main([(reconstructSurface, 'test')])
