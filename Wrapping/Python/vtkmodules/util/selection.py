"""Pythonic API for vtkSelection and vtkSelectionNode.

Simplifies selection construction and composition::

    from vtkmodules.vtkCommonDataModel import vtkSelection, vtkSelectionNode
    import numpy as np

    # One-line index selection (creates a single-node selection):
    sel = vtkSelection(content_type="INDICES", field_type="CELL",
                       selection_list=np.array([0, 5, 10]))

    # Threshold selection via populate helper:
    temp_sel = vtkSelection().from_thresholds("Temperature", (0, 100))

    # Boolean composition — produces multi-node selections:
    combined = sel & temp_sel          # AND
    either   = sel | temp_sel          # OR
    inverted = ~sel                    # NOT
    compound = sel & (temp_sel | ~inverted)

    # Container interface:
    len(sel)                           # number of nodes
    sel[0]                             # node by index
    sel["NodeName"]                    # node by name
    for node in sel: ...               # iterate nodes
    sel.expression                     # get/set expression string

    # SelectionNode properties:
    node = sel[0]
    node.content_type                  # "INDICES"
    node.field_type                    # "CELL"
    node.selection_list                # vtkAbstractArray
"""

import string

from vtkmodules.vtkCommonDataModel import vtkSelection, vtkSelectionNode

# ---------------------------------------------------------------------------
# String <-> int maps for content type and field type
# ---------------------------------------------------------------------------

_CONTENT_TYPE_MAP = {
    "GLOBALIDS": vtkSelectionNode.GLOBALIDS,
    "PEDIGREEIDS": vtkSelectionNode.PEDIGREEIDS,
    "VALUES": vtkSelectionNode.VALUES,
    "INDICES": vtkSelectionNode.INDICES,
    "FRUSTUM": vtkSelectionNode.FRUSTUM,
    "LOCATIONS": vtkSelectionNode.LOCATIONS,
    "THRESHOLDS": vtkSelectionNode.THRESHOLDS,
    "BLOCKS": vtkSelectionNode.BLOCKS,
    "BLOCK_SELECTORS": vtkSelectionNode.BLOCK_SELECTORS,
    "QUERY": vtkSelectionNode.QUERY,
    "USER": vtkSelectionNode.USER,
}
_CONTENT_TYPE_REVERSE = {v: k for k, v in _CONTENT_TYPE_MAP.items()}

_FIELD_TYPE_MAP = {
    "CELL": vtkSelectionNode.CELL,
    "POINT": vtkSelectionNode.POINT,
    "FIELD": vtkSelectionNode.FIELD,
    "VERTEX": vtkSelectionNode.VERTEX,
    "EDGE": vtkSelectionNode.EDGE,
    "ROW": vtkSelectionNode.ROW,
}
_FIELD_TYPE_REVERSE = {v: k for k, v in _FIELD_TYPE_MAP.items()}

# Qualifier keys that map to vtkInformationIntegerKey on node properties
_INT_QUALIFIER_KEYS = {
    "inverse": "INVERSE",
    "containing_cells": "CONTAINING_CELLS",
    "connected_layers": "CONNECTED_LAYERS",
    "connected_layers_remove_seed": "CONNECTED_LAYERS_REMOVE_SEED",
    "connected_layers_remove_intermediate_layers": "CONNECTED_LAYERS_REMOVE_INTERMEDIATE_LAYERS",
    "composite_index": "COMPOSITE_INDEX",
    "component_number": "COMPONENT_NUMBER",
    "process_id": "PROCESS_ID",
    "hierarchical_level": "HIERARCHICAL_LEVEL",
    "hierarchical_index": "HIERARCHICAL_INDEX",
}


def _resolve_content_type(value):
    """Accept string or int, return int content type."""
    if isinstance(value, str):
        key = value.upper()
        if key not in _CONTENT_TYPE_MAP:
            raise ValueError(
                "Unknown content type %r. Valid types: %s"
                % (value, ", ".join(sorted(_CONTENT_TYPE_MAP)))
            )
        return _CONTENT_TYPE_MAP[key]
    ivalue = int(value)
    if ivalue not in _CONTENT_TYPE_REVERSE:
        raise ValueError(
            "Unknown content type %r. Valid types: %s"
            % (value, ", ".join(sorted(_CONTENT_TYPE_MAP)))
        )
    return ivalue


def _resolve_field_type(value):
    """Accept string or int, return int field type."""
    if isinstance(value, str):
        key = value.upper()
        if key not in _FIELD_TYPE_MAP:
            raise ValueError(
                "Unknown field type %r. Valid types: %s"
                % (value, ", ".join(sorted(_FIELD_TYPE_MAP)))
            )
        return _FIELD_TYPE_MAP[key]
    ivalue = int(value)
    if ivalue not in _FIELD_TYPE_REVERSE:
        raise ValueError(
            "Unknown field type %r. Valid types: %s"
            % (value, ", ".join(sorted(_FIELD_TYPE_MAP)))
        )
    return ivalue


def _set_qualifiers(node, qualifiers):
    """Set qualifier properties on a vtkSelectionNode from a dict."""
    props = node.GetProperties()
    for pyname, vtkname in _INT_QUALIFIER_KEYS.items():
        if pyname in qualifiers:
            val = qualifiers[pyname]
            key = getattr(vtkSelectionNode, vtkname)()
            # int() already handles bool (int(True) == 1).
            props.Set(key, int(val))

    if "epsilon" in qualifiers:
        node.epsilon = qualifiers["epsilon"]


def _to_vtk_array(values, array_type=None, num_components=1, name=None):
    """Convert a Python sequence or numpy array to a VTK array."""
    import numpy as np

    from vtkmodules.vtkCommonCore import vtkAbstractArray

    if isinstance(values, vtkAbstractArray):
        return values

    arr = np.ascontiguousarray(values)

    if array_type == "string":
        from vtkmodules.vtkCommonCore import vtkStringArray

        sa = vtkStringArray()
        if name:
            sa.SetName(name)
        for v in arr.flat:
            sa.InsertNextValue(str(v))
        return sa

    from vtkmodules.util.numpy_support import numpy_to_vtk

    vtk_arr = numpy_to_vtk(arr)
    if num_components > 1:
        vtk_arr.SetNumberOfComponents(num_components)
        vtk_arr.SetNumberOfTuples(len(arr) // num_components)
    if name:
        vtk_arr.SetName(name)
    return vtk_arr


# ---------------------------------------------------------------------------
# _SelectionNodeMixin
# ---------------------------------------------------------------------------

class _SelectionNodeMixin:
    """Pythonic properties for vtkSelectionNode."""

    @property
    def content_type(self):
        """Content type as a string (e.g. 'INDICES', 'THRESHOLDS')."""
        ct = self.GetContentType()
        return _CONTENT_TYPE_REVERSE.get(ct, str(ct))

    @content_type.setter
    def content_type(self, value):
        self.SetContentType(_resolve_content_type(value))

    @property
    def field_type(self):
        """Field type as a string (e.g. 'CELL', 'POINT')."""
        ft = self.GetFieldType()
        return _FIELD_TYPE_REVERSE.get(ft, str(ft))

    @field_type.setter
    def field_type(self, value):
        self.SetFieldType(_resolve_field_type(value))

    @property
    def selection_list(self):
        """The selection list array."""
        return self.GetSelectionList()

    @selection_list.setter
    def selection_list(self, value):
        from vtkmodules.vtkCommonCore import vtkAbstractArray

        if isinstance(value, vtkAbstractArray):
            self.SetSelectionList(value)
        else:
            self.SetSelectionList(_to_vtk_array(value))

    # Qualifier properties

    @property
    def inverse(self):
        props = self.GetProperties()
        key = vtkSelectionNode.INVERSE()
        return bool(props.Get(key)) if props.Has(key) else False

    @inverse.setter
    def inverse(self, value):
        self.GetProperties().Set(vtkSelectionNode.INVERSE(), int(bool(value)))

    @property
    def containing_cells(self):
        props = self.GetProperties()
        key = vtkSelectionNode.CONTAINING_CELLS()
        return bool(props.Get(key)) if props.Has(key) else False

    @containing_cells.setter
    def containing_cells(self, value):
        self.GetProperties().Set(
            vtkSelectionNode.CONTAINING_CELLS(), int(bool(value))
        )

    @property
    def connected_layers(self):
        props = self.GetProperties()
        key = vtkSelectionNode.CONNECTED_LAYERS()
        return props.Get(key) if props.Has(key) else 0

    @connected_layers.setter
    def connected_layers(self, value):
        self.GetProperties().Set(
            vtkSelectionNode.CONNECTED_LAYERS(), int(value)
        )

    @property
    def composite_index(self):
        props = self.GetProperties()
        key = vtkSelectionNode.COMPOSITE_INDEX()
        return props.Get(key) if props.Has(key) else None

    @composite_index.setter
    def composite_index(self, value):
        self.GetProperties().Set(
            vtkSelectionNode.COMPOSITE_INDEX(), int(value)
        )

    @property
    def component_number(self):
        props = self.GetProperties()
        key = vtkSelectionNode.COMPONENT_NUMBER()
        return props.Get(key) if props.Has(key) else None

    @component_number.setter
    def component_number(self, value):
        self.GetProperties().Set(
            vtkSelectionNode.COMPONENT_NUMBER(), int(value)
        )

    @property
    def epsilon(self):
        """Distance tolerance for LOCATIONS selections."""
        props = self.GetProperties()
        key = vtkSelectionNode.EPSILON()
        return props.Get(key) if props.Has(key) else None

    @epsilon.setter
    def epsilon(self, value):
        self.GetProperties().Set(vtkSelectionNode.EPSILON(), float(value))

    @property
    def assembly_name(self):
        """Data assembly name for BLOCK_SELECTORS selections."""
        props = self.GetProperties()
        key = vtkSelectionNode.ASSEMBLY_NAME()
        return props.Get(key) if props.Has(key) else None

    @assembly_name.setter
    def assembly_name(self, value):
        self.GetProperties().Set(vtkSelectionNode.ASSEMBLY_NAME(), str(value))

    def __repr__(self):
        ct = self.content_type
        ft = self.field_type
        sl = self.GetSelectionList()
        n = sl.GetNumberOfTuples() if sl else 0
        return "vtkSelectionNode(content_type=%r, field_type=%r, n_values=%d)" % (
            ct, ft, n
        )


# ---------------------------------------------------------------------------
# _SelectionMixin
# ---------------------------------------------------------------------------

# Label generator: A, B, C, ..., Z, AA, AB, ...
def _next_label(labels_used):
    """Return the next unused label."""
    for i in range(1, 100):
        if i <= 26:
            label = string.ascii_uppercase[i - 1]
        else:
            # AA, AB, etc.
            label = string.ascii_uppercase[(i - 1) // 26 - 1] + string.ascii_uppercase[(i - 1) % 26]
        if label not in labels_used:
            return label
    raise RuntimeError("Exhausted label space")


def _remap_expr(expr, name_map):
    """Replace node names in an expression simultaneously.

    Uses regex word-boundary matching to avoid partial replacements,
    and replaces all names in a single pass to prevent collisions
    when a new name equals another old name.
    """
    import re

    if not name_map or not expr:
        return expr
    # Sort by length descending so longer names match first
    pattern = "|".join(
        re.escape(old) for old in sorted(name_map, key=len, reverse=True)
    )
    # Use word boundaries to match whole names only
    pattern = r"\b(" + pattern + r")\b"
    return re.sub(pattern, lambda m: name_map[m.group(0)], expr)


class _SelectionMixin:
    """Pythonic container and operator interface for vtkSelection."""

    # Class-level default so the attribute always exists, even for objects
    # that VTK reconstructs from an existing C++ pointer (which skip __init__
    # below). Without this, the boolean operators raise AttributeError.
    _expr_label = None

    def __init__(self, *args, content_type=None, field_type=None,
                 selection_list=None, **kwargs):
        # SWIG pointer reconstruction: tp_new already returned the
        # existing object; skip init to avoid clobbering state.
        if args and isinstance(args[0], str):
            return

        # Internal state for expression composition
        self._expr_label = None

        # Extract qualifier kwargs
        qualifier_keys = set(_INT_QUALIFIER_KEYS) | {"epsilon"}
        qualifiers = {}
        for key in list(kwargs):
            if key in qualifier_keys:
                qualifiers[key] = kwargs.pop(key)

        if content_type is not None:
            self._build_node(content_type, field_type, selection_list, qualifiers)

    def _build_node(self, content_type, field_type=None, selection_list=None,
                    qualifiers=None):
        """Create, configure, and append a vtkSelectionNode to this selection.

        Returns the assigned node name.
        """
        # Configure the node through its own pythonic API so the resolution and
        # array-conversion logic lives only in the node property setters.
        node = vtkSelectionNode()
        node.content_type = content_type
        if field_type is not None:
            node.field_type = field_type
        if selection_list is not None:
            node.selection_list = selection_list
        if qualifiers:
            _set_qualifiers(node, qualifiers)
        name = self.AddNode(node)
        self._expr_label = name
        return name

    # -- Container interface --

    def __len__(self):
        return self.GetNumberOfNodes()

    def __getitem__(self, key):
        if isinstance(key, str):
            node = self.GetNode(key)
            if node is None:
                raise KeyError(key)
            return node
        if isinstance(key, int):
            if key < 0:
                key += self.GetNumberOfNodes()
            if key < 0 or key >= self.GetNumberOfNodes():
                raise IndexError("node index out of range")
            return self.GetNode(key)
        raise TypeError("index must be int or str, not %s" % type(key).__name__)

    def __iter__(self):
        for i in range(self.GetNumberOfNodes()):
            yield self.GetNode(i)

    def append(self, node):
        """Add a node and return its assigned name."""
        return self.AddNode(node)

    def clear(self):
        """Remove all nodes."""
        self.RemoveAllNodes()

    @property
    def expression(self):
        """The boolean expression combining nodes."""
        return self.GetExpression()

    @expression.setter
    def expression(self, value):
        self.SetExpression(value)

    @property
    def node_names(self):
        """List of node names."""
        return [self.GetNodeNameAtIndex(i) for i in range(self.GetNumberOfNodes())]

    # -- Boolean operators --

    def _get_expr_label(self):
        """Get the expression label for this selection."""
        if self._expr_label is not None:
            return self._expr_label
        # For selections not created via our API, build a default expression
        n = self.GetNumberOfNodes()
        if n == 0:
            return ""
        if n == 1:
            return self.GetNodeNameAtIndex(0)
        # Multi-node: combine all names with |
        names = [self.GetNodeNameAtIndex(i) for i in range(n)]
        return "(%s)" % " | ".join(names)

    def _combine(self, other, op_str):
        """Combine two selections with a boolean operator."""
        result = vtkSelection()

        labels_used = set()

        # Copy nodes from self with fresh labels
        self_name_map = {}
        for i in range(self.GetNumberOfNodes()):
            old_name = self.GetNodeNameAtIndex(i)
            new_name = _next_label(labels_used)
            labels_used.add(new_name)
            self_name_map[old_name] = new_name
            node = vtkSelectionNode()
            node.DeepCopy(self.GetNode(i))
            result.SetNode(new_name, node)

        # Copy nodes from other with fresh labels
        other_name_map = {}
        for i in range(other.GetNumberOfNodes()):
            old_name = other.GetNodeNameAtIndex(i)
            new_name = _next_label(labels_used)
            labels_used.add(new_name)
            other_name_map[old_name] = new_name
            node = vtkSelectionNode()
            node.DeepCopy(other.GetNode(i))
            result.SetNode(new_name, node)

        # Remap expressions using simultaneous replacement to avoid
        # collisions when a new name matches an old name.
        self_expr = _remap_expr(self._get_expr_label(), self_name_map)
        other_expr = _remap_expr(other._get_expr_label(), other_name_map)

        # Build combined expression
        expr = "(%s) %s (%s)" % (self_expr, op_str, other_expr)
        result.SetExpression(expr)
        result._expr_label = expr
        return result

    def __and__(self, other):
        if not isinstance(other, vtkSelection):
            return NotImplemented
        return self._combine(other, "&")

    def __or__(self, other):
        if not isinstance(other, vtkSelection):
            return NotImplemented
        return self._combine(other, "|")

    def __xor__(self, other):
        if not isinstance(other, vtkSelection):
            return NotImplemented
        return self._combine(other, "^")

    def __invert__(self):
        result = vtkSelection()
        labels_used = set()
        name_map = {}
        for i in range(self.GetNumberOfNodes()):
            old_name = self.GetNodeNameAtIndex(i)
            new_name = _next_label(labels_used)
            labels_used.add(new_name)
            name_map[old_name] = new_name
            node = vtkSelectionNode()
            node.DeepCopy(self.GetNode(i))
            result.SetNode(new_name, node)

        inner_expr = _remap_expr(self._get_expr_label(), name_map)

        expr = "!(%s)" % inner_expr
        result.SetExpression(expr)
        result._expr_label = expr
        return result

    # -- Populate helpers for tricky content types --
    #
    # These are instance methods that clear any existing nodes and configure
    # this selection in place, returning self so they can be chained, e.g.
    # ``vtkSelection().from_thresholds("Temperature", (0, 100))``.

    def from_thresholds(self, array_name, ranges, field_type="CELL", **qualifiers):
        """Select elements whose array values fall within ranges.

        Parameters
        ----------
        array_name : str
            Name of the data array to threshold on.
        ranges : tuple or list of tuples
            A single (min, max) pair or a list of (min, max) pairs.
        field_type : str or int
            Field type (default "CELL").
        **qualifiers
            Optional qualifiers (inverse, composite_index, etc.).
        """
        import numpy as np

        # Accept a single (min, max) pair given as either a tuple or a list,
        # wrapping it into a one-element list of ranges.
        if (isinstance(ranges, (tuple, list)) and len(ranges) == 2
                and not isinstance(ranges[0], (list, tuple))):
            ranges = [ranges]

        flat = []
        for lo, hi in ranges:
            flat.extend([lo, hi])
        arr = np.array(flat, dtype=np.float64)
        vtk_arr = _to_vtk_array(arr, num_components=2)
        vtk_arr.SetName(array_name)

        self.RemoveAllNodes()
        self._build_node("THRESHOLDS", field_type, vtk_arr, qualifiers)
        return self

    def from_values(self, array_name, values, field_type="CELL",
                    component=-1, **qualifiers):
        """Select elements with exact array values.

        Parameters
        ----------
        array_name : str
            Name of the data array to match.
        values : array-like
            Values to match.
        field_type : str or int
            Field type (default "CELL").
        component : int
            Component to compare (-1 for magnitude).
        **qualifiers
            Optional qualifiers.
        """
        vtk_arr = _to_vtk_array(values)
        vtk_arr.SetName(array_name)

        qualifiers["component_number"] = component
        self.RemoveAllNodes()
        self._build_node("VALUES", field_type, vtk_arr, qualifiers)
        return self

    def from_locations(self, points, field_type="POINT", epsilon=None,
                       **qualifiers):
        """Select elements near world coordinates.

        Parameters
        ----------
        points : array-like
            (N, 3) array of world coordinates.
        field_type : str or int
            Field type (default "POINT").
        epsilon : float, optional
            Distance tolerance.
        **qualifiers
            Optional qualifiers.
        """
        import numpy as np

        pts = np.ascontiguousarray(points, dtype=np.float64).ravel()
        vtk_arr = _to_vtk_array(pts, num_components=3)

        if epsilon is not None:
            qualifiers["epsilon"] = epsilon
        self.RemoveAllNodes()
        self._build_node("LOCATIONS", field_type, vtk_arr, qualifiers)
        return self

    def from_frustum(self, corners, field_type="CELL", **qualifiers):
        """Select elements within a viewing frustum.

        Parameters
        ----------
        corners : array-like
            (8, 4) array of frustum corners in homogeneous coordinates,
            flattened to 32 values.
        field_type : str or int
            Field type (default "CELL").
        **qualifiers
            Optional qualifiers.
        """
        import numpy as np

        arr = np.ascontiguousarray(corners, dtype=np.float64).ravel()
        if len(arr) != 32:
            raise ValueError("Frustum requires exactly 32 values (8 corners x 4 components), got %d" % len(arr))
        vtk_arr = _to_vtk_array(arr)

        self.RemoveAllNodes()
        self._build_node("FRUSTUM", field_type, vtk_arr, qualifiers)
        return self

    def from_blocks(self, block_indices, **qualifiers):
        """Select blocks in a composite dataset by flat index.

        Parameters
        ----------
        block_indices : array-like
            List or array of block indices.
        **qualifiers
            Optional qualifiers.
        """
        import numpy as np

        arr = np.ascontiguousarray(block_indices, dtype=np.int64)
        vtk_arr = _to_vtk_array(arr)

        self.RemoveAllNodes()
        self._build_node("BLOCKS", None, vtk_arr, qualifiers)
        return self

    def from_block_selectors(self, selectors, assembly_name=None, **qualifiers):
        """Select blocks using selector expressions.

        Parameters
        ----------
        selectors : list of str
            Selector expression strings.
        assembly_name : str, optional
            Name of the data assembly to apply selectors to.
        **qualifiers
            Optional qualifiers.
        """
        vtk_arr = _to_vtk_array(selectors, array_type="string")

        self.RemoveAllNodes()
        self._build_node("BLOCK_SELECTORS", None, vtk_arr, qualifiers)
        if assembly_name is not None:
            self.GetNode(0).assembly_name = assembly_name
        return self

    # -- repr --

    def __repr__(self):
        n = self.GetNumberOfNodes()
        if n == 1:
            node = self.GetNode(0)
            ct = _CONTENT_TYPE_REVERSE.get(node.GetContentType(),
                                           str(node.GetContentType()))
            ft = _FIELD_TYPE_REVERSE.get(node.GetFieldType(),
                                         str(node.GetFieldType()))
            sl = node.GetSelectionList()
            nv = sl.GetNumberOfTuples() if sl else 0
            return "vtkSelection(content_type=%r, field_type=%r, %d values)" % (
                ct, ft, nv
            )
        expr = self.GetExpression()
        if expr:
            return "vtkSelection(%d nodes, expression=%r)" % (n, expr)
        return "vtkSelection(%d nodes)" % n


# ---------------------------------------------------------------------------
# Apply overrides
# ---------------------------------------------------------------------------

@vtkSelectionNode.override
class SelectionNode(_SelectionNodeMixin, vtkSelectionNode):
    pass


@vtkSelection.override
class Selection(_SelectionMixin, vtkSelection):
    pass
