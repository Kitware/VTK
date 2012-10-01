/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBridgeCell.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkBridgeCell - Implementation of vtkGenericAdaptorCell
// .SECTION Description
// It is just an example that show how to implement the Generic. It is also
// used for testing and evaluating the Generic.
// .SECTION See Also
// vtkGenericAdaptorCell, vtkBridgeDataSet

#ifndef __vtkBridgeCell_h
#define __vtkBridgeCell_h

#include "vtkBridgeExport.h" //for module export macro
#include "vtkGenericAdaptorCell.h"

class vtkCell;
class vtkBridgeDataSet;
class vtkBridgeCellIterator;

class VTKTESTINGGENERICBRIDGE_EXPORT vtkBridgeCell : public vtkGenericAdaptorCell
{
public:
  static vtkBridgeCell *New();
  vtkTypeMacro(vtkBridgeCell,vtkGenericAdaptorCell);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Unique identification number of the cell over the whole
  // data set. This unique key may not be contiguous.
  virtual vtkIdType GetId();

  // Description:
  // Does `this' a cell of a dataset? (otherwise, it is a boundary cell)
  virtual int IsInDataSet();

  // Description:
  // Type of the current cell.
  // \post (result==VTK_HIGHER_ORDER_EDGE)||
  //       (result==VTK_HIGHER_ORDER_TRIANGLE)||
  //       (result==VTK_HIGHER_ORDER_TETRAHEDRON)
  virtual int GetType();

  // Description:
  // Topological dimension of the current cell.
  // \post valid_result: result>=0 && result<=3
  virtual int GetDimension();

  // Description:
  // Interpolation order of the geometry.
  // \post positive_result: result>=0
  virtual int GetGeometryOrder();

  // Description:
  // Does the cell have no higher-order interpolation for geometry?
  // \post definition: result==(GetGeometryOrder()==1)
  int IsGeometryLinear();

  // Description:
  // Interpolation order of attribute `a' on the cell (may differ by cell).
  // \pre a_exists: a!=0
  // \post positive_result: result>=0
  virtual int GetAttributeOrder(vtkGenericAttribute *a);

  // Description:
  // Does the attribute `a' have no higher-order interpolation for the cell?
  // \pre a_exists: a!=0
  // \post definition: result==(GetAttributeOrder()==1)
  int IsAttributeLinear(vtkGenericAttribute *a);

  // Description:
  // Is the cell primary (i.e. not composite) ?
  virtual int IsPrimary();

  // Description:
  // Number of points that compose the cell.
  // \post positive_result: result>=0
  virtual int GetNumberOfPoints();

  // Description:
  // Return the number of boundaries of dimension `dim' (or all dimensions
  // greater than 0 and less than GetDimension() if -1) of the cell.
  // When \a dim is -1, the number of vertices is not included in the
  // count because vertices are a special case: a vertex will have
  // at most a single field value associated with it; DOF nodes may have
  // an arbitrary number of field values associated with them.
  // \pre valid_dim_range: (dim==-1) || ((dim>=0)&&(dim<GetDimension()))
  // \post positive_result: result>=0
  virtual int GetNumberOfBoundaries(int dim=-1);

  // Description:
  // Accumulated number of DOF nodes of the current cell. A DOF node is
  // a component of cell with a given topological dimension. e.g.: a triangle
  // has 4 DOF: 1 face and 3 edges. An hexahedron has 19 DOF:
  // 1 region, 6 faces, and 12 edges.
  //
  // The number of vertices is not included in the
  // count because vertices are a special case: a vertex will have
  // at most a single field value associated with it; DOF nodes may have
  // an arbitrary number of field values associated with them.
  // \post valid_result: result==GetNumberOfBoundaries(-1)+1
  virtual int GetNumberOfDOFNodes();

  // Description:
  // Return the points of cell into `it'.
  // \pre it_exists: it!=0
  virtual void GetPointIterator(vtkGenericPointIterator *it);

  // Description:
  // Create an empty cell iterator.
  // \post result_exists: result!=0
  virtual vtkGenericCellIterator *NewCellIterator();

  // Description:
  // Return in `boundaries' the cells of dimension `dim' (or all dimensions
  // less than GetDimension() if -1) that are part of the boundary of the cell.
  // \pre valid_dim_range: (dim==-1) || ((dim>=0)&&(dim<GetDimension()))
  // \pre boundaries_exist: boundaries!=0
  virtual void GetBoundaryIterator(vtkGenericCellIterator *boundaries,
                                   int dim=-1);

  // Description:
  // Number of cells (dimension>boundary->GetDimension()) of the dataset
  // that share the boundary `boundary' of `this'.
  // `this' IS NOT INCLUDED.
  // \pre boundary_exists: boundary!=0
  // \pre real_boundary: !boundary->IsInDataSet()
  // \pre cell_of_the_dataset: IsInDataSet()
  // \pre boundary: HasBoundary(boundary)
  // \post positive_result: result>=0
  virtual int CountNeighbors(vtkGenericAdaptorCell *boundary);
  void CountEdgeNeighbors( int* sharing );

  // Description:
  // Put into `neighbors' the cells (dimension>boundary->GetDimension())
  // of the dataset that share the boundary `boundary' of `this'.
  // `this' IS NOT INCLUDED.
  // \pre boundary_exists: boundary!=0
  // \pre real_boundary: !boundary->IsInDataSet()
  // \pre cell_of_the_dataset: IsInDataSet()
  // \pre boundary: HasBoundary(boundary)
  // \pre neighbors_exist: neighbors!=0
  virtual void GetNeighbors(vtkGenericAdaptorCell *boundary,
                            vtkGenericCellIterator *neighbors);

  // Description:
  // Compute the closest boundary of the current sub-cell `subId' for point
  // `pcoord' (in parametric coordinates) in `boundary', and return whether
  // the point is inside the cell or not. `boundary' is of dimension
  // GetDimension()-1.
  // \pre positive_subId: subId>=0
  virtual int FindClosestBoundary(int subId,
                                  double pcoords[3],
                                  vtkGenericCellIterator* &boundary);

  // Description:
  // Is `x' inside the current cell? It also evaluate parametric coordinates
  // `pcoords', sub-cell id `subId' (0 means primary cell), distance squared
  // to the sub-cell in `dist2' and closest corner point `closestPoint'.
  // `dist2' and `closestPoint' are not evaluated if `closestPoint'==0.
  // If a numerical error occurred, -1 is returned and all other results
  // should be ignored.
  // \post valid_result: result==-1 || result==0 || result==1
  // \post positive_distance: result!=-1 implies (closestPoint!=0 implies
  //                                               dist2>=0)
  virtual int EvaluatePosition(double x[3],
                               double *closestPoint,
                               int &subId,
                               double pcoords[3],
                               double &dist2);

// Description:
  // Determine global coordinates `x' from sub-cell `subId' and parametric
  // coordinates `pcoords' in the cell.
  // \pre positive_subId: subId>=0
  // \pre clamped_pcoords: (0<=pcoords[0])&&(pcoords[0]<=1)&&(0<=pcoords[1])
  // &&(pcoords[1]<=1)&&(0<=pcoords[2])&&(pcoords[2]<=1)
  virtual void EvaluateLocation(int subId,
                                double pcoords[3],
                                double x[3]);

  // Description:
  // Interpolate the attribute `a' at local position `pcoords' of the cell into
  // `val'.
  // \pre a_exists: a!=0
  // \pre a_is_point_centered: a->GetCentering()==vtkPointCentered
  // \pre clamped_point: pcoords[0]>=0 && pcoords[0]<=1 && pcoords[1]>=0 &&
  //                     pcoords[1]<=1 && pcoords[2]>=0 && pcoords[2]<=1
  // \pre val_exists: val!=0
  // \pre valid_size: sizeof(val)==a->GetNumberOfComponents()
  virtual void InterpolateTuple(vtkGenericAttribute *a, double pcoords[3],
                                double *val);

  // Description:
  // Interpolate the whole collection of attributes `c' at local position
  // `pcoords' of the cell into `val'. Only point centered attributes are
  // taken into account.
  // \pre c_exists: c!=0
  // \pre clamped_point: pcoords[0]>=0 && pcoords[0]<=1 && pcoords[1]>=0 &&
  //                     pcoords[1]<=1 && pcoords[2]>=0 && pcoords[2]<=1
  // \pre val_exists: val!=0
  // \pre valid_size: sizeof(val)==c->GetNumberOfPointCenteredComponents()
  virtual void InterpolateTuple(vtkGenericAttributeCollection *c, double pcoords[3],
                                double *val);
#if 0
  // Description:
  // Generate a contour (contouring primitives) for each `values' or with
  // respect to an implicit function `f'. Contouring
  // is performed on the scalar attribute (`attributes->GetActiveAttribute()',
  // `attributes->GetActiveComponent()').
  // Contouring interpolates the
  // `attributes->GetNumberOfattributesToInterpolate()' attributes
  // `attributes->GetAttributesToInterpolate()'.
  // `locator', `verts', `lines', `polys', `outPd' and `outCd' are cumulative
  // data arrays over cell iterations: they store the result of each call
  // to Contour():
  // - `locator' is points list that merges points as they are inserted (i.e.,
  // prevents duplicates).
  // - `verts' is an array of generated vertices
  // - `lines' is an array of generated lines
  // - `polys' is an array of generated polygons
  // - `outPd' is an array of interpolated point data along the edge (if
  // not-NULL)
  // - `outCd' is an array of copied cell data of the current cell (if
  // not-NULL)
  // Note: the CopyAllocate() method must be invoked on both the output cell
  // and point data.
  //
  // NOTE: `vtkGenericAttributeCollection *attributes' will be replaced by a
  //       `vtkInformation'.
  //
  // \pre values_exist: (values!=0 && f==0) || (values==0 && f!=0)
  // \pre attributes_exist: attributes!=0
  // \pre locator_exists: locator!=0
  // \pre verts_exist: verts!=0
  // \pre lines_exist: lines!=0
  // \pre polys_exist: polys!=0
  virtual void Contour(vtkContourValues *values,
                       vtkImplicitFunction *f,
                       vtkGenericAttributeCollection *attributes,
                       vtkPointLocator *locator,
                       vtkCellArray *verts,
                       vtkCellArray *lines,
                       vtkCellArray *polys,
                       vtkPointData *outPd,
                       vtkCellData *outCd);
#endif
#if 0
  // Description:
  // Cut (or clip) the current cell with respect to the contour defined by the
  // `value' or the implicit function `f' of the scalar attribute
  // (`attributes->GetActiveAttribute()',`attributes->GetActiveComponent()').
  // If `f' exists, `value' is not used. The output is the part
  // of the current cell which is inside the contour.
  // The output is a set of zero, one or more cells of the same topological
  // dimension as the current cell. Normally, cell points whose scalar value
  // is greater than "value" are considered inside. If `insideOut' is on, this
  // is reversed.
  //  Clipping interpolates the
  // `attributes->GetNumberOfattributesToInterpolate()' attributes
  // `attributes->GetAttributesToInterpolate()'.
  // `locator', `connectivity', `outPd' and `outCd' are cumulative
  // data arrays over cell iterations: they store the result of each call
  // to Clip():
  // - `locator' is points list that merges points as they are inserted (i.e.,
  // prevents duplicates).
  // - `connectivity' is an array of generated cells
  // - `outPd' is an array of interpolated point data along the edge (if
  // not-NULL)
  // - `outCd' is an array of copied cell data of the current cell (if
  // not-NULL)
  // Note: the CopyAllocate() method must be invoked on both the output cell
  // and point data.
  // Also, if the output cell data is
  // non-NULL, the cell data from the clipped cell is passed to the generated
  // contouring primitives. (Note: the CopyAllocate() method must be invoked on
  // both the output cell and point data.)
  //
  // NOTE: `vtkGenericAttributeCollection *attributes' will be replaced by a
  //       `vtkInformation'.
  //
  // \pre attributes_exist: attributes!=0
  // \pre tess_exists: tess!=0
  // \pre locator_exists: locator!=0
  // \pre connectivity_exists: connectivity!=0
  virtual void Clip(double value,
                    vtkImplicitFunction *f,
                    vtkGenericAttributeCollection *attributes,
                    vtkGenericCellTessellator *tess,
                    int insideOut,
                    vtkPointLocator *locator,
                    vtkCellArray *connectivity,
                    vtkPointData *outPd,
                    vtkCellData *outCd);
#endif
  // Description:
  // Is there an intersection between the current cell and the ray (`p1',`p2')
  // according to a tolerance `tol'? If true, `x' is the global intersection,
  // `t' is the parametric coordinate for the line, `pcoords' are the
  // parametric coordinates for cell. `subId' is the sub-cell where
  // the intersection occurs.
  // \pre positive_tolerance: tol>0
  virtual int IntersectWithLine(double p1[3],
                                double p2[3],
                                double tol,
                                double &t,
                                double x[3],
                                double pcoords[3],
                                int &subId);

  // Description:
  // Compute derivatives `derivs' of the attribute `attribute' (from its
  // values at the corner points of the cell) given sub-cell `subId' (0 means
  // primary cell) and parametric coordinates `pcoords'.
  // Derivatives are in the x-y-z coordinate directions for each data value.
  // \pre positive_subId: subId>=0
  // \pre clamped_pcoords: (0<=pcoords[0])&&(pcoords[0]<=1)&&(0<=pcoords[1])
  // &&(pcoords[1]<=1)&&(0<=pcoords[2])%%(pcoords[2]<=1)
  // \pre attribute_exists: attribute!=0
  // \pre derivs_exists: derivs!=0
  // \pre valid_size: sizeof(derivs)>=attribute->GetNumberOfComponents()*3
  virtual void Derivatives(int subId,
                           double pcoords[3],
                           vtkGenericAttribute *attribute,
                           double *derivs);

  // Description:
  // Compute the bounding box of the current cell in `bounds' in global
  // coordinates.
  // THREAD SAFE
  virtual void GetBounds(double bounds[6]);

  // Description:
  // Return the bounding box of the current cell in global coordinates.
  // NOT THREAD SAFE
  // \post result_exists: result!=0
  // \post valid_size: sizeof(result)>=6
  virtual double *GetBounds();

  // Description:
  // Bounding box diagonal squared of the current cell.
  // \post positive_result: result>=0
  virtual double GetLength2();

  // Description:
  // Center of the current cell in parametric coordinates `pcoords'.
  // If the current cell is a composite, the return value is the sub-cell id
  // that the center is in.
  // \post valid_result: (result>=0) && (IsPrimary() implies result==0)
  virtual int GetParametricCenter(double pcoords[3]);

  // Description:
  // Distance of the parametric coordinate `pcoords' to the current cell.
  // If inside the cell, a distance of zero is returned. This is used during
  // picking to get the correct cell picked. (The tolerance will occasionally
  // allow cells to be picked who are not really intersected "inside" the
  // cell.)
  // \post positive_result: result>=0
  virtual double GetParametricDistance(double pcoords[3]);

  // Description:
  // Return a contiguous array of parametric coordinates of the points defining
  // the current cell. In other words, (px,py,pz, px,py,pz, etc..) The
  // coordinates are ordered consistent with the definition of the point
  // ordering for the cell. Note that 3D parametric coordinates are returned
  // no matter what the topological dimension of the cell. It includes the DOF
  // nodes.
  // \post valid_result_exists: ((IsPrimary()) && (result!=0)) ||
  //                             ((!IsPrimary()) && (result==0))
  //                     result!=0 implies sizeof(result)==GetNumberOfPoints()
  virtual double *GetParametricCoords();
#if 0
  // Description:
  // Tessellate the cell if it is not linear or if at least one attribute of
  // `attributes' is not linear. The output are linear cells of the same
  // dimension than than cell. If the cell is linear and all attributes are
  // linear, the output is just a copy of the current cell.
  // `points', `cellArray', `pd' and `cd' are cumulative output data arrays
  // over cell iterations: they store the result of each call to Tessellate().
  // \pre attributes_exist: attributes!=0
  // \pre points_exist: points!=0
  // \pre cellArray_exists: cellArray!=0
  // \pre pd_exist: pd!=0
  // \pre cd_exists: cd!=0
  virtual void Tessellate(vtkGenericAttributeCollection *attributes,
                          vtkPoints *points, vtkCellArray* cellArray,
                          vtkPointData *pd, vtkCellData* cd);
#endif
  // For the internals of the tesselation algorithm (the hash table in particular)
  virtual int IsFaceOnBoundary(vtkIdType faceId);
  virtual int IsOnBoundary();

  // Description:
  // Put into `id' the list of ids the point of the cell.
  // \pre id_exists: id!=0
  // \pre valid_size: sizeof(id)==GetNumberOfPoints();
  virtual void GetPointIds(vtkIdType *id);
#if 0
  virtual void TriangulateFace(vtkGenericAttributeCollection *attributes,
                               vtkGenericCellTessellator *tess,
                               int index,
                               vtkPoints *pts, vtkCellArray *cellArray,
                               vtkPointData *pd,
                               vtkCellData *cd );
#endif

  // Description:
  // Return the ids of the vertices defining face `faceId'.
  // \pre is_3d: this->GetDimension()==3
  // \pre valid_faceId_range: faceId>=0 && faceId<this->GetNumberOfBoundaries(2)
  // \post result_exists: result!=0
  // \post valid_size: sizeof(result)>=GetNumberOfVerticesOnFace(faceId)
  int *GetFaceArray(int faceId);

  // Description:
  // Return the number of vertices defining face `faceId'
  // \pre is_3d: this->GetDimension()==3
  // \pre valid_faceId_range: faceId>=0 && faceId<this->GetNumberOfBoundaries(2)
  // \post positive_result: && result>0
  int GetNumberOfVerticesOnFace(int faceId);

  // Description:
  // Return the ids of the vertices defining edge `edgeId'.
  // \pre valid_dimension: this->GetDimension()>=2
  // \pre valid_edgeId_range: edgeId>=0 && edgeId<this->GetNumberOfBoundaries(1)
  // \post result_exists: result!=0
  // \post valid_size: sizeof(result)==2
  int *GetEdgeArray(int edgeId);

  // Description:
  // Used internally for the Bridge.
  // Initialize the cell from a dataset `ds' and `cellid'.
  // \pre ds_exists: ds!=0
  // \pre valid_cellid: (cellid>=0) && (cellid<ds->GetNumberOfCells())
  void Init(vtkBridgeDataSet *ds,
            vtkIdType cellid);

  // Description:
  // Used internally for the Bridge.
  // Initialize the cell from a cell `c' and an `id'.
  // \pre c_exists: c!=0
  void InitWithCell(vtkCell *c,
                    vtkIdType id);

  // Description:
  // Recursive copy of `other' into `this'.
  // \pre other_exists: other!=0
  // \pre other_differ: this!=other
  void DeepCopy(vtkBridgeCell *other);

protected:
  vtkBridgeCell();
  virtual ~vtkBridgeCell();

  // Description:
  // Allocate an array for the weights, only if it does not exist yet or if
  // the capacity is too small.
  void AllocateWeights();

  // Description:
  // Compute the weights for parametric coordinates `pcoords'.
  void InterpolationFunctions(double pcoords[3], double *weights);

  friend class vtkBridgeDataSet;
  friend class vtkBridgeAttribute;
  friend class vtkBridgeCellIterator;
  friend class vtkBridgeCellIteratorOnDataSet;
  friend class vtkBridgeCellIteratorOne;
  friend class vtkBridgeCellIteratorOnCellBoundaries;
  friend class vtkBridgePointIteratorOnCell;

  vtkCell *Cell;
  vtkBridgeDataSet *DataSet;
  vtkIdType Id; // what does it mean for boundary cells?
  int BoolIsInDataSet;
  vtkBridgeCellIterator *InternalIterator; // used in Contour

  double *Weights; // interpolation functions
  int WeightsCapacity;

private:
  vtkBridgeCell(const vtkBridgeCell&);  // Not implemented.
  void operator=(const vtkBridgeCell&);  // Not implemented.
};

#endif
