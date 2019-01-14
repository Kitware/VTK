/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericDataSet.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkGenericDataSet
 * @brief   defines dataset interface
 *
 * In VTK, spatial-temporal data is defined in terms of a dataset. The
 * dataset consists of geometry (e.g., points), topology (e.g., cells), and
 * attributes (e.g., scalars, vectors, etc.) vtkGenericDataSet is an abstract
 * class defining this abstraction.
 *
 * Since vtkGenericDataSet provides a general interface to manipulate data,
 * algorithms that process it tend to be slower than those specialized for a
 * particular data type. For this reason, there are concrete, non-abstract
 * subclasses that represent and provide access to data more efficiently.
 * Note that filters to process this dataset type are currently found in the
 * VTK/GenericFiltering/ subdirectory.
 *
 * Unlike the vtkDataSet class, vtkGenericDataSet provides a more flexible
 * interface including support for iterators. vtkGenericDataSet is also
 * designed to interface VTK to external simulation packages without the
 * penalty of copying memory (see VTK/GenericFiltering/README.html) for
 * more information. Thus vtkGenericDataSet plays a central role in the
 * adaptor framework.
 *
 * Please note that this class introduces the concepts of "boundary cells".
 * This refers to the boundaries of a cell (e.g., face of a tetrahedron)
 * which may in turn be represented as a cell. Boundary cells are derivative
 * topological features of cells, and are therefore never explicitly
 * represented in the dataset. Often in visualization algorithms, looping
 * over boundaries (edges or faces) is employed, while the actual dataset
 * cells may not traversed. Thus there are methods to loop over these
 * boundary cells.
 *
 * Finally, as a point of clarification, points are not the same as vertices.
 * Vertices refer to points, and points specify a position is space. Vertices
 * are a type of 0-D cell. Also, the concept of a DOFNode, which is where
 * coefficients for higher-order cells are kept, is a new concept introduced
 * by the adaptor framework (see vtkGenericAdaptorCell for more information).
 *
 * @sa
 * vtkGenericAdaptorCell vtkDataSet
*/

#ifndef vtkGenericDataSet_h
#define vtkGenericDataSet_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkDataObject.h"

class vtkCellTypes;
class vtkGenericCellIterator;
class vtkGenericAttributeCollection;
class vtkGenericCellTessellator;
class vtkGenericPointIterator;

class VTKCOMMONDATAMODEL_EXPORT vtkGenericDataSet : public vtkDataObject
{
public:
  //@{
  /**
   * Standard VTK type and print macros.
   */
  vtkTypeMacro(vtkGenericDataSet,vtkDataObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  /**
   * Return the number of points composing the dataset. See NewPointIterator()
   * for more details.
   * \post positive_result: result>=0
   */
  virtual vtkIdType GetNumberOfPoints() = 0;

  /**
   * Return the number of cells that explicitly define the dataset. See
   * NewCellIterator() for more details.
   * \pre valid_dim_range: (dim>=-1) && (dim<=3)
   * \post positive_result: result>=0
   */
  virtual vtkIdType GetNumberOfCells(int dim=-1) = 0;

  /**
   * Return -1 if the dataset is explicitly defined by cells of varying
   * dimensions or if there are no cells. If the dataset is explicitly
   * defined by cells of a unique dimension, return this dimension.
   * \post valid_range: (result>=-1) && (result<=3)
   */
  virtual int GetCellDimension() = 0;

  /**
   * Get a list of types of cells in a dataset. The list consists of an array
   * of types (not necessarily in any order), with a single entry per type.
   * For example a dataset 5 triangles, 3 lines, and 100 hexahedra would
   * result a list of three entries, corresponding to the types VTK_TRIANGLE,
   * VTK_LINE, and VTK_HEXAHEDRON.
   * THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
   * THE DATASET IS NOT MODIFIED
   * \pre types_exist: types!=0
   */
  virtual void GetCellTypes(vtkCellTypes *types);

  /**
   * Return an iterator to traverse cells of dimension `dim' (or all
   * dimensions if -1) that explicitly define the dataset. For instance, it
   * will return only tetrahedra if the mesh is defined by tetrahedra. If the
   * mesh is composed of two parts, one with tetrahedra and another part with
   * triangles, it will return both, but will not return the boundary edges
   * and vertices of these cells. The user is responsible for deleting the
   * iterator.
   * \pre valid_dim_range: (dim>=-1) && (dim<=3)
   * \post result_exists: result!=0
   */
  virtual vtkGenericCellIterator *NewCellIterator(int dim=-1) = 0;

  /**
   * Return an iterator to traverse cell boundaries of dimension `dim' (or
   * all dimensions if -1) of the dataset.  If `exteriorOnly' is true, only
   * the exterior cell boundaries of the dataset will be returned, otherwise
   * it will return exterior and interior cell boundaries. The user is
   * responsible for deleting the iterator.
   * \pre valid_dim_range: (dim>=-1) && (dim<=2)
   * \post result_exists: result!=0
   */
  virtual vtkGenericCellIterator *NewBoundaryIterator(int dim=-1,
                                                      int exteriorOnly=0) = 0;

  /**
   * Return an iterator to traverse the points composing the dataset; they
   * can be points that define a cell (corner points) or isolated points.
   * The user is responsible for deleting the iterator.
   * \post result_exists: result!=0
   */
  virtual vtkGenericPointIterator *NewPointIterator()=0;

  /**
   * Locate the closest cell to position `x' (global coordinates) with
   * respect to a tolerance squared `tol2' and an initial guess `cell' (if
   * valid). The result consists in the `cell', the `subId' of the sub-cell
   * (0 if primary cell), the parametric coordinates `pcoord' of the
   * position. It returns whether the position is inside the cell or
   * not (boolean). Tolerance is used to control how close the point is to be
   * considered "in" the cell.
   * THIS METHOD IS NOT THREAD SAFE.
   * \pre not_empty: GetNumberOfCells()>0
   * \pre cell_exists: cell!=0
   * \pre positive_tolerance: tol2>0
   */
  virtual int FindCell(double x[3],
                       vtkGenericCellIterator* &cell,
                       double tol2,
                       int &subId,
                       double pcoords[3]) = 0;

  /**
   * Locate the closest point `p' to position `x' (global coordinates).
   * \pre not_empty: GetNumberOfPoints()>0
   * \pre p_exists: p!=0
   */
  virtual void FindPoint(double x[3],
                         vtkGenericPointIterator *p)=0;

  /**
   * Datasets are composite objects and need to check each part for their
   * modified time.
   */
  vtkMTimeType GetMTime() override;

  /**
   * Compute the geometry bounding box.
   */
  virtual void ComputeBounds()=0;

  /**
   * Return a pointer to the geometry bounding box in the form
   * (xmin,xmax, ymin,ymax, zmin,zmax).
   * The return value is VOLATILE.
   * \post result_exists: result!=0
   */
  virtual double *GetBounds();

  /**
   * Return the geometry bounding box in global coordinates in
   * the form (xmin,xmax, ymin,ymax, zmin,zmax) in the `bounds' array.
   */
  virtual void GetBounds(double bounds[6]);

  /**
   * Get the center of the bounding box in global coordinates.
   * The return value is VOLATILE.
   * \post result_exists: result!=0
   */
  virtual double *GetCenter();

  /**
   * Get the center of the bounding box in global coordinates.
   */
  virtual void GetCenter(double center[3]);

  /**
   * Return the length of the diagonal of the bounding box.
   * \post positive_result: result>=0
   */
  virtual double GetLength();

  //@{
  /**
   * Get the collection of attributes associated with this dataset.
   */
  vtkGetObjectMacro(Attributes, vtkGenericAttributeCollection);
  //@}

  /**
   * Returns the attributes of the data object of the specified
   * attribute type. The type may be:
   * <ul>
   * <li>POINT  - Defined in vtkDataSet subclasses.
   * <li>CELL   - Defined in vtkDataSet subclasses.
   * <li>VERTEX - Defined in vtkGraph subclasses.
   * <li>EDGE   - Defined in vtkGraph subclasses.
   * <li>ROW    - Defined in vtkTable.
   * </ul>
   * The other attribute type, FIELD, will return nullptr since
   * field data is stored as a vtkFieldData instance, not a
   * vtkDataSetAttributes instance. To retrieve field data, use
   * GetAttributesAsFieldData.
   */
  vtkDataSetAttributes* GetAttributes(int type) override
    { return this->Superclass::GetAttributes(type); }

  //@{
  /**
   * Set/Get a cell tessellator if cells must be tessellated during
   * processing.
   * \pre tessellator_exists: tessellator!=0
   */
  virtual void SetTessellator(vtkGenericCellTessellator *tessellator);
  vtkGetObjectMacro(Tessellator,vtkGenericCellTessellator);
  //@}

  /**
   * Actual size of the data in kibibytes (1024 bytes); only valid after the pipeline has
   * updated. It is guaranteed to be greater than or equal to the memory
   * required to represent the data.
   */
  unsigned long GetActualMemorySize() override;

  /**
   * Return the type of data object.
   */
  int GetDataObjectType() override;

  /**
   * Estimated size needed after tessellation (or special operation)
   */
  virtual vtkIdType GetEstimatedSize() = 0;

  //@{
  /**
   * Retrieve an instance of this class from an information object.
   */
  static vtkGenericDataSet* GetData(vtkInformation* info);
  static vtkGenericDataSet* GetData(vtkInformationVector* v, int i=0);
  //@}

protected:
  /**
   * Constructor with uninitialized bounds (1,-1, 1,-1, 1,-1),
   * empty attribute collection and default tessellator.
   */
  vtkGenericDataSet();

  ~vtkGenericDataSet() override;

  vtkGenericAttributeCollection *Attributes;

  //Main helper class to tessellate a higher order cell into linear ones.
  vtkGenericCellTessellator *Tessellator;

  double Bounds[6];  // (xmin,xmax, ymin,ymax, zmin,zmax) geometric bounds
  double Center[3]; // Center of the geometric bounding box
  vtkTimeStamp ComputeTime; // Time at which bounds, center, etc. computed

private:
  vtkGenericDataSet(const vtkGenericDataSet&) = delete;
  void operator=(const vtkGenericDataSet&) = delete;
};

#endif
