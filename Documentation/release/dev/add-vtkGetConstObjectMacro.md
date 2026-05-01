## vtkGetConstObjectMacro for const object getters

VTK now provides `vtkGetConstObjectMacro`, a new macro in `vtkSetGet.h` that
generates const getter methods for `vtkObject` member variables. This macro
complements the existing `vtkGetObjectMacro` by providing const-correct access
to object pointers.

### Usage

The macro generates a virtual const method that returns a const pointer:

```cpp
class vtkMyClass : public vtkObject
{
public:
  vtkSetObjectMacro(Points, vtkPoints);
  vtkGetConstObjectMacro(Points, vtkPoints);  // Generates: virtual const vtkPoints* GetPoints() const

protected:
  vtkPoints* Points = nullptr;
};
```
