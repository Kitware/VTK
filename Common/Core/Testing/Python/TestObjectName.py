import unittest
from vtkmodules.vtkCommonCore import (vtkCommand,
                                      vtkDoubleArray,
                                      vtkObject,
                                      VTK_STRING)
from vtkmodules.vtkCommonExecutionModel import (vtkTrivialProducer,
                                                vtkExecutive)
from vtkmodules.vtkRenderingCore        import vtkActor
from vtkmodules.vtkFiltersCore import vtkArrayCalculator
from vtkmodules.util.misc import calldata_type
from vtk.test import Testing

class MessageObserver(Testing.vtkTest):

   def __init__(self):
       self.event    = None
       self.message  = str()

   @calldata_type(VTK_STRING)
   def observe(self, obj, event, data):
       self.event    = event
       self.message  = str(data)

class TestObjectName(Testing.vtkTest):

    def test_SetObjectNameSetsObjectName(self):
        producer = vtkTrivialProducer()
        producer.SetObjectName("foo")
        self.assertEqual("foo",producer.GetObjectName())


    def testObjectDescriptionContainsObjectName(self):
        producer = vtkTrivialProducer()
        producer.SetObjectName("foo")
        description=producer.GetObjectDescription()
        self.assertTrue(description.endswith("'foo'"))


    def testSetObjectNameDoesNotChangeMTime(self):
        a = vtkDoubleArray()
        mtime = a.GetMTime()
        a.SetObjectName("foo")
        self.assertEqual(mtime,a.GetMTime())


    def testErrorMessageContainsObjectName(self):
        algorithm = vtkArrayCalculator()
        algorithm.SetObjectName("foo")
        observer = MessageObserver()
        algorithm.GetExecutive().AddObserver(vtkCommand.ErrorEvent,observer.observe)
        algorithm.Update() # this invokes an error event
        self.assertEqual("ErrorEvent",observer.event)
        self.assertTrue("'foo'" in observer.message, observer.message)


    def testObjectNameNotCopied(self):
        foo = vtkActor()
        foo.SetObjectName("foo")
        bar = vtkActor()
        bar.SetObjectName("bar")
        foo.ShallowCopy(bar)
        self.assertNotEqual(foo.GetObjectName(),bar.GetObjectName())


if __name__ == "__main__":
    Testing.main([(TestObjectName, 'test')])
