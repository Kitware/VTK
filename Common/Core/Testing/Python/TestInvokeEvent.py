import unittest
from vtkmodules.vtkCommonCore import (
    VTK_DOUBLE,
    VTK_INT,
    VTK_OBJECT,
    VTK_STRING,
    vtkCommand,
    vtkObject,
)
from vtkmodules.util.misc import calldata_type
from vtkmodules.test import Testing

testInt = 12
testString = "test string"
testFloat = 5.4


class VTKPythonObjectCalldataInvokeEventTest(Testing.vtkTest):

    @calldata_type(VTK_INT)
    def callbackInt(self, caller, event, calldata):
        self.calldata = calldata

    @calldata_type(VTK_STRING)
    def callbackString(self, caller, event, calldata):
        self.calldata = calldata

    @calldata_type(VTK_DOUBLE)
    def callbackFloat(self, caller, event, calldata):
        self.calldata = calldata

    @calldata_type(VTK_OBJECT)
    def callbackObj(self, caller, event, calldata):
        self.calldata = calldata

    def setUp(self):
        self.vtkObj = vtkObject()

        self.vtkObjForCallData = vtkObject()

    def test_int(self):
        self.vtkObj.AddObserver(vtkCommand.AnyEvent, self.callbackInt)
        self.vtkObj.InvokeEvent(vtkCommand.ModifiedEvent, testInt)
        self.assertEqual(self.calldata, testInt)

    def test_string(self):
        self.vtkObj.AddObserver(vtkCommand.AnyEvent, self.callbackString)
        self.vtkObj.InvokeEvent(vtkCommand.ModifiedEvent, testString)
        self.assertEqual(self.calldata, testString)

    def test_float(self):
        self.vtkObj.AddObserver(vtkCommand.AnyEvent, self.callbackFloat)
        self.vtkObj.InvokeEvent(vtkCommand.ModifiedEvent, testFloat)
        self.assertAlmostEqual(self.calldata, testFloat)

    def test_obj(self):
        self.vtkObj.AddObserver(vtkCommand.AnyEvent, self.callbackObj)
        self.vtkObj.InvokeEvent(vtkCommand.ModifiedEvent, self.vtkObjForCallData)
        self.assertEqual(self.calldata, self.vtkObjForCallData)



if __name__ == '__main__':
    Testing.main([(VTKPythonObjectCalldataInvokeEventTest, 'test')])
