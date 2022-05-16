## Pain Point
When writing tests for features that act on HyperTreeGrids (HTGs) it can be annoying and error prone to have to generate the data every time. This leads to either testing that takes a long time to write or that only is tested on one kind of HTG.

## Feature
Introduce a helper class `vtkTestHTGGenerator` that just serves to generate some specific types of HTGs for testing.
