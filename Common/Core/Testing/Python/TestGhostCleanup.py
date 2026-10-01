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
import weakref
from vtkmodules.util.numpy_support import numpy_to_vtk
from vtkmodules.vtkCommonCore import (
    vtkCallbackCommand,
    vtkFloatArray,
    vtkIntArray,
    vtkObject,
    vtkObjectBase,
    vtkSOADataArrayTemplate,
    vtkVariant,
    vtkVariantArray,
)
from vtkmodules.vtkCommonDataModel import (
    VTK_LINE,
    vtkCellArray,
    vtkMultiBlockDataSet,
    vtkPolyData,
    vtkRectilinearGrid,
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

    def testUnobservedGhostReleased(self):
        """A ghost of a non-vtkObject cannot have a DeleteEvent observer.
        When its C++ object is deleted, its __dict__ must still be released
        by the next ghost creation."""

        class Payload:
            pass

        holder = vtkVariantArray()
        cmd = vtkCallbackCommand()
        payload = Payload()
        cmd.payload = payload
        ref = weakref.ref(payload)
        del payload
        holder.InsertNextValue(vtkVariant(cmd))
        del cmd
        _collect()

        # The command is a ghost; deleting the C++ object makes it stale
        holder.SetValue(0, vtkVariant())
        _collect()

        o = vtkObject()
        o.x = 1
        holder.InsertNextValue(vtkVariant(o))
        del o
        _collect()
        self.assertIsNone(ref())

    def testWrappingLeavesDictEmpty(self):
        """Wrapping an existing C++ object must not populate __dict__,
        otherwise every discarded wrapper is saved as a ghost."""
        pd = vtkPolyData()
        a = vtkFloatArray()
        a.SetNumberOfValues(4)
        pd.GetPointData().AddArray(a)
        s = vtkSOADataArrayTemplate["float32"]()
        s.SetNumberOfComponents(2)
        s.SetNumberOfTuples(3)
        pd.GetPointData().AddArray(s)
        del a, s
        mb = vtkMultiBlockDataSet()
        mb.SetBlock(0, pd)
        for obj in (
            pd,
            pd.GetPointData(),
            pd.GetPointData().GetArray(0),
            pd.GetPointData().GetArray(1),
            mb,
            vtkRectilinearGrid(),
        ):
            self.assertEqual(vars(obj), {}, type(obj).__name__)

    def testManyGhosts(self):
        """Ghosts stay correct when many exist at once, including when some
        of their C++ objects are deleted between ghost creations."""
        n = 1000
        holder = vtkVariantArray()
        for i in range(n):
            o = vtkObject()
            o.index = i
            holder.InsertNextValue(o)
        del o
        _collect()

        # Delete every other object; their ghosts must go away and must
        # not be resurrected by objects that reuse their addresses.
        for i in range(0, n, 2):
            holder.SetValue(i, vtkVariant())
        _collect()
        fresh = [vtkObject() for _ in range(n // 2)]
        for f in fresh:
            self.assertFalse(hasattr(f, "index"))

        # Round-trip every survivor twice, which removes and re-adds its
        # ghost each time.
        for _ in range(2):
            for i in range(1, n, 2):
                o = holder.GetValue(i).ToVTKObject()
                self.assertEqual(o.index, i)
            del o
            _collect()


if __name__ == "__main__":
    Testing.main([(TestGhostCleanup, "test")])
