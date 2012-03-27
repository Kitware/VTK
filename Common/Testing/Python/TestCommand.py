"""Test the wrapping of vtkCommand

The following vtkCommmand functionality must be tested
- Event enum constants
- Event names
- Adding/removing observers

Created on Mar 22, 2012 by David Gobbi
"""

import sys
import exceptions
import vtk
from vtk.test import Testing

class callback:
    def __init__(self):
        self.caller = None
        self.event = None

    def __call__(self, o, e):
        self.caller = o
        self.event = e

class TestCommand(Testing.vtkTest):
    def testEnumConstants(self):
        """Make sure the event id constants are wrapped
        """
        self.assertEqual(vtk.vtkCommand.ErrorEvent, 39)

    def testCommandWithArgs(self):
        """Test binding a command that has arguments.
        """
        cb = callback()
        o = vtk.vtkObject()
        o.AddObserver(vtk.vtkCommand.ModifiedEvent, cb)
        o.Modified()

        self.assertEqual(cb.caller, o)
        self.assertEqual(cb.event, "ModifiedEvent")

    def testUseEventNameString(self):
        """Test binding with a string event name.
        """
        cb = callback()
        o = vtk.vtkObject()
        o.AddObserver("ModifiedEvent", cb)
        o.Modified()

        self.assertEqual(cb.caller, o)
        self.assertEqual(cb.event, "ModifiedEvent")

    def testPriorityArg(self):
        """Test the optional priority argument
        """
        cb = callback()
        o = vtk.vtkObject()
        o.AddObserver(vtk.vtkCommand.ModifiedEvent, cb, 0.5)
        o.Modified()

        self.assertEqual(cb.caller, o)

    def testRemoveCommand(self):
        """Test the removal of an observer.
        """
        cb = callback()
        o = vtk.vtkObject()
        o.AddObserver(vtk.vtkCommand.ModifiedEvent, cb)
        o.Modified()
        self.assertEqual(cb.caller, o)
        o.RemoveObservers(vtk.vtkCommand.ModifiedEvent)
        cb.caller = None
        cb.event = None
        o.Modified()
        self.assertEqual(cb.caller, None)

    def testGetCommand(self):
        """Test getting the vtkCommand object
        """
        cb = callback()
        o = vtk.vtkObject()
        n = o.AddObserver(vtk.vtkCommand.ModifiedEvent, cb)
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

if __name__ == "__main__":
    Testing.main([(TestCommand, 'test')])
