/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataObjectToDataSetFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkDataObjectToDataSetFilter.h"
#include "vtkFieldDataToAttributeDataFilter.h"
#include "vtkPolyData.h"
#include "vtkStructuredPoints.h"
#include "vtkStructuredGrid.h"
#include "vtkRectilinearGrid.h"
#include "vtkUnstructuredGrid.h"
#include "vtkCellArray.h"
#include "vtkObjectFactory.h"



//----------------------------------------------------------------------------
vtkDataObjectToDataSetFilter* vtkDataObjectToDataSetFilter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkDataObjectToDataSetFilter");
  if(ret)
    {
    return (vtkDataObjectToDataSetFilter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkDataObjectToDataSetFilter;
}




//----------------------------------------------------------------------------
// Instantiate object with no input and no defined output.
vtkDataObjectToDataSetFilter::vtkDataObjectToDataSetFilter()
{
  int i;
  this->Updating = 0;

  this->NumberOfRequiredInputs = 1;
  this->DataSetType = VTK_POLY_DATA;
  this->vtkSource::SetNthOutput(0,vtkPolyData::New());
  // Releasing data for pipeline parallism.
  // Filters will know it is empty. 
  this->Outputs[0]->ReleaseData();
  this->Outputs[0]->Delete();
  
  for (i=0; i < 3; i++)
    {
    this->PointArrays[i] = NULL;
    this->PointArrayComponents[i] = -1; //uninitialized
    this->PointComponentRange[i][0] = this->PointComponentRange[i][1] = -1;
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
  
  this->Dimensions[0] = this->Dimensions[1] = this->Dimensions[2] = 0;
  this->Spacing[0] = this->Spacing[1] = this->Spacing[2] = 0.0;
  this->Origin[0] = this->Origin[1] = this->Origin[2] = 0.0;
}

//----------------------------------------------------------------------------
vtkDataObjectToDataSetFilter::~vtkDataObjectToDataSetFilter()
{
  int i;

  for (i=0; i<3; i++) 
    {
    if ( this->PointArrays[i] != NULL )
      {
      delete [] this->PointArrays[i];
      }
    }
  if ( this->VertsArray != NULL )
    {
    delete [] this->VertsArray;
    }
  if ( this->LinesArray != NULL )
    {
    delete [] this->LinesArray;
    }
  if ( this->PolysArray != NULL )
    {
    delete [] this->PolysArray;
    }
  if ( this->StripsArray != NULL )
    {
    delete [] this->StripsArray;
    }
  if ( this->CellTypeArray != NULL )
    {
    delete [] this->CellTypeArray;
    }
  if ( this->CellConnectivityArray != NULL )
    {
    delete [] this->CellConnectivityArray;
    }
  if ( this->DimensionsArray != NULL )
    {
    delete [] this->DimensionsArray;
    }
  if ( this->SpacingArray != NULL )
    {
    delete [] this->SpacingArray;
    }
  if ( this->OriginArray != NULL )
    {
    delete [] this->OriginArray;
    }
}

void vtkDataObjectToDataSetFilter::SetDataSetType(int dt)
{
  if (dt == this->DataSetType)
    {
    return;
    }
  
  switch (dt)
    {
    case VTK_POLY_DATA:
      this->SetNthOutput(0,vtkPolyData::New());
      this->Outputs[0]->Delete();
      break;
    case VTK_STRUCTURED_GRID:
      this->SetNthOutput(0,vtkStructuredGrid::New());
      this->Outputs[0]->Delete();
      break;
    case VTK_STRUCTURED_POINTS:
      this->SetNthOutput(0,vtkStructuredPoints::New());
      this->Outputs[0]->Delete();
      break;
    case VTK_UNSTRUCTURED_GRID:
      this->SetNthOutput(0,vtkUnstructuredGrid::New());
      this->Outputs[0]->Delete();
      break;
    case VTK_RECTILINEAR_GRID:
      this->SetNthOutput(0,vtkRectilinearGrid::New());
      this->Outputs[0]->Delete();
      break;
    default:
      vtkWarningMacro("unknown type in SetDataSetType");
    }
  this->DataSetType = dt;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkDataObjectToDataSetFilter::SetInput(vtkDataObject *input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
vtkDataObject *vtkDataObjectToDataSetFilter::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return (vtkDataObject *)(this->Inputs[0]);
}


//----------------------------------------------------------------------------
void vtkDataObjectToDataSetFilter::ExecuteInformation()
{
  vtkDataObject *input = this->GetInput();

  switch (this->DataSetType)
    {
    case VTK_POLY_DATA:
      break;

    case VTK_STRUCTURED_POINTS:
      // We need the array to get the dimensions
      input->Update();
      this->ConstructDimensions();
      this->ConstructSpacing();
      this->ConstructOrigin();
      
      this->GetStructuredPointsOutput()->SetWholeExtent(
		  0, this->Dimensions[0]-1, 0, this->Dimensions[1]-1, 
                  0, this->Dimensions[2]-1);
      this->GetStructuredPointsOutput()->SetOrigin(this->Origin);
      this->GetStructuredPointsOutput()->SetSpacing(this->Spacing);
      break;

    case VTK_STRUCTURED_GRID:
      // We need the array to get the dimensions
      input->Update();
      this->ConstructDimensions();
      this->GetStructuredGridOutput()->SetWholeExtent(
		  0, this->Dimensions[0]-1, 0, this->Dimensions[1]-1, 
                  0, this->Dimensions[2]-1);
      break;

    case VTK_RECTILINEAR_GRID:
      // We need the array to get the dimensions
      input->Update();
      this->ConstructDimensions();
      this->GetRectilinearGridOutput()->SetWholeExtent(
		  0, this->Dimensions[0]-1, 0, this->Dimensions[1]-1, 
                  0, this->Dimensions[2]-1);
      break;

    case VTK_UNSTRUCTURED_GRID:
      break;

    default:
      vtkErrorMacro(<<"Unsupported dataset type!");
    }
}

//----------------------------------------------------------------------------
void vtkDataObjectToDataSetFilter::Execute()
{
  int npts;
  vtkDataObject *input = this->GetInput();

  vtkDebugMacro(<<"Generating dataset from field data");

  switch (this->DataSetType)
    {
    case VTK_POLY_DATA:
      if ( (npts=this->ConstructPoints(this->GetPolyDataOutput())) ) 
        {
        this->ConstructCells(this->GetPolyDataOutput());
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
      
      this->GetStructuredPointsOutput()->SetDimensions(this->Dimensions);
      this->GetStructuredPointsOutput()->SetOrigin(this->Origin);
      this->GetStructuredPointsOutput()->SetSpacing(this->Spacing);
      break;

    case VTK_STRUCTURED_GRID:
      if ( (npts=this->ConstructPoints(this->GetStructuredGridOutput())) )
        {
        this->ConstructDimensions();
        if ( npts == (this->Dimensions[0] * this->Dimensions[1] * this->Dimensions[2]) )
          {
          this->GetStructuredGridOutput()->SetDimensions(this->Dimensions);
          }
        else
          {
          vtkErrorMacro(<<"Number of points don't match dimensions");
          }
        }
      break;

    case VTK_RECTILINEAR_GRID:
      if ( (npts=this->ConstructPoints(this->GetRectilinearGridOutput())) )
        {
        this->ConstructDimensions();
        if ( npts == (this->Dimensions[0] * this->Dimensions[1] * this->Dimensions[2]) )
          {
          this->GetRectilinearGridOutput()->SetDimensions(this->Dimensions);
          }
        else
          {
          vtkErrorMacro(<<"Number of points don't match dimensions");
          }
        }
      break;

    case VTK_UNSTRUCTURED_GRID:
      if ( this->ConstructPoints(this->GetUnstructuredGridOutput()) ) 
        {
        this->ConstructCells(this->GetUnstructuredGridOutput());
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
  if ( this->GetOutput() )
    {
    this->GetOutput()->SetFieldData(input->GetFieldData());
    }
}

// Get the output as vtkPolyData.
vtkPolyData *vtkDataObjectToDataSetFilter::GetPolyDataOutput() 
{
  vtkDataSet *ds = this->GetOutput();
  if (!ds) 
    {
    return NULL;
    }
  if (ds->GetDataObjectType() == VTK_POLY_DATA)
    {
    return (vtkPolyData *)ds;
    }
  return NULL;
}

//----------------------------------------------------------------------------
vtkDataSet *vtkDataObjectToDataSetFilter::GetOutput()
{
  if (this->NumberOfOutputs < 1)
    {
    return NULL;
    }
  
  return (vtkDataSet *)(this->Outputs[0]);
}

// Get the output as vtkStructuredPoints.
vtkStructuredPoints *vtkDataObjectToDataSetFilter::GetStructuredPointsOutput() 
{
  vtkDataSet *ds = this->GetOutput();
  if (!ds) 
    {
    return NULL;
    }
  if (ds->GetDataObjectType() == VTK_STRUCTURED_POINTS)
    {
    return (vtkStructuredPoints *)ds;
    }
  return NULL;
}

// Get the output as vtkStructuredGrid.
vtkStructuredGrid *vtkDataObjectToDataSetFilter::GetStructuredGridOutput()
{
  vtkDataSet *ds = this->GetOutput();
  if (!ds) 
    {
    return NULL;
    }
  if (ds->GetDataObjectType() == VTK_STRUCTURED_GRID)
    {
    return (vtkStructuredGrid *)ds;
    }
  return NULL;
}

// Get the output as vtkUnstructuredGrid.
vtkUnstructuredGrid *vtkDataObjectToDataSetFilter::GetUnstructuredGridOutput()
{
  vtkDataSet *ds = this->GetOutput();
  if (!ds) 
    {
    return NULL;
    }
  if (ds->GetDataObjectType() == VTK_UNSTRUCTURED_GRID)
    {
    return (vtkUnstructuredGrid *)ds;
    }
  return NULL;
}

// Get the output as vtkRectilinearGrid. 
vtkRectilinearGrid *vtkDataObjectToDataSetFilter::GetRectilinearGridOutput()
{
  vtkDataSet *ds = this->GetOutput();
  if (!ds) 
    {
    return NULL;
    }
  if (ds->GetDataObjectType() == VTK_RECTILINEAR_GRID)
    {
    return (vtkRectilinearGrid *)ds;
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkDataObjectToDataSetFilter::ComputeInputUpdateExtents( 
				       vtkDataObject *vtkNotUsed(output))
{
  if (this->GetInput())
    {
    // what should we do here?
    if (this->GetInput()->GetDataObjectType() != VTK_DATA_OBJECT)
      {
      this->GetInput()->SetUpdateExtent(0, 1, 0);
      }
    this->GetInput()->RequestExactExtentOn();
    }
}

//----------------------------------------------------------------------------
void vtkDataObjectToDataSetFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkSource::PrintSelf(os,indent);

  os << indent << "Data Set Type: ";
  if ( this->DataSetType == VTK_POLY_DATA )
    {
    os << "vtkPolyData\n";
    }
  else if ( this->DataSetType == VTK_STRUCTURED_POINTS )
    {
    os << "vtkStructuredPoints\n";
    }
  else if ( this->DataSetType == VTK_STRUCTURED_GRID )
    {
    os << "vtkStructuredGrid\n";
    }
  else if ( this->DataSetType == VTK_RECTILINEAR_GRID )
    {
    os << "vtkRectilinearGrid\n";
    }
  else // if ( this->DataSetType == VTK_UNSTRUCTURED_GRID )
    {
    os << "vtkUnstructuredGrid\n";
    }
  
  os << indent << "Dimensions: (" << this->Dimensions[0] << ", "
                                  << this->Dimensions[1] << ", "
                                  << this->Dimensions[2] << ")\n";

  os << indent << "Spacing: (" << this->Spacing[0] << ", "
                               << this->Spacing[1] << ", "
                               << this->Spacing[2] << ")\n";

  os << indent << "Origin: (" << this->Origin[0] << ", "
                              << this->Origin[1] << ", "
                              << this->Origin[2] << ")\n";

  os << indent << "Default Normalize: " 
     << (this->DefaultNormalize ? "On\n" : "Off\n");
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
const char *vtkDataObjectToDataSetFilter::GetPointComponentArrayName(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 2 ? 2 : comp));
  return this->PointArrays[comp];
}

//----------------------------------------------------------------------------
int vtkDataObjectToDataSetFilter::GetPointComponentArrayComponent(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 2 ? 2 : comp));
  return this->PointArrayComponents[comp];
}

//----------------------------------------------------------------------------
int vtkDataObjectToDataSetFilter::GetPointComponentMinRange(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 2 ? 2 : comp));
  return this->PointComponentRange[comp][0];
}

//----------------------------------------------------------------------------
int vtkDataObjectToDataSetFilter::GetPointComponentMaxRange(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 2 ? 2 : comp));
  return this->PointComponentRange[comp][1];
}

//----------------------------------------------------------------------------
int vtkDataObjectToDataSetFilter::GetPointComponentNormailzeFlag(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 2 ? 2 : comp));
  return this->PointNormalize[comp];
}

//----------------------------------------------------------------------------
int vtkDataObjectToDataSetFilter::ConstructPoints(vtkPointSet *ps)
{
  int i, updated=0;
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
    updated |= vtkFieldDataToAttributeDataFilter::UpdateComponentRange(fieldArray[i], 
                                                            this->PointComponentRange[i]);
    }

  npts = this->PointComponentRange[0][1] - this->PointComponentRange[0][0] + 1;
  if ( npts != (this->PointComponentRange[1][1] - this->PointComponentRange[1][0] + 1) ||
       npts != (this->PointComponentRange[2][1] - this->PointComponentRange[2][0] + 1) )
    {
    vtkErrorMacro(<<"Number of point components not consistent");
    return 0;
    }

  // Try using the arrays directly if possible; otherwise copy data
  vtkPoints *newPts = vtkPoints::New();
  if ( fieldArray[0]->GetNumberOfComponents() == 3 && 
       fieldArray[0] == fieldArray[1] && fieldArray[1] == fieldArray[2] &&
       fieldArray[0]->GetNumberOfTuples() == npts &&
       !this->PointNormalize[0] && !this->PointNormalize[1] && !this->PointNormalize[2] )
    {
    newPts->SetData(fieldArray[0]);
    }
  else //have to copy data into created array
    {
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
    }

  ps->SetPoints(newPts);
  newPts->Delete();
  if ( updated ) //reset for next execution pass
    {
    for (i=0; i < 3; i++)
      {
      this->PointComponentRange[i][0] = this->PointComponentRange[i][1] = -1;
      }
    }
  
  return npts;
}

//----------------------------------------------------------------------------
int vtkDataObjectToDataSetFilter::ConstructPoints(vtkRectilinearGrid *rg)
{
  int i, nXpts, nYpts, nZpts, npts, updated=0;
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
    updated |= vtkFieldDataToAttributeDataFilter::UpdateComponentRange(fieldArray[i], 
                                                            this->PointComponentRange[i]);
    }

  nXpts = this->PointComponentRange[0][1] - this->PointComponentRange[0][0] + 1;
  nYpts = this->PointComponentRange[1][1] - this->PointComponentRange[1][0] + 1;
  nZpts = this->PointComponentRange[2][1] - this->PointComponentRange[2][0] + 1;
  npts = nXpts * nYpts * nZpts;
  
  // Create the coordinate arrays
  vtkDataArray *XPts;
  vtkDataArray *YPts;
  vtkDataArray *ZPts;

  // Decide whether to use the field array or whether to copy data
  // First look at the x-coordinates
  if ( fieldArray[0]->GetNumberOfComponents() == 1 && 
       fieldArray[0]->GetNumberOfTuples() == nXpts &&
       !this->PointNormalize[0] )
    {
    XPts = fieldArray[0];
    XPts->Register(this);
    }
  else //have to copy data into created array
    {
    XPts = vtkDataArray::CreateDataArray(
      vtkFieldDataToAttributeDataFilter::GetComponentsType(1, fieldArray));
    XPts->SetNumberOfComponents(1);
    XPts->SetNumberOfTuples(nXpts);

    if ( vtkFieldDataToAttributeDataFilter::ConstructArray(
      XPts, 0, fieldArray[0], this->PointArrayComponents[0],
      this->PointComponentRange[0][0],
      this->PointComponentRange[0][1],
      this->PointNormalize[0]) == 0 )
      {
      XPts->Delete();
      return 0;
      }
    }
  
  // Look at the y-coordinates
  if ( fieldArray[1]->GetNumberOfComponents() == 1 && 
       fieldArray[1]->GetNumberOfTuples() == nYpts &&
       !this->PointNormalize[1] )
    {
    YPts = fieldArray[1];
    YPts->Register(this);
    }
  else //have to copy data into created array
    {
    YPts = vtkDataArray::CreateDataArray(
      vtkFieldDataToAttributeDataFilter::GetComponentsType(1, fieldArray+1));
    YPts->SetNumberOfComponents(1);
    YPts->SetNumberOfTuples(nYpts);

    if ( vtkFieldDataToAttributeDataFilter::ConstructArray(
      YPts, 0, fieldArray[1], this->PointArrayComponents[1],
      this->PointComponentRange[1][0],
      this->PointComponentRange[1][1],
      this->PointNormalize[1]) == 0 )
      {
      XPts->Delete(); YPts->Delete();
      return 0;
      }
    }
  
  // Look at the z-coordinates
  if ( fieldArray[2]->GetNumberOfComponents() == 1 && 
       fieldArray[2]->GetNumberOfTuples() == nZpts &&
       !this->PointNormalize[2] )
    {
    ZPts = fieldArray[2];
    ZPts->Register(this);
    }
  else //have to copy data into created array
    {
    ZPts = vtkDataArray::CreateDataArray(
      vtkFieldDataToAttributeDataFilter::GetComponentsType(1, fieldArray+2));
    ZPts->SetNumberOfComponents(1);
    ZPts->SetNumberOfTuples(nZpts);

    if ( vtkFieldDataToAttributeDataFilter::ConstructArray(
      ZPts, 0, fieldArray[2], this->PointArrayComponents[2],
      this->PointComponentRange[2][0],
      this->PointComponentRange[2][1],
      this->PointNormalize[2]) == 0 )
      {
      XPts->Delete(); YPts->Delete(); ZPts->Delete(); 
      return 0;
      }
    }
  
  rg->SetXCoordinates(XPts);
  rg->SetYCoordinates(YPts);
  rg->SetZCoordinates(ZPts);
  XPts->Delete(); YPts->Delete(); ZPts->Delete(); 
  
  if ( updated ) //reset for next execution pass
    {
    for (i=0; i < 3; i++)
      {
      this->PointComponentRange[i][0] = this->PointComponentRange[i][1] = -1;
      }
    }

  return npts;
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
const char *vtkDataObjectToDataSetFilter::GetVertsComponentArrayName()
{
  return this->VertsArray;
}

//----------------------------------------------------------------------------
int vtkDataObjectToDataSetFilter::GetVertsComponentArrayComponent()
{
  return this->VertsArrayComponent;
}

//----------------------------------------------------------------------------
int vtkDataObjectToDataSetFilter::GetVertsComponentMinRange()
{
  return this->VertsComponentRange[0];
}

//----------------------------------------------------------------------------
int vtkDataObjectToDataSetFilter::GetVertsComponentMaxRange()
{
  return this->VertsComponentRange[1];
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
const char *vtkDataObjectToDataSetFilter::GetLinesComponentArrayName()
{
  return this->LinesArray;
}

//----------------------------------------------------------------------------
int vtkDataObjectToDataSetFilter::GetLinesComponentArrayComponent()
{
  return this->LinesArrayComponent;
}

//----------------------------------------------------------------------------
int vtkDataObjectToDataSetFilter::GetLinesComponentMinRange()
{
  return this->LinesComponentRange[0];
}

//----------------------------------------------------------------------------
int vtkDataObjectToDataSetFilter::GetLinesComponentMaxRange()
{
  return this->LinesComponentRange[1];
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
const char *vtkDataObjectToDataSetFilter::GetPolysComponentArrayName()
{
  return this->PolysArray;
}

//----------------------------------------------------------------------------
int vtkDataObjectToDataSetFilter::GetPolysComponentArrayComponent()
{
  return this->PolysArrayComponent;
}

//----------------------------------------------------------------------------
int vtkDataObjectToDataSetFilter::GetPolysComponentMinRange()
{
  return this->PolysComponentRange[0];
}

//----------------------------------------------------------------------------
int vtkDataObjectToDataSetFilter::GetPolysComponentMaxRange()
{
  return this->PolysComponentRange[1];
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
const char *vtkDataObjectToDataSetFilter::GetStripsComponentArrayName()
{
  return this->StripsArray;
}

//----------------------------------------------------------------------------
int vtkDataObjectToDataSetFilter::GetStripsComponentArrayComponent()
{
  return this->StripsArrayComponent;
}

//----------------------------------------------------------------------------
int vtkDataObjectToDataSetFilter::GetStripsComponentMinRange()
{
  return this->StripsComponentRange[0];
}

//----------------------------------------------------------------------------
int vtkDataObjectToDataSetFilter::GetStripsComponentMaxRange()
{
  return this->StripsComponentRange[1];
}

//----------------------------------------------------------------------------
// Stuff related to vtkUnstructuredGrid --------------------------------------
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

//----------------------------------------------------------------------------
const char *vtkDataObjectToDataSetFilter::GetCellTypeComponentArrayName()
{
  return this->CellTypeArray;
}

//----------------------------------------------------------------------------
int vtkDataObjectToDataSetFilter::GetCellTypeComponentArrayComponent()
{
  return this->CellTypeArrayComponent;
}

//----------------------------------------------------------------------------
int vtkDataObjectToDataSetFilter::GetCellTypeComponentMinRange()
{
  return this->CellTypeComponentRange[0];
}

//----------------------------------------------------------------------------
int vtkDataObjectToDataSetFilter::GetCellTypeComponentMaxRange()
{
  return this->CellTypeComponentRange[1];
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
const char *vtkDataObjectToDataSetFilter::GetCellConnectivityComponentArrayName()
{
  return this->CellConnectivityArray;
}

//----------------------------------------------------------------------------
int vtkDataObjectToDataSetFilter::GetCellConnectivityComponentArrayComponent()
{
  return this->CellConnectivityArrayComponent;
}

//----------------------------------------------------------------------------
int vtkDataObjectToDataSetFilter::GetCellConnectivityComponentMinRange()
{
  return this->CellConnectivityComponentRange[0];
}

//----------------------------------------------------------------------------
int vtkDataObjectToDataSetFilter::GetCellConnectivityComponentMaxRange()
{
  return this->CellConnectivityComponentRange[1];
}

//----------------------------------------------------------------------------
int vtkDataObjectToDataSetFilter::ConstructCells(vtkPolyData *pd)
{
  int ncells=0;
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
    this->VertsComponentRange[0] = this->VertsComponentRange[1] = -1;
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
    this->LinesComponentRange[0] = this->LinesComponentRange[1] = -1;
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
    this->PolysComponentRange[0] = this->PolysComponentRange[1] = -1;
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
    this->StripsComponentRange[0] = this->StripsComponentRange[1] = -1;
    }

  return ncells;
}

//----------------------------------------------------------------------------
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
    this->CellTypeComponentRange[0] = this->CellTypeComponentRange[1] = -1;

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
      this->CellConnectivityComponentRange[0] = this->CellConnectivityComponentRange[1] = -1;
      }
    if ( typesAllocated )
      {
      delete [] types;
      }
    }
  
  return ncells;
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
// Alternative methods for Dimensions, Spacing, and Origin -------------------
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

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
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
      this->Dimensions[i] = (int)(fieldArray->GetComponent(this->DimensionsComponentRange[0]+i,
                                                     this->DimensionsArrayComponent));
      }
    }

  this->DimensionsComponentRange[0] = this->DimensionsComponentRange[1] = -1;
}

//----------------------------------------------------------------------------
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
  this->SpacingComponentRange[0] = this->SpacingComponentRange[1] = -1;
}

//----------------------------------------------------------------------------
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

  this->OriginComponentRange[0] = this->OriginComponentRange[1] = -1;
}


