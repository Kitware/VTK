package vtk;

/**
 * Provide a mapping to VTK CellType enum
 *
 * @author sebastien jourdain - sebastien.jourdain@kitware.com
 */
public enum CellType {

  VERTEX(1, 1), //
  POLY_VERTEX(2, -1), //
  LINE(3, 2), //
  POLY_LINE(4, -1), //
  TRIANGLE(5, 3), //
  TRIANGLE_STRIP(6, -1), //
  POLYGON(7, -1), //
  PIXEL(8, 4), //
  QUAD(9, 4), //
  TETRA(10, 4), //
  VOXEL(11, 8), //
  HEXAHEDRON(12, 8), //
  WEDGE(13, 6), //
  PYRAMID(14, 5), //
  PENTAGONAL_PRISM(15, 10), //
  HEXAGONAL_PRISM(16, 12), //
  QUADRATRIC_EDGE(21, 3), //
  QUADRATRIC_TRIANGLE(22, 6), //
  QUADRATRIC_QUAD(23, 8), //
  QUADRATRIC_TETRA(24, 10), //
  QUADRATRIC_HEXAHEDRON(25, 20), //
  QUADRATRIC_WEDGE(26, 15), //
  QUADRATRIC_PYRAMID(27, 13);

  private CellType(int id, int nbPoints) {
    this.id = id;
    this.nbPoints = nbPoints;
  }

  /**
   * @return the id that VTK is using to identify it cell type.
   */
  public int GetId() {
    return id;
  }

  /**
   * @return the number of points that cell type own or -1 for cell that have
   *         a dynamic number of points.
   */
  public int GetNumberOfPoints() {
    return nbPoints;
  }

  /**
   * @return true if the number of points can not be given by the cell type
   */
  public boolean IsDynamicNumberOfPoints() {
    return nbPoints == -1;
  }

  /**
   * @param vtkCellId
   * @return an instance of CellType based on the vtk cell id.
   */
  public static CellType GetCellType(int vtkCellId) {
    if (MAPPING == null) {
      // build it lazyly
      int max = 0;
      for (CellType cellType : values()) {
        max = Math.max(max, cellType.GetId());
      }
      MAPPING = new CellType[max + 1];
      for (CellType cellType : values()) {
        MAPPING[cellType.GetId()] = cellType;
      }
    }
    return MAPPING[vtkCellId];
  }

  private int id;
  private int nbPoints;
  private static CellType[] MAPPING;
}
