#!/usr/bin/env python
# -*- coding: utf-8 -*-



from vtkmodules.vtkCommonDataModel import vtkQuadric
from vtkmodules.vtkFiltersSources import vtkSphereSource
from vtkmodules.vtkFiltersTexture import vtkImplicitTextureCoords
from vtkmodules.vtkImagingHybrid import vtkBooleanTexture
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkDataSetMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
    vtkTexture,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
import vtkmodules.test.Testing
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

class quadricCut(vtkmodules.test.Testing.vtkTest):

    def testQuadricCut(self):

        solidTexture = (255, 255)
        clearTexture = (255, 0)
        edgeTexture = (0, 255)

        def makeBooleanTexture(caseNumber, resolution, thickness):
            #global solidTexture, clearTexture, edgeTexture
            booleanTexturecaseNumber = vtkBooleanTexture()

            booleanTexturecaseNumber.SetXSize(resolution)
            booleanTexturecaseNumber.SetYSize(resolution)
            booleanTexturecaseNumber.SetThickness(thickness)

            if caseNumber == 0:
                booleanTexturecaseNumber.SetInIn(solidTexture)
                booleanTexturecaseNumber.SetOutIn(solidTexture)
                booleanTexturecaseNumber.SetInOut(solidTexture)
                booleanTexturecaseNumber.SetOutOut(solidTexture)
                booleanTexturecaseNumber.SetOnOn(solidTexture)
                booleanTexturecaseNumber.SetOnIn(solidTexture)
                booleanTexturecaseNumber.SetOnOut(solidTexture)
                booleanTexturecaseNumber.SetInOn(solidTexture)
                booleanTexturecaseNumber.SetOutOn(solidTexture)
            elif caseNumber == 1:
                booleanTexturecaseNumber.SetInIn(clearTexture)
                booleanTexturecaseNumber.SetOutIn(solidTexture)
                booleanTexturecaseNumber.SetInOut(solidTexture)
                booleanTexturecaseNumber.SetOutOut(solidTexture)
                booleanTexturecaseNumber.SetOnOn(edgeTexture)
                booleanTexturecaseNumber.SetOnIn(edgeTexture)
                booleanTexturecaseNumber.SetOnOut(solidTexture)
                booleanTexturecaseNumber.SetInOn(edgeTexture)
            elif caseNumber == 2:
                booleanTexturecaseNumber.SetInIn(solidTexture)
                booleanTexturecaseNumber.SetOutIn(clearTexture)
                booleanTexturecaseNumber.SetInOut(solidTexture)
                booleanTexturecaseNumber.SetOutOut(solidTexture)
                booleanTexturecaseNumber.SetOnOn(edgeTexture)
                booleanTexturecaseNumber.SetOnIn(edgeTexture)
                booleanTexturecaseNumber.SetOnOut(solidTexture)
                booleanTexturecaseNumber.SetInOn(solidTexture)
                booleanTexturecaseNumber.SetOutOn(edgeTexture)
            elif caseNumber == 3:
                booleanTexturecaseNumber.SetInIn(clearTexture)
                booleanTexturecaseNumber.SetOutIn(clearTexture)
                booleanTexturecaseNumber.SetInOut(solidTexture)
                booleanTexturecaseNumber.SetOutOut(solidTexture)
                booleanTexturecaseNumber.SetOnOn(edgeTexture)
                booleanTexturecaseNumber.SetOnIn(clearTexture)
                booleanTexturecaseNumber.SetOnOut(solidTexture)
                booleanTexturecaseNumber.SetInOn(edgeTexture)
                booleanTexturecaseNumber.SetOutOn(edgeTexture)
            elif caseNumber == 4:
                booleanTexturecaseNumber.SetInIn(solidTexture)
                booleanTexturecaseNumber.SetOutIn(solidTexture)
                booleanTexturecaseNumber.SetInOut(clearTexture)
                booleanTexturecaseNumber.SetOutOut(solidTexture)
                booleanTexturecaseNumber.SetOnOn(edgeTexture)
                booleanTexturecaseNumber.SetOnIn(solidTexture)
                booleanTexturecaseNumber.SetOnOut(edgeTexture)
                booleanTexturecaseNumber.SetInOn(edgeTexture)
                booleanTexturecaseNumber.SetOutOn(solidTexture)
            elif caseNumber == 5:
                booleanTexturecaseNumber.SetInIn(clearTexture)
                booleanTexturecaseNumber.SetOutIn(solidTexture)
                booleanTexturecaseNumber.SetInOut(clearTexture)
                booleanTexturecaseNumber.SetOutOut(solidTexture)
                booleanTexturecaseNumber.SetOnOn(edgeTexture)
                booleanTexturecaseNumber.SetOnIn(edgeTexture)
                booleanTexturecaseNumber.SetOnOut(edgeTexture)
                booleanTexturecaseNumber.SetInOn(clearTexture)
                booleanTexturecaseNumber.SetOutOn(solidTexture)
            elif caseNumber == 6:
                booleanTexturecaseNumber.SetInIn(solidTexture)
                booleanTexturecaseNumber.SetOutIn(clearTexture)
                booleanTexturecaseNumber.SetInOut(clearTexture)
                booleanTexturecaseNumber.SetOutOut(solidTexture)
                booleanTexturecaseNumber.SetOnOn(edgeTexture)
                booleanTexturecaseNumber.SetOnIn(edgeTexture)
                booleanTexturecaseNumber.SetOnOut(edgeTexture)
                booleanTexturecaseNumber.SetInOn(edgeTexture)
                booleanTexturecaseNumber.SetOutOn(edgeTexture)
            elif caseNumber == 7:
                booleanTexturecaseNumber.SetInIn(clearTexture)
                booleanTexturecaseNumber.SetOutIn(clearTexture)
                booleanTexturecaseNumber.SetInOut(clearTexture)
                booleanTexturecaseNumber.SetOutOut(solidTexture)
                booleanTexturecaseNumber.SetOnOn(edgeTexture)
                booleanTexturecaseNumber.SetOnIn(clearTexture)
                booleanTexturecaseNumber.SetOnOut(edgeTexture)
                booleanTexturecaseNumber.SetInOn(clearTexture)
                booleanTexturecaseNumber.SetOutOn(edgeTexture)
            elif caseNumber == 8:
                booleanTexturecaseNumber.SetInIn(solidTexture)
                booleanTexturecaseNumber.SetOutIn(solidTexture)
                booleanTexturecaseNumber.SetInOut(solidTexture)
                booleanTexturecaseNumber.SetOutOut(clearTexture)
                booleanTexturecaseNumber.SetOnOn(edgeTexture)
                booleanTexturecaseNumber.SetOnIn(solidTexture)
                booleanTexturecaseNumber.SetOnOut(edgeTexture)
                booleanTexturecaseNumber.SetInOn(solidTexture)
                booleanTexturecaseNumber.SetOutOn(edgeTexture)
            elif caseNumber == 9:
                booleanTexturecaseNumber.SetInIn(clearTexture)
                booleanTexturecaseNumber.SetInOut(solidTexture)
                booleanTexturecaseNumber.SetOutIn(solidTexture)
                booleanTexturecaseNumber.SetOutOut(clearTexture)
                booleanTexturecaseNumber.SetOnOn(edgeTexture)
                booleanTexturecaseNumber.SetOnIn(edgeTexture)
                booleanTexturecaseNumber.SetOnOut(edgeTexture)
                booleanTexturecaseNumber.SetInOn(edgeTexture)
                booleanTexturecaseNumber.SetOutOn(edgeTexture)
            elif caseNumber == 10:
                booleanTexturecaseNumber.SetInIn(solidTexture)
                booleanTexturecaseNumber.SetInOut(solidTexture)
                booleanTexturecaseNumber.SetOutIn(clearTexture)
                booleanTexturecaseNumber.SetOutOut(clearTexture)
                booleanTexturecaseNumber.SetOnOn(edgeTexture)
                booleanTexturecaseNumber.SetOnIn(edgeTexture)
                booleanTexturecaseNumber.SetOnOut(edgeTexture)
                booleanTexturecaseNumber.SetInOn(solidTexture)
                booleanTexturecaseNumber.SetOutOn(clearTexture)
            elif caseNumber == 11:
                booleanTexturecaseNumber.SetInIn(clearTexture)
                booleanTexturecaseNumber.SetInOut(solidTexture)
                booleanTexturecaseNumber.SetOutIn(clearTexture)
                booleanTexturecaseNumber.SetOutOut(clearTexture)
                booleanTexturecaseNumber.SetOnOn(edgeTexture)
                booleanTexturecaseNumber.SetOnIn(clearTexture)
                booleanTexturecaseNumber.SetOnOut(edgeTexture)
                booleanTexturecaseNumber.SetInOn(edgeTexture)
                booleanTexturecaseNumber.SetOutOn(clearTexture)
            elif caseNumber == 12:
                booleanTexturecaseNumber.SetInIn(solidTexture)
                booleanTexturecaseNumber.SetInOut(clearTexture)
                booleanTexturecaseNumber.SetOutIn(solidTexture)
                booleanTexturecaseNumber.SetOutOut(clearTexture)
                booleanTexturecaseNumber.SetOnOn(edgeTexture)
                booleanTexturecaseNumber.SetOnIn(solidTexture)
                booleanTexturecaseNumber.SetOnOut(clearTexture)
                booleanTexturecaseNumber.SetInOn(edgeTexture)
                booleanTexturecaseNumber.SetOutOn(edgeTexture)
            elif caseNumber == 13:
                booleanTexturecaseNumber.SetInIn(clearTexture)
                booleanTexturecaseNumber.SetInOut(clearTexture)
                booleanTexturecaseNumber.SetOutIn(solidTexture)
                booleanTexturecaseNumber.SetOutOut(clearTexture)
                booleanTexturecaseNumber.SetOnOn(edgeTexture)
                booleanTexturecaseNumber.SetOnIn(edgeTexture)
                booleanTexturecaseNumber.SetOnOut(clearTexture)
                booleanTexturecaseNumber.SetInOn(clearTexture)
                booleanTexturecaseNumber.SetOutOn(edgeTexture)
            elif caseNumber == 14:
                booleanTexturecaseNumber.SetInIn(solidTexture)
                booleanTexturecaseNumber.SetInOut(clearTexture)
                booleanTexturecaseNumber.SetOutIn(clearTexture)
                booleanTexturecaseNumber.SetOutOut(clearTexture)
                booleanTexturecaseNumber.SetOnOn(edgeTexture)
                booleanTexturecaseNumber.SetOnIn(edgeTexture)
                booleanTexturecaseNumber.SetOnOut(clearTexture)
                booleanTexturecaseNumber.SetInOn(edgeTexture)
                booleanTexturecaseNumber.SetOutOn(clearTexture)
            elif caseNumber == 15:
                booleanTexturecaseNumber.SetInIn(clearTexture)
                booleanTexturecaseNumber.SetInOut(clearTexture)
                booleanTexturecaseNumber.SetOutIn(clearTexture)
                booleanTexturecaseNumber.SetOutOut(clearTexture)
                booleanTexturecaseNumber.SetOnOn(clearTexture)
                booleanTexturecaseNumber.SetOnIn(clearTexture)
                booleanTexturecaseNumber.SetOnOut(clearTexture)
                booleanTexturecaseNumber.SetInOn(clearTexture)
                booleanTexturecaseNumber.SetOutOn(clearTexture)


            booleanTexturecaseNumber.Update()
            return booleanTexturecaseNumber

        # A list of positions
        positions = []
        positions.append((-4, 4, 0))
        positions.append((-2, 4, 0))
        positions.append((0, 4, 0))
        positions.append((2, 4, 0))
        positions.append((-4, 2, 0))
        positions.append((-2, 2, 0))
        positions.append((0, 2, 0))
        positions.append((2, 2, 0))
        positions.append((-4, 0, 0))
        positions.append((-2, 0, 0))
        positions.append((0, 0, 0))
        positions.append((2, 0, 0))
        positions.append((-4, -2, 0))
        positions.append((-2, -2, 0))
        positions.append((0, -2, 0))
        positions.append((2, -2, 0))

        ren = vtkRenderer()
        renWin = vtkRenderWindow()
        renWin.AddRenderer(ren)

        # define two elliptical cylinders
        quadric1 = vtkQuadric()
        quadric1.SetCoefficients(1, 2, 0, 0, 0, 0, 0, 0, 0, -.07)

        quadric2 = vtkQuadric()
        quadric2.SetCoefficients(2, 1, 0, 0, 0, 0, 0, 0, 0, -.07)

        # create a sphere for all to use
        aSphere = vtkSphereSource()
        aSphere.SetPhiResolution(50)
        aSphere.SetThetaResolution(50)

        # create texture coordinates for all
        tcoords = vtkImplicitTextureCoords()
        tcoords.SetInputConnection(aSphere.GetOutputPort())
        tcoords.SetRFunction(quadric1)
        tcoords.SetSFunction(quadric2)

        aMapper = vtkDataSetMapper()
        aMapper.SetInputConnection(tcoords.GetOutputPort())

        # create a mapper, sphere and texture map for each case
        aTexture = []
        anActor = []
        for i in range(0, 16):
            aTexture.append(vtkTexture())
            aTexture[i].SetInputData(makeBooleanTexture(i, 256, 1).GetOutput())
            aTexture[i].InterpolateOff()
            aTexture[i].RepeatOff()
            anActor.append(vtkActor())
            anActor[i].SetMapper(aMapper)
            anActor[i].SetTexture(aTexture[i])
            anActor[i].SetPosition(positions[i])
            anActor[i].SetScale(2.0, 2.0, 2.0)
            ren.AddActor(anActor[i])


        ren.SetBackground(0.4392, 0.5020, 0.5647)
        ren.ResetCamera()
        ren.GetActiveCamera().Zoom(1.4)
        renWin.SetSize(500, 500)

        # render and interact with data

        iRen = vtkRenderWindowInteractor()
        iRen.SetRenderWindow(renWin);
        renWin.Render()

        img_file = "quadricCut.png"
        vtkmodules.test.Testing.compareImage(iRen.GetRenderWindow(), vtkmodules.test.Testing.getAbsImagePath(img_file), threshold=25)
        vtkmodules.test.Testing.interact()

if __name__ == "__main__":
     vtkmodules.test.Testing.main([(quadricCut, 'test')])
