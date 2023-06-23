## vtkDistancePolyDataFilter: Added directions as extra output

vtkDistancePolyDataFilter can output directions in conjunction with the (signed/unsigned) distances. The direction is given as two arrays, one on cells and one on points. The directions are enabled using `ComputeDirection` (default:off)
