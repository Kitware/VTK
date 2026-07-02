"""Tests for the Pythonic vtkSelection / vtkSelectionNode API."""

import sys

import numpy as np
from vtkmodules.vtkCommonCore import vtkIdTypeArray, vtkStringArray
from vtkmodules.vtkCommonDataModel import vtkSelection, vtkSelectionNode


def test_basic_construction():
    """Single-node selection from constructor."""
    sel = vtkSelection(
        content_type="INDICES",
        field_type="CELL",
        selection_list=np.array([0, 5, 10], dtype=np.int64),
    )
    assert len(sel) == 1, "Expected 1 node, got %d" % len(sel)
    node = sel[0]
    assert node.content_type == "INDICES"
    assert node.field_type == "CELL"
    assert node.selection_list.GetNumberOfTuples() == 3


def test_int_enum_construction():
    """Content/field type as integer enum values."""
    sel = vtkSelection(
        content_type=vtkSelectionNode.INDICES,
        field_type=vtkSelectionNode.POINT,
        selection_list=np.array([1, 2, 3], dtype=np.int64),
    )
    assert sel[0].content_type == "INDICES"
    assert sel[0].field_type == "POINT"


def test_qualifiers():
    """Qualifier kwargs on construction."""
    sel = vtkSelection(
        content_type="INDICES",
        field_type="CELL",
        selection_list=np.array([0], dtype=np.int64),
        inverse=True,
        containing_cells=True,
        connected_layers=2,
        composite_index=5,
        component_number=1,
    )
    node = sel[0]
    assert node.inverse is True
    assert node.containing_cells is True
    assert node.connected_layers == 2
    assert node.composite_index == 5
    assert node.component_number == 1


def test_empty_selection():
    """Empty selection has len 0."""
    sel = vtkSelection()
    assert len(sel) == 0


def test_container_interface():
    """len, getitem, iter, append, clear, node_names, expression."""
    sel = vtkSelection()

    node1 = vtkSelectionNode()
    node1.content_type = "INDICES"
    node1.field_type = "CELL"
    name1 = sel.append(node1)

    node2 = vtkSelectionNode()
    node2.content_type = "INDICES"
    node2.field_type = "POINT"
    name2 = sel.append(node2)

    # len
    assert len(sel) == 2

    # getitem by index
    assert sel[0].field_type == "CELL"
    assert sel[1].field_type == "POINT"

    # getitem by name
    assert sel[name1] is not None
    assert sel[name2] is not None

    # negative index
    assert sel[-1].field_type == "POINT"

    # iteration
    types = [n.field_type for n in sel]
    assert types == ["CELL", "POINT"]

    # node_names
    assert name1 in sel.node_names
    assert name2 in sel.node_names

    # expression get/set
    sel.expression = "A & B"
    assert sel.expression == "A & B"

    # clear
    sel.clear()
    assert len(sel) == 0


def test_getitem_errors():
    """Error cases for __getitem__."""
    sel = vtkSelection()
    try:
        sel[0]
        assert False, "Expected IndexError"
    except IndexError:
        pass

    try:
        sel["nonexistent"]
        assert False, "Expected KeyError"
    except KeyError:
        pass


def test_and_operator():
    """sel1 & sel2 produces 2-node selection with & expression."""
    sel1 = vtkSelection(
        content_type="INDICES", field_type="CELL",
        selection_list=np.array([0, 1], dtype=np.int64),
    )
    sel2 = vtkSelection(
        content_type="INDICES", field_type="POINT",
        selection_list=np.array([2, 3], dtype=np.int64),
    )
    combined = sel1 & sel2
    assert len(combined) == 2
    assert "&" in combined.expression


def test_or_operator():
    """sel1 | sel2 produces selection with | expression."""
    sel1 = vtkSelection(
        content_type="INDICES", field_type="CELL",
        selection_list=np.array([0], dtype=np.int64),
    )
    sel2 = vtkSelection(
        content_type="INDICES", field_type="CELL",
        selection_list=np.array([1], dtype=np.int64),
    )
    combined = sel1 | sel2
    assert len(combined) == 2
    assert "|" in combined.expression


def test_xor_operator():
    """sel1 ^ sel2 produces selection with ^ expression."""
    sel1 = vtkSelection(
        content_type="INDICES", field_type="CELL",
        selection_list=np.array([0], dtype=np.int64),
    )
    sel2 = vtkSelection(
        content_type="INDICES", field_type="CELL",
        selection_list=np.array([1], dtype=np.int64),
    )
    combined = sel1 ^ sel2
    assert len(combined) == 2
    assert "^" in combined.expression


def test_invert_operator():
    """~sel produces selection with ! expression."""
    sel = vtkSelection(
        content_type="INDICES", field_type="CELL",
        selection_list=np.array([0, 1], dtype=np.int64),
    )
    inverted = ~sel
    assert len(inverted) == 1
    assert "!" in inverted.expression


def test_compound_expression():
    """(sel1 & sel2) | ~sel3 — 3 nodes, correct expression."""
    sel1 = vtkSelection(
        content_type="INDICES", field_type="CELL",
        selection_list=np.array([0], dtype=np.int64),
    )
    sel2 = vtkSelection(
        content_type="INDICES", field_type="CELL",
        selection_list=np.array([1], dtype=np.int64),
    )
    sel3 = vtkSelection(
        content_type="INDICES", field_type="CELL",
        selection_list=np.array([2], dtype=np.int64),
    )
    compound = (sel1 & sel2) | ~sel3
    assert len(compound) == 3
    assert "&" in compound.expression
    assert "|" in compound.expression
    assert "!" in compound.expression


def test_node_content_preserved():
    """Node content is preserved through operators."""
    sel1 = vtkSelection(
        content_type="INDICES", field_type="CELL",
        selection_list=np.array([10, 20], dtype=np.int64),
    )
    sel2 = vtkSelection(
        content_type="THRESHOLDS", field_type="POINT",
        selection_list=np.array([0.0, 1.0]),
    )
    combined = sel1 & sel2

    # Find the INDICES node and THRESHOLDS node
    content_types = sorted([n.content_type for n in combined])
    assert "INDICES" in content_types
    assert "THRESHOLDS" in content_types


def test_from_thresholds_single():
    """from_thresholds with a single range."""
    sel = vtkSelection().from_thresholds("Temperature", (0, 100))
    assert len(sel) == 1
    node = sel[0]
    assert node.content_type == "THRESHOLDS"
    assert node.field_type == "CELL"
    sl = node.selection_list
    assert sl.GetName() == "Temperature"
    assert sl.GetNumberOfComponents() == 2
    assert sl.GetNumberOfTuples() == 1
    assert sl.GetValue(0) == 0.0
    assert sl.GetValue(1) == 100.0


def test_from_thresholds_multiple():
    """from_thresholds with multiple ranges."""
    sel = vtkSelection().from_thresholds("Temp", [(0, 10), (50, 100)])
    node = sel[0]
    sl = node.selection_list
    assert sl.GetNumberOfComponents() == 2
    assert sl.GetNumberOfTuples() == 2


def test_from_thresholds_single_range_as_list():
    """A single (min, max) range given as a list is accepted like a tuple."""
    sel = vtkSelection().from_thresholds("Temperature", [0, 100])
    node = sel[0]
    sl = node.selection_list
    assert sl.GetNumberOfComponents() == 2
    assert sl.GetNumberOfTuples() == 1
    assert sl.GetValue(0) == 0.0
    assert sl.GetValue(1) == 100.0


def test_invalid_int_content_type_raises():
    """An int that is not a valid content type raises ValueError."""
    try:
        vtkSelection(content_type=999, field_type="CELL",
                     selection_list=np.array([0], dtype=np.int64))
        assert False, "Expected ValueError"
    except ValueError:
        pass


def test_invalid_int_field_type_raises():
    """An int that is not a valid field type raises ValueError."""
    try:
        vtkSelection(content_type="INDICES", field_type=999,
                     selection_list=np.array([0], dtype=np.int64))
        assert False, "Expected ValueError"
    except ValueError:
        pass


def test_from_values():
    """from_values creates VALUES selection."""
    sel = vtkSelection().from_values("CellType", [1, 3, 5])
    node = sel[0]
    assert node.content_type == "VALUES"
    assert node.field_type == "CELL"
    assert node.selection_list.GetName() == "CellType"
    assert node.component_number == -1


def test_from_locations():
    """from_locations creates LOCATIONS selection."""
    pts = np.array([[0, 0, 0], [1, 1, 1]], dtype=np.float64)
    sel = vtkSelection().from_locations(pts, epsilon=0.01)
    node = sel[0]
    assert node.content_type == "LOCATIONS"
    assert node.field_type == "POINT"
    sl = node.selection_list
    assert sl.GetNumberOfComponents() == 3
    assert sl.GetNumberOfTuples() == 2
    # Check epsilon
    assert abs(node.epsilon - 0.01) < 1e-10


def test_from_frustum():
    """from_frustum creates FRUSTUM selection."""
    corners = np.zeros((8, 4), dtype=np.float64)
    corners[:, 3] = 1.0  # homogeneous coords
    # Set some distinct values
    for i in range(8):
        corners[i, 0] = float(i)

    sel = vtkSelection().from_frustum(corners)
    node = sel[0]
    assert node.content_type == "FRUSTUM"
    assert node.field_type == "CELL"
    sl = node.selection_list
    assert sl.GetNumberOfTuples() == 32


def test_from_frustum_wrong_size():
    """from_frustum rejects wrong array size."""
    try:
        vtkSelection().from_frustum(np.zeros(16))
        assert False, "Expected ValueError"
    except ValueError:
        pass


def test_from_blocks():
    """from_blocks creates BLOCKS selection."""
    sel = vtkSelection().from_blocks([0, 2, 5])
    node = sel[0]
    assert node.content_type == "BLOCKS"
    sl = node.selection_list
    assert sl.GetNumberOfTuples() == 3


def test_from_block_selectors():
    """from_block_selectors creates BLOCK_SELECTORS selection."""
    sel = vtkSelection().from_block_selectors(
        ["//Block[@name='Mesh']"],
        assembly_name="Hierarchy",
    )
    node = sel[0]
    assert node.content_type == "BLOCK_SELECTORS"
    sl = node.selection_list
    assert isinstance(sl, vtkStringArray)
    assert sl.GetNumberOfTuples() == 1
    assert sl.GetValue(0) == "//Block[@name='Mesh']"
    # Check assembly name
    assert node.assembly_name == "Hierarchy"


def test_selection_node_properties():
    """SelectionNode property get/set."""
    node = vtkSelectionNode()
    node.content_type = "INDICES"
    assert node.content_type == "INDICES"
    assert node.GetContentType() == vtkSelectionNode.INDICES

    node.field_type = "POINT"
    assert node.field_type == "POINT"
    assert node.GetFieldType() == vtkSelectionNode.POINT

    arr = np.array([1, 2, 3], dtype=np.int64)
    node.selection_list = arr
    assert node.selection_list.GetNumberOfTuples() == 3

    # Qualifier properties
    node.inverse = True
    assert node.inverse is True
    node.inverse = False
    assert node.inverse is False

    node.containing_cells = True
    assert node.containing_cells is True

    node.connected_layers = 3
    assert node.connected_layers == 3

    node.composite_index = 7
    assert node.composite_index == 7

    node.component_number = 2
    assert node.component_number == 2

    node.epsilon = 0.25
    assert abs(node.epsilon - 0.25) < 1e-10

    node.assembly_name = "Hierarchy"
    assert node.assembly_name == "Hierarchy"


def test_selection_node_repr():
    """SelectionNode repr."""
    node = vtkSelectionNode()
    node.content_type = "INDICES"
    node.field_type = "CELL"
    ids = vtkIdTypeArray()
    ids.InsertNextValue(0)
    ids.InsertNextValue(5)
    node.selection_list = ids
    r = repr(node)
    assert "INDICES" in r
    assert "CELL" in r
    assert "2" in r


def test_selection_repr_single_node():
    """Selection repr for single-node selection."""
    sel = vtkSelection(
        content_type="INDICES", field_type="CELL",
        selection_list=np.array([0, 5, 10], dtype=np.int64),
    )
    r = repr(sel)
    assert "INDICES" in r
    assert "CELL" in r
    assert "3 values" in r


def test_selection_repr_multi_node():
    """Selection repr for multi-node selection."""
    sel1 = vtkSelection(
        content_type="INDICES", field_type="CELL",
        selection_list=np.array([0], dtype=np.int64),
    )
    sel2 = vtkSelection(
        content_type="INDICES", field_type="CELL",
        selection_list=np.array([1], dtype=np.int64),
    )
    combined = sel1 & sel2
    r = repr(combined)
    assert "2 nodes" in r
    assert "expression=" in r


def test_vtk_array_passthrough():
    """Passing a VTK array directly as selection_list."""
    ids = vtkIdTypeArray()
    ids.InsertNextValue(42)
    sel = vtkSelection(
        content_type="INDICES", field_type="CELL",
        selection_list=ids,
    )
    assert sel[0].selection_list.GetValue(0) == 42


def test_node_selection_list_vtk_array_setter():
    """Setting selection_list with a VTK array on a node."""
    node = vtkSelectionNode()
    ids = vtkIdTypeArray()
    ids.InsertNextValue(99)
    node.selection_list = ids
    assert node.selection_list.GetValue(0) == 99


def test_from_thresholds_with_qualifiers():
    """from_thresholds passes through qualifiers."""
    sel = vtkSelection().from_thresholds(
        "Temp", (0, 100), inverse=True, composite_index=3,
    )
    node = sel[0]
    assert node.inverse is True
    assert node.composite_index == 3


def test_from_values_with_component():
    """from_values sets component_number."""
    sel = vtkSelection().from_values("Velocity", [1.0, 2.0], component=1)
    node = sel[0]
    assert node.component_number == 1


def test_operators_with_non_selection():
    """Operators with non-vtkSelection return NotImplemented."""
    sel = vtkSelection(
        content_type="INDICES", field_type="CELL",
        selection_list=np.array([0], dtype=np.int64),
    )
    try:
        _ = sel & 42
        assert False, "Expected TypeError"
    except TypeError:
        pass

    try:
        _ = sel | "foo"
        assert False, "Expected TypeError"
    except TypeError:
        pass


def _make_indices_node(ids):
    node = vtkSelectionNode()
    node.content_type = "INDICES"
    node.field_type = "CELL"
    arr = vtkIdTypeArray()
    for i in ids:
        arr.InsertNextValue(i)
    node.selection_list = arr
    return node


def test_reconstructed_selection_operators():
    """Operators must work on selections that skipped our __init__.

    When VTK rewraps an existing C++ vtkSelection from a pointer, the
    Pythonic __init__ takes its early-return path and never runs the
    body, so the instance never gets a per-instance _expr_label. The
    boolean operators must still work via the class-level default rather
    than raising AttributeError. Regression test for that crash.
    """
    src = vtkSelection(
        content_type="INDICES", field_type="CELL",
        selection_list=np.array([0, 5, 10], dtype=np.int64),
    )

    # Drive the pointer-reconstruction code path explicitly: passing an
    # address string triggers the early-return branch in __init__, exactly
    # as VTK's wrapping layer does when it rewraps a C++ object.
    rewrapped = type(src)(src.__this__)
    assert rewrapped._expr_label is None, \
        "class-level _expr_label default missing"
    rewrapped.append(_make_indices_node([1, 2, 3]))

    # None of these may raise AttributeError.
    inverted = ~rewrapped
    assert inverted.expression, "invert produced empty expression"

    combined = rewrapped & src
    assert combined.expression, "combine produced empty expression"
    assert len(combined) == len(rewrapped) + len(src)

    either = rewrapped | src
    assert either.expression


# ---------------------------------------------------------------------------
# Run all tests
# ---------------------------------------------------------------------------

def main():
    tests = [v for k, v in sorted(globals().items()) if k.startswith("test_")]
    passed = 0
    failed = 0
    for t in tests:
        try:
            t()
            passed += 1
            print("  PASS: %s" % t.__name__)
        except Exception as e:
            failed += 1
            print("  FAIL: %s — %s" % (t.__name__, e))
            import traceback
            traceback.print_exc()
    print("\n%d passed, %d failed" % (passed, failed))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
