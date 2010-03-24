package vtk;

/**
 * Enum used to load native library more easily
 * 
 * @author sebastien jourdain - sebastien.jourdain@kitware.com
 */
public enum vtkNativeLibrary {

  COMMON("vtkCommonJava"), //
  FILTERING("vtkFilteringJava"),//
  GEOVIS("vtkGeovisJava"),//
  GRAPHICS("vtkGraphicsJava"),//
  HYBRID("vtkHybridJava"),//
  IMAGING("vtkImagingJava"),//
  INFOVIS("vtkInfovisJava"),//
  IO("vtkIOJava"),//
  RENDERING("vtkRenderingJava"),//
  VIEWS("vtkViewsJava"),//
  VOLUME_RENDERING("vtkVolumeRenderingJava"),//
  WIDGETS("vtkWidgetsJava"),
  CHARTS("vtkChartsJava");

  /**
   * Try to load all library
   *
   * @return true if all library have been successfully loaded
   */
  public static boolean LoadAllNativeLibraries() {
    boolean isEveryThingLoaded = true;
    for (vtkNativeLibrary lib : values()) {
      try {
        lib.LoadLibrary();
      } catch (Exception e) {
        isEveryThingLoaded = false;
      }
    }

    return isEveryThingLoaded;
  }

  /**
   * Load the set of given library and trows runtime exception if any given library failed in the loading process
   * 
   * @param nativeLibraries
   */
  public static void LoadNativeLibraries(vtkNativeLibrary... nativeLibraries) {
    for (vtkNativeLibrary lib : nativeLibraries) {
      lib.LoadLibrary();
    }
  }

  private vtkNativeLibrary(String nativeLibraryName) {
    this.nativeLibraryName = nativeLibraryName;
    this.loaded = false;
  }

  /**
   * Load the library and throws runtime exception if the library failed in the loading process
   */
  public void LoadLibrary() {
    if (!loaded) {
      System.loadLibrary(nativeLibraryName);
    }
    loaded = true;
  }

  /**
   * @return true if the library has already been succefuly loaded
   */
  public boolean IsLoaded() {
    return loaded;
  }

  /**
   * @return the library name
   */
  public String GetLibraryName() {
    return nativeLibraryName;
  }

  private String nativeLibraryName;
  private boolean loaded;
}
