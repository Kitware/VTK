package vtk;

public class SampleAlgorithm extends Algorithm {
  public void initialize() {
    VTKAlgorithm.SetParameter("Rows", 10);
    VTKAlgorithm.SetParameter("Columns", 10);
    VTKAlgorithm.SetParameter("Default Value", 0.0);
    VTKAlgorithm.SetNumberOfInputPorts(0);
  }

  public boolean fillInputPortInformation(int port, vtkInformation info) {
    vtkAlgorithm temp = new vtkAlgorithm();
    info.Set(temp.INPUT_REQUIRED_DATA_TYPE(), "vtkTable", 0);
    return true;
  }

  public boolean fillOutputPortInformation(int port,
      vtkInformation info) {
    vtkDataObject temp = new vtkDataObject();
    info.Set(temp.DATA_TYPE_NAME(), "vtkTable");
    return true;
  }

  public boolean requestData(
      vtkInformation request,
      vtkInformationVector[] inputInfo,
      vtkInformationVector outputInfo) {
    System.out.println("in requestData");
    vtkTable output = (vtkTable)VTKAlgorithm.GetOutputDataObject(0);
    int rows = VTKAlgorithm.GetIntParameter("Rows");
    int cols = VTKAlgorithm.GetIntParameter("Columns");
    double value = VTKAlgorithm.GetDoubleParameter("Default Value");
    for (int i = 0; i < cols; ++i) {
      vtkDoubleArray column = new vtkDoubleArray();
      column.SetName("Column " + i);
      column.SetNumberOfTuples(rows);
      for (int j = 0; j < rows; ++j) {
        column.SetValue(j, value);
      }
      output.AddColumn(column);
    }
    return true;
  }
}

