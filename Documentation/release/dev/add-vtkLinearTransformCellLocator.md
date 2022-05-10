## Add vtkLinearTransformCellLocator

vtkLinearTransformCellLocator is a cell locator adaptor that can accept any cell locator, e.g.
vtkStaticCellLocator, vtkCellLocator, calculate the transformation matrix from the cell
locator adaptor's dataset to the given dataset inside BuildLocator, and then use the cell locator
and transformation to perform cell locator operations. The transformation matrix is computed
using the https://en.wikipedia.org/wiki/Kabsch_algorithm. UseAllPoints allows to compute the
transformation using all the points of the dataset (use that when you are not if it's a linear
transformation) or 100 sample points (or less if the dataset is smaller) that are chosen
every-nth. IsLinearTransformation validates if the dataset is a linear transformation of the cell
locator's dataset based on the used points. This functionality is particularly useful for
datasets that have time-steps and each timestep is a linear transformation of the first timestep. Finally, the
vtkCellLocator, vtkStaticCellLocator, vtkCellTreeLocator, vtkModifiedBSPTree, and vtkLinearTransformCellLocator cell
locators also support ShallowCopy now.
