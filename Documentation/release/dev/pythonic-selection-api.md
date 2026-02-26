## Pythonic API for vtkSelection and vtkSelectionNode

A new Pythonic API makes working with selections simpler and more composable.

### One-line construction

Instead of manually creating nodes, setting integer enums, and wiring arrays:

```python
from vtkmodules.vtkCommonDataModel import vtkSelection
import numpy as np

# Index selection — one line instead of ~10
sel = vtkSelection(content_type="INDICES", field_type="CELL",
                   selection_list=np.array([0, 5, 10]))
```

### Boolean composition with Python operators

Selections can be combined using `&` (AND), `|` (OR), `^` (XOR), and `~` (NOT).
Each operator returns a new selection with the correct expression string:

```python
combined = sel1 & sel2          # AND
either   = sel1 | sel2          # OR
inverted = ~sel                 # NOT
compound = sel1 & (sel2 | ~sel3)  # compound expression
```

### Class methods for complex content types

```python
# Threshold selection
sel = vtkSelection.from_thresholds("Temperature", (0, 100))
sel = vtkSelection.from_thresholds("Temp", [(0, 10), (50, 100)])

# Value matching
sel = vtkSelection.from_values("CellType", [1, 3, 5])

# Location-based
sel = vtkSelection.from_locations(np.array([[0,0,0], [1,1,1]]), epsilon=0.01)

# Frustum selection
sel = vtkSelection.from_frustum(corners_array)

# Block selection
sel = vtkSelection.from_blocks([0, 2, 5])
sel = vtkSelection.from_block_selectors(["//Block[@name='Mesh']"],
                                        assembly_name="Hierarchy")
```

### Container interface

```python
len(sel)              # number of nodes
sel[0]                # node by index
sel["NodeName"]       # node by name
for node in sel: ...  # iterate nodes
sel.append(node)      # add a node, returns assigned name
sel.clear()           # remove all nodes
sel.expression        # get/set expression string
sel.node_names        # list of node names
```

### SelectionNode properties

```python
node = sel[0]
node.content_type           # "INDICES" (string, get/set)
node.field_type             # "CELL" (string, get/set)
node.selection_list         # vtkAbstractArray (get/set)
node.inverse                # bool qualifier
node.containing_cells       # bool qualifier
node.connected_layers       # int qualifier
node.composite_index        # int qualifier
node.component_number       # int qualifier
```
