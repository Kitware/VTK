/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericAdaptorCell.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGenericAdaptorCell.h"

#include <assert.h>

#include "vtkPoints.h"
#include "vtkTetra.h"
#include "vtkTriangle.h"
#include "vtkLine.h"
#include "vtkVertex.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkCellArray.h"
#include "vtkContourValues.h"
#include "vtkImplicitFunction.h"
#include "vtkGenericAttributeCollection.h"
#include "vtkGenericAttribute.h"
#include "vtkGenericCellTessellator.h"

vtkCxxRevisionMacro(vtkGenericAdaptorCell, "1.7");

vtkGenericAdaptorCell::vtkGenericAdaptorCell()
{
  this->Tetra = vtkTetra::New();
  this->Triangle = vtkTriangle::New();
  this->Line = vtkLine::New();
  this->Vertex = vtkVertex::New();

  this->Scalars = vtkDoubleArray::New();
  this->Scalars->SetNumberOfTuples(4);
  this->PointData = vtkPointData::New();
  this->CellData = vtkCellData::New();
  

  // Internal array to avoid New/ Delete
  this->InternalPoints = vtkDoubleArray::New();
  this->InternalPoints->SetNumberOfComponents(3);
  this->InternalScalars = vtkDoubleArray::New();
  this->InternalCellArray = vtkCellArray::New();

  this->PointDataScalars = vtkDoubleArray::New();
  this->PointData->SetScalars( this->PointDataScalars );
  this->PointDataScalars->Delete();
}

//----------------------------------------------------------------------------
vtkGenericAdaptorCell::~vtkGenericAdaptorCell()
{
  this->Tetra->Delete();
  this->Triangle->Delete();
  this->Line->Delete();
  this->Vertex->Delete();

  this->Scalars->Delete();
  this->PointData->Delete();
  this->CellData->Delete();
  
  this->InternalPoints->Delete();
  this->InternalScalars->Delete();
  this->InternalCellArray->Delete();
  
}

//----------------------------------------------------------------------------

void vtkGenericAdaptorCell::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
//----------------------------------------------------------------------------
// Description:
// Does the cell have no higher-order interpolation for geometry?
// \post definition: result==(GetGeometryOrder()==1)
int vtkGenericAdaptorCell::IsGeometryLinear()
{
  return this->GetGeometryOrder() == 1;
}

//----------------------------------------------------------------------------
// Description:
// Does the attribute `a' have no higher-order interpolation for the cell?
// \pre a_exists: a!=0
// \post definition: result==(GetAttributeOrder()==1)
int vtkGenericAdaptorCell::IsAttributeLinear(vtkGenericAttribute *a)
{
  return this->GetAttributeOrder(a) == 1;
}

//----------------------------------------------------------------------------
void vtkGenericAdaptorCell::GetBounds(double bounds[6])
{
#if 0
  double x[3];
  int i, numPts=this->GetNumberOfPoints();

  if (numPts)
    {
    this->GetPoints()->GetPoint(0, x);
    bounds[0] = x[0];
    bounds[2] = x[1];
    bounds[4] = x[2];
    bounds[1] = x[0];
    bounds[3] = x[1];
    bounds[5] = x[2];
    for (i=1; i<numPts; i++)
      {
      this->GetPoints()->GetPoint(i, x);
      bounds[0] = (x[0] < bounds[0] ? x[0] : bounds[0]);
      bounds[1] = (x[0] > bounds[1] ? x[0] : bounds[1]);
      bounds[2] = (x[1] < bounds[2] ? x[1] : bounds[2]);
      bounds[3] = (x[1] > bounds[3] ? x[1] : bounds[3]);
      bounds[4] = (x[2] < bounds[4] ? x[2] : bounds[4]);
      bounds[5] = (x[2] > bounds[5] ? x[2] : bounds[5]);
      }
    }
#endif
  memset(bounds,0,sizeof(double));
  vtkErrorMacro("TO BE DONE");
}

//----------------------------------------------------------------------------
double *vtkGenericAdaptorCell::GetBounds()
{
  static double bounds[6];
  this->GetBounds(bounds);
  
  return bounds;
}

//----------------------------------------------------------------------------
// Reset
void vtkGenericAdaptorCell::Reset()
{
  this->InternalPoints->Reset();
  this->InternalCellArray->Reset();
  this->InternalScalars->Reset();
}

//----------------------------------------------------------------------------
void vtkGenericAdaptorCell::Contour(vtkContourValues *contourValues, 
                                    vtkImplicitFunction *f,
                                    vtkGenericAttributeCollection *attributes,
                                    vtkGenericCellTessellator *tess,
                                    vtkPointLocator *locator, 
                                    vtkCellArray *verts,
                                    vtkCellArray *lines,
                                    vtkCellArray *polys,
                                    vtkPointData *outPd,
                                    vtkCellData *outCd,
                                    vtkPointData *internalPd,
                                    vtkPointData *secondaryPd,
                                    vtkCellData *secondaryCd)
{
  assert("pre: values_exist" && ((contourValues!=0 && f==0) || (contourValues!=0 && f!=0)));
  assert("pre: attributes_exist" && attributes!=0);
  assert("pre: tessellator_exists" && tess!=0);
  assert("pre: locator_exists" && locator!=0);
  assert("pre: verts_exist" && verts!=0);
  assert("pre: lines_exist" && lines!=0);
  assert("pre: polys_exist" && polys!=0);
  assert("pre: internalPd_exists" && internalPd!=0);
  assert("pre: secondaryPd_exists" && secondaryPd!=0);
  assert("pre: secondaryCd_exists" && secondaryCd!=0);
  
  int i;
  int j;
  int c;
  double range[2];
  double contVal;
  double *values;
  
  vtkCell *linearCell;
  vtkIdType ptsCount;
 
  this->Reset();
  internalPd->Reset();
  
  switch(this->GetDimension())
    {
    case 3:
      tess->Tessellate(this, attributes, this->InternalPoints,
                       this->InternalCellArray, internalPd);
      linearCell=this->Tetra;
      ptsCount=4;
      break;
    case 2:
      tess->Triangulate(this, attributes, this->InternalPoints,
                        this->InternalCellArray, internalPd);
      linearCell=this->Triangle;
      ptsCount=3;
      break;
    default:
      assert("TODO: dimension 1 and 0" && 0);
    }
    
  vtkIdType npts, *pts = 0;
  double *point  = this->InternalPoints->GetPointer(0);
  
  // for each cell-centered attribute: copy the value in the secondary
  // cell data.
  secondaryCd->Reset();
  int attrib=0;
  while(attrib<attributes->GetNumberOfAttributes())
    {
    if(attributes->GetAttribute(attrib)->GetCentering()==vtkCellCentered)
      {
      assert("with the test there is no reason to be here" && 0);
      vtkDataArray *array=secondaryCd->GetArray(attributes->GetAttribute(attrib)->GetName());
      values=attributes->GetAttribute(attrib)->GetTuple(this);
      array->InsertNextTuple(values);
      }
    attrib++;
    }
  vtkDataArray *scalars=internalPd->GetArray(attributes->GetActiveAttribute());
  int currComp=attributes->GetActiveComponent();
  
  
  values  = contourValues->GetValues();
  int numContours = contourValues->GetNumberOfContours();
  
  c=internalPd->GetNumberOfArrays();
  int dataIndex=0;
  
  // for each linear sub-tetra, Build it and its pointdata
  // then contour it.
  for(this->InternalCellArray->InitTraversal(); 
      this->InternalCellArray->GetNextCell(npts, pts);)
    {
    assert("check: valid number of points" && npts == ptsCount);
    range[1] = range[0] = scalars->GetComponent(dataIndex,currComp);
    for(i=0; i<ptsCount; i++, point+=3)
      {
      linearCell->PointIds->SetId(i, pts[i]);
      linearCell->Points->SetPoint(i, point );
      if(f)
        {
        contVal = f->FunctionValue( point );
        }
      else
        {
        contVal = scalars->GetComponent(dataIndex,currComp);
        }
      this->Scalars->SetTuple1( i, contVal ); // value at point i of the
      // current linear tetra.
      range[0] = range[0] < contVal ? range[0] : contVal;
      range[1] = range[1] > contVal ? range[1] : contVal;   
      // for each point-centered attribute
      secondaryPd->Reset();
      j=0;
      while(j<c)
        {
        secondaryPd->GetArray(j)->InsertTuple(pts[i],
                                              internalPd->GetArray(j)->GetTuple(dataIndex));
        ++j;
        }
      ++dataIndex;
      
      }
    for( int vv = 0; vv < numContours; vv++ )
      {
      if(values[vv] >= range[0] && values[vv] <= range[1])
        {
        linearCell->Contour(values[vv],this->Scalars,locator,verts,lines,
                            polys,secondaryPd, outPd, secondaryCd,
                            0, outCd);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkGenericAdaptorCell::Clip(double value, 
                                 vtkImplicitFunction *f,
                                 vtkGenericAttributeCollection *attributes,
                                 vtkGenericCellTessellator *tess,
                                 int insideOut,
                                 vtkPointLocator *locator, 
                                 vtkCellArray *connectivity,
                                 vtkPointData *outPd,
                                 vtkCellData *outCd,
                                 vtkPointData *internalPd,
                                 vtkPointData *secondaryPd,
                                 vtkCellData *secondaryCd)
{
  assert("pre: attributes_exist" && attributes!=0);
  assert("pre: tessellator_exists" && tess!=0);
  assert("pre: locator_exists" && locator!=0);
  assert("pre: connectivity_exist" && connectivity!=0);
  assert("pre: internalPd_exists" && internalPd!=0);
  assert("pre: secondaryPd_exists" && secondaryPd!=0);
  assert("pre: secondaryCd_exists" && secondaryCd!=0);
  
  int i;
  int j;
  int c;
  double contVal;
  double *values;
  
  vtkCell *linearCell;
  vtkIdType ptsCount;
 
  this->Reset();
  internalPd->Reset();
  
  switch(this->GetDimension())
    {
    case 3:
      tess->Tessellate(this, attributes, this->InternalPoints,
                       this->InternalCellArray, internalPd);
      linearCell=this->Tetra;
      ptsCount=4;
      break;
    case 2:
      tess->Triangulate(this, attributes, this->InternalPoints,
                        this->InternalCellArray, internalPd);
      linearCell=this->Triangle;
      ptsCount=3;
      break;
    default:
      assert("TODO: dimension 1 and 0" && 0);
    }
    
  vtkIdType npts, *pts = 0;
  double *point  = this->InternalPoints->GetPointer(0);
  
  // for each cell-centered attribute: copy the value in the secondary
  // cell data.
  secondaryCd->Reset();
  int attrib=0;
  while(attrib<attributes->GetNumberOfAttributes())
    {
    if(attributes->GetAttribute(attrib)->GetCentering()==vtkCellCentered)
      {
      assert("with the test there is no reason to be here" && 0);
      vtkDataArray *array=secondaryCd->GetArray(attributes->GetAttribute(attrib)->GetName());
      values=attributes->GetAttribute(attrib)->GetTuple(this);
      array->InsertNextTuple(values);
      }
    attrib++;
    }
  vtkDataArray *scalars=internalPd->GetArray(attributes->GetActiveAttribute());
  int currComp=attributes->GetActiveComponent();
  
  c=internalPd->GetNumberOfArrays();
  int dataIndex=0;
  
  // for each linear sub-tetra, Build it and its pointdata
  // then contour it.
  for(this->InternalCellArray->InitTraversal(); 
      this->InternalCellArray->GetNextCell(npts, pts);)
    {
    assert("check: valid number of points" && npts == ptsCount);
    for(i=0; i<ptsCount; i++, point+=3)
      {
      linearCell->PointIds->SetId(i, pts[i]);
      linearCell->Points->SetPoint(i, point );
      if(f)
        {
        contVal = f->FunctionValue( point );
        }
      else
        {
        contVal = scalars->GetComponent(dataIndex,currComp);
        }
      this->Scalars->SetTuple1( i, contVal ); // value at point i of the
      // current linear tetra.
      // for each point-centered attribute
      secondaryPd->Reset();
      j=0;
      while(j<c)
        {
        secondaryPd->GetArray(j)->InsertTuple(pts[i],
                                              internalPd->GetArray(j)->GetTuple(dataIndex));
        ++j;
        }
      ++dataIndex;
      
      }
    linearCell->Clip(value,this->Scalars,locator,connectivity, 
                     secondaryPd, outPd, secondaryCd, 0, outCd, 
                     insideOut);
    }
}

//----------------------------------------------------------------------------
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
// // \pre scalars_exists: scalars!=0
// \pre pd_exist: pd!=0
// \pre cd_exists: cd!=0
void vtkGenericAdaptorCell::Tessellate(vtkGenericAttributeCollection *attributes, 
                                       vtkGenericCellTessellator *tess,
                                       vtkPoints *points,
                                       vtkCellArray *cellArray,
                                       vtkPointData *internalPd,
                                       vtkPointData *pd,
                                       vtkCellData *cd)
{
  assert("pre: attributes_exist" && attributes!=0);
  assert("pre: tessellator_exists" && tess!=0);
  assert("pre: points_exist" && points!=0);
  assert("pre: cellArray_exists" && cellArray!=0);
  assert("pre: internalPd_exists" && internalPd!=0);
  assert("pre: pd_exist" && pd!=0);
  assert("pre: cd_exist" && cd!=0);
  
  int i;
  int j;
  
  this->Reset();

  if( this->GetDimension() == 3)
    {
    internalPd->Reset();
    tess->Tessellate(this, attributes, this->InternalPoints,
                     this->InternalCellArray, internalPd);

    vtkIdType npts = 0;
    vtkIdType *pts = 0;
    double *point  = this->InternalPoints->GetPointer(0);
    
   
   // for each cell-centered attribute: copy the value
    int c=this->InternalCellArray->GetNumberOfCells();
    cout<<"this->InternalCellArray->GetNumberOfCells()="<<c<<endl;
    int attrib=0;
    while(attrib<attributes->GetNumberOfAttributes())
      {
      if(attributes->GetAttribute(attrib)->GetCentering()==vtkCellCentered)
        {
        assert("with the test there is no reason to be here" && 0);
        vtkDataArray *array=cd->GetArray(attributes->GetAttribute(attrib)->GetName());
        double *values=attributes->GetAttribute(attrib)->GetTuple(this);
        i=0;
        while(i<c)
          {
          array->InsertNextTuple(values);
          ++i;
          }
        }
      attrib++;
      }
    
    c=internalPd->GetNumberOfArrays(); // same as pd->GetNumberOfArrays();
    
    int dataIndex=0;
    
    for(this->InternalCellArray->InitTraversal(); 
      this->InternalCellArray->GetNextCell(npts, pts);)
      {
      assert("check: is_a_tetra" && npts == 4);
      cellArray->InsertNextCell(npts, pts );
      
      for(i=0;i<npts;i++, point+=3) //, scalar+=numComp)
        {
        points->InsertPoint(pts[i], point );
        // for each point-centered attribute
        j=0;
        while(j<c)
          {
          pd->GetArray(j)->InsertTuple(pts[i],
                                       internalPd->GetArray(j)->GetTuple(dataIndex));
          ++j;
          }
        ++dataIndex;
        }
      }
    }
  else if( this->GetDimension() == 2)
    {
    internalPd->Reset();
    tess->Triangulate(this, attributes, this->InternalPoints, 
      this->InternalCellArray, internalPd);

    //temporary:
    vtkIdType npts = 0;
    vtkIdType *pts = 0;
    double *point = this->InternalPoints->GetPointer(0);
    
    // for each cell-centered attribute: copy the value
    int c=this->InternalCellArray->GetNumberOfCells();
    int attrib=0;
    while(attrib<attributes->GetNumberOfAttributes())
      {
      if(attributes->GetAttribute(attrib)->GetCentering()==vtkCellCentered)
        {
        vtkDataArray *array=cd->GetArray(attributes->GetAttribute(attrib)->GetName());
        double *values=attributes->GetAttribute(attrib)->GetTuple(this);
        i=0;
        while(i<c)
          {
          array->InsertNextTuple(values);
          ++i;
          }
        }
      attrib++;
      }
    
    c=internalPd->GetNumberOfArrays();
    int dataIndex=0;
    for(this->InternalCellArray->InitTraversal(); 
      this->InternalCellArray->GetNextCell(npts, pts);)
      {
      assert("check: is_a_triangle" && npts == 3);
      cellArray->InsertNextCell(npts, pts );
      
      for(i=0;i<npts;i++, point+=3) //, scalar+=numComp)
        {
        points->InsertPoint(pts[i], point );
        // for each point-centered attribute
        j=0;
        while(j<c)
          {
          pd->GetArray(j)->InsertTuple(pts[i],
                                       internalPd->GetArray(j)->GetTuple(dataIndex));
          ++j;
          ++dataIndex;
          }
        }
      }
    }
  else
    {
    assert("check: TODO: Tessellate only works with 2D and 3D cells" && 0);
    }
}

//----------------------------------------------------------------------------
void vtkGenericAdaptorCell::TriangulateFace(vtkGenericAttributeCollection *attributes,
                                            vtkGenericCellTessellator *tess,
                                            int index, vtkPoints *points,
                                            vtkCellArray *cellArray,
                                            vtkPointData *internalPd,
                                            vtkPointData *pd,
                                            vtkCellData *cd )
{
  assert("pre: cell_is_3d" && this->GetDimension()==3);
  assert("pre: attributes_exist" && attributes!=0);
  assert("pre: tessellator_exists" && tess!=0);
  assert("pre: valid_face" && index>=0);
  assert("pre: points_exist" && points!=0);
  assert("pre: cellArray_exists" && cellArray!=0);
  assert("pre: internalPd_exists" && internalPd!=0);
  assert("pre: pd_exist" && pd!=0);
  assert("pre: cd_exists" && cd!=0);
  
  int i;
  int j;
  
  this->Reset();

  internalPd->Reset();
  
  tess->TessellateTriangleFace(this, attributes, index, 
                               this->InternalPoints, this->InternalCellArray,
                               internalPd);
//  tess->Tessellate(this, attributes, this->InternalPoints,
//                   this->InternalCellArray, internalPd);
  

  //temporary:
  vtkIdType npts, *pts = 0;
  double *point = this->InternalPoints->GetPointer(0);
  
  // for each cell-centered attribute: copy the value
  int c=this->InternalCellArray->GetNumberOfCells();
  int attrib=0;
  while(attrib<attributes->GetNumberOfAttributes())
    {
    if(attributes->GetAttribute(attrib)->GetCentering()==vtkCellCentered)
      {
      vtkDataArray *array=cd->GetArray(attributes->GetAttribute(attrib)->GetName());
      double *values=attributes->GetAttribute(attrib)->GetTuple(this);
      i=0;
      while(i<c)
        {
        array->InsertNextTuple(values);
        ++i;
        }
      }
    attrib++;
    }
  
  c=internalPd->GetNumberOfArrays();
  int dataIndex=0;
  for(this->InternalCellArray->InitTraversal(); 
      this->InternalCellArray->GetNextCell(npts, pts);)
    {
    assert("check: is_a_triangle" && npts == 3);
    cellArray->InsertNextCell(npts, pts );
    
    for(i=0;i<npts;i++, point+=3) //, scalar+=numComp)
      {
      points->InsertPoint(pts[i], point );
      // for each point-centered attribute
      j=0;
      while(j<c)
        {
        pd->GetArray(j)->InsertTuple(pts[i],
                                     internalPd->GetArray(j)->GetTuple(dataIndex));
        ++j;
        ++dataIndex;
        }
      }
    }
}
