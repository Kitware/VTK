// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDGCell
 * @brief   Base class for a discontinuous Galerkin cells of all shapes.
 *
 * This class exists to offer each shape's parameterization via a uniform API.
 *
 * All DG cells have shapes that can be described by corner points in
 * a reference (parametric) coordinate system. Sides (boundaries) of
 * the element of any dimension can be fetched as offsets into the list
 * of corners. You can also obtain a list of the coordinates in parametric
 * space of all the corner points.
 */

#ifndef vtkDGCell_h
#define vtkDGCell_h

#include "vtkFiltersCellGridModule.h" // for export macro

#include "vtkCellMetadata.h"
#include "vtkStringToken.h" // for vtkStringToken::Hash
#include "vtkVector.h"      // for IsInside API.

#include <vector> // for side connectivity

VTK_ABI_NAMESPACE_BEGIN

class vtkCellAttribute;
class vtkCellGrid;
class vtkDataArray;
class vtkDataSetAttributes;
class vtkTypeFloat32Array;
class vtkTypeInt32Array;

class VTKFILTERSCELLGRID_EXPORT vtkDGCell : public vtkCellMetadata
{
public:
  /// All possible shapes for DG cells.
  enum Shape : int
  {
    Vertex,        //!< A corner point.
    Edge,          //!< A curve connecting two vertices.
    Triangle,      //!< A three-cornered face bounded by 3 edges.
    Quadrilateral, //!< A four-cornered face bounded by 4 edges.
    Tetrahedron,   //!< A four-cornered volume bounded by 4 triangular shapes.
    Hexahedron,    //!< An eight-cornered volume; a quadrilateral prism.
    Wedge,         //!< A volumetric, triangular prism.
    Pyramid,       //!< A volumetric shape whose quadrilateral base attaches to a vertex.

    None //!< A placeholder for an indeterminate or invalid shape.
    // It is probably more efficient to create a new cell type for arbitrary shapes
    // than to attempt generalizing vtkDGCell, but for the sake of completeness:
    // Polygon       //!< We may one day support an n-sided polygonal face.
    // Polyhedron    //!< We may one day support an n-faced polyhedral volume with polygonal sides.
  };

  /// Records describing the source arrays for cells or cell-sides.
  struct Source
  {
    Source() = default;
    Source(const Source&) = default;
    Source(vtkDataArray* conn, vtkIdType off, bool blank, Shape shape, int sideType)
      : Connectivity(conn)
      , Offset(off)
      , Blanked(blank)
      , SourceShape(shape)
      , SideType(sideType)
    {
    }
    Source& operator=(const Source& other) = default;

    /// An array holding cell connectivity or (cell-id, side-id) tuples.
    ///
    /// If the array is cell connectivity, then each component is a point ID
    /// and the number of components matches the number of corners for each cell.
    /// If the array is side connectivity, then each tuple consists of a
    /// cell ID for component 0 and a side ID for component 1.
    vtkDataArray* Connectivity{ nullptr };

    /// Offset (start ID; used for picking) of the first cell or side in \a Connectivity.
    vtkIdType Offset{ 0 };

    /// True when the cells/sides should be omitted from processing.
    bool Blanked{ false };

    /// The shape of this type of cell/side.
    Shape SourceShape{ Shape::None };

    /// The type of the side (for calling GetSideRangeForType).
    /// The default of -1 indicates the that the source is the cell-type itself, not any side.
    int SideType{ -1 };
  };

  vtkTypeMacro(vtkDGCell, vtkCellMetadata);
  vtkInheritanceHierarchyOverrideMacro(vtkDGCell);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Provide access to the connectivity array used to define cells of this type.
  Source& GetCellSpec() { return this->CellSpec; }

  /// Provide access to the (cellId,sideId)-arrays used to define side-cells of this type.
  std::vector<Source>& GetSideSpecs() { return this->SideSpecs; }

  /// Return the number of cells (and sides) of this type present in this cell grid.
  vtkIdType GetNumberOfCells() override;

  ///@{
  /// Copy cell-specific data from \a other into ourselves.
  void ShallowCopy(vtkCellMetadata* other) override;
  void DeepCopy(vtkCellMetadata* other) override;
  ///@}

  /// Return true if the parametric coordinates (\a rst) lie inside the reference
  /// cell or its closure and false otherwise.
  ///
  /// The \a tolerance specifies a margin that should be included as part of
  /// the reference cell's interior to account for numerical imprecision.
  virtual bool IsInside(const vtkVector3d& rst, double tolerance = 1e-6) = 0;

  static int GetShapeCornerCount(Shape shape);
  static int GetShapeDimension(Shape shape);
  static vtkStringToken GetShapeName(Shape shape);
  /// Given a string description of a cell shape, return the DG equivalent enum.
  ///
  /// Note that this also converts IOSS shape names to DG enums, so there are
  /// additional cases to handle spheres as points, springs as lines, etc.
  static Shape GetShapeEnum(vtkStringToken shapeName);

  /// Return the topological shape of this cell or side type.
  virtual Shape GetShape() const = 0;

  /// Return the parametric dimension of this cell type (0, 1, 2, or 3).
  virtual int GetDimension() const { return vtkDGCell::GetShapeDimension(this->GetShape()); }

  /// Return the number of corner points for this cell type.
  virtual int GetNumberOfCorners() const
  {
    return vtkDGCell::GetShapeCornerCount(this->GetShape());
  }

  /// Return the coordinates of the reference element's \a corner vertex.
  ///
  /// Any well-formed element in world coordinates can be mapped to its reference element
  /// by transforming its corner points to these; this mapping is frequently used to
  /// visualize and analyze elements.
  virtual const std::array<double, 3>& GetCornerParameter(int corner) const = 0;

  /// Return the number of different side shapes of this cell type.
  ///
  /// Example: a wedge has 4 side shapes: Quadrilateral, Triangle, Edge, and Vertex.
  virtual int GetNumberOfSideTypes() const = 0;

  /// Return the range of sides of the \a ii-th type,
  /// where \a ii is in [-2, this->GetNumberOfSideTypes()[.
  ///
  /// The returned pair of integers is a half-open interval of side IDs.
  /// The difference between the returned values is the number of sides of \a sideType.
  ///
  /// Values of \a ii 0 and above are for "strict" sides (i.e., sides whose
  /// dimension is less than the cell's dimension).
  /// If you pass \a ii = -2, this will return the total number of strict sides of all types.
  /// If you pass \a ii = -1, this will return an entry for the cell's "self" type [-1,0[.
  ///
  /// Example: a tetrahedron will return the following:
  /// + for \a sideType -2 (all strict sides): [0, 14[
  /// + for \a sideType -1 (self): [-1, 0[
  /// + for \a sideType 0 (triangles): [0, 4[
  /// + for \a sideType 1 (edges): [4, 10[
  /// + for \a sideType 2 (verts): [10, 14[
  ///
  /// Side types are ordered from highest dimension to lowest as \a ii increases.
  /// Note that it is possible to have multiple types of side that have the same
  /// dimension. For example, wedges and pyramids each have triangle and quadrilateral
  /// sides (of dimension 2, but of different types).
  virtual std::pair<int, int> GetSideRangeForType(int sideType) const = 0;

  /// A python-wrapped version of GetSideRangeForType.
  int* GetSideRangeForSideType(int sideType) VTK_SIZEHINT(2);

  /// Return the number of boundaries this type of cell has of a given \a dimension.
  ///
  /// DG cells can be thought of as CW-complex cells; they are tuples of corner points
  /// which represent an open point set plus its closure decomposed into a union of
  /// open sets of lower dimension.
  /// For example, a hexahedron is an 8-tuple of corner points representing an
  /// underlying space shaped as an open, rectangular prism plus six 4-tuples
  /// representing its quadrilateral faces, plus twelve 2-tuples representing its
  /// edges, plus 8 1-tuples representing its corners.
  /// Thus, a hexahedron has 6 + 12 + 8 = 26 sides (plus its interior).
  virtual int GetNumberOfSidesOfDimension(int dimension) const = 0;

  /// For a given \a side, return its cell shape.
  ///
  /// The sides of a vtkDGCell are always ordered from highest dimension to lowest.
  /// For example, a hexahedron's quadrilateral sides are numbered 0–5, its line
  /// sides are numbered 6–17, and its corner point sides are numbered 18–25.
  /// Sometimes, the interior of the element is considered a side labeled -1.
  virtual Shape GetSideShape(int side) const = 0;

  /// Return the side type index for the given shape (or -1).
  ///
  /// This method simply loops over the cell's side types until it finds one
  /// of the proper shape.
  virtual int GetSideTypeForShape(Shape s) const;

  /// Return the connectivity of the given \a side.
  ///
  /// The side connectivity is a vector holding the indexes of corner-points of the
  /// side into the connectivity vector of this cell.
  ///
  /// Passing a \a side of -1 should return the connectivity of the cell's interior
  /// (a vector of the counting numbers from 0 to this->GetNumberOfCorners()).
  /// This feature is used when rendering cells of dimension 2 or lower.
  virtual const std::vector<vtkIdType>& GetSideConnectivity(int side) const = 0;

  /// Return a singleton array initialized with the reference-cell's corner point coordinates.
  ///
  /// When implementing a subclass, call FillReferencePoints() in your override.
  /// This should always return the same vtkTypeFloat32Array each time so that the array
  /// is only uploaded to the GPU once.
  virtual vtkTypeFloat32Array* GetReferencePoints() const = 0;

  /// Return a singleton array initialized with point-ids of each side's corners.
  /// To make use of this array, you should also call GetSideOffsetsAndShapes()
  /// and GetShapeCornerCount().
  ///
  /// When implementing a subclass, call FillSideConnectivity() in your override.
  /// This should always return the same vtkTypeInt32Array each time so that the array
  /// is only uploaded to the GPU once.
  virtual vtkTypeInt32Array* GetSideConnectivity() const = 0;

  /// Return a singleton array initialized with 2-tuples of (offset, shape) values.
  ///
  /// When implementing a subclass, call FillSideOffsetsAndShapes() in your override.
  /// This should always return the same vtkTypeInt32Array each time so that the array
  /// is only uploaded to the GPU once.
  virtual vtkTypeInt32Array* GetSideOffsetsAndShapes() const = 0;

  /// Fill the passed array with the parametric coordinates of all the element's corners.
  void FillReferencePoints(vtkTypeFloat32Array* arr) const;

  /// Fill the passed array with the connectivity (point IDs) of all the element's sides.
  void FillSideConnectivity(vtkTypeInt32Array* arr) const;

  /// Fill the passed array with tuples of (1) offsets into the side-connectivity
  /// and (2) shapes for each type of side. Note that the final tuple contains the total
  /// size of the offset array and a shape corresponding to the element itself.
  ///
  /// Each element's vertex side-connectivity (the penultimate offset) can also be used
  /// as the connectivity for the element's connectivity.
  ///
  /// Simple example: a vtkDGTri has 3 tuples:
  /// + (0, Shape::Edge),
  /// + (3, Shape::Vertex),
  /// + (6, Shape::Triangle).
  ///
  /// Complex example: a vtkDGWedge has 5 tuples:
  /// + (0, Shape::Quadrilateral),
  /// + (3, Shape::Triangle),
  /// + (5, Shape::Edge),
  /// + (14, Shape::Vertex),
  /// + (20, Shape::Wedge).
  ///
  /// Note that the wedge has multiple 2-d sides (both quadilaterals and triangles).
  void FillSideOffsetsAndShapes(vtkTypeInt32Array* arr) const;

protected:
  vtkDGCell();
  ~vtkDGCell() override;

  /// The connectivity array specifying cells.
  /// There may be only one \a Source for all the cells of one type in a vtkCellGrid.
  Source CellSpec;
  /// The connectivity array(s) specifying sides.
  /// There may be zero or more \a Source instances for sides in a vtkCellGrid.
  std::vector<Source> SideSpecs;

private:
  vtkDGCell(const vtkDGCell&) = delete;
  void operator=(const vtkDGCell&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
