import sys
import vtk
from vtk.test import Testing

class TestNewickTreeReadWrite(Testing.vtkTest):

    def testReadWrite(self):
        t = '(A:1,B:2,(C:3,D:4)E:5)F;'

        reader = vtk.vtkNewickTreeReader()
        reader.SetReadFromInputString(True)
        reader.SetInputString(t)
        reader.Update()

        writer = vtk.vtkNewickTreeWriter()
        writer.WriteToOutputStringOn()
        writer.SetInputData(reader.GetOutput())
        writer.Update()
        t_return = writer.GetOutputStdString()

        self.assertEqual(t,t_return)
