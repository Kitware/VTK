/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataObjectToDataSetFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkDataObjectToDataSetFilter.h"
#include "vtkFieldDataToAttributeDataFilter.h"
#include "vtkPolyData.h"
#include "vtkStructuredPoints.h"
#include "vtkStructuredGrid.h"
#include "vtkRectilinearGrid.h"
#include "vtkUnstructuredGrid.h"
#include "vtkCellArray.h"

// Instantiate object with no input and no defined output.
vtkDataObjectToDataSetFilter::vtkDataObjectToDataSetFilter()
{
  int i;
  this->Input = NULL;
  this->Updating = 0;

  this->PolyData = vtkPolyData::New();
  this->PolyData->SetSource(this);
  this->StructuredPoints = vtkStructuredPoints::New();
  this->StructuredPoints->SetSource(this);
  this->StructuredGrid = vtkStructuredGrid::New();
  this->StructuredGrid->SetSource(this);
  this->UnstructuredGrid = vtkUnstructuredGrid::New();
  this->UnstructuredGrid->SetSource(this);
  this->RectilinearGrid = vtkRectilinearGrid::New();
  this->RectilinearGrid->SetSource(this);

  this->DataSetType = VTK_POLY_DATA;
  this->Output = this->PolyData;
  
  for (i=0; i < 3; i++)
    {
    this->PointArrays[i] = NULL;
    this->PointArrayComponents[i] = -1; //uninitialized
    this->PointComponentRange[i][0] = this->PointComponentRange[0][1] = -1;
    this->PointNormalize[i] = 1; //yes, normalize
    }

  this->VertsArray = NULL;
  this->VertsArrayComponent = -1;
  this->VertsComponentRange[0] = this->VertsComponentRange[1] = -1;

  this->LinesArray = NULL;
  this->LinesArrayComponent = -1;
  this->LinesComponentRange[0] = this->LinesComponentRange[1] = -1;

  this->PolysArray = NULL;
  this->PolysArrayComponent = -1;
  this->PolysComponentRange[0] = this->PolysComponentRange[1] = -1;

  this->StripsArray = NULL;
  this->StripsArrayComponent = -1;
  this->StripsComponentRange[0] = this->StripsComponentRange[1] = -1;

  this->CellTypeArray = NULL;
  this->CellTypeArrayComponent = -1;
  this->CellTypeComponentRange[0] = this->CellTypeComponentRange[1] = -1;

  this->CellConnectivityArray = NULL;
  this->CellConnectivityArrayComponent = -1;
  this->CellConnectivityComponentRange[0] = this->CellConnectivityComponentRange[1] = -1;
  
  this->DefaultNormalize = 0;

  this->DimensionsArray = NULL;; //the name of the array
  this->DimensionsArrayComponent = -1;
  this->DimensionsComponentRange[0] = this->DimensionsComponentRange[1] = -1;
  
  this->SpacingArray = NULL;; //the name of the array
  this->SpacingArrayComponent = -1;
  this->SpacingComponentRange[0] = this->SpacingComponentRange[1] = -1;
  
  this->OriginArray = NULL;; //the name of the array
  this->OriginArrayComponent = -1;
  this->OriginComponentRange[0] = this->OriginComponentRange[1] = -1;
  
}

vtkDataObjectToDataSetFilter::~vtkDataObjectToDataSetFilter()
{
  int i;

  if (this->Input) {this->Input->UnRegister(this);}
  this->PolyData->Delete();
  this->StructuredPoints->Delete();
  this->StructuredGrid->Delete();
  this->UnstructuredGrid->Delete();
  this->RectilinearGrid->Delete();
  this->Output = NULL;
  
  for (i=0; i<3; i++) 
    if ( this->PointArrays[i] != NULL )
      delete [] this->PointArrays[i];

  if ( this->VertsArray != NULL ) delete [] this->VertsArray;
  if ( this->LinesArray != NULL ) delete [] this->LinesArray;
  if ( this->PolysArray != NULL ) delete [] this->PolysArray;
  if ( this->StripsArray != NULL ) delete [] this->StripsArray;
  if ( this->CellTypeArray != NULL ) delete [] this->CellTypeArray;
  if ( this->CellConnectivityArray != NULL ) delete [] this->CellConnectivityArray;

  if ( this->DimensionsArray != NULL ) delete [] this->DimensionsArray;
  if ( this->SpacingArray != NULL ) delete [] this->SpacingArray;
  if ( this->OriginArray != NULL ) delete [] this->OriginArray;
}

// Stuff related to filter interface------------------------------------------
//
// Specify the input data or filter.
void vtkDataObjectToDataSetFilter::SetInput(vtkDataObject *input)
{
  if ( this->Input != input )
    {
    vtkDebugMacro(<<" setting Input to " << (void *)input);
    if (this->Input) {this->Input->UnRegister(this);}
    this->Input = input;
    if (this->Input) {this->Input->Register(this);}
    this->Modified();
    }
}

void vtkDataObjectToDataSetFilter::Update()
{
  // make sure input is available
  if ( !this->Input )
    {
    vtkErrorMacro(<< "No input...can't execute!");
    return;
    }

  // prevent chasing our tail
  if (this->Updating) return;

  this->Updating = 1;
  this->Input->Update();
  this->Updating = 0;

  if (this->Input->GetMTime() > this->ExecuteTime ||
  this->GetMTime() > this->ExecuteTime )
    {
    if ( this->Input->GetDataReleased() )
      {
      this->Input->ForceUpdate();
      }

    if ( this->StartMethod ) (*this->StartMethod)(this->StartMethodArg);

    // reset Abort flag
    this->AbortExecute = 0;
    this->Progress = 0.0;
    this->PolyData->Initialize(); //clear output
    this->StructuredPoints->Initialize();
    this->StructuredGrid->Initialize();
    this->RectilinearGrid->Initialize();
    this->UnstructuredGrid->Initialize();
    this->Execute();
    this->ExecuteTime.Modified();
    if ( !this->AbortExecute ) this->UpdateProgress(1.0);
    this->SetDataReleased(0);
    if ( this->EndMethod ) (*this->EndMethod)(this->EndMethodArg);
    }

  if ( this->Input->ShouldIReleaseData() ) this->Input->ReleaseData();
}

void vtkDataObjectToDataSetFilter::Execute()
{
  int npts, ncells;

  vtkDebugMacro(<<"Generating dataset from field data");

  this->Output = NULL;

  switch (this->DataSetType)
    {
    case VTK_POLY_DATA:
      if ( (npts=this->ConstructPoints(this->PolyData)) ) 
        {
        this->Output = this->PolyData;
        ncells = this->ConstructCells(this->PolyData);
        }
      else
        {
        vtkErrorMacro(<<"Couldn't create any points");
        }
      break;

    case VTK_STRUCTURED_POINTS:
      this->ConstructDimensions();
      this->ConstructSpacing();
      this->ConstructOrigin();
      
      this->StructuredPoints->SetDimensions(this->Dimensions);
      this->StructuredPoints->SetOrigin(this->Origin);
      this->StructuredPoints->SetSpacing(this->Spacing);
      this->Output = this->StructuredPoints;
      break;

    case VTK_STRUCTURED_GRID:
      if ( (npts=this->ConstructPoints(this->StructuredGrid)) )
        {
        this->ConstructDimensions();
        if ( npts == (this->Dimensions[0] * this->Dimensions[1] * this->Dimensions[2]) )
          {
          this->Output = this->StructuredGrid;
          this->StructuredGrid->SetDimensions(this->Dimensions);
          }
        else
          {
          vtkErrorMacro(<<"Number of points don't match dimensions");
          }
        }
      break;

    case VTK_RECTILINEAR_GRID:
      if ( (npts=this->ConstructPoints(this->RectilinearGrid)) )
        {
        this->ConstructDimensions();
        if ( npts == (this->Dimensions[0] * this->Dimensions[1] * this->Dimensions[2]) )
          {
          this->Output = this->RectilinearGrid;
          this->RectilinearGrid->SetDimensions(this->Dimensions);
          }
        else
          {
          vtkErrorMacro(<<"Number of points don't match dimensions");
          }
        }
      break;

    case VTK_UNSTRUCTURED_GRID:
      if ( this->ConstructPoints(this->UnstructuredGrid) ) 
        {
        this->Output = this->UnstructuredGrid;
        this->ConstructCells(this->UnstructuredGrid);
        }
      else
        {
        vtkErrorMacro(<<"Couldn't create any points");
        }
      break;

    default:
      vtkErrorMacro(<<"Unsupported dataset type!");
    }

  //Pass field data through to output
  if ( this->Output )
    {
    this->Output->SetFieldData(this->Input->GetFieldData());
    }
}

vtkDataSet *vtkDataObjectToDataSetFilter::GetOutput()
{
  return (vtkDataSet *)this->Output;
}

vtkPolyData *vtkDataObjectToDataSetFilter::GetPolyDataOutput()
{
  return this->PolyData;
}

vtkStructuredPoints *vtkDataObjectToDataSetFilter::GetStructuredPointsOutput()
{
  return this->StructuredPoints;
}

vtkStructuredGrid *vtkDataObjectToDataSetFilter::GetStructuredGridOutput()
{
  return this->StructuredGrid;
}

vtkUnstructuredGrid *vtkDataObjectToDataSetFilter::GetUnstructuredGridOutput()
{
  return this->UnstructuredGrid;
}

vtkRectilinearGrid *vtkDataObjectToDataSetFilter::GetRectilinearGridOutput()
{
  return this->RectilinearGrid;
}

void vtkDataObjectToDataSetFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkSource::PrintSelf(os,indent);

}

// Stuff related to points --------------------------------------------
//
void vtkDataObjectToDataSetFilter::SetPointComponent(int comp, char *arrayName, 
                              int arrayComp, int min, int max, int normalize)
{
  if ( comp < 0 || comp > 2 )
    {
    vtkErrorMacro(<<"Point component must be between (0,2)");
    return;
    }
  
  vtkFieldDataToAttributeDataFilter::SetArrayName(this, this->PointArrays[comp], arrayName);
  if ( this->PointArrayComponents[comp] != arrayComp )
    {
    this->PointArrayComponents[comp] = arrayComp;
    this->Modified();
    }
  if ( this->PointComponentRange[comp][0] != min )
    {
    this->PointComponentRange[comp][0] = min;
    this->Modified();
    }
  if ( this->PointComponentRange[comp][1] != max )
    {
    this->PointComponentRange[comp][1] = max;
    this->Modified();
    }
  if ( this->PointNormalize[comp] != normalize )
    {
    this->PointNormalize[comp] = normalize;  
    this->Modified();
    }
}

const char *vtkDataObjectToDataSetFilter::GetPointComponentArrayName(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 2 ? 2 : comp));
  return this->PointArrays[comp];
}

int vtkDataObjectToDataSetFilter::GetPointComponentArrayComponent(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 2 ? 2 : comp));
  return this->PointArrayComponents[comp];
}

int vtkDataObjectToDataSetFilter::GetPointComponentMinRange(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 2 ? 2 : comp));
  return this->PointComponentRange[comp][0];
}

int vtkDataObjectToDataSetFilter::GetPointComponentMaxRange(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 2 ? 2 : comp));
  return this->PointComponentRange[comp][1];
}

int vtkDataObjectToDataSetFilter::GetPointComponentNormailzeFlag(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 2 ? 2 : comp));
  return this->PointNormalize[comp];
}

int vtkDataObjectToDataSetFilter::ConstructPoints(vtkPointSet *ps)
{
  int i;
  vtkDataArray *fieldArray[3];
  int npts;
  vtkFieldData *fd=this->GetInput()->GetFieldData();

  for ( i=0; i < 3; i++ )
    {
    fieldArray[i] = vtkFieldDataToAttributeDataFilter::GetFieldArray(fd, this->PointArrays[i], 
                                                             this->PointArrayComponents[i]);

    if ( fieldArray[i] == NULL )
      {
      vtkErrorMacro(<<"Can't find array requested");
      return 0;
      }
    vtkFieldDataToAttributeDataFilter::UpdateComponentRange(fieldArray[i], 
                                                            this->PointComponentRange[i]);
    }

  npts = this->PointComponentRange[0][1] - this->PointComponentRange[0][0] + 1;
  if ( npts != (this->PointComponentRange[1][1] - this->PointComponentRange[1][0] + 1) ||
       npts != (this->PointComponentRange[2][1] - this->PointComponentRange[2][0] + 1) )
    {
    vtkErrorMacro(<<"Number of point components not consistent");
    return 0;
    }

  vtkPoints *newPts = vtkPoints::New();
  newPts->SetDataType(vtkFieldDataToAttributeDataFilter::GetComponentsType(3, fieldArray));
  newPts->SetNumberOfPoints(npts);
  
  for ( i=0; i < 3; i++ )
    {
    if ( vtkFieldDataToAttributeDataFilter::ConstructArray(
      newPts->GetData(), i, fieldArray[i], this->PointArrayComponents[i],
      this->PointComponentRange[i][0],
      this->PointComponentRange[i][1],
      this->PointNormalize[i]) == 0 )
      {
      newPts->Delete();
      return 0;
      }
    }
  
  ps->SetPoints(newPts);
  newPts->Delete();
  
  return npts;
}

int vtkDataObjectToDataSetFilter::ConstructPoints(vtkRectilinearGrid *rg)
{
  int i, nXpts, nYpts, nZpts, npts;
  vtkDataArray *fieldArray[3];
  vtkFieldData *fd=this->GetInput()->GetFieldData();

  for ( i=0; i < 3; i++ )
    {
    fieldArray[i] = vtkFieldDataToAttributeDataFilter::GetFieldArray(fd, this->PointArrays[i], 
                                                             this->PointArrayComponents[i]);

    if ( fieldArray[i] == NULL )
      {
      vtkErrorMacro(<<"Can't find array requested");
      return 0;
      }
    }
  
  for (i=0; i<3; i++)
    {
    vtkFieldDataToAttributeDataFilter::UpdateComponentRange(fieldArray[i], 
                                                            this->PointComponentRange[i]);
    }

  nXpts = this->PointComponentRange[0][1] - this->PointComponentRange[0][0] + 1;
  nYpts = this->PointComponentRange[1][1] - this->PointComponentRange[1][0] + 1;
  nZpts = this->PointComponentRange[2][1] - this->PointComponentRange[2][0] + 1;
  npts = nXpts * nYpts * nZpts;
  
  vtkScalars *XPts = vtkScalars::New();
  XPts->SetDataType(vtkFieldDataToAttributeDataFilter::GetComponentsType(1, fieldArray));
  XPts->SetNumberOfScalars(nXpts);
  XPts->SetNumberOfComponents(1);
  
  vtkScalars *YPts = vtkScalars::New();
  YPts->SetDataType(vtkFieldDataToAttributeDataFilter::GetComponentsType(1, fieldArray+1));
  YPts->SetNumberOfScalars(nYpts);
  YPts->SetNumberOfComponents(1);
  
  vtkScalars *ZPts = vtkScalars::New();
  ZPts->SetDataType(vtkFieldDataToAttributeDataFilter::GetComponentsType(1, fieldArray+2));
  ZPts->SetNumberOfScalars(nZpts);
  ZPts->SetNumberOfComponents(1);
  
  if ( vtkFieldDataToAttributeDataFilter::ConstructArray(
    XPts->GetData(), 0, fieldArray[0], this->PointArrayComponents[0],
    this->PointComponentRange[0][0],
    this->PointComponentRange[0][1],
    this->PointNormalize[0]) == 0 )
    {
    XPts->Delete(); YPts->Delete(); ZPts->Delete(); 
    return 0;
    }
  
  if ( vtkFieldDataToAttributeDataFilter::ConstructArray(
    YPts->GetData(), 0, fieldArray[1], this->PointArrayComponents[1],
    this->PointComponentRange[1][0],
    this->PointComponentRange[1][1],
    this->PointNormalize[1]) == 0 )
    {
    XPts->Delete(); YPts->Delete(); ZPts->Delete(); 
    return 0;
    }
  
  if ( vtkFieldDataToAttributeDataFilter::ConstructArray(
    ZPts->GetData(), 0, fieldArray[2], this->PointArrayComponents[2],
    this->PointComponentRange[2][0],
    this->PointComponentRange[2][1],
    this->PointNormalize[2]) == 0 )
    {
    XPts->Delete(); YPts->Delete(); ZPts->Delete(); 
    return 0;
    }
  
  rg->SetXCoordinates(XPts);
  rg->SetYCoordinates(YPts);
  rg->SetZCoordinates(ZPts);
  XPts->Delete(); YPts->Delete(); ZPts->Delete(); 
  
  return npts;
}

// Stuff related to vtkPolyData --------------------------------------------
//
void vtkDataObjectToDataSetFilter::SetVertsComponent(char *arrayName, int arrayComp, 
                                                      int min, int max)
{
  vtkFieldDataToAttributeDataFilter::SetArrayName(this, this->VertsArray, arrayName);
  if ( this->VertsArrayComponent != arrayComp )
    {
    this->VertsArrayComponent = arrayComp;
    this->Modified();
    }
  if ( this->VertsComponentRange[0] != min )
    {
    this->VertsComponentRange[0] = min;
    this->Modified();
    }
  if ( this->VertsComponentRange[1] != max )
    {
    this->VertsComponentRange[1] = max;
    this->Modified();
    }
}

const char *vtkDataObjectToDataSetFilter::GetVertsComponentArrayName()
{
  return this->VertsArray;
}

int vtkDataObjectToDataSetFilter::GetVertsComponentArrayComponent()
{
  return this->VertsArrayComponent;
}

int vtkDataObjectToDataSetFilter::GetVertsComponentMinRange()
{
  return this->VertsComponentRange[0];
}

int vtkDataObjectToDataSetFilter::GetVertsComponentMaxRange()
{
  return this->VertsComponentRange[1];
}

void vtkDataObjectToDataSetFilter::SetLinesComponent(char *arrayName, int arrayComp, 
                                                    int min, int max)
{
  vtkFieldDataToAttributeDataFilter::SetArrayName(this, this->LinesArray, arrayName);
  if ( this->LinesArrayComponent != arrayComp )
    {
    this->LinesArrayComponent = arrayComp;
    this->Modified();
    }
  if ( this->LinesComponentRange[0] != min )
    {
    this->LinesComponentRange[0] = min;
    this->Modified();
    }
  if ( this->LinesComponentRange[1] != max )
    {
    this->LinesComponentRange[1] = max;
    this->Modified();
    }
}

const char *vtkDataObjectToDataSetFilter::GetLinesComponentArrayName()
{
  return this->LinesArray;
}

int vtkDataObjectToDataSetFilter::GetLinesComponentArrayComponent()
{
  return this->LinesArrayComponent;
}

int vtkDataObjectToDataSetFilter::GetLinesComponentMinRange()
{
  return this->LinesComponentRange[0];
}

int vtkDataObjectToDataSetFilter::GetLinesComponentMaxRange()
{
  return this->LinesComponentRange[1];
}

void vtkDataObjectToDataSetFilter::SetPolysComponent(char *arrayName, int arrayComp, 
                                                       int min, int max)
{
  vtkFieldDataToAttributeDataFilter::SetArrayName(this, this->PolysArray, arrayName);
  if ( this->PolysArrayComponent != arrayComp )
    {
    this->PolysArrayComponent = arrayComp;
    this->Modified();
    }
  if ( this->PolysComponentRange[0] != min )
    {
    this->PolysComponentRange[0] = min;
    this->Modified();
    }
  if ( this->PolysComponentRange[1] != max )
    {
    this->PolysComponentRange[1] = max;
    this->Modified();
    }
}

const char *vtkDataObjectToDataSetFilter::GetPolysComponentArrayName()
{
  return this->PolysArray;
}

int vtkDataObjectToDataSetFilter::GetPolysComponentArrayComponent()
{
  return this->PolysArrayComponent;
}

int vtkDataObjectToDataSetFilter::GetPolysComponentMinRange()
{
  return this->PolysComponentRange[0];
}

int vtkDataObjectToDataSetFilter::GetPolysComponentMaxRange()
{
  return this->PolysComponentRange[1];
}

void vtkDataObjectToDataSetFilter::SetStripsComponent(char *arrayName, int arrayComp, 
                                                             int min, int max)
{
  vtkFieldDataToAttributeDataFilter::SetArrayName(this, this->StripsArray, arrayName);
  if ( this->StripsArrayComponent != arrayComp )
    {
    this->StripsArrayComponent = arrayComp;
    this->Modified();
    }
  if ( this->StripsComponentRange[0] != min )
    {
    this->StripsComponentRange[0] = min;
    this->Modified();
    }
  if ( this->StripsComponentRange[1] != max )
    {
    this->StripsComponentRange[1] = max;
    this->Modified();
    }
}

const char *vtkDataObjectToDataSetFilter::GetStripsComponentArrayName()
{
  return this->StripsArray;
}

int vtkDataObjectToDataSetFilter::GetStripsComponentArrayComponent()
{
  return this->StripsArrayComponent;
}

int vtkDataObjectToDataSetFilter::GetStripsComponentMinRange()
{
  return this->StripsComponentRange[0];
}

int vtkDataObjectToDataSetFilter::GetStripsComponentMaxRange()
{
  return this->StripsComponentRange[1];
}

// Stuff related to vtkUnstructuredGrid --------------------------------------------
//
void vtkDataObjectToDataSetFilter::SetCellTypeComponent(char *arrayName, int arrayComp, 
                                                        int min, int max)
{
  vtkFieldDataToAttributeDataFilter::SetArrayName(this, this->CellTypeArray, arrayName);
  if ( this->CellTypeArrayComponent != arrayComp )
    {
    this->CellTypeArrayComponent = arrayComp;
    this->Modified();
    }
  if ( this->CellTypeComponentRange[0] != min )
    {
    this->CellTypeComponentRange[0] = min;
    this->Modified();
    }
  if ( this->CellTypeComponentRange[1] != max )
    {
    this->CellTypeComponentRange[1] = max;
    this->Modified();
    }
}

const char *vtkDataObjectToDataSetFilter::GetCellTypeComponentArrayName()
{
  return this->CellTypeArray;
}

int vtkDataObjectToDataSetFilter::GetCellTypeComponentArrayComponent()
{
  return this->CellTypeArrayComponent;
}

int vtkDataObjectToDataSetFilter::GetCellTypeComponentMinRange()
{
  return this->CellTypeComponentRange[0];
}

int vtkDataObjectToDataSetFilter::GetCellTypeComponentMaxRange()
{
  return this->CellTypeComponentRange[1];
}

void vtkDataObjectToDataSetFilter::SetCellConnectivityComponent(char *arrayName, int arrayComp, 
                                                                int min, int max)
{
  vtkFieldDataToAttributeDataFilter::SetArrayName(this, this->CellConnectivityArray, arrayName);
  if ( this->CellConnectivityArrayComponent != arrayComp )
    {
    this->CellConnectivityArrayComponent = arrayComp;
    this->Modified();
    }
  if ( this->CellConnectivityComponentRange[0] != min )
    {
    this->CellConnectivityComponentRange[0] = min;
    this->Modified();
    }
  if ( this->CellConnectivityComponentRange[1] != max )
    {
    this->CellConnectivityComponentRange[1] = max;
    this->Modified();
    }
}

const char *vtkDataObjectToDataSetFilter::GetCellConnectivityComponentArrayName()
{
  return this->CellConnectivityArray;
}

int vtkDataObjectToDataSetFilter::GetCellConnectivityComponentArrayComponent()
{
  return this->CellConnectivityArrayComponent;
}

int vtkDataObjectToDataSetFilter::GetCellConnectivityComponentMinRange()
{
  return this->CellConnectivityComponentRange[0];
}

int vtkDataObjectToDataSetFilter::GetCellConnectivityComponentMaxRange()
{
  return this->CellConnectivityComponentRange[1];
}

int vtkDataObjectToDataSetFilter::ConstructCells(vtkPolyData *pd)
{
  int i, ncells=0;
  vtkDataArray *fieldArray[4];
  vtkFieldData *fd=this->GetInput()->GetFieldData();

  fieldArray[0] = vtkFieldDataToAttributeDataFilter::GetFieldArray(fd, this->VertsArray, 
                                                        this->VertsArrayComponent);
  if ( this->VertsArray && fieldArray[0] == NULL )
    {
    vtkErrorMacro(<<"Can't find array requested for vertices");
    return 0;
    }
  
  fieldArray[1] = vtkFieldDataToAttributeDataFilter::GetFieldArray(fd, this->LinesArray, 
                                                                   this->LinesArrayComponent);
  if ( this->LinesArray && fieldArray[1] == NULL )
    {
    vtkErrorMacro(<<"Can't find array requested for lines");
    return 0;
    }
  
  fieldArray[2] = vtkFieldDataToAttributeDataFilter::GetFieldArray(fd, this->PolysArray, 
                                                                   this->PolysArrayComponent);
  if ( this->PolysArray && fieldArray[2] == NULL )
    {
    vtkErrorMacro(<<"Can't find array requested for polygons");
    return 0;
    }
  
  fieldArray[3] = vtkFieldDataToAttributeDataFilter::GetFieldArray(fd, this->StripsArray, 
                                                           this->StripsArrayComponent);
  if ( this->StripsArray && fieldArray[3] == NULL )
    {
    vtkErrorMacro(<<"Can't find array requested for triangle strips");
    return 0;
    }

  if ( fieldArray[0] )
    {
    vtkFieldDataToAttributeDataFilter::UpdateComponentRange(fieldArray[0], 
                                                            this->VertsComponentRange);
    vtkCellArray *verts = this->ConstructCellArray(fieldArray[0], this->VertsArrayComponent,
                                                   this->VertsComponentRange);
    if ( verts != NULL )
      {
      pd->SetVerts(verts);
      ncells += verts->GetNumberOfCells();
      verts->Delete();
      }
    }
  
  if ( fieldArray[1] )
    {
    vtkFieldDataToAttributeDataFilter::UpdateComponentRange(fieldArray[1], 
                                                            this->LinesComponentRange);
    vtkCellArray *lines = this->ConstructCellArray(fieldArray[1], this->LinesArrayComponent,
                                                   this->LinesComponentRange);
    if ( lines != NULL )
      {
      pd->SetLines(lines);
      ncells += lines->GetNumberOfCells();
      lines->Delete();
      }
    }
  
  if ( fieldArray[2] )
    {
    vtkFieldDataToAttributeDataFilter::UpdateComponentRange(fieldArray[2], 
                                                            this->PolysComponentRange);
    vtkCellArray *polys = this->ConstructCellArray(fieldArray[2], this->PolysArrayComponent,
                                                   this->PolysComponentRange);
    if ( polys != NULL )
      {
      pd->SetPolys(polys);
      ncells += polys->GetNumberOfCells();
      polys->Delete();
      }
    }
  
  if ( fieldArray[3] )
    {
    vtkFieldDataToAttributeDataFilter::UpdateComponentRange(fieldArray[3], 
                                                            this->StripsComponentRange);
    vtkCellArray *triStrips = this->ConstructCellArray(fieldArray[3],
                       this->StripsArrayComponent, this->StripsComponentRange);
    if ( triStrips != NULL )
      {
      pd->SetStrips(triStrips);
      ncells += triStrips->GetNumberOfCells();
      triStrips->Delete();
      }
    }

  return ncells;
}

int vtkDataObjectToDataSetFilter::ConstructCells(vtkUnstructuredGrid *ug)
{
  int i, *types, typesAllocated=0;
  vtkDataArray *fieldArray[2];
  int ncells;
  vtkFieldData *fd=this->GetInput()->GetFieldData();

  fieldArray[0] = vtkFieldDataToAttributeDataFilter::GetFieldArray(fd, this->CellTypeArray, 
                                                        this->CellTypeArrayComponent);

   if ( fieldArray[0] == NULL )
    {
    vtkErrorMacro(<<"Can't find array requested for cell types");
    return 0;
    }
  
  vtkFieldDataToAttributeDataFilter::UpdateComponentRange(fieldArray[0], 
                                                          this->CellTypeComponentRange);
  ncells = this->CellTypeComponentRange[1] - this->CellTypeComponentRange[0] + 1;

  fieldArray[1] = vtkFieldDataToAttributeDataFilter::GetFieldArray(fd, 
                                                        this->CellConnectivityArray, 
                                                        this->CellConnectivityArrayComponent);
  if ( fieldArray[1] == NULL )
    {
    vtkErrorMacro(<<"Can't find array requested for cell connectivity");
    return 0;
    }
  
  // Okay, let's piece it together
  if ( fieldArray[0] ) //cell types defined
    {
    // first we create the integer array of types
    vtkDataArray *da=fieldArray[0];
    
    if ( da->GetDataType() == VTK_INT && da->GetNumberOfComponents() == 1 && 
         this->CellTypeArrayComponent == 0 && this->CellTypeComponentRange[0] == 0 && 
         this->CellTypeComponentRange[1] == da->GetMaxId() )
      {
      types = ((vtkIntArray *)da)->GetPointer(0);
      }
    // Otherwise, we'll copy the data by inserting it into a vtkCellArray
    else
      {
      typesAllocated = 1;
      types = new int[ncells];
      for (i=this->CellTypeComponentRange[0]; i <= this->CellTypeComponentRange[1]; i++)
        {
        types[i] = (int) da->GetComponent(i,this->CellTypeArrayComponent);
        }
      }

    // create connectivity
    if ( fieldArray[1] ) //cell connectivity defined
      {
      vtkFieldDataToAttributeDataFilter::UpdateComponentRange(fieldArray[1], 
                                         this->CellConnectivityComponentRange);
      vtkCellArray *carray = this->ConstructCellArray(fieldArray[1],
               this->CellConnectivityArrayComponent, this->CellConnectivityComponentRange);
      if ( carray != NULL ) //insert into unstructured grid
        {
        ug->SetCells(types,carray);
        carray->Delete();
        }
      }
    if ( typesAllocated ) delete [] types;
    }
  
  return ncells;
}

vtkCellArray *vtkDataObjectToDataSetFilter::ConstructCellArray(vtkDataArray *da, int comp,
                                                               int compRange[2])
{
  int ncells, i, j, npts, min, max, numComp=da->GetNumberOfComponents();
  vtkCellArray *carray;

  min = 0;
  max = da->GetMaxId();

  if ( comp < 0 || comp >= numComp )
    {
    vtkErrorMacro(<<"Bad component specification");
    return NULL;
    }

  carray = vtkCellArray::New();

  // If the data type is an integer, and the number of components is 1, then
  // we can directly use the data array without copying it. We just have to
  // figure out how many cells we have.
  if ( da->GetDataType() == VTK_INT && da->GetNumberOfComponents() == 1 
    && comp == 0 && compRange[0] == 0 && compRange[1] == max )
    {
    vtkIntArray *ia = (vtkIntArray *)da;
    for (ncells=i=0; i<ia->GetMaxId(); i+=(npts+1))
      {
      ncells++;
      npts = ia->GetValue(i);
      }
    carray->SetCells(ncells,ia);
    }
  // Otherwise, we'll copy the data by inserting it into a vtkCellArray
  else
    {
    for (ncells=0, i=min; i<max; i+=(npts+1))
      {
      ncells++;
      npts = (int) da->GetComponent(i,comp);
      if ( npts <= 0 )
        {
        vtkErrorMacro(<<"Error constructing cell array");
        carray->Delete();
        return NULL;
        }
      else
        {
        carray->InsertNextCell(npts);
        for (j=1; j<=npts; j++)
          {
          carray->InsertCellPoint((int)da->GetComponent(i+j,comp));
          }
        }
      }
    }
  
  return carray;
}

// Alternative methods for Dimensions, Spacing, and Origin ---------------------------------
//
void vtkDataObjectToDataSetFilter::SetDimensionsComponent(char *arrayName, int arrayComp, 
							  int min, int max)
{
  vtkFieldDataToAttributeDataFilter::SetArrayName(this, this->DimensionsArray, arrayName);
  if ( this->DimensionsArrayComponent != arrayComp )
    {
    this->DimensionsArrayComponent = arrayComp;
    this->Modified();
    }
  if ( this->DimensionsComponentRange[0] != min )
    {
    this->DimensionsComponentRange[0] = min;
    this->Modified();
    }
  if ( this->DimensionsComponentRange[1] != max )
    {
    this->DimensionsComponentRange[1] = max;
    this->Modified();
    }
}

void vtkDataObjectToDataSetFilter::SetSpacingComponent(char *arrayName, int arrayComp, 
						       int min, int max)
{
  vtkFieldDataToAttributeDataFilter::SetArrayName(this, this->SpacingArray, arrayName);
  if ( this->SpacingArrayComponent != arrayComp )
    {
    this->SpacingArrayComponent = arrayComp;
    this->Modified();
    }
  if ( this->SpacingComponentRange[0] != min )
    {
    this->SpacingComponentRange[0] = min;
    this->Modified();
    }
  if ( this->SpacingComponentRange[1] != max )
    {
    this->SpacingComponentRange[1] = max;
    this->Modified();
    }
}

void vtkDataObjectToDataSetFilter::SetOriginComponent(char *arrayName, int arrayComp, 
						      int min, int max)
{
  vtkFieldDataToAttributeDataFilter::SetArrayName(this, this->OriginArray, arrayName);
  if ( this->OriginArrayComponent != arrayComp )
    {
    this->OriginArrayComponent = arrayComp;
    this->Modified();
    }
  if ( this->OriginComponentRange[0] != min )
    {
    this->OriginComponentRange[0] = min;
    this->Modified();
    }
  if ( this->OriginComponentRange[1] != max )
    {
    this->OriginComponentRange[1] = max;
    this->Modified();
    }
}

void vtkDataObjectToDataSetFilter::ConstructDimensions()
{
  if ( this->DimensionsArray == NULL || this->DimensionsArrayComponent < 0 )
    {
    return; //assume dimensions have been set
    }
  else
    {
    vtkFieldData *fd=this->GetInput()->GetFieldData();
    vtkDataArray *fieldArray = vtkFieldDataToAttributeDataFilter::GetFieldArray(fd,
                                          this->DimensionsArray, this->DimensionsArrayComponent);
    if ( fieldArray == NULL )
      {
      vtkErrorMacro(<<"Can't find array requested for dimensions");
      return;
      }

    vtkFieldDataToAttributeDataFilter::UpdateComponentRange(fieldArray, 
                                                            this->DimensionsComponentRange);
    
    for (int i=0; i<3; i++)
      {
      this->Dimensions[i] = fieldArray->GetComponent(this->DimensionsComponentRange[0]+i,
                                                     this->DimensionsArrayComponent);
      }
    }
}

void vtkDataObjectToDataSetFilter::ConstructSpacing()
{
  if ( this->SpacingArray == NULL || this->SpacingArrayComponent < 0 )
    {
    return; //assume Spacing have been set
    }
  else
    {
    vtkFieldData *fd=this->GetInput()->GetFieldData();
    vtkDataArray *fieldArray = vtkFieldDataToAttributeDataFilter::GetFieldArray(fd,
                                          this->SpacingArray, this->SpacingArrayComponent);
    if ( fieldArray == NULL )
      {
      vtkErrorMacro(<<"Can't find array requested for Spacing");
      return;
      }

    vtkFieldDataToAttributeDataFilter::UpdateComponentRange(fieldArray, 
                                                            this->SpacingComponentRange);
    
    for (int i=0; i<3; i++)
      {
      this->Spacing[i] = fieldArray->GetComponent(this->SpacingComponentRange[0]+i,
                                                     this->SpacingArrayComponent);
      }
    }
}

void vtkDataObjectToDataSetFilter::ConstructOrigin()
{
  if ( this->OriginArray == NULL || this->OriginArrayComponent < 0 )
    {
    return; //assume Origin have been set
    }
  else
    {
    vtkFieldData *fd=this->GetInput()->GetFieldData();
    vtkDataArray *fieldArray = vtkFieldDataToAttributeDataFilter::GetFieldArray(fd,
                                          this->OriginArray, this->OriginArrayComponent);
    if ( fieldArray == NULL )
      {
      vtkErrorMacro(<<"Can't find array requested for Origin");
      return;
      }

    vtkFieldDataToAttributeDataFilter::UpdateComponentRange(fieldArray, 
                                                            this->OriginComponentRange);
    
    for (int i=0; i<3; i++)
      {
      this->Origin[i] = fieldArray->GetComponent(this->OriginComponentRange[0]+i,
                                                     this->OriginArrayComponent);
      }
    }
}


