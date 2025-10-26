# vtkObject Observers

Every `vtkObject` maintains a **priority-ordered** list of *observers* (callbacks) that may be invoked by
`vtkObject::InvokeEvent`. In practice, this means:

* **Priority:** Higher numeric priority runs first (default is `0`).
* **Stable order at equal priority:** Observers with the same priority generally run in **insertion order**.
* **Legacy exceptions:**
  * When a **new lowest priority** observer is added, that observer is **pinned to the end** of the list.
  * An observer may **grab focus**; if it handles an event, remaining observers are **not** invoked.

If an application requires two observers to execute in a particular relative order, then the application ***MUST*** add
those observers with appropriate priorities. The behavior described here is maintained for legacy compatibility reasons
only, and may be modified in the future.

### Example Ordering

For example, adding four observers with default priority results in invocation order:

```python
AddObserver(priority=0)  # tag = 1
AddObserver(priority=0)  # tag = 2
AddObserver(priority=0)  # tag = 3
AddObserver(priority=0)  # tag = 4

InvokeEvent()  # order 2 3 4 1
```

Higher priorities are not pinned (they run first in insertion order). Lower priorities trigger the pin-at-end behavior.

```python
AddObserver(priority=+1)  # tag = 5
AddObserver(priority=+1)  # tag = 6

AddObserver(priority=-1)  # tag = 7
AddObserver(priority=-1)  # tag = 8

InvokeEvent()  # order 5 6 / 2 3 4 1 / 8 7
```

Since observer `tag = 1` is no longer at the end, its pin is "released" and subsequent default-priority observers are
invoked in insertion order.

```python
AddObserver(priority=0)  # tag = 9

InvokeEvent()  # order 5 6 / 2 3 4 1 9 / 8 7
```
