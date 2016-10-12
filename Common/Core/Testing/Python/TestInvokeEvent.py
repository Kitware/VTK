import unittest
import vtk

from vtk.test import Testing

testInt = 12
testString = "test string"
testFloat = 5.4


class VTKPythonObjectCalldataInvokeEventTest(Testing.vtkTest):

    @vtk.calldata_type(vtk.VTK_INT)
    def callbackInt(self, caller, event, calldata):
        self.calldata = calldata

    @vtk.calldata_type(vtk.VTK_STRING)
    def callbackString(self, caller, event, calldata):
        self.calldata = calldata

    @vtk.calldata_type(vtk.VTK_DOUBLE)
    def callbackFloat(self, caller, event, calldata):
        self.calldata = calldata

    @vtk.calldata_type(vtk.VTK_OBJECT)
    def callbackObj(self, caller, event, calldata):
        self.calldata = calldata

    def setUp(self):
        self.vtkObj = vtk.vtkObject()

        self.vtkObjForCallData = vtk.vtkObject()

    def test_int(self):
        self.vtkObj.AddObserver(vtk.vtkCommand.AnyEvent, self.callbackInt)
        self.vtkObj.InvokeEvent(vtk.vtkCommand.ModifiedEvent, testInt)
        self.assertEqual(self.calldata, testInt)

    def test_string(self):
        self.vtkObj.AddObserver(vtk.vtkCommand.AnyEvent, self.callbackString)
        self.vtkObj.InvokeEvent(vtk.vtkCommand.ModifiedEvent, testString)
        self.assertEqual(self.calldata, testString)

    def test_float(self):
        self.vtkObj.AddObserver(vtk.vtkCommand.AnyEvent, self.callbackFloat)
        self.vtkObj.InvokeEvent(vtk.vtkCommand.ModifiedEvent, testFloat)
        self.assertAlmostEqual(self.calldata, testFloat)

    def test_obj(self):
        self.vtkObj.AddObserver(vtk.vtkCommand.AnyEvent, self.callbackObj)
        self.vtkObj.InvokeEvent(vtk.vtkCommand.ModifiedEvent, self.vtkObjForCallData)
        self.assertEqual(self.calldata, self.vtkObjForCallData)



if __name__ == '__main__':
    Testing.main([(VTKPythonObjectCalldataInvokeEventTest, 'test')])
