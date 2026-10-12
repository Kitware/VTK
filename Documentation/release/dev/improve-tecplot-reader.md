## Improve vtkTecplotReader

### Support SOLUTIONTIME keyword

It is now possible to read time varying data in **.dat** format with the **vtkTecplotReader** with the keyword **SOLUTIONTIME**

### Support CONNECTIVITYSHARE keyword

For TecPlot data with SolutionTime, the key word **CONNECTIVITYSHARE** is now supported to read cell connectivity that are shared across different zones

### Support explicit NODAL keyword in VARLOCATION

The **vtkTecPlotReader** now supports the reading of explicit **NODAL** variables in the definition of **VARLOCATION**

### Support VARSHARELIST variable

The **vtkTecPlotReader** can now read **VARSHARELIST** keyword to read variables that are shared across different zones and timesteps

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
- **GetArraysFromBlockPackingZone**
- **GetArraysFromPointPackingZone**
- **GetUnstructuredGridCells**

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
