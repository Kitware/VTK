## Add deprecated decorator to Python

VTK now provides a `@deprecated` decorator that emits a `DeprecationWarning` when a decorated function is called. You can edit the message shown to the user.

To mark a function as deprecated, use:
```python
@deprecated(version=1.2, message="Use 'new_function' instead.")
def old_function():
    pass
```
