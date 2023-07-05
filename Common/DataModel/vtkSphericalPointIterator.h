// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkSphericalPointIterator
 * @brief   Traverse a collection of points in spherical ordering.
 *
 *
 * vtkSphericalPointIterator is a state-based iterator for traversing a set
 * of points (i.e., a neighborhood of points) in a dataset, providing a point
 * traversal order across user-defined "axes" which span a 2D or 3D space
 * (typically a circle or sphere). The points along each axes may be sorted
 * in increasing radial order. To define the points, specify a dataset (i.e.,
 * its associated points, whether the points are represented implicitly or
 * explicitly) and an associated neighborhood over which to iterate. Methods
 * for iterating over the points are provided.
 *
 * For example, consider the axes of iteration to be the four rays emanating
 * from the center of a square and passing through the center of each of the
 * four edges of the square. Points to be iterated over are associated (using
 * a dot product) with each of the four axes, and then can be sorted along
 * each axis. Then the order of iteration is then: (axis0,pt0), (axis1,pt0),
 * (axis2,pt0), (axis3,pt0), (axis0,pt1), (axis1,pt1), (axis2,pt1),
 * (axis3,pt1), (axis0,pt2), (axis1,pt2), (axis2,pt2), (axis3,pt2), and so on
 * in a "spiraling" fashion until all points are visited. Thus the order of
 * visitation is: iteration i visits all N axes in order, returning the jth
 * point sorted along each of the N axes (i.e., i increases the fastest).
 * Alternatively, methods exist to randomly access points, or points
 * associated with an axes, so that custom iteration methods can be defined.
 *
 * The iterator can be defined with any number of axes (defined by 3D
 * vectors). The axes must not be coincident, and typically are equally
 * spaced from one another. The order which the axes are defined determines
 * the order in which the axes (and hence the points) are traversed. So for
 * example, in a 2D sphere, four axes in the (-x,+x,-y,+y) directions would
 * provide a "ping pong" iteration, while four axes ordered in the
 * (+x,+y,-x,-y) directions would provide a counterclockwise rotation
 * iteration.
 *
 * The iterator provides thread-safe iteration of dataset points. It supports
 * both random and forward iteration.
 *
 * @warning
 * The behavior of the iterator depends on the ordering of the iteration
 * axes. It is possible to obtain a wide variety of iteration patterns
 * depending on these axes. For example, if only one axis is defined, then a
 * "linear" pattern is possible (i.e., visiting points in the half space
 * defined by the vector); if two axes, then a "diagonal" iteration pattern;
 * and so on. Note that points are sorted along the iteration axes depending
 * on the their projection onto them (e.g., using the dot product). Because
 * only points with positive projection are associated with an axis, it is
 * possible that some points in the neighborhood will not be processed (i.e.,
 * if a point in the neighborhood does not positively project onto any of the
 * axes, then it will not be iterated over). Thus if all points are to be
 * iterated over, then the axes must form a basis which covers all points
 * using positive projections.
 *
 * @sa
 * vtkVoronoi2D vtkVoronoi3D vtkStaticPointLocator vtkPointLocator
 */

#ifndef vtkSphericalPointIterator_h
#define vtkSphericalPointIterator_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkDataSet.h"               // the dataset and its points to iterate over
#include "vtkDoubleArray.h"           // For axes
#include "vtkObject.h"
#include "vtkSmartPointer.h" // auto destruct

#include <memory> // for std::unique_ptr

VTK_ABI_NAMESPACE_BEGIN
class vtkDoubleArray;
class vtkPolyData;
struct SpiralPointIterator;

class VTKCOMMONDATAMODEL_EXPORT vtkSphericalPointIterator : public vtkObject
{
public:
  ///@{
  /**
   * Standard methods to instantiate, obtain type information, and
   * print information about an instance of the class.
   */
  static vtkSphericalPointIterator* New();
  vtkAbstractTypeMacro(vtkSphericalPointIterator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Define the dataset and its associated points over which to iterate.
   */
  vtkSetSmartPointerMacro(DataSet, vtkDataSet);
  vtkGetSmartPointerMacro(DataSet, vtkDataSet);
  ///@}

  ///@{
  /**
   * Define the axes for the point iterator. This only needs to be defined
   * once (typically immediately after instantiation). The axes data array
   * must be a 3-component array, where each 3-tuple defines a vector
   * defining a axis. The number of axes is limited to 100,000 or less
   * (typically the numbers of axes are <=20). The order in which the axes
   * are defined determines the order in which the axes are
   * traversed. Depending on the order, it's possible to create a variety of
   * traversal patterns, ranging from clockwise/counterclockwise to spiral to
   * ping pong (e.g., -x,+x, -y,+y, -z,+z). Note: the defining axes need not
   * be normalized, they are normalized and copied into internal iterator
   * storage in the Initialize() method.
   */
  vtkSetSmartPointerMacro(Axes, vtkDoubleArray);
  vtkGetSmartPointerMacro(Axes, vtkDoubleArray);
  ///@}

  /**
   * While the axes can be arbitrarily specified, it is possible to select
   * axes from a menu of predefined axes sets. For example, XY_CW_AXES refer
   * to axes that rotate in clockwise direction starting with the first axis
   * parallel to the x-axis; with the total number of axes given by the
   * resolution.  Some 3D regular polyhedral solids are also referred to: the
   * axes pass through the center of the faces of the solid. So DODECAHEDRON
   * axes refer to the 12 axes that pass through the center of the 12 faces
   * of the dodecahedron. In some cases the resolution parameter need not be
   * specified.
   */
  enum AxesType
  {
    XY_CW_AXES = 0,      // axes clockwise around center in x-y plane (resolution required)
    XY_CCW_AXES = 1,     // axes counterclockwise around center (resolution required)
    XY_SQUARE_AXES = 2,  // axes +x,-x, +y,-y: axes through the four faces of a square
    CUBE_AXES = 3,       // axes +x,-x, +y,-y, +z,-z: axes through the six faces of a cube
    OCTAHEDRON_AXES = 4, // axes through the eight faces of a regular octahedron
    CUBE_OCTAHEDRON_AXES =
      5, // axes through the eight faces of a regular octahedron and six faces of a cube
    DODECAHEDRON_AXES = 6, // axes through the twelve faces of a dedecahdron
    ICOSAHEDRON_AXES = 7,  // axes through the twenty faces of a icosahedron
  };

  /**
   * A convenience method to set the iterator axes from the predefined set
   * enumerated above.  The resolution parameter is optional in some cases -
   * it is used by axes types that are non-fixed such as rotation of a vector
   * around a center point in the plane (e.g., x-y plane).
   */
  void SetAxes(int axesType, int resolution = 6);

  /**
   * Points can be sorted along each axis. By default, no sorting is
   * performed.  Other options are ascending and descending radial
   * order. Ascended sorting results in point traversal starting near the
   * center of the iterator, and proceeding radially outward. Descended
   * sorting results in point traversal starting away from the center of the
   * iterator, and proceeding radially inward.
   */
  enum SortType
  {
    SORT_NONE = 0,
    SORT_ASCENDING = 1,
    SORT_DESCENDING = 2
  };

  ///@{
  /**
   * Specify whether points along each axis are radially sorted, and if so,
   * whether in an ascending or descending direction. (Note that some
   * operators such as the locator query FindClosestNPoints() return radially
   * sorted neighborhoods in ascending direction and often do not need
   * sorting - this can save significant time.)
   */
  vtkSetClampMacro(Sorting, int, SORT_NONE, SORT_DESCENDING);
  vtkGetMacro(Sorting, int);
  void SetSortTypeToNone() { this->SetSorting(SORT_NONE); }
  void SetSortTypeToAscending() { this->SetSorting(SORT_ASCENDING); }
  void SetSortTypeToDescending() { this->SetSorting(SORT_DESCENDING); }
  ///@}

  // The following methods support point iteration. The data members referred
  // to previously must be defined before these iteration methods can be
  // successfully invoked.

  ///@{
  /**
   * Initialize the iteration process around a position [x], over a set of
   * points (the neighborhood) defined by a list of numNei point ids. (The
   * point ids refer to the points contained in the dataset.) If
   * initialization fails (because the Axes or the DataSet have not been
   * defined) then false is returned; true otherwise. One of the Initialize()
   * variants enables iteration over all points in the dataset.
   */
  bool Initialize(double center[3], vtkIdList* neighborhood);
  bool Initialize(double center[3], vtkIdType numNei, vtkIdType* neighborhood);
  bool Initialize(double center[3]); // all points of the specified dataset
  ///@}

  /**
   * Begin iterating over the neighborhood of points. It is possible that
   * not all points are iterated over - those points not projecting onto
   * any axis with a positive dot product are not visited.
   */
  void GoToFirstPoint();

  /**
   * Return true if set traversal is completed. Otherwise false.
   */
  bool IsDoneWithTraversal();

  /**
   * Go to the next point in the neighborhood. This is only valid when
   * IsDoneWithTraversal() returns false;
   */
  void GoToNextPoint();

  /**
   * Get the current point (point id and coordinates) during
   * forward iteration.
   */
  void GetCurrentPoint(vtkIdType& ptId, double x[3]);

  /**
   * Return the current point id during forward iteration.
   */
  vtkIdType GetCurrentPoint();

  /**
   * Provide random access to the jth point of the ith axis. Returns the point id
   * located at (axis,ptIdx); or a value <0 if the requested point does not exist.
   */
  vtkIdType GetPoint(int axis, int ptIdx);

  /**
   * Return the number of axes defined. The value returned is valid only
   * after Initialize() is invoked.
   */
  vtkIdType GetNumberOfAxes();

  /**
   * Return the list of points along the specified ith axis.
   */
  void GetAxisPoints(int axis, vtkIdType& npts, const vtkIdType*& pts) VTK_SIZEHINT(pts, npts);

  /**
   * A convenience method that produces a geometric representation of the
   * iterator (e.g., axes + center). The representation simply draws lines
   * for each of the axes emanating from the center point. Each line (or line
   * cell) is assigned cell data which is the axis number. This is typically
   * used for debugging or educational purposes. Note that the method
   * is valid only after Initialize() has been invoked.
   */
  void BuildRepresentation(vtkPolyData* pd);

protected:
  vtkSphericalPointIterator();
  ~vtkSphericalPointIterator() override = default;

  // Information needed to define the spherical iterator.
  vtkSmartPointer<vtkDataSet> DataSet;  // The points to iterate over
  vtkSmartPointer<vtkDoubleArray> Axes; // The axes defining the iteration pattern
  int Sorting;                          // The direction of sorting, if sorting required

  // Iterator internals are represented using a PIMPL idiom
  struct SphericalPointIterator;
  std::unique_ptr<SphericalPointIterator> Iterator;

  // Changes to the VTK class must be propagated to the internal iterator
  vtkTimeStamp BuildTime;

private:
  vtkSphericalPointIterator(const vtkSphericalPointIterator&) = delete;
  void operator=(const vtkSphericalPointIterator&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkSphericalPointIterator_h
