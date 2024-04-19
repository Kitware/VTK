"""Test the wrapping of vtkCommand

The following vtkCommmand functionality must be tested
- Event enum constants
- Event names

Created on Mar 22, 2012 by David Gobbi
Updated on Feb 12, 2014 by Jean-Christophe Fillion-Robin
"""

import sys
import gc
from vtkmodules.vtkCommonCore import (
    VTK_STRING,
    vtkCommand,
    vtkLookupTable,
    vtkObject,
)
from vtkmodules.util.misc import calldata_type
from vtkmodules.test import Testing

class callback:
    def __init__(self):
        self.reset()

    def __call__(self, o, e, d = None):
        self.caller = o
        self.event = e
        self.calldata = d

    def reset(self):
        self.caller = None
        self.event = None
        self.calldata = None


class TestCommand(Testing.vtkTest):
    def testEnumConstants(self):
        """Make sure the event id constants are wrapped
        """
        self.assertEqual(vtkCommand.ErrorEvent, 39)

    def testCommandWithArgs(self):
        """Test binding a command that has arguments.
        """
        cb = callback()
        o = vtkObject()
        o.AddObserver(vtkCommand.ModifiedEvent, cb)
        o.Modified()

        self.assertEqual(cb.caller, o)
        self.assertEqual(cb.event, "ModifiedEvent")

    def testUseEventNameString(self):
        """Test binding with a string event name.
        """
        cb = callback()
        o = vtkObject()
        o.AddObserver("ModifiedEvent", cb)
        o.Modified()

        self.assertEqual(cb.caller, o)
        self.assertEqual(cb.event, "ModifiedEvent")

    def testPriorityArg(self):
        """Test the optional priority argument
        """
        cb = callback()
        o = vtkObject()
        o.AddObserver(vtkCommand.ModifiedEvent, cb, 0.5)
        o.Modified()

        self.assertEqual(cb.caller, o)

    def testRemoveCommand(self):
        """Test the removal of an observer.
        """
        cb = callback()
        o = vtkObject()
        o.AddObserver(vtkCommand.ModifiedEvent, cb)
        o.Modified()
        self.assertEqual(cb.caller, o)
        o.RemoveObservers(vtkCommand.ModifiedEvent)
        cb.caller = None
        cb.event = None
        o.Modified()
        self.assertEqual(cb.caller, None)

    def testGetCommand(self):
        """Test getting the vtkCommand object
        """
        cb = callback()
        o = vtkObject()
        n = o.AddObserver(vtkCommand.ModifiedEvent, cb)
        o.Modified()
        self.assertEqual(cb.caller, o)
        c = o.GetCommand(n)
        self.assertEqual((c.IsA("vtkCommand") != 0), True)
        # in the future, o.RemoveObserver(c) should also be made to work
        o.RemoveObserver(n)
        cb.caller = None
        cb.event = None
        o.Modified()
        self.assertEqual(cb.caller, None)

    def testCommandCircularRef(self):
        """Test correct reference loop reporting for commands
        """
        cb = callback()
        o = vtkObject()
        o.AddObserver(vtkCommand.ModifiedEvent, cb)
        cb.circular_ref = o
        # referent to check if "o" is deleted
        referent = vtkObject()
        o.referent = referent
        # make sure gc removes referrer "o" from referent
        s1 = repr(gc.get_referrers(referent))
        del o
        del cb
        gc.collect()
        s2 = repr(gc.get_referrers(referent))

        self.assertNotEqual(s1, s2)
        self.assertNotEqual(s1.count("vtkObject"),0)
        self.assertEqual(s2.count("vtkObject"),0)

    def testAddRemoveObservers(self):
        """Test adding and removing observers
        """
        cb = callback()
        cb2 = callback()
        o = vtkObject()
        n = o.AddObserver(vtkCommand.ModifiedEvent, cb)
        n2 = o.AddObserver(vtkCommand.ModifiedEvent, cb2)
        o.Modified()
        self.assertEqual(cb.caller, o)
        self.assertEqual(cb2.caller, o)
        o.RemoveObserver(n)
        cb.reset()
        cb2.reset()
        o.Modified()
        self.assertEqual(cb.caller, None)
        self.assertEqual(cb2.caller, o)
        o.RemoveObserver(n2)
        cb.reset()
        cb2.reset()
        o.Modified()
        self.assertEqual(cb.caller, None)
        self.assertEqual(cb2.caller, None)

    def testUseCallDataType(self):
        """Test adding an observer associated with a callback expecting a CallData
        """
        cb = callback()
        cb.CallDataType = VTK_STRING
        lt = vtkLookupTable()
        lt.AddObserver(vtkCommand.ErrorEvent, cb)
        lt.SetTableRange(2, 1)
        self.assertEqual(cb.caller, lt)
        self.assertEqual(cb.event, "ErrorEvent")
        self.assertTrue(cb.calldata.startswith("ERROR: In"))

    def testUseCallDataTypeWithDecoratorAsString0(self):
        """Test adding an observer associated with a callback expecting a CallData.
        This test ensures backward compatibility checking the CallDataType can
        be set to the string 'string0'.
        """
        self.onErrorCalldata = ''

        @calldata_type('string0')
        def onError(caller, event, calldata):
            self.onErrorCalldata = calldata

        lt = vtkLookupTable()
        lt.AddObserver(vtkCommand.ErrorEvent, onError)
        lt.SetTableRange(2, 1)
        self.assertTrue(self.onErrorCalldata.startswith("ERROR: In"))

    def testUseCallDataTypeWithDecorator(self):
        """Test adding an observer associated with a callback expecting a CallData
        """
        self.onErrorCalldata = ''

        @calldata_type(VTK_STRING)
        def onError(caller, event, calldata):
            self.onErrorCalldata = calldata

        lt = vtkLookupTable()
        lt.AddObserver(vtkCommand.ErrorEvent, onError)
        lt.SetTableRange(2, 1)
        self.assertTrue(self.onErrorCalldata.startswith("ERROR: In"))

if __name__ == "__main__":
    Testing.main([(TestCommand, 'test')])
