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

vtkCxxRevisionMacro(vtkGenericAdaptorCell, "1.1");

vtkGenericAdaptorCell::vtkGenericAdaptorCell()
{
  this->CurrentCellDimension = MRegion;
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
                             vtkCellData *outCd)  
{
  assert("pre: values_exist" && ((contourValues!=0 && f==0) || (contourValues!=0 && f!=0)));
  assert("pre: attributes_exist" && attributes!=0);
  assert("pre: locator_exists" && locator!=0);
  assert("pre: verts_exist" && verts!=0);
  assert("pre: lines_exist" && lines!=0);
  assert("pre: polys_exist" && polys!=0);
  this->Reset();

  //Tessellate this cell into linear elements:
  if( this->CurrentCellDimension == MRegion )
    {
    //int numComp = this->CurrentSimmetrixAttribute->GetNumberOfComponents();  //FIXME
    int numComp = attributes->GetAttribute( 
      attributes->GetActiveAttribute() )->GetNumberOfComponents();
    int currComp = attributes->GetActiveComponent();
    this->InternalScalars->SetNumberOfComponents(numComp);
    tess->Tessellate(this, attributes, this->InternalPoints, 
      this->InternalCellArray, this->InternalScalars);

    //Update the number of field data
    //this->PointDataScalars->SetNumberOfComponents(numComp);
    this->PointDataScalars->Allocate( this->InternalScalars->GetSize() );

    double range[2];
    double *values  = contourValues->GetValues();
    int numContours = contourValues->GetNumberOfContours();

    vtkIdType npts, *pts = 0;
    double *point  = this->InternalPoints->GetPointer(0);
    double *scalar = this->InternalScalars->GetPointer(0) + currComp;

    double contVal;

    for(this->InternalCellArray->InitTraversal(); 
      this->InternalCellArray->GetNextCell(npts, pts);)
      {
      assert("check: cell has 4 points" && npts == 4);
      range[1] = range[0] = *scalar;
      for(int i=0; i<4; i++, point+=3, scalar+=numComp )
        {
        this->Tetra->PointIds->SetId(i, pts[i]);
        this->Tetra->Points->SetPoint(i, point );
        if(f)
          {
          contVal = f->FunctionValue( point );
          }
        else
          {
          contVal = *scalar;
          }

        this->Scalars->SetTuple1( i, contVal );
        range[0] = range[0] < contVal ? range[0] : contVal;
        range[1] = range[1] > contVal ? range[1] : contVal;

        //VTK expect organized data:
        //this->PointDataScalars->SetTuple1( pts[i], *scalar );
        this->PointDataScalars->InsertTuple1( pts[i], *scalar );
        }
      for( int vv = 0; vv < numContours; vv++ )
        {
        if(values[vv] >= range[0] && values[vv] <= range[1])
          {
          this->Tetra->Contour(values[vv],this->Scalars,locator,verts,lines,polys,
                               this->PointData, outPd, this->CellData, 0, outCd);
          }
        }
      }
    }
  else if( CurrentCellDimension == MFace )
    {
    //int numComp = this->CurrentSimmetrixAttribute->GetNumberOfComponents();
    int numComp = attributes->GetAttribute( attributes->GetActiveAttribute() )->GetNumberOfComponents();
    int currComp = attributes->GetActiveComponent();
    this->InternalScalars->SetNumberOfComponents(numComp);
    tess->Triangulate(this, attributes, this->InternalPoints, 
      this->InternalCellArray, this->InternalScalars);
    
    //Update the number of field data
    //this->PointDataScalars->SetNumberOfComponents(numComp);
    this->PointDataScalars->Allocate( this->InternalScalars->GetSize() );

    double range[2];
    double *values = contourValues->GetValues();
    int numContours = contourValues->GetNumberOfContours();

    vtkIdType npts, *pts = 0;
    double *point = this->InternalPoints->GetPointer(0);
    double *scalar = this->InternalScalars->GetPointer(0) + currComp;
    double contVal;

    for(this->InternalCellArray->InitTraversal(); 
      this->InternalCellArray->GetNextCell(npts, pts); )
      {
      assert("check: cell has 3 points" && npts == 3);
      range[1] = range[0] = *scalar;
      for(int i=0; i<3; i++, point+=3, scalar+=numComp )
        {
        this->Triangle->PointIds->SetId(i, pts[i]);
        this->Triangle->Points->SetPoint(i, point );
        if(f)
          {
          contVal = f->FunctionValue( point );
          }
        else
          {
          contVal = *scalar;
          }

        this->Scalars->SetTuple1( i, contVal );
        range[0] = range[0] < contVal ? range[0] : contVal;
        range[1] = range[1] > contVal ? range[1] : contVal;

        //VTK assume organized data:
        //this->PointDataScalars->SetTuple1( pts[i], *scalar );
        this->PointDataScalars->InsertTuple1( pts[i], *scalar );
        }

      for( int vv = 0; vv < numContours; vv++ )
        {
        if(values[vv] >= range[0] && values[vv] <= range[1])
          {
          this->Triangle->Contour(values[vv],this->Scalars,locator,verts,lines,polys,
                                  this->PointData, outPd, this->CellData, 0, outCd);
          }
        }
      }
    }
}
//----------------------------------------------------------------------------
void vtkGenericAdaptorCell::Clip(double value, vtkImplicitFunction *f,
                              vtkGenericAttributeCollection *attributes,
                              vtkGenericCellTessellator *tess,
                              int insideOut, vtkPointLocator *locator, 
                              vtkCellArray *connectivity, 
                              vtkPointData *outPd, vtkCellData *outCd)
{ 
  assert("pre: attributes_exist" && attributes!=0);
  assert("pre: locator_exists" && locator!=0);
  assert("pre: connectivity_exists" && connectivity!=0);

  this->Reset();

  //Tessellate this cell into linear elements:
  if( this->CurrentCellDimension == MRegion )
    {
    //int numComp = this->CurrentSimmetrixAttribute->GetNumberOfComponents();
    int numComp = attributes->GetAttribute( attributes->GetActiveAttribute() )->GetNumberOfComponents();
    int currComp = attributes->GetActiveComponent();
    this->InternalScalars->SetNumberOfComponents(numComp);
    tess->Tessellate(this, attributes, this->InternalPoints, 
      this->InternalCellArray, this->InternalScalars);

    //Update the number of field data
    this->PointDataScalars->Allocate( this->InternalScalars->GetSize() );

    vtkIdType npts, *pts = 0;
    double *point  = this->InternalPoints->GetPointer(0);
    double *scalar = this->InternalScalars->GetPointer(0) + currComp;

    for(this->InternalCellArray->InitTraversal(); 
      this->InternalCellArray->GetNextCell(npts, pts); )
      {
      assert("check: cell has 4 points" && npts == 4);
      for(int i=0;i<4;i++, point+=3, scalar+=numComp )
        {
        this->Tetra->PointIds->SetId(i, pts[i]);
        this->Tetra->Points->SetPoint(i, point );
        if( f )
          {
          //We are clipping with an implicit function
          this->Scalars->SetTuple1( i, f->FunctionValue( point ) );
          }
        else
          {
          this->Scalars->SetTuple1( i, *scalar );
          }

        //VTK expect organized data:
        //this->PointDataScalars->SetTuple1( pts[i], *scalar );
        this->PointDataScalars->InsertTuple1( pts[i], *scalar );
        }
      this->Tetra->Clip(value, this->Scalars, locator, connectivity, 
                        this->PointData, outPd, this->CellData, 0, outCd, 
                        insideOut);
      }
  }
  else
  {
    //int numComp = this->CurrentSimmetrixAttribute->GetNumberOfComponents();
    int numComp = attributes->GetAttribute( attributes->GetActiveAttribute() )->GetNumberOfComponents();
    int currComp = attributes->GetActiveComponent();
    this->InternalScalars->SetNumberOfComponents(numComp);
    tess->Triangulate(this, attributes, this->InternalPoints, 
      this->InternalCellArray, this->InternalScalars);

    //cerr << "GetSize" << this->InternalScalars->GetSize() << endl;

    //Update the number of field data
    this->PointDataScalars->Allocate( 3*this->InternalScalars->GetSize() );

    vtkIdType npts, *pts=0;
    double *point  = this->InternalPoints->GetPointer(0);
    double *scalar = this->InternalScalars->GetPointer(0) + currComp;

    for(this->InternalCellArray->InitTraversal(); 
      this->InternalCellArray->GetNextCell(npts, pts); )
      {
      assert("check: cell has 3 points" && npts == 3);
      for(int i=0;i<3;i++, point+=3, scalar+=numComp )
        {
        this->Triangle->PointIds->SetId(i, pts[i]);
        this->Triangle->Points->SetPoint(i, point );
        if(f)
          {
          //cerr << pts[i] << endl;
          this->Scalars->SetTuple1( i, f->FunctionValue( point ));
          }
        else
          {
          this->Scalars->SetTuple1( i, *scalar );
          }

        //VTK expect organized data:
        //this->PointDataScalars->SetTuple1( pts[i], *scalar );
        this->PointDataScalars->InsertTuple1( pts[i], *scalar );
        }
      this->Triangle->Clip(value, this->Scalars, locator, connectivity, 
                        this->PointData, outPd, this->CellData, 0, outCd, 
//                        this->PointData, outPd, inCd, cellId, outCd, 
                        insideOut);
      }

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
                                vtkPoints *outpoints,
                                vtkCellArray *cellArray,  
                                vtkPointData *pd,
                                vtkCellData *cd)
{
  assert("pre: attributes_exist" && attributes!=0);
  assert("pre: points_exist" && outpoints!=0);
  assert("pre: cellArray_exists" && cellArray!=0);
  assert("pre: pd_exist" && pd!=0);
  assert("pre: cd_exist" && cd!=0);
  
  this->Reset();

  if( this->CurrentCellDimension == MRegion)
    {
    int numComp = attributes->GetAttribute( 
        attributes->GetActiveAttribute() )->GetNumberOfComponents();
    int currComp = attributes->GetActiveComponent();
    this->InternalScalars->SetNumberOfComponents(numComp);
    // FIXME: The tessellator should take all attributes in argument, not only
    // the active attribute
    
    tess->Tessellate(this, attributes, this->InternalPoints,
      this->InternalCellArray, this->InternalScalars);

    //Update the number of field data
    //this->PointDataScalars->SetNumberOfComponents(numComp);
    this->PointDataScalars->Allocate( this->InternalScalars->GetSize() );

    vtkIdType npts, *pts = 0;
    double *point  = this->InternalPoints->GetPointer(0);
    double *scalar = this->InternalScalars->GetPointer(0) + currComp;

    vtkDataArray* tetsScalars = pd->GetScalars();
    if(!tetsScalars)
      {
      tetsScalars = vtkDoubleArray::New();
      tetsScalars->SetName(attributes->GetAttribute( 
                             attributes->GetActiveAttribute() )->GetName());
      pd->SetScalars(tetsScalars);
      tetsScalars->Delete();
      }
   
    for(this->InternalCellArray->InitTraversal(); 
      this->InternalCellArray->GetNextCell(npts, pts);)
      {
      assert("check: is_a_tetra" && npts == 4);
      cellArray->InsertNextCell(npts, pts );
      for(int i=0;i<npts;i++, point+=3, scalar+=numComp)
        {
        outpoints->InsertPoint(pts[i], point );
        tetsScalars->InsertTuple(pts[i], scalar);
        }
      }
    }
  else if( this->CurrentCellDimension == MFace)
    {
    // FIXME: The tessellator should take all attributes in argument, not only
    // the active attribute
    
    tess->Triangulate(this, attributes, this->InternalPoints, 
      this->InternalCellArray, this->InternalScalars);

    //temporary:
    vtkIdType npts, *pts = 0;
    double *point = this->InternalPoints->GetPointer(0);
    int currComp = attributes->GetActiveComponent();
    double *scalar = this->InternalScalars->GetPointer(0) + currComp;
    
    
    vtkDataArray* tetsScalars = pd->GetScalars(); //FIXME
    if(tetsScalars==0)
      {
      tetsScalars= vtkDoubleArray::New();
      pd->SetScalars(tetsScalars);
      tetsScalars->Delete();
      }
    
    for(this->InternalCellArray->InitTraversal(); 
      this->InternalCellArray->GetNextCell(npts, pts);)
      {
      assert("check: is_a_triangle" && npts == 3);
      cellArray->InsertNextCell(npts, pts );
      for(int i=0;i<npts;i++,point+=3,scalar++)
        {
        outpoints->InsertPoint(pts[i], point );
        pd->InsertTuple(pts[i], scalar);
        tetsScalars->InsertTuple(pts[i], scalar); //FIXME
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
                                     vtkCellArray *cellArray, vtkPointData *pd,
                                     vtkCellData *vtkNotUsed(cd) )
{
  this->Reset();

  if( this->CurrentCellDimension == MRegion)
    {
    tess->TessellateTriangleFace(this, attributes, index, 
      this->InternalPoints, this->InternalCellArray, this->InternalScalars);

    //temporary:
    vtkIdType npts, *pts = 0;
    double *point  = this->InternalPoints->GetPointer(0);
    int currComp   = attributes->GetActiveComponent();
    double *scalar = this->InternalScalars->GetPointer(0) + currComp;
    
    vtkDataArray* tetsScalars = pd->GetScalars();//FIXME
    if(tetsScalars==0)
      {
      tetsScalars = vtkDoubleArray::New();
      pd->SetScalars(tetsScalars);
      tetsScalars->Delete();
      }

    for(this->InternalCellArray->InitTraversal(); 
      this->InternalCellArray->GetNextCell(npts, pts);)
      {
      assert("check: cell has 3 points" && npts == 3);
      cellArray->InsertNextCell(npts, pts );
      for(int i=0;i<npts;i++, point+=3, scalar++)
        {
        points->InsertPoint(pts[i], point );
        tetsScalars->InsertTuple(pts[i], scalar);
        }
      }
    }
  else
    {
    vtkErrorMacro( << "This is not a 3D cell" );
    }
}  

