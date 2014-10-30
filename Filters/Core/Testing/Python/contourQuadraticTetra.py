

'''
=========================================================================

  Program:   Visualization Toolkit
  Module:    TestNamedColorsIntegration.py

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================
'''

import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Contour the quadratic tet type
class contourQuadraticTetra(vtk.test.Testing.vtkTest):

    def testContourQuadraticTetra(self):
	# Create a reader to load the data (quadratic tetrahedra)
        reader = vtk.vtkUnstructuredGridReader()
        reader.SetFileName(VTK_DATA_ROOT + "/Data/quadTetEdgeTest.vtk")

        tetContours = vtk.vtkContourFilter()
        tetContours.SetInputConnection(reader.GetOutputPort())
        tetContours.SetValue(0, 0.5)
        aTetContourMapper = vtk.vtkDataSetMapper()
        aTetContourMapper.SetInputConnection(tetContours.GetOutputPort())
        aTetContourMapper.ScalarVisibilityOff()
        aTetMapper = vtk.vtkDataSetMapper()
        aTetMapper.SetInputConnection(reader.GetOutputPort())
        aTetMapper.ScalarVisibilityOff()
        aTetActor = vtk.vtkActor()
        aTetActor.SetMapper(aTetMapper)
        aTetActor.GetProperty().SetRepresentationToWireframe()
        aTetActor.GetProperty().SetAmbient(1.0)
        aTetContourActor = vtk.vtkActor()
        aTetContourActor.SetMapper(aTetContourMapper)
        aTetContourActor.GetProperty().SetAmbient(1.0)

        # Create the rendering related stuff.
        # Since some of our actors are a single vertex, we need to remove all
        # cullers so the single vertex actors will render
        ren1 = vtk.vtkRenderer()
        ren1.GetCullers().RemoveAllItems()
        renWin = vtk.vtkRenderWindow()
        renWin.AddRenderer(ren1)
        iren = vtk.vtkRenderWindowInteractor()
        iren.SetRenderWindow(renWin)

        ren1.SetBackground(.1, .2, .3)

        renWin.SetSize(400, 250)

        # specify properties
        ren1.AddActor(aTetActor)
        ren1.AddActor(aTetContourActor)

        ren1.ResetCamera()
        ren1.GetActiveCamera().Dolly(1.5)
        ren1.ResetCameraClippingRange()

        renWin.Render()

        img_file = "contourQuadraticTetra.png"
        vtk.test.Testing.compareImage(iren.GetRenderWindow(), vtk.test.Testing.getAbsImagePath(img_file), threshold=25)
        vtk.test.Testing.interact()

if __name__ == "__main__":
     vtk.test.Testing.main([(contourQuadraticTetra, 'test')])