#!/usr/bin/env python

# Run this test like so:
# $ vtkpython TestImagePlaneWidget.py  -D $VTK_DATA_ROOT \
#   -B $VTK_DATA_ROOT/Baseline/Widgets/
#
# $ vtkpython TestImagePlaneWidget.py --help
# provides more details on other options.

import os
import os.path
import vtk
from vtk.test import Testing

class TestImagePlaneWidget(Testing.vtkTest):
    def testBug(self):
        # Uncomment the next line if you want to run this via
        # `gdb python`.
        #raw_input('Hit Ctrl-C')

        # Load some data.
        v16 = vtk.vtkVolume16Reader()
        v16.SetDataDimensions(64, 64)
        v16.SetDataByteOrderToLittleEndian()
        v16.SetFilePrefix(os.path.join(Testing.VTK_DATA_ROOT,
                                       "Data", "headsq", "quarter"))
        v16.SetImageRange(1, 93)
        v16.SetDataSpacing(3.2, 3.2, 1.5)
        v16.Update()

        xMin, xMax, yMin, yMax, zMin, zMax = v16.GetOutput().GetWholeExtent()
        img_data = v16.GetOutput()

        # **************************************************
        # Look here for wierdness.

        # Lets create this data using the data from the reader.
        my_img_data = vtk.vtkImageData()
        my_img_data.SetDimensions(img_data.GetDimensions())
        my_img_data.SetWholeExtent(img_data.GetWholeExtent())
        my_img_data.SetExtent(img_data.GetExtent())
        my_img_data.SetUpdateExtent(img_data.GetUpdateExtent())
        my_img_data.SetSpacing(img_data.GetSpacing())
        my_img_data.SetOrigin(img_data.GetOrigin())
        my_img_data.SetScalarType(img_data.GetScalarType())
        my_img_data.GetPointData().SetScalars(img_data.GetPointData().GetScalars())
        my_img_data.Update()
        # hang on to original image data.
        orig_img_data = img_data

        # hijack img_data with our own.  If you comment this out everything is
        # fine.
        img_data = my_img_data
        # **************************************************

        spacing = img_data.GetSpacing()
        sx, sy, sz = spacing

        origin = img_data.GetOrigin()
        ox, oy, oz = origin

        # An outline is shown for context.
        outline = vtk.vtkOutlineFilter()
        outline.SetInput(img_data)

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
        planeWidgetX.SetInput(img_data)
        planeWidgetX.SetPlaneOrientationToXAxes()
        planeWidgetX.SetSliceIndex(32)
        planeWidgetX.SetPicker(picker)
        planeWidgetX.SetKeyPressActivationValue("x")
        prop1 = planeWidgetX.GetPlaneProperty()
        prop1.SetColor(1, 0, 0)

        planeWidgetY = vtk.vtkImagePlaneWidget()
        planeWidgetY.DisplayTextOn()
        planeWidgetY.SetInput(img_data)
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
        planeWidgetZ.SetInput(img_data)
        planeWidgetZ.SetPlaneOrientationToZAxes()
        planeWidgetZ.SetSliceIndex(46)
        planeWidgetZ.SetPicker(picker)
        planeWidgetZ.SetKeyPressActivationValue("z")
        prop3 = planeWidgetZ.GetPlaneProperty()
        prop3.SetColor(0, 0, 1)
        planeWidgetZ.SetLookupTable(planeWidgetX.GetLookupTable())

        # Create the RenderWindow and Renderer
        ren = vtk.vtkRenderer()
        renWin = vtk.vtkRenderWindow()
        renWin.AddRenderer(ren)

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

        # Compare the images and test.
        img_file = "TestImagePlaneWidget.png"
        Testing.compareImage(renWin, Testing.getAbsImagePath(img_file))
        # Interact if necessary.
        if Testing.isInteractive():
            iact.Start()

if __name__ == "__main__":
    Testing.main([(TestImagePlaneWidget, 'test')])
