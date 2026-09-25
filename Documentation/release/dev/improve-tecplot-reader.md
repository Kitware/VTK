## Improve vtkTecplotReader

### Support SOLUTIONTIME keyword

It is now possible to read time varying data in **.dat** format with the **vtkTecplotReader** with the keyword **SOLUTIONTIME**

### Support CONNECTIVITYSHARE keyword

For TecPlot data with SolutionTime, the key word **CONNECTIVITYSHARE** is now supported to read cell connectivity that are shared across different zones

### Removed protected methods and members

The following methods from **vtkTecplotReader** have been removed:
- **ReadFile**
- **GetStructuredGridFromBlockPackingZone**
- **GetStructuredGridFromPointPackingZone**
- **GetUnstructuredGridFromBlockPackingZone**
- **GetPolyhedralGridFromBlockPackingZone**
- **GetPolygonalGridFromBlockPackingZone**
- **GetPolyhedralGridCells**
- **GetPolygonalGridCells**
- **GetUnstructuredGridFromPointPackingZone**

The following variables from **vtkTecplotReader** have been removed:
- **NumberOfVariables**
- **FileName**
- **SelectionObserver**
- **DataArraySelection**
- **Internal**
- **DataTitle**
- **CellBased**
- **ZoneNames**
- **Variables**
