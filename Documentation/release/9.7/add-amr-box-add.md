## Add vtkAMRBox::Add

vtkAMRBox now provides an API to add multiple vtkAMRBox together,
growing the box until it contains itself and provided box.

```cpp
vtkAMRBox::Add(int i, int j, int k);
vtkAMRBox::Add(int ijk[3]);
```
