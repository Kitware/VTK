## Linear vtkCell: Add Drawing, Add Clip, use contour tables, and Cleanup code

The code of all linear cells has been cleaned up, and the following improvements have been made:

1. All linear cells now have a drawing showing the numbering of their points.
2. Most linear cells now have a clip method implemented using `vtkMarchingCellsClipCases`
3. The contour tables of most linear cells, and `vtkMarchingSquaresLineCases`, `vtkMarchingCubesTriangleCases`,
   `vtkMarchingCubesPolygonCases` tables have now been moved to `vtkMarchingCellsContourCases`. Because of that,
   `vtkMarchingSquaresLineCases`, `vtkMarchingCubesTriangleCases`, `vtkMarchingCubesPolygonCases` have been deprecated.
4. It should be noted that the clip algorithm is inclusive of the IsoValue value, when InsideOut is false, and exclusive
   of the IsoValue value when InsideOut is true.
