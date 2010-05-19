/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericSubdivisionErrorMetric.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGenericSubdivisionErrorMetric - Objects that compute
// error during cell tessellation.
//
// .SECTION Description
// Objects of that class answer the following question during the cell
// subdivision: "does the edge need to be subdivided?" through
// RequiresEdgeSubdivision(). 
// The answer depends on the criterium actually used in the subclass of this
// abstract class: a geometric-based error
// metric (variation of edge from a straight line), an attribute-based error
// metric (variation of the active attribute/component value from a linear
// ramp) , a view-depend error metric, ...
// Cell subdivision is performed in the context of the adaptor
// framework: higher-order, or complex cells, are automatically tessellated
// into simplices so that they can be processed with conventional
// visualization algorithms.

// .SECTION See Also
// vtkGenericCellTessellator

#ifndef __vtkGenericSubdivisionErrorMetric_h
#define __vtkGenericSubdivisionErrorMetric_h

#include "vtkObject.h"

class vtkGenericAttributeCollection;
class vtkGenericAdaptorCell;
class vtkGenericDataSet;

class VTK_FILTERING_EXPORT vtkGenericSubdivisionErrorMetric : public vtkObject
{
public:  
  // Description:
  // Standard VTK type and error macros.
  vtkTypeMacro(vtkGenericSubdivisionErrorMetric,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
 
  // Description:
  // Does the edge need to be subdivided according to the implemented
  // computation?
  // The edge is defined by its `leftPoint' and its `rightPoint'.
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
  virtual int RequiresEdgeSubdivision(double *leftPoint, double *midPoint,
                                      double *rightPoint, double alpha)=0;
  
  // Description:
  // Return the error at the mid-point. The type of error depends on the state
  // of the concrete error metric. For instance, it can return an absolute
  // or relative error metric.
  // See RequiresEdgeSubdivision() for a description of the arguments.
  // \pre leftPoint_exists: leftPoint!=0
  // \pre midPoint_exists: midPoint!=0
  // \pre rightPoint_exists: rightPoint!=0
  // \pre clamped_alpha: alpha>0 && alpha<1
  // \pre valid_size: sizeof(leftPoint)=sizeof(midPoint)=sizeof(rightPoint)
  //          =GetAttributeCollection()->GetNumberOfPointCenteredComponents()+6
  // \post positive_result: result>=0
  virtual double GetError(double *leftPoint, double *midPoint,
                          double *rightPoint, double alpha)=0;
  
  // Description:
  // The cell that the edge belongs to.
  void SetGenericCell(vtkGenericAdaptorCell *cell);
  vtkGetObjectMacro(GenericCell,vtkGenericAdaptorCell);

  // Description:
  // Set/Get the dataset to be tessellated.
  void SetDataSet(vtkGenericDataSet *ds);
  vtkGetObjectMacro(DataSet,vtkGenericDataSet);

protected:
  vtkGenericSubdivisionErrorMetric();
  ~vtkGenericSubdivisionErrorMetric();

  vtkGenericAdaptorCell *GenericCell;
  vtkGenericDataSet *DataSet;
  
private:
  vtkGenericSubdivisionErrorMetric(const vtkGenericSubdivisionErrorMetric&);  // Not implemented.
  void operator=(const vtkGenericSubdivisionErrorMetric&);  // Not implemented.
};

#endif
