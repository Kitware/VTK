import sys
import vtk
import array
from vtk.test import Testing


class TestDataEncoder(Testing.vtkTest):
    def testEncodings(self):
        # Render something
        cylinder = vtk.vtkCylinderSource()
        cylinder.SetResolution(8)

        cylinderMapper = vtk.vtkPolyDataMapper()
        cylinderMapper.SetInputConnection(cylinder.GetOutputPort())

        cylinderActor = vtk.vtkActor()
        cylinderActor.SetMapper(cylinderMapper)
        cylinderActor.RotateX(30.0)
        cylinderActor.RotateY(-45.0)

        ren = vtk.vtkRenderer()
        renWin = vtk.vtkRenderWindow()
        renWin.AddRenderer(ren)
        ren.AddActor(cylinderActor)
        renWin.SetSize(200, 200)

        ren.ResetCamera()
        ren.GetActiveCamera().Zoom(1.5)
        renWin.Render()

        # Get a vtkImageData with the rendered output
        w2if = vtk.vtkWindowToImageFilter()
        w2if.SetInput(renWin)
        w2if.SetShouldRerender(1)
        w2if.SetReadFrontBuffer(0)
        w2if.Update()
        imgData = w2if.GetOutput()

        # Use vtkDataEncoder to convert the image to PNG format and Base64 encode it
        encoder = vtk.vtkDataEncoder()
        base64String = encoder.EncodeAsBase64Png(imgData).encode('ascii')

        # Now Base64 decode the string back to PNG image data bytes
        inputArray = array.array('B', base64String)
        outputBuffer = bytearray(len(inputArray))

        utils = None
        try:
            utils = vtk.vtkIOCore.vtkBase64Utilities()
        except:
            try:
                utils = vtkIOCore.vtkBase64Utilities()
            except:
                print('Unable to import required vtkIOCore.vtkBase64Utilities')
                return

        actualLength = utils.DecodeSafely(inputArray, len(inputArray), outputBuffer, len(outputBuffer))
        outputArray = bytearray(actualLength)
        outputArray[:] = outputBuffer[0:actualLength]

        # And write those bytes to the disk as an actual PNG image file
        with open('TestDataEncoder.png', 'wb') as fd:
            fd.write(outputArray)

        # Create a vtkTesting object and specify a baseline image
        rtTester = vtk.vtkTesting()
        for arg in sys.argv[1:]:
            rtTester.AddArgument(arg)
        rtTester.AddArgument("-V")
        rtTester.AddArgument("TestDataEncoder.png")

        # Perform the image comparison test and print out the result.
        result = rtTester.RegressionTest("TestDataEncoder.png", 0.0)

        if result == 0:
            raise Exception("TestDataEncoder failed.")

if __name__ == "__main__":
    Testing.main([(TestDataEncoder, 'test')])
