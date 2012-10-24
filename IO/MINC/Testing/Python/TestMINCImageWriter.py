#!/usr/bin/env python

# Image pipeline
reader = vtk.vtkMINCImageReader()
reader.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/t3_grid_0.mnc")
reader.RescaleRealValuesOn()
attributes = vtk.vtkMINCImageAttributes()
image = reader
#
# write to the temp directory if possible, otherwise use .
#
dir = "."
if (info.commands(globals(), locals(),  "rtTester") == "rtTester"):
    dir = rtTester.GetTempDirectory()
    pass
# make sure it is writeable first
if (catch.catch(globals(),"""channel = open("" + str(dir) + "/test.tmp", "w")""") == 0):
    channel.close()
    file.delete("-force", "" + str(dir) + "/test.tmp")
    minc1 = vtk.vtkMINCImageWriter()
    minc1.SetInputConnection(reader.GetOutputPort())
    minc1.SetFileName("" + str(dir) + "/minc1.mnc")
    attributes.ShallowCopy(reader.GetImageAttributes())
    attributes.SetAttributeValueAsString("patient","full_name","DOE^JOHN DAVID")
    minc2 = vtk.vtkMINCImageWriter()
    minc2.SetImageAttributes(attributes)
    minc2.SetInputConnection(reader.GetOutputPort())
    minc2.SetFileName("" + str(dir) + "/minc2.mnc")
    minc3 = vtk.vtkMINCImageWriter()
    minc3.SetImageAttributes(attributes)
    minc3.AddInputConnection(reader.GetOutputPort())
    minc3.AddInputConnection(reader.GetOutputPort())
    minc3.SetFileName("" + str(dir) + "/minc3.mnc")
    minc1.Write()
    minc2.Write()
    minc3.Write()
    reader2 = vtk.vtkMINCImageReader()
    reader2.SetFileName("" + str(dir) + "/minc3.mnc")
    reader2.RescaleRealValuesOn()
    reader2.SetTimeStep(1)
    reader2.Update()
    image = reader2
    file.delete("-force", "" + str(dir) + "/minc1.mnc")
    file.delete("-force", "" + str(dir) + "/minc2.mnc")
    file.delete("-force", "" + str(dir) + "/minc3.mnc")
    # write out the file header for coverage
    attributes.PrintFileHeader()
    pass
viewer = vtk.vtkImageViewer()
viewer.SetInputConnection(image.GetOutputPort())
viewer.SetColorWindow(100)
viewer.SetColorLevel(0)
viewer.Render()
# --- end of script --
