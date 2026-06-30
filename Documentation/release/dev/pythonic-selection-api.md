## Pythonic API for vtkSelection and vtkSelectionNode

`vtkSelection` and `vtkSelectionNode` now have a Pythonic API that
collapses the usual boilerplate of constructing nodes, looking up
integer enums, building qualifier maps, and wiring expression strings
into a few keyword-argument constructions and Python operators.

The new behavior is enabled automatically when `vtkCommonDataModel` is
loaded — no extra import is required.

### One-line construction

The classic pattern requires a node, an integer content type, an
integer field type, an explicit `vtkAbstractArray`, and an `AddNode`
call. The new constructor accepts those as keyword arguments and
creates and registers the node in one step:

```python
from vtkmodules.vtkCommonDataModel import vtkSelection
import numpy as np

sel = vtkSelection(
    content_type="INDICES",   # string or vtkSelectionNode.* int
    field_type="CELL",
    selection_list=np.array([0, 5, 10]),
)
```

Content types (`"INDICES"`, `"VALUES"`, `"THRESHOLDS"`, `"FRUSTUM"`,
`"LOCATIONS"`, `"BLOCKS"`, `"BLOCK_SELECTORS"`, `"GLOBALIDS"`,
`"PEDIGREEIDS"`, `"QUERY"`, `"USER"`) and field types (`"CELL"`,
`"POINT"`, `"FIELD"`, `"VERTEX"`, `"EDGE"`, `"ROW"`) accept either the
string name or the integer enum from `vtkSelectionNode`.

Selection lists accept any numpy array, Python sequence, or existing
`vtkAbstractArray`; the helper converts as needed.

### Qualifiers as keyword arguments

`vtkSelectionNode` qualifiers — previously set via `node.GetProperties().Set(KEY(), value)` — are now plain kwargs:

```python
sel = vtkSelection(
    content_type="INDICES",
    field_type="CELL",
    selection_list=ids,
    inverse=True,             # INVERSE
    containing_cells=True,    # CONTAINING_CELLS
    connected_layers=2,       # CONNECTED_LAYERS
    composite_index=3,        # COMPOSITE_INDEX
    component_number=0,       # COMPONENT_NUMBER
)
```

Supported qualifier kwargs: `inverse`, `containing_cells`,
`connected_layers`, `connected_layers_remove_seed`,
`connected_layers_remove_intermediate_layers`, `composite_index`,
`component_number`, `process_id`, `hierarchical_level`,
`hierarchical_index`, and `epsilon` (for `LOCATIONS`).

### Boolean composition with Python operators

Selections compose with `&` (AND), `|` (OR), `^` (XOR), and `~` (NOT).
Each operator deep-copies the operand nodes, assigns fresh non-
colliding names, and rewrites the expression string accordingly:

```python
combined = sel1 & sel2          # AND
either   = sel1 | sel2          # OR
exclusive = sel1 ^ sel2          # XOR
inverted = ~sel                  # NOT
compound = sel1 & (sel2 | ~sel3) # arbitrary nesting
```

The result is a new `vtkSelection` whose `expression` reflects the
combined logic — ready to pass to `vtkExtractSelection`.

### Populate helpers for non-trivial content types

For content types whose selection lists have specific shape or naming
requirements, dedicated helpers handle the conversion. They populate the
selection in place and return it, so they chain off a constructor call:

```python
# Threshold (single range or list of ranges)
sel = vtkSelection().from_thresholds("Temperature", (0, 100))
sel = vtkSelection().from_thresholds("Temperature", [(0, 10), (50, 100)])

# Exact value matching, optionally on a specific component
sel = vtkSelection().from_values("CellType", [1, 3, 5])
sel = vtkSelection().from_values("Velocity", [0.0, 1.0], component=2)

# World-space points (with optional distance tolerance)
sel = vtkSelection().from_locations(
    np.array([[0, 0, 0], [1, 1, 1]]), epsilon=0.01,
)

# Viewing frustum (8 corners x 4 homogeneous components = 32 values)
sel = vtkSelection().from_frustum(corners_array)

# Composite-dataset block selection
sel = vtkSelection().from_blocks([0, 2, 5])
sel = vtkSelection().from_block_selectors(
    ["//Block[@name='Mesh']"], assembly_name="Hierarchy",
)
```

All of these helpers accept the same qualifier kwargs as the constructor,
so e.g. `vtkSelection().from_thresholds("T", (0, 100), inverse=True)`
selects everything *outside* the range.

### Container interface

`vtkSelection` behaves like a container of nodes:

```python
len(sel)              # number of nodes
sel[0]                # node by integer index (negative indexing supported)
sel["Node0"]          # node by name (KeyError if missing)
for node in sel: ...  # iterate

sel.append(node)      # add a node, returns the assigned name
sel.clear()           # remove all nodes
sel.expression        # get/set the boolean expression string
sel.node_names        # list of node names
```

### `vtkSelectionNode` properties

Node attributes are exposed as Python properties so getters and setters
read like data access:

```python
node = sel[0]

# Core
node.content_type     # "INDICES" — string, get/set
node.field_type       # "CELL"    — string, get/set
node.selection_list   # vtkAbstractArray, get/set

# Qualifiers
node.inverse                   # bool
node.containing_cells          # bool
node.connected_layers          # int
node.composite_index           # int
node.component_number          # int
```

Setting `content_type` or `field_type` accepts either the string name
or the integer enum.

### Informative `repr`

```python
>>> vtkSelection(content_type="INDICES", field_type="CELL",
...              selection_list=np.array([0, 5, 10]))
vtkSelection(content_type='INDICES', field_type='CELL', 3 values)

>>> sel1 & sel2
vtkSelection(2 nodes, expression='(A) & (B)')
```

### Backwards compatibility

The classic API (`AddNode`, `GetNode`, `SetContentType(int)`,
`GetProperties().Set(...)`, manual expression strings) continues to
work exactly as before — the Pythonic features are additive and
installed via the standard `@vtkClassName.override` mechanism.
