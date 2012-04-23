"""Test ghost object support in VTK-Python

When PyVTKObject is destroyed, the vtkObjectBase that it
contained often continues to exist because references to
it still exist within VTK.  When that vtkObjectBase is
returned to python, a new PyVTKObject is created.

If the PyVTKObject has a custom class or a custom dict,
then we make a "ghost" of the PyVTKObject when it is
destroyed, so that if its vtkObjectBase returns to python,
the PyVTKObject can be restored with the proper class and
dict.  Each ghost has a weak pointer to its vtkObjectBase
so that it can be erased if the vtkObjectBase is destroyed.

To be tested:
 - make sure custom dicts are restored
 - make sure custom classes are restored

Created on Aug 19, 2010 by David Gobbi

"""

import sys
import exceptions
import vtk
from vtk.test import Testing

class vtkCustomObject(vtk.vtkObject):
    pass

class TestGhost(Testing.vtkTest):
    def testGhostForDict(self):
        """Ghost an object to save the dict"""
        o = vtk.vtkObject()
        o.customattr = 'hello'
        a = vtk.vtkVariantArray()
        a.InsertNextValue(o)
        i = id(o)
        del o
        o = vtk.vtkObject()
        o = a.GetValue(0).ToVTKObject()
        # make sure the id has changed, but dict the same
        self.assertEqual(o.customattr, 'hello')
        self.assertNotEqual(i, id(o))

    def testGhostForClass(self):
        """Ghost an object to save the class"""
        o = vtkCustomObject()
        a = vtk.vtkVariantArray()
        a.InsertNextValue(o)
        i = id(o)
        del o
        o = vtk.vtkObject()
        o = a.GetValue(0).ToVTKObject()
        # make sure the id has changed, but class the same
        self.assertEqual(o.__class__, vtkCustomObject)
        self.assertNotEqual(i, id(o))

if __name__ == "__main__":
    Testing.main([(TestGhost, 'test')])
