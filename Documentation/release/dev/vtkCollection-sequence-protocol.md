## vtkCollection now supports Python sequence protocol

All `vtkCollection` subclasses now support `len()`, indexing with `[]`
(including negative indices), and membership testing with `in`:

```python
collection = vtkActorCollection()
collection.AddItem(actor1)
collection.AddItem(actor2)

len(collection)        # 2
collection[0]          # actor1
collection[-1]         # actor2
actor1 in collection   # True
```

This is implemented at the C level via `tp_as_sequence`, so it is
automatically inherited by all ~23 subclasses of `vtkCollection`
(e.g., `vtkRendererCollection`, `vtkDataArrayCollection`, etc.)
without any additional Python code.

Pythonic mutation methods are also available:

```python
collection.append(actor3)     # AddItem
collection.insert(0, actor4)  # InsertItem
collection.remove(actor1)     # RemoveItem
collection.clear()            # RemoveAllItems
```

All of this is implemented at the C level and automatically inherited
by all ~23 subclasses of `vtkCollection` (e.g., `vtkRendererCollection`,
`vtkDataArrayCollection`, etc.) without any additional Python code.

The existing iteration protocol (`for item in collection`) continues
to work as before.
