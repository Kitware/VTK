"""Regression tests for proactive ghost eviction in VTK-Python.

When a Python wrapper for a vtkObject is destroyed while the C++ object is
still alive (e.g., held by a container), VTK saves the wrapper's __dict__
into a "ghost map" so that attributes survive a round-trip through C++.

Historically, stale ghosts (entries whose C++ object has since been
destroyed) were only cleaned up lazily during the next ghost creation.
This left anything reachable from the saved __dict__ pinned for an
indeterminate amount of time -- including VTK Python wrappers and the
C++ objects they Register'd.

These tests cover the proactive eviction path: a DeleteEvent observer is
attached to the C++ object when its ghost is created, and the observer
evicts the ghost the moment C++ destruction runs.
"""

import gc
import sys
from vtkmodules.util.numpy_support import numpy_to_vtk
from vtkmodules.vtkCommonCore import (
    vtkIntArray,
    vtkObject,
    vtkObjectBase,
    vtkVariantArray,
)
from vtkmodules.vtkCommonDataModel import (
    VTK_LINE,
    vtkCellArray,
    vtkUnstructuredGrid,
)
from vtkmodules.test import Testing


def _collect():
    if hasattr(sys, "_is_gil_enabled") and not sys._is_gil_enabled():
        gc.collect()


def _vtk_object_ids():
    return {id(o) for o in gc.get_objects() if isinstance(o, vtkObjectBase)}


class TestGhostCleanup(Testing.vtkTest):
    def testNoLeakWhenContainerOutlivesWrapper(self):
        """Buffer reachable through ghosted __dict__ is freed when grid dies."""
        gc.collect()
        before = _vtk_object_ids()

        def create_grid():
            # numpy_to_vtk + SetCells triggers __getitem__ during overload
            # scoring of SetCells(int*, ...), which populates the array's
            # __dict__ via VTKAOSArray's array-view cache. When the local
            # dies and the grid still holds C++ ownership, the dict goes
            # into the GhostMap with strong refs to the array's buffer.
            cell_type = numpy_to_vtk([VTK_LINE])
            grid = vtkUnstructuredGrid()
            grid.SetCells(cell_type, vtkCellArray())
            return grid

        grid = create_grid()
        del grid
        _collect()

        leaked = [
            o
            for o in gc.get_objects()
            if isinstance(o, vtkObjectBase) and id(o) not in before
        ]
        self.assertEqual(
            leaked,
            [],
            f"unexpected vtkObjectBase survivors: "
            f"{[o.__class__.__name__ for o in leaked]}",
        )

    def testNoLeakWithIndexedAOSArray(self):
        """Same pattern without numpy_to_vtk: indexing alone populates __dict__."""
        gc.collect()
        before = _vtk_object_ids()

        def create_grid():
            a = vtkIntArray()
            a.InsertNextValue(VTK_LINE)
            _ = a[0]  # populates VTKAOSArray cache in __dict__
            g = vtkUnstructuredGrid()
            g.SetCells(a, vtkCellArray())
            return g

        g = create_grid()
        del g
        _collect()

        leaked = [
            o
            for o in gc.get_objects()
            if isinstance(o, vtkObjectBase) and id(o) not in before
        ]
        self.assertEqual(
            leaked,
            [],
            f"unexpected vtkObjectBase survivors: "
            f"{[o.__class__.__name__ for o in leaked]}",
        )

    def testAttributesStillSurviveRoundTrip(self):
        """Proactive eviction must not break attribute persistence when the
        C++ object is still alive at the time of Python lookup."""
        o = vtkObject()
        o.customattr = "hello"
        a = vtkVariantArray()
        a.InsertNextValue(o)
        original_id = id(o)
        del o
        _collect()

        # Bump the allocator so the new wrapper lands at a different address.
        _filler = vtkObject()
        o2 = a.GetValue(0).ToVTKObject()
        self.assertEqual(o2.customattr, "hello")
        self.assertNotEqual(original_id, id(o2))


if __name__ == "__main__":
    Testing.main([(TestGhostCleanup, "test")])
