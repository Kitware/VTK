/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericCellTessellator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGenericCellTessellator - helper class to perform cell tessellation
// .SECTION Description
// vtkGenericCellTessellator is a helper class to perform adaptive tessellation
// of particular cell topologies. The major purpose for this class is to
// transform higher-order cell types (e.g., higher-order finite elements)
// into linear cells that can then be easily visualized by VTK. This class
// works in conjunction with the vtkGenericDataSet and vtkGenericAdaptorCell
// classes.
//
// This algorithm is based on edge subdivision. An error metric along each
// edge is evaluated, and if the error is greater than some tolerance, the
// edge is subdivided (as well as all connected 2D and 3D cells). The process
// repeats until the error metric is satisfied. 
//
// A significant issue addressed by this algorithm is to insure face
// compatibility across neigboring cells. That is, diagonals due to face
// triangulation must match to insure that the mesh is compatible. The
// algorithm employs a precomputed table to accelerate the tessellation
// process. The table was generated with the help of vtkOrderedTriangulator;
// the basic idea is that the choice of diagonal is made by considering the
// relative value of the point ids.


#ifndef __vtkGenericCellTessellator_h
#define __vtkGenericCellTessellator_h

#include "vtkObject.h"

class vtkCellArray;
class vtkDoubleArray;
class vtkCollection;
class vtkGenericAttributeCollection;
class vtkGenericAdaptorCell;
class vtkGenericCellIterator;
class vtkPointData;
class vtkGenericDataSet;

//-----------------------------------------------------------------------------
//
// The tessellation object
class VTK_FILTERING_EXPORT vtkGenericCellTessellator : public vtkObject
{
public:
  vtkTypeMacro(vtkGenericCellTessellator,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Tessellate a face of a 3D `cell'. The face is specified by the
  // index value.
  // The result is a set of smaller linear triangles in `cellArray' with
  // `points' and point data `internalPd'.
  // \pre cell_exists: cell!=0
  // \pre valid_dimension: cell->GetDimension()==3
  // \pre valid_index_range: (index>=0) && (index<cell->GetNumberOfBoundaries(2))
  // \pre att_exists: att!=0
  // \pre points_exists: points!=0
  // \pre cellArray_exists: cellArray!=0
  // \pre internalPd_exists: internalPd!=0
  virtual void TessellateFace(vtkGenericAdaptorCell *cell,
                              vtkGenericAttributeCollection *att,
                              vtkIdType index,
                              vtkDoubleArray *points,
                              vtkCellArray *cellArray,
                              vtkPointData *internalPd)=0;

  // Description:
  // Tessellate a 3D `cell'. The result is a set of smaller linear
  // tetrahedra in `cellArray' with `points' and point data `internalPd'.
  // \pre cell_exists: cell!=0
  // \pre valid_dimension: cell->GetDimension()==3
  // \pre att_exists: att!=0
  // \pre points_exists: points!=0
  // \pre cellArray_exists: cellArray!=0
  // \pre internalPd_exists: internalPd!=0
  virtual void Tessellate(vtkGenericAdaptorCell *cell,
                          vtkGenericAttributeCollection *att,
                          vtkDoubleArray *points, 
                          vtkCellArray *cellArray,
                          vtkPointData *internalPd )=0;

  // Description:
  // Triangulate a 2D `cell'. The result is a set of smaller linear triangles
  // in `cellArray' with `points' and point data `internalPd'.
  // \pre cell_exists: cell!=0
  // \pre valid_dimension: cell->GetDimension()==2
  // \pre att_exists: att!=0
  // \pre points_exists: points!=0
  // \pre cellArray_exists: cellArray!=0
  // \pre internalPd_exists: internalPd!=0
  virtual void Triangulate(vtkGenericAdaptorCell *cell,
                           vtkGenericAttributeCollection *att,
                           vtkDoubleArray *points,
                           vtkCellArray *cellArray,
                           vtkPointData *internalPd)=0;

  // Description:
  // Specify the list of error metrics used to decide if an edge has to be
  // splitted or not. It is a collection of vtkGenericSubdivisionErrorMetric-s.
  virtual void SetErrorMetrics(vtkCollection *someErrorMetrics);
  vtkGetObjectMacro(ErrorMetrics,vtkCollection);
  
  // Description:
  // Initialize the tessellator with a data set `ds'.
  virtual void Initialize(vtkGenericDataSet *ds)=0;
  
  // Description:
  // Init the error metric with the dataset. Should be called in each filter
  // before any tessellation of any cell.
  void InitErrorMetrics(vtkGenericDataSet *ds);
  
  // Description:
  // If true, measure the quality of the fixed subdivision.
  vtkGetMacro(Measurement,int);
  vtkSetMacro(Measurement,int);
  
  // Description:
  // Get the maximum error measured after the fixed subdivision.
  // \pre errors_exists: errors!=0
  // \pre valid_size: sizeof(errors)==GetErrorMetrics()->GetNumberOfItems()
  void GetMaxErrors(double *errors);
  
protected:
  vtkGenericCellTessellator();
  ~vtkGenericCellTessellator();
  
  // Description:
  // Does the edge need to be subdivided according to at least one error
  // metric? The edge is defined by its `leftPoint' and its `rightPoint'.
  // `leftPoint', `midPoint' and `rightPoint' have to be initialized before
  // calling RequiresEdgeSubdivision().
  // Their format is global coordinates, parametric coordinates and
  // point centered attributes: xyx rst abc de...
  // `alpha' is the normalized abscissa of the midpoint along the edge.
  // (close to 0 means close to the left point, close to 1 means close to the
  // right point)
  // \pre leftPoint_exists: leftPoint!=0
  // \pre midPoint_exists: midPoint!=0
  // \pre rightPoint_exists: rightPoint!=0
  // \pre clamped_alpha: alpha>0 && alpha<1
  // \pre valid_size: sizeof(leftPoint)=sizeof(midPoint)=sizeof(rightPoint)
  //          =GetAttributeCollection()->GetNumberOfPointCenteredComponents()+6
  int RequiresEdgeSubdivision(double *left, double *mid, double *right,
                              double alpha);
  
  
  // Description:
  // Update the max error of each error metric according to the error at the
  // mid-point. The type of error depends on the state
  // of the concrete error metric. For instance, it can return an absolute
  // or relative error metric.
  // See RequiresEdgeSubdivision() for a description of the arguments.
  // \pre leftPoint_exists: leftPoint!=0
  // \pre midPoint_exists: midPoint!=0
  // \pre rightPoint_exists: rightPoint!=0
  // \pre clamped_alpha: alpha>0 && alpha<1
  // \pre valid_size: sizeof(leftPoint)=sizeof(midPoint)=sizeof(rightPoint)
  //          =GetAttributeCollection()->GetNumberOfPointCenteredComponents()+6
  virtual void UpdateMaxError(double *leftPoint, double *midPoint,
                              double *rightPoint, double alpha);
  
  // Description:
  // Reset the maximal error of each error metric. The purpose of the maximal
  // error is to measure the quality of a fixed subdivision.
  void ResetMaxErrors();
  
  // Description:
  // List of error metrics. Collection of vtkGenericSubdivisionErrorMetric
  vtkCollection *ErrorMetrics;
  
  // Description:
  // Send the current cell to error metrics. Should be called at the beginning
  // of the implementation of Tessellate(), Triangulate()
  // or TessellateFace()
  // \pre cell_exists: cell!=0
  void SetGenericCell(vtkGenericAdaptorCell *cell);
  
  vtkGenericDataSet *DataSet;
  
  int Measurement; // if true, measure the quality of the fixed subdivision.
  double *MaxErrors; // max error for each error metric, for measuring the
  // quality of a fixed subdivision.
  int MaxErrorsCapacity;
  
private:
  vtkGenericCellTessellator(const vtkGenericCellTessellator&);  // Not implemented.
  void operator=(const vtkGenericCellTessellator&);  // Not implemented.
};

#endif
