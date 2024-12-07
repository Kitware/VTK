#!/usr/bin/env python
# -*- coding: utf-8 -*-



from io import StringIO
import sys
from vtkmodules.vtkFiltersCore import (
    vtkMassProperties,
    vtkTriangleFilter,
)
from vtkmodules.vtkFiltersSources import (
    vtkConeSource,
    vtkCubeSource,
    vtkSphereSource,
)
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
from vtkmodules.vtkRenderingFreeType import vtkVectorText
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
import vtkmodules.test.Testing
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

class MassProperties(vtkmodules.test.Testing.vtkTest):

    def testMassProperties(self):

        # Create the RenderWindow, Renderer and both Actors
        #
        ren = vtkRenderer()
        renWin = vtkRenderWindow()
        renWin.AddRenderer(ren)

        cone = vtkConeSource()
        cone.SetResolution(50)

        sphere = vtkSphereSource()
        sphere.SetPhiResolution(50)
        sphere.SetThetaResolution(50)

        cube = vtkCubeSource()
        cube.SetXLength(1)
        cube.SetYLength(1)
        cube.SetZLength(1)

        sphereMapper = vtkPolyDataMapper()
        sphereMapper.SetInputConnection(sphere.GetOutputPort())

        sphereActor = vtkActor()
        sphereActor.SetMapper(sphereMapper)
        sphereActor.GetProperty().SetDiffuseColor(1, .2, .4)
        coneMapper = vtkPolyDataMapper()
        coneMapper.SetInputConnection(cone.GetOutputPort())

        coneActor = vtkActor()
        coneActor.SetMapper(coneMapper)
        coneActor.GetProperty().SetDiffuseColor(.2, .4, 1)

        cubeMapper = vtkPolyDataMapper()
        cubeMapper.SetInputConnection(cube.GetOutputPort())

        cubeActor = vtkActor()
        cubeActor.SetMapper(cubeMapper)
        cubeActor.GetProperty().SetDiffuseColor(.2, 1, .4)

        #Add the actors to the renderer, set the background and size
        #
        sphereActor.SetPosition(-5, 0, 0)
        ren.AddActor(sphereActor)
        coneActor.SetPosition(0, 0, 0)
        ren.AddActor(coneActor)
        coneActor.SetPosition(5, 0, 0)
        ren.AddActor(cubeActor)

        tf = dict()
        mp = dict()
        vt = dict()
        pdm = dict()
        ta = dict()

        def MakeText(primitive):

            tf.update({primitive: vtkTriangleFilter()})
            tf[primitive].SetInputConnection(primitive.GetOutputPort())

            mp.update({primitive: vtkMassProperties()})
            mp[primitive].SetInputConnection(tf[primitive].GetOutputPort())

            # here we capture stdout and write it to a variable for processing.
            summary = StringIO()
            # save the original stdout
            old_stdout = sys.stdout
            sys.stdout = summary

            print(mp[primitive])
            summary = summary.getvalue()

            startSum = summary.find("  VolumeX")
            endSum = len(summary)
            print(summary[startSum:])
            # Restore stdout
            sys.stdout = old_stdout

            vt.update({primitive: vtkVectorText()})
            vt[primitive].SetText(summary[startSum:])

            pdm.update({primitive: vtkPolyDataMapper()})
            pdm[primitive].SetInputConnection(vt[primitive].GetOutputPort())

            ta.update({primitive: vtkActor()})
            ta[primitive].SetMapper(pdm[primitive])
            ta[primitive].SetScale(.2, .2, .2)
            return ta[primitive]


        ren.AddActor(MakeText(sphere))
        ren.AddActor(MakeText(cube))
        ren.AddActor(MakeText(cone))

        ta[sphere].SetPosition(sphereActor.GetPosition())
        ta[sphere].AddPosition(-2, -1, 0)
        ta[cube].SetPosition(cubeActor.GetPosition())
        ta[cube].AddPosition(-2, -1, 0)
        ta[cone].SetPosition(coneActor.GetPosition())
        ta[cone].AddPosition(-2, -1, 0)

        ren.SetBackground(0.1, 0.2, 0.4)
        renWin.SetSize(786, 256)

        # render the image
        #
        ren.ResetCamera()
        cam1 = ren.GetActiveCamera()
        cam1.Dolly(3)
        ren.ResetCameraClippingRange()

        # render and interact with data

        iRen = vtkRenderWindowInteractor()
        iRen.SetRenderWindow(renWin);
        renWin.Render()

        img_file = "MassProperties.png"
        vtkmodules.test.Testing.compareImage(iRen.GetRenderWindow(), vtkmodules.test.Testing.getAbsImagePath(img_file))
        vtkmodules.test.Testing.interact()

if __name__ == "__main__":
     vtkmodules.test.Testing.main([(MassProperties, 'test')])
