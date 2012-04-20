#!/usr/bin/env python

import vtk

class TerminationCrashTestCase:

    """This test case should produce a crash on termination if we aren't
    careful in DECREFing a Python callback via vtkPythonCommand.

    The problem occurs only when you have an uncollectable reference
    cycle with an observer callback.    This should cause a crash upon
    termination (trying to delete python object after interpreter
    shutdown), or an assertion error if the observed event is
    'DeleteEvent' or 'AnyEvent' (trying to invoke python method after
    interpreter shutdown). The test case basically consists of a python
    class which contains a vtk object which is being observed by that
    same class. There is no error if the event handler is a member of a
    different class, even if the instance of that class is contained by
    the same python object which contains the vtk object. There is also
    no error if a vtk object is subclassed in python and that subclass
    has an event handler for itself (self.AddObserver(Event,
    self.Handler)). Finally, the problem disappears if the container
    class has a cyclic reference to itself (self.CyclicReference =
    self).  """

    def __init__(self) :
        self.Object = vtk.vtkObject()
        self.Object.AddObserver('StartEvent', self.Handler)
    def Handler(self, obj, evt) :
        print 'event received'

test = TerminationCrashTestCase()

