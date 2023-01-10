## vtkResampleToImage: Improve performance

The performance of the vtkResampleToImage filter has been improved by:

1) Caching the cell arrays
2) Extracting the diagonal of the cell once
3) Extracting the cell only if needed
4) Optimizing the computation of the maximum cell volume using sqrt instead of pow
5) Assuming that the EvaluateLocation/EvaluationPosition function work only with double points
6) Skipping points that have been processed already
