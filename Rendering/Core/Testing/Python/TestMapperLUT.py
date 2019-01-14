#!/usr/bin/env python

"""This test ensures that the mapper does not change the LookupTable
that may be shared among different mappers.  Specifically, the vtkMapper
and vtkScalarsToColorsPainter classes use the LookupTable to map scalars
to colors.  When they do this they set the LookupTable's Alpha ivar.
They should do it in a manner that the LUT's original value is reset.
If this is not done strange errors show up.  For example if you use an
ImagePlaneWidget and share its LUT with another mapper that has either a
different opacity, then the image plane widget will pick up the wrong
opacity leading to subtle errors.  This test ensures that this does not
happen by testing MapScalars explicitly and also by creating an
ImagePlaneWidget.  """

import os
import os.path
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

class TestMapperLUT(Testing.vtkTest):
    def testMapScalars(self):
        """Test if the mapper sets the Alpha of the LUT."""
        # Create dummy data.
        p = vtk.vtkPolyData()
        pts = vtk.vtkPoints()
        pts.InsertNextPoint((0,0,0))
        sc = vtk.vtkFloatArray()
        sc.InsertNextValue(10.0)
        p.SetPoints(pts)
        p.GetPointData().SetScalars(sc)
        l = vtk.vtkLookupTable()
        m = vtk.vtkPolyDataMapper()
        m.SetInputData(p)
        m.SetScalarRange(0.0, 10.0)
        m.SetLookupTable(l)
        ret = m.MapScalars(0.5)
        self.assertEqual(l.GetAlpha(), 1.0)

    def testImagePlaneWidget(self):
        "A more rigorous test using the image plane widget."
        # This test is largely copied from
        # Widgets/Python/TestImagePlaneWidget.py

        # Load some data.
        v16 = vtk.vtkVolume16Reader()
        v16.SetDataDimensions(64, 64)
        v16.SetDataByteOrderToLittleEndian()
        v16.SetFilePrefix(os.path.join(VTK_DATA_ROOT,
                                       "Data", "headsq", "quarter"))
        v16.SetImageRange(1, 93)
        v16.SetDataSpacing(3.2, 3.2, 1.5)
        v16.Update()

        xMin, xMax, yMin, yMax, zMin, zMax = v16.GetExecutive().GetWholeExtent(v16.GetOutputInformation(0))
        img_data = v16.GetOutput()
        spacing = img_data.GetSpacing()
        sx, sy, sz = spacing

        origin = img_data.GetOrigin()
        ox, oy, oz = origin

        # An outline is shown for context.
        outline = vtk.vtkOutlineFilter()
        outline.SetInputData(img_data)

        outlineMapper = vtk.vtkPolyDataMapper()
        outlineMapper.SetInputConnection(outline.GetOutputPort())

        outlineActor = vtk.vtkActor()
        outlineActor.SetMapper(outlineMapper)

        # The shared picker enables us to use 3 planes at one time
        # and gets the picking order right
        picker = vtk.vtkCellPicker()
        picker.SetTolerance(0.005)

        # The 3 image plane widgets are used to probe the dataset.
        planeWidgetX = vtk.vtkImagePlaneWidget()
        planeWidgetX.DisplayTextOn()
        planeWidgetX.SetInputData(img_data)
        planeWidgetX.SetPlaneOrientationToXAxes()
        planeWidgetX.SetSliceIndex(32)
        planeWidgetX.SetPicker(picker)
        planeWidgetX.SetKeyPressActivationValue("x")
        prop1 = planeWidgetX.GetPlaneProperty()
        prop1.SetColor(1, 0, 0)

        planeWidgetY = vtk.vtkImagePlaneWidget()
        planeWidgetY.DisplayTextOn()
        planeWidgetY.SetInputData(img_data)
        planeWidgetY.SetPlaneOrientationToYAxes()
        planeWidgetY.SetSliceIndex(32)
        planeWidgetY.SetPicker(picker)
        planeWidgetY.SetKeyPressActivationValue("y")
        prop2 = planeWidgetY.GetPlaneProperty()
        prop2.SetColor(1, 1, 0)
        planeWidgetY.SetLookupTable(planeWidgetX.GetLookupTable())

        # for the z-slice, turn off texture interpolation:
        # interpolation is now nearest neighbour, to demonstrate
        # cross-hair cursor snapping to pixel centers
        planeWidgetZ = vtk.vtkImagePlaneWidget()
        planeWidgetZ.DisplayTextOn()
        planeWidgetZ.SetInputData(img_data)
        planeWidgetZ.SetPlaneOrientationToZAxes()
        planeWidgetZ.SetSliceIndex(46)
        planeWidgetZ.SetPicker(picker)
        planeWidgetZ.SetKeyPressActivationValue("z")
        prop3 = planeWidgetZ.GetPlaneProperty()
        prop3.SetColor(0, 0, 1)
        planeWidgetZ.SetLookupTable(planeWidgetX.GetLookupTable())

        # Now create another actor with an opacity < 1 and with some
        # scalars.
        p = vtk.vtkPolyData()
        pts = vtk.vtkPoints()
        pts.InsertNextPoint((0,0,0))
        sc = vtk.vtkFloatArray()
        sc.InsertNextValue(1.0)
        p.SetPoints(pts)
        p.GetPointData().SetScalars(sc)
        m = vtk.vtkPolyDataMapper()
        m.SetInputData(p)
        # Share the lookup table of the widgets.
        m.SetLookupTable(planeWidgetX.GetLookupTable())
        m.UseLookupTableScalarRangeOn()
        dummyActor = vtk.vtkActor()
        dummyActor.SetMapper(m)
        dummyActor.GetProperty().SetOpacity(0.0)

        # Create the RenderWindow and Renderer
        ren = vtk.vtkRenderer()
        renWin = vtk.vtkRenderWindow()
        renWin.SetMultiSamples(0)
        renWin.AddRenderer(ren)

        # Add the dummy actor.
        ren.AddActor(dummyActor)
        # Add the outline actor to the renderer, set the background
        # color and size
        ren.AddActor(outlineActor)
        renWin.SetSize(600, 600)
        ren.SetBackground(0.1, 0.1, 0.2)

        current_widget = planeWidgetZ
        mode_widget = planeWidgetZ

        # Set the interactor for the widgets
        iact = vtk.vtkRenderWindowInteractor()
        iact.SetRenderWindow(renWin)
        planeWidgetX.SetInteractor(iact)
        planeWidgetX.On()
        planeWidgetY.SetInteractor(iact)
        planeWidgetY.On()
        planeWidgetZ.SetInteractor(iact)
        planeWidgetZ.On()

        # Create an initial interesting view
        ren.ResetCamera();
        cam1 = ren.GetActiveCamera()
        cam1.Elevation(110)
        cam1.SetViewUp(0, 0, -1)
        cam1.Azimuth(45)
        ren.ResetCameraClippingRange()

        iact.Initialize()
        renWin.Render()

if __name__ == "__main__":
    Testing.main([(TestMapperLUT, 'test')])

