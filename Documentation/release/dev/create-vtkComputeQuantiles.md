## Create new vtkComputeQuantiles class from existing vtkComputeQuartiles

```vtkComputeNtiles``` is a new super class of ```vtkComputeQuartiles```. It is more generic than ```vtkComputeQuartiles``` since the number of intervals can be freely chosen. Hence it is possible to calculate deciles, quintiles etc.

The code from ```vtkComputeQuartiles``` has been shifted into the new superclass. ```vtkComputeQuartiles``` becomes a new subclass which simply sets the number of intervals to 4.
