package vtk;

public abstract class Algorithm {

  public Algorithm() {
    VTKAlgorithm = null;
  }

  public void initialize(vtkJavaProgrammableFilter alg) {
    VTKAlgorithm = alg;
    initialize();
  }

  public void initialize() {
  }

  public boolean requestData(
      vtkInformation request,
      vtkInformationVector[] inputInfo,
      vtkInformationVector outputInfo) {
    return true;
  }

  public boolean requestDataObject(
      vtkInformation request,
      vtkInformationVector[] inputInfo,
      vtkInformationVector outputInfo) {
    return true;
  }

  public boolean requestInformation(
      vtkInformation request,
      vtkInformationVector[] inputInfo,
      vtkInformationVector outputInfo) {
    return true;
  }

  public boolean requestUpdateExtent(
      vtkInformation request,
      vtkInformationVector[] inputInfo,
      vtkInformationVector outputInfo) {
    return true;
  }

  public boolean fillInputPortInformation(int port, vtkInformation info) {
    return true;
  }

  public boolean fillOutputPortInformation(int port,
      vtkInformation info) {
    return true;
  }

  protected vtkJavaProgrammableFilter VTKAlgorithm;

  static {
    System.loadLibrary("vtkCommonJava");
    System.loadLibrary("vtkFilteringJava");
    System.loadLibrary("vtkIOJava");
    System.loadLibrary("vtkImagingJava");
    System.loadLibrary("vtkGraphicsJava");
    System.loadLibrary("vtkRenderingJava");
    System.loadLibrary("vtkInfovisJava");
    System.loadLibrary("vtkViewsJava");
    System.loadLibrary("vtkGeovisJava");
  }
}
