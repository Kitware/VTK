/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericCellTessellator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGenericCellTessellator.h"
#include "vtkObjectFactory.h"

#include "vtkPoints.h"
#include "vtkIdList.h"
#include "vtkGenericAdaptorCell.h"
#include "vtkPointData.h"
#include "vtkDoubleArray.h"
#include "vtkMergePoints.h"
#include "vtkCellArray.h"
#include "vtkCollection.h"
#include "vtkGenericSubdivisionErrorMetric.h"
#include "vtkGenericAttribute.h"
#include "vtkGenericAttributeCollection.h"
#include "vtkGenericCellIterator.h"


#include <assert.h>

#include "vtkMath.h"

vtkCxxRevisionMacro(vtkGenericCellTessellator, "1.9");
vtkCxxSetObjectMacro(vtkGenericCellTessellator, ErrorMetrics, vtkCollection);

//-----------------------------------------------------------------------------
// Create the tessellator helper with a default of 0.25 for threshold
//
vtkGenericCellTessellator::vtkGenericCellTessellator()
{
  this->ErrorMetrics = vtkCollection::New();
}

//-----------------------------------------------------------------------------
vtkGenericCellTessellator::~vtkGenericCellTessellator()
{
  this->SetErrorMetrics( 0 );
}


//-----------------------------------------------------------------------------
void vtkGenericCellTessellator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ErrorMetrics: " 
     << this->ErrorMetrics << endl;
  
}
 
//-----------------------------------------------------------------------------
// Description:
// Does the edge need to be subdivided according to at least one error
// metric? The edge is defined by its `leftPoint' and its `rightPoint'.
// `leftPoint', `midPoint' and `rightPoint' have to be initialized before
// calling NeedEdgeSubdivision().
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
int vtkGenericCellTessellator::NeedEdgeSubdivision(double *leftPoint,
                                                   double *midPoint,
                                                   double *rightPoint,
                                                   double alpha)
{
  assert("pre: leftPoint_exists" && leftPoint!=0);
  assert("pre: midPoint_exists" && midPoint!=0);
  assert("pre: rightPoint_exists" && rightPoint!=0);
  assert("pre: clamped_alpha" && alpha>0 && alpha<1);
  
  int result=0;
  this->ErrorMetrics->InitTraversal();
  vtkGenericSubdivisionErrorMetric *e=static_cast<vtkGenericSubdivisionErrorMetric *>(this->ErrorMetrics->GetNextItemAsObject());
  
  // Once we found at least one error metric that need subdivision,
  // the subdivision has to be done and there is no need to check for other
  // error metrics.
  while(!result&&(e!=0))
    {
    result=e->NeedEdgeSubdivision(leftPoint,midPoint,rightPoint,alpha);
    e=static_cast<vtkGenericSubdivisionErrorMetric *>(this->ErrorMetrics->GetNextItemAsObject());
    }
  
  return result;
}

//-----------------------------------------------------------------------------
// Description:
// Init the error metric with the dataset. Should be called in each filter
// before any tessellation of any cell.
void vtkGenericCellTessellator::InitErrorMetrics(vtkGenericDataSet *ds)
{
  this->ErrorMetrics->InitTraversal();
  vtkGenericSubdivisionErrorMetric *e=static_cast<vtkGenericSubdivisionErrorMetric *>(this->ErrorMetrics->GetNextItemAsObject());
  
  while(e!=0)
    {
    e->SetDataSet(ds);
    e=static_cast<vtkGenericSubdivisionErrorMetric *>(this->ErrorMetrics->GetNextItemAsObject());
    }
}

//-----------------------------------------------------------------------------
// Description:
// Send the current cell to error metrics. Should be called at the beginning
// of the implementation of Tessellate(), Triangulate()
// or TessellateTriangleFace()
// \pre cell_exists: cell!=0
void vtkGenericCellTessellator::SetGenericCell(vtkGenericAdaptorCell *cell)
{
  assert("pre: cell_exists" && cell!=0);
  
  this->ErrorMetrics->InitTraversal();
  vtkGenericSubdivisionErrorMetric *e=static_cast<vtkGenericSubdivisionErrorMetric *>(this->ErrorMetrics->GetNextItemAsObject());
  
  while(e!=0)
    {
    e->SetGenericCell(cell);
    e=static_cast<vtkGenericSubdivisionErrorMetric *>(this->ErrorMetrics->GetNextItemAsObject());
    }
}
