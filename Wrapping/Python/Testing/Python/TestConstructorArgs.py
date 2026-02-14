#!/usr/bin/env python
#
# Test that VTK object constructors support multiple positional arguments,
# keyword arguments, Python override classes (mixins), and SWIG pointer
# reconstruction after the tp_new relaxation in PyVTKObject.cxx.

import vtkmodules.test.Testing as vtkTesting
from vtkmodules.vtkCommonCore import vtkCollection, vtkPoints, vtkObject
from vtkmodules.vtkCommonDataModel import vtkPolyData
from vtkmodules.vtkFiltersSources import vtkSphereSource


class TestConstructorArgs(vtkTesting.vtkTest):

    # ------------------------------------------------------------------
    # SWIG pointer reconstruction (no override)
    # ------------------------------------------------------------------

    def test_swig_pointer_plain_object(self):
        """SWIG pointer reconstruction on a plain VTK class (no override)."""
        pts = vtkPoints()
        pts.InsertNextPoint(1, 2, 3)
        addr = pts.__this__
        pts2 = vtkPoints(addr)
        self.assertIs(pts, pts2)
        self.assertEqual(pts2.GetNumberOfPoints(), 1)

    # ------------------------------------------------------------------
    # SWIG pointer reconstruction (with data_model override)
    # ------------------------------------------------------------------

    def test_swig_pointer_with_override(self):
        """SWIG pointer reconstruction on a class with a Python override."""
        pd = vtkPolyData()
        pts = vtkPoints()
        pts.InsertNextPoint(1, 2, 3)
        pd.SetPoints(pts)
        addr = pd.__this__
        pd2 = vtkPolyData(addr)
        self.assertIs(pd, pd2)
        self.assertEqual(pd2.GetNumberOfPoints(), 1)

    # ------------------------------------------------------------------
    # Multiple positional arguments with a custom override
    # ------------------------------------------------------------------

    def test_multiple_positional_args_override(self):
        """Override class __init__ receives all positional arguments."""
        class MyCollection(vtkCollection):
            def __init__(self, *args, **kwargs):
                if args and isinstance(args[0], str):
                    return
                self.captured_args = args
                self.captured_kwargs = kwargs

        vtkCollection.override(MyCollection)
        try:
            # 0 args
            c0 = vtkCollection()
            self.assertEqual(c0.captured_args, ())

            # 1 non-string arg
            c1 = vtkCollection(42)
            self.assertEqual(c1.captured_args, (42,))

            # 2 args
            c2 = vtkCollection(10, 20)
            self.assertEqual(c2.captured_args, (10, 20))

            # 3 args (like affine array: shape, slope, intercept)
            c3 = vtkCollection(100, 1.0, 0.0)
            self.assertEqual(c3.captured_args, (100, 1.0, 0.0))

            # mixed positional + keyword
            c4 = vtkCollection(5, name="test")
            self.assertEqual(c4.captured_args, (5,))
            self.assertEqual(c4.captured_kwargs, {"name": "test"})
        finally:
            # Restore original type (remove override).  There is no
            # public un-override API, but the test is isolated enough
            # that this is acceptable.
            pass

    def test_swig_pointer_with_custom_override(self):
        """SWIG pointer reconstruction still works after custom override."""
        # vtkCollection already has the MyCollection override from the
        # previous test (overrides are permanent per-process).
        c = vtkCollection()
        obj = vtkObject()
        c.AddItem(obj)
        addr = c.__this__
        c2 = vtkCollection(addr)
        self.assertIs(c, c2)
        self.assertEqual(c2.GetNumberOfItems(), 1)

    # ------------------------------------------------------------------
    # Keyword arguments (existing VTK kwarg support in tp_init)
    # ------------------------------------------------------------------

    def test_kwargs_still_work(self):
        """Keyword arguments are still processed by PyVTKObject_Init."""
        s = vtkSphereSource(radius=10, theta_resolution=20)
        self.assertEqual(s.radius, 10)
        self.assertEqual(s.theta_resolution, 20)


if __name__ == '__main__':
    vtkTesting.main([(TestConstructorArgs, 'test')])
