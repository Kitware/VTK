## Create a new class QVTKTableModelAdapter

**Goal:** Provide an adapter for QAbstractItemModel to vtkTable

Qt's QAbstractItemModel provides functionality to connect data from a source to a view, like a table view etc. The Qt library provides plotting features via its QtCharts modules; however these are either GPL or require a commercial license. There are not many good 3rd party library to create charts using widgets in Qt.

The 2D charts/plots in vtk are great though a little bit under-developed. The intention here is to provide a coupling mechanism from a generic QAbstractItemModel to create a vtkTable output. This output can then be used directly in vtkPlot subclasses, or be used as an input to vtkTableAlgorithm to further modify the output prior to plotting or creating other useful data from it.

In order to achieve this the following changes were made:

**vtkTable:** More functionality was added and some code was cleaned up. Tests were expanded.

**vtkAlgorithm:** Minor change to use proper vtkTable API.

**vtkPlot subclasses and vtkChartXY:** Modifications were made so that vtkChartXY can work with vtkTableAlgorithm outputs instead of only using vtkTable as input data. The vtkPlot subclasses were quite fragmented in their use of caches; the API has been cleaned up to provide a more uniform approach.

**QVTKTableModelAdapter:** A new class which takes a QAbstractItemModel as input was created. This class converts the Qt table model into an internal table, which then can be used by vtkTableAlgorithm or the vtkPlot subclasses. Note that the input to this class is QAbstractItemModel, not QAbstractTableModel, so that the class can work with QAbstractProxyModel subclasses (like filters) as well.
