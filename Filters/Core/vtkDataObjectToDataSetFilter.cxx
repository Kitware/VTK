/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataObjectToDataSetFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDataObjectToDataSetFilter.h"

#include "vtkFieldData.h"
#include "vtkFieldDataToAttributeDataFilter.h"
#include "vtkInformationExecutivePortKey.h"
#include "vtkPolyData.h"
#include "vtkStructuredPoints.h"
#include "vtkStructuredGrid.h"
#include "vtkRectilinearGrid.h"
#include "vtkUnstructuredGrid.h"
#include "vtkCellArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkDataObjectToDataSetFilter);

//----------------------------------------------------------------------------
// Instantiate object with no input and no defined output.
vtkDataObjectToDataSetFilter::vtkDataObjectToDataSetFilter()
{
  int i;
  this->Updating = 0;

  this->DataSetType = VTK_POLY_DATA;
  vtkPolyData *output = vtkPolyData::New();
  this->GetExecutive()->SetOutputData(0,output);
  // Releasing data for pipeline parallism.
  // Filters will know it is empty.
  output->ReleaseData();
  output->Delete();

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
  this->CellConnectivityComponentRange[0] =
    this->CellConnectivityComponentRange[1] = -1;

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
  for (int i=0; i<3; i++)
  {
    delete [] this->PointArrays[i];
  }
  delete [] this->VertsArray;
  delete [] this->LinesArray;
  delete [] this->PolysArray;
  delete [] this->StripsArray;
  delete [] this->CellTypeArray;
  delete [] this->CellConnectivityArray;
  delete [] this->DimensionsArray;
  delete [] this->SpacingArray;
  delete [] this->OriginArray;
}

void vtkDataObjectToDataSetFilter::SetDataSetType(int dt)
{
  if (dt == this->DataSetType)
  {
    return;
  }

  vtkDataSet *output;
  switch (dt)
  {
    case VTK_POLY_DATA:
      output = vtkPolyData::New();
      this->GetExecutive()->SetOutputData(0,output);
      output->Delete();
      break;
    case VTK_STRUCTURED_GRID:
      output = vtkStructuredGrid::New();
      this->GetExecutive()->SetOutputData(0,output);
      output->Delete();
      break;
    case VTK_STRUCTURED_POINTS:
      output = vtkStructuredPoints::New();
      this->GetExecutive()->SetOutputData(0,output);
      output->Delete();
      break;
    case VTK_UNSTRUCTURED_GRID:
      output = vtkUnstructuredGrid::New();
      this->GetExecutive()->SetOutputData(0,output);
      output->Delete();
      break;
    case VTK_RECTILINEAR_GRID:
      output = vtkRectilinearGrid::New();
      this->GetExecutive()->SetOutputData(0,output);
      output->Delete();
      break;
    default:
      vtkWarningMacro("unknown type in SetDataSetType");
  }
  this->DataSetType = dt;
  this->Modified();
}

//----------------------------------------------------------------------------
vtkDataObject *vtkDataObjectToDataSetFilter::GetInput()
{
  if (this->GetNumberOfInputConnections(0) < 1)
  {
    return NULL;
  }

  return this->GetExecutive()->GetInputData(0, 0);
}

//----------------------------------------------------------------------------
int vtkDataObjectToDataSetFilter::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  vtkDataObject *input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkExecutive* inputExec =
    vtkExecutive::PRODUCER()->GetExecutive(inInfo);

  switch (this->DataSetType)
  {
    case VTK_POLY_DATA:
      break;

    case VTK_STRUCTURED_POINTS:
      // We need the array to get the dimensions
      inputExec->Update();
      this->ConstructDimensions(input);
      this->ConstructSpacing(input);
      this->ConstructOrigin(input);

      outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
                   0, this->Dimensions[0]-1, 0, this->Dimensions[1]-1,
                   0, this->Dimensions[2]-1);
      outInfo->Set(vtkDataObject::ORIGIN(), this->Origin, 3);
      outInfo->Set(vtkDataObject::SPACING(), this->Spacing, 3);
      break;

    case VTK_STRUCTURED_GRID:
      // We need the array to get the dimensions
      inputExec->Update();
      this->ConstructDimensions(input);
      outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
                   0, this->Dimensions[0]-1, 0, this->Dimensions[1]-1,
                   0, this->Dimensions[2]-1);
      break;

    case VTK_RECTILINEAR_GRID:
      // We need the array to get the dimensions
      inputExec->Update();
      this->ConstructDimensions(input);
      outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
                   0, this->Dimensions[0]-1, 0, this->Dimensions[1]-1,
                   0, this->Dimensions[2]-1);
      break;

    case VTK_UNSTRUCTURED_GRID:
      break;

    default:
      vtkErrorMacro(<<"Unsupported dataset type!");
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkDataObjectToDataSetFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataObject *input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkDataSet *output = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkIdType npts;

  vtkDebugMacro(<<"Generating dataset from field data");

  switch (this->DataSetType)
  {
    case VTK_POLY_DATA:
      if ( (npts=this->ConstructPoints(input,
                                       vtkPolyData::SafeDownCast(output))) )
      {
        this->ConstructCells(input, vtkPolyData::SafeDownCast(output));
      }
      else
      {
        vtkErrorMacro(<<"Couldn't create any points");
      }
      break;

    case VTK_STRUCTURED_POINTS:
    {
      this->ConstructDimensions(input);
      this->ConstructSpacing(input);
      this->ConstructOrigin(input);
      vtkStructuredPoints *sp = vtkStructuredPoints::SafeDownCast(output);
      sp->SetDimensions(this->Dimensions);
      sp->SetOrigin(this->Origin);
      sp->SetSpacing(this->Spacing);
      break;
    }

    case VTK_STRUCTURED_GRID:
      if ( (npts=this->ConstructPoints(input,
                                       this->GetStructuredGridOutput())) )
      {
        this->ConstructDimensions(input);
        if ( npts == (this->Dimensions[0] * this->Dimensions[1] *
                      this->Dimensions[2]) )
        {
          vtkStructuredGrid *sg = vtkStructuredGrid::SafeDownCast(output);
          sg->SetDimensions(this->Dimensions);
        }
        else
        {
          vtkErrorMacro(<<"Number of points don't match dimensions");
        }
      }
      break;

    case VTK_RECTILINEAR_GRID:
      if ( (npts=this->ConstructPoints(input,
                                       this->GetRectilinearGridOutput())) )
      {
        this->ConstructDimensions(input);
        if ( npts == (this->Dimensions[0] * this->Dimensions[1] *
                      this->Dimensions[2]) )
        {
          vtkRectilinearGrid *rg = vtkRectilinearGrid::SafeDownCast(output);
          rg->SetDimensions(this->Dimensions);
        }
        else
        {
          vtkErrorMacro(<<"Number of points don't match dimensions");
        }
      }
      break;

    case VTK_UNSTRUCTURED_GRID:
      if ( this->ConstructPoints(input,
                                 vtkUnstructuredGrid::SafeDownCast(output)) )
      {
        this->ConstructCells(input, vtkUnstructuredGrid::SafeDownCast(output));
      }
      else
      {
        vtkErrorMacro(<<"Couldn't create any points");
      }
      break;

    default:
      vtkErrorMacro(<<"Unsupported dataset type!");
  }

  vtkFieldData *inFD = input->GetFieldData();
  vtkFieldData *outFD = output->GetFieldData();
  outFD->CopyAllOn();
  outFD->PassData(inFD);

  return 1;
}

// Get the output as vtkPolyData.
vtkPolyData *vtkDataObjectToDataSetFilter::GetPolyDataOutput()
{
  return vtkPolyData::SafeDownCast(this->GetOutput());
}

//----------------------------------------------------------------------------
vtkDataSet *vtkDataObjectToDataSetFilter::GetOutput()
{
  if (this->GetNumberOfOutputPorts() < 1)
  {
    return NULL;
  }

  return vtkDataSet::SafeDownCast(
    this->GetExecutive()->GetOutputData(0));
}

// Get the output as vtkStructuredPoints.
vtkStructuredPoints *vtkDataObjectToDataSetFilter::GetStructuredPointsOutput()
{
  return vtkStructuredPoints::SafeDownCast(this->GetOutput());
}

// Get the output as vtkStructuredGrid.
vtkStructuredGrid *vtkDataObjectToDataSetFilter::GetStructuredGridOutput()
{
  return vtkStructuredGrid::SafeDownCast(this->GetOutput());
}

// Get the output as vtkUnstructuredGrid.
vtkUnstructuredGrid *vtkDataObjectToDataSetFilter::GetUnstructuredGridOutput()
{
  return vtkUnstructuredGrid::SafeDownCast(this->GetOutput());
}

// Get the output as vtkRectilinearGrid.
vtkRectilinearGrid *vtkDataObjectToDataSetFilter::GetRectilinearGridOutput()
{
  return vtkRectilinearGrid::SafeDownCast(this->GetOutput());
}

//----------------------------------------------------------------------------
int vtkDataObjectToDataSetFilter::RequestUpdateExtent(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *vtkNotUsed(outputVector))
{
  // get the info object
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
              0);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);
  return 1;
}

//----------------------------------------------------------------------------
void vtkDataObjectToDataSetFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

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
void vtkDataObjectToDataSetFilter::SetPointComponent(int comp,
                                                     char *arrayName,
                                                     int arrayComp,
                                                     int min, int max,
                                                     int normalize)
{
  if ( comp < 0 || comp > 2 )
  {
    vtkErrorMacro(<<"Point component must be between (0,2)");
    return;
  }

  vtkFieldDataToAttributeDataFilter::SetArrayName(
    this, this->PointArrays[comp], arrayName);
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
vtkIdType vtkDataObjectToDataSetFilter::ConstructPoints(vtkDataObject *input,
                                                        vtkPointSet *ps)
{
  int i, updated=0;
  vtkDataArray *fieldArray[3];
  vtkIdType npts;
  vtkFieldData *fd=input->GetFieldData();

  for ( i=0; i < 3; i++ )
  {
    fieldArray[i] = vtkFieldDataToAttributeDataFilter::GetFieldArray(
      fd, this->PointArrays[i],
      this->PointArrayComponents[i]);

    if ( fieldArray[i] == NULL )
    {
      vtkErrorMacro(<<"Can't find array requested");
      return 0;
    }
    updated |= vtkFieldDataToAttributeDataFilter::UpdateComponentRange(
      fieldArray[i],
      this->PointComponentRange[i]);
  }

  npts = this->PointComponentRange[0][1] - this->PointComponentRange[0][0] + 1;
  if ( npts != (this->PointComponentRange[1][1] -
                this->PointComponentRange[1][0] + 1) ||
       npts != (this->PointComponentRange[2][1] -
                this->PointComponentRange[2][0] + 1) )
  {
    vtkErrorMacro(<<"Number of point components not consistent");
    return 0;
  }

  // Try using the arrays directly if possible; otherwise copy data
  vtkPoints *newPts = vtkPoints::New();
  if ( fieldArray[0]->GetNumberOfComponents() == 3 &&
       fieldArray[0] == fieldArray[1] && fieldArray[1] == fieldArray[2] &&
       fieldArray[0]->GetNumberOfTuples() == npts &&
       !this->PointNormalize[0] && !this->PointNormalize[1] &&
       !this->PointNormalize[2] )
  {
    newPts->SetData(fieldArray[0]);
  }
  else //have to copy data into created array
  {
    newPts->SetDataType(
      vtkFieldDataToAttributeDataFilter::GetComponentsType(3, fieldArray));
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
vtkIdType vtkDataObjectToDataSetFilter::ConstructPoints(vtkDataObject *input,
                                                        vtkRectilinearGrid *rg)
{
  int i, nXpts, nYpts, nZpts, updated=0;
  vtkIdType npts;
  vtkDataArray *fieldArray[3];
  vtkFieldData *fd=input->GetFieldData();

  for ( i=0; i < 3; i++ )
  {
    fieldArray[i] = vtkFieldDataToAttributeDataFilter::GetFieldArray(
      fd, this->PointArrays[i],
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
void vtkDataObjectToDataSetFilter::SetVertsComponent(char *arrayName,
                                                     int arrayComp,
                                                     int min, int max)
{
  vtkFieldDataToAttributeDataFilter::SetArrayName(
    this, this->VertsArray, arrayName);
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
void vtkDataObjectToDataSetFilter::SetLinesComponent(char *arrayName,
                                                     int arrayComp,
                                                     int min, int max)
{
  vtkFieldDataToAttributeDataFilter::SetArrayName(
    this, this->LinesArray, arrayName);
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
void vtkDataObjectToDataSetFilter::SetPolysComponent(char *arrayName,
                                                     int arrayComp,
                                                     int min, int max)
{
  vtkFieldDataToAttributeDataFilter::SetArrayName(
    this, this->PolysArray, arrayName);
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
void vtkDataObjectToDataSetFilter::SetStripsComponent(char *arrayName,
                                                      int arrayComp,
                                                      int min, int max)
{
  vtkFieldDataToAttributeDataFilter::SetArrayName(
    this, this->StripsArray, arrayName);
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
  vtkFieldDataToAttributeDataFilter::SetArrayName(
    this, this->CellTypeArray, arrayName);
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
void vtkDataObjectToDataSetFilter::SetCellConnectivityComponent(
  char *arrayName, int arrayComp, int min, int max)
{
  vtkFieldDataToAttributeDataFilter::SetArrayName(
    this, this->CellConnectivityArray, arrayName);
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
const char *
vtkDataObjectToDataSetFilter::GetCellConnectivityComponentArrayName()
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
int vtkDataObjectToDataSetFilter::ConstructCells(vtkDataObject *input,
                                                 vtkPolyData *pd)
{
  vtkIdType ncells=0;
  vtkDataArray *fieldArray[4];
  vtkFieldData *fd=input->GetFieldData();

  fieldArray[0] = vtkFieldDataToAttributeDataFilter::GetFieldArray(
    fd, this->VertsArray, this->VertsArrayComponent);
  if ( this->VertsArray && fieldArray[0] == NULL )
  {
    vtkErrorMacro(<<"Can't find array requested for vertices");
    return 0;
  }

  fieldArray[1] = vtkFieldDataToAttributeDataFilter::GetFieldArray(
    fd, this->LinesArray, this->LinesArrayComponent);
  if ( this->LinesArray && fieldArray[1] == NULL )
  {
    vtkErrorMacro(<<"Can't find array requested for lines");
    return 0;
  }

  fieldArray[2] = vtkFieldDataToAttributeDataFilter::GetFieldArray(
    fd, this->PolysArray, this->PolysArrayComponent);
  if ( this->PolysArray && fieldArray[2] == NULL )
  {
    vtkErrorMacro(<<"Can't find array requested for polygons");
    return 0;
  }

  fieldArray[3] = vtkFieldDataToAttributeDataFilter::GetFieldArray(
    fd, this->StripsArray, this->StripsArrayComponent);
  if ( this->StripsArray && fieldArray[3] == NULL )
  {
    vtkErrorMacro(<<"Can't find array requested for triangle strips");
    return 0;
  }

  if ( fieldArray[0] )
  {
    vtkFieldDataToAttributeDataFilter::UpdateComponentRange(
      fieldArray[0], this->VertsComponentRange);
    vtkCellArray *verts = this->ConstructCellArray(
      fieldArray[0], this->VertsArrayComponent,
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
    vtkFieldDataToAttributeDataFilter::UpdateComponentRange(
      fieldArray[1], this->LinesComponentRange);
    vtkCellArray *lines = this->ConstructCellArray(
      fieldArray[1], this->LinesArrayComponent, this->LinesComponentRange);
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
    vtkFieldDataToAttributeDataFilter::UpdateComponentRange(
      fieldArray[2], this->PolysComponentRange);
    vtkCellArray *polys = this->ConstructCellArray(
      fieldArray[2], this->PolysArrayComponent,
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
    vtkFieldDataToAttributeDataFilter::UpdateComponentRange(
      fieldArray[3], this->StripsComponentRange);
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
int vtkDataObjectToDataSetFilter::ConstructCells(vtkDataObject *input,
                                                 vtkUnstructuredGrid *ug)
{
  int i, *types, typesAllocated=0;
  vtkDataArray *fieldArray[2];
  int ncells;
  vtkFieldData *fd=input->GetFieldData();

  fieldArray[0] = vtkFieldDataToAttributeDataFilter::GetFieldArray(
    fd, this->CellTypeArray,
    this->CellTypeArrayComponent);

   if ( fieldArray[0] == NULL )
   {
    vtkErrorMacro(<<"Can't find array requested for cell types");
    return 0;
   }

  vtkFieldDataToAttributeDataFilter::UpdateComponentRange(
    fieldArray[0],
    this->CellTypeComponentRange);
  ncells = this->CellTypeComponentRange[1] -
    this->CellTypeComponentRange[0] + 1;

  fieldArray[1] = vtkFieldDataToAttributeDataFilter::GetFieldArray(
    fd,
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
         this->CellTypeArrayComponent == 0 &&
         this->CellTypeComponentRange[0] == 0 &&
         this->CellTypeComponentRange[1] == da->GetMaxId() )
    {
      types = static_cast<vtkIntArray *>(da)->GetPointer(0);
    }
    // Otherwise, we'll copy the data by inserting it into a vtkCellArray
    else
    {
      typesAllocated = 1;
      types = new int[ncells];
      for (i=this->CellTypeComponentRange[0];
           i <= this->CellTypeComponentRange[1]; i++)
      {
        types[i] =
          static_cast<int>(da->GetComponent(i,this->CellTypeArrayComponent));
      }
    }
    this->CellTypeComponentRange[0] = this->CellTypeComponentRange[1] = -1;

    // create connectivity
    if ( fieldArray[1] ) //cell connectivity defined
    {
      vtkFieldDataToAttributeDataFilter::UpdateComponentRange(fieldArray[1],
                                         this->CellConnectivityComponentRange);
      vtkCellArray *carray = this->ConstructCellArray(
        fieldArray[1],
        this->CellConnectivityArrayComponent,
        this->CellConnectivityComponentRange);
      if ( carray != NULL ) //insert into unstructured grid
      {
        ug->SetCells(types,carray);
        carray->Delete();
      }
      this->CellConnectivityComponentRange[0]
        = this->CellConnectivityComponentRange[1] = -1;
    }
    if ( typesAllocated )
    {
      delete [] types;
    }
  }

  return ncells;
}

//----------------------------------------------------------------------------
vtkCellArray *vtkDataObjectToDataSetFilter::ConstructCellArray(
  vtkDataArray *da,
  int comp,
  vtkIdType compRange[2])
{
  int j, min, max, numComp=da->GetNumberOfComponents();
  vtkCellArray *carray;
  vtkIdType npts, ncells, i;

  min = 0;
  max = da->GetMaxId();

  if ( comp < 0 || comp >= numComp )
  {
    vtkErrorMacro(<<"Bad component specification");
    return NULL;
  }

  carray = vtkCellArray::New();

  // If the data type is vtkIdType, and the number of components is 1, then
  // we can directly use the data array without copying it. We just have to
  // figure out how many cells we have.
  if ( da->GetDataType() == VTK_ID_TYPE && da->GetNumberOfComponents() == 1
    && comp == 0 && compRange[0] == 0 && compRange[1] == max )
  {
    vtkIdTypeArray *ia = static_cast<vtkIdTypeArray *>(da);
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
    for (i=min; i<max; i+=(npts+1))
    {
      npts = static_cast<int>(da->GetComponent(i,comp));
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
          carray->InsertCellPoint(static_cast<int>(
                                    da->GetComponent(i+j,comp)));
        }
      }
    }
  }

  return carray;
}

//----------------------------------------------------------------------------
// Alternative methods for Dimensions, Spacing, and Origin -------------------
//
void vtkDataObjectToDataSetFilter::SetDimensionsComponent(char *arrayName,
                                                          int arrayComp,
                                                          int min, int max)
{
  vtkFieldDataToAttributeDataFilter::SetArrayName(this, this->DimensionsArray,
                                                  arrayName);
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
void vtkDataObjectToDataSetFilter::SetSpacingComponent(char *arrayName,
                                                       int arrayComp,
                                                       int min, int max)
{
  vtkFieldDataToAttributeDataFilter::SetArrayName(this, this->SpacingArray,
                                                  arrayName);
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
void vtkDataObjectToDataSetFilter::SetOriginComponent(char *arrayName,
                                                      int arrayComp,
                                                      int min, int max)
{
  vtkFieldDataToAttributeDataFilter::SetArrayName(this, this->OriginArray,
                                                  arrayName);
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
void vtkDataObjectToDataSetFilter::ConstructDimensions(vtkDataObject *input)
{
  if ( this->DimensionsArray == NULL || this->DimensionsArrayComponent < 0 )
  {
    return; //assume dimensions have been set
  }
  else
  {
    vtkFieldData *fd=input->GetFieldData();
    vtkDataArray *fieldArray
      = vtkFieldDataToAttributeDataFilter::GetFieldArray(
        fd, this->DimensionsArray, this->DimensionsArrayComponent);
    if ( fieldArray == NULL )
    {
      vtkErrorMacro(<<"Can't find array requested for dimensions");
      return;
    }

    vtkFieldDataToAttributeDataFilter::UpdateComponentRange(
      fieldArray, this->DimensionsComponentRange);

    for (int i=0; i<3; i++)
    {
      this->Dimensions[i] = static_cast<int>(
        fieldArray->GetComponent(this->DimensionsComponentRange[0]+i,
                                 this->DimensionsArrayComponent));
    }
  }

  this->DimensionsComponentRange[0] = this->DimensionsComponentRange[1] = -1;
}

//----------------------------------------------------------------------------
void vtkDataObjectToDataSetFilter::ConstructSpacing(vtkDataObject *input)
{
  if ( this->SpacingArray == NULL || this->SpacingArrayComponent < 0 )
  {
    return; //assume Spacing have been set
  }
  else
  {
    vtkFieldData *fd=input->GetFieldData();
    vtkDataArray *fieldArray
      = vtkFieldDataToAttributeDataFilter::GetFieldArray(
        fd, this->SpacingArray, this->SpacingArrayComponent);
    if ( fieldArray == NULL )
    {
      vtkErrorMacro(<<"Can't find array requested for Spacing");
      return;
    }

    vtkFieldDataToAttributeDataFilter::UpdateComponentRange(
      fieldArray, this->SpacingComponentRange);

    for (int i=0; i<3; i++)
    {
      this->Spacing[i] = fieldArray->GetComponent(
        this->SpacingComponentRange[0]+i,
        this->SpacingArrayComponent);
    }
  }
  this->SpacingComponentRange[0] = this->SpacingComponentRange[1] = -1;
}

//----------------------------------------------------------------------------
vtkDataSet *vtkDataObjectToDataSetFilter::GetOutput(int idx)
{
  return vtkDataSet::SafeDownCast(this->GetExecutive()->GetOutputData(idx));
}

//----------------------------------------------------------------------------
void vtkDataObjectToDataSetFilter::ConstructOrigin(vtkDataObject *input)
{
  if ( this->OriginArray == NULL || this->OriginArrayComponent < 0 )
  {
    return; //assume Origin have been set
  }
  else
  {
    vtkFieldData *fd=input->GetFieldData();
    vtkDataArray *fieldArray
      = vtkFieldDataToAttributeDataFilter::GetFieldArray(
        fd, this->OriginArray, this->OriginArrayComponent);
    if ( fieldArray == NULL )
    {
      vtkErrorMacro(<<"Can't find array requested for Origin");
      return;
    }

    vtkFieldDataToAttributeDataFilter::UpdateComponentRange(
      fieldArray,
      this->OriginComponentRange);

    for (int i=0; i<3; i++)
    {
      this->Origin[i] = fieldArray->GetComponent(
        this->OriginComponentRange[0]+i,
        this->OriginArrayComponent);
    }
  }

  this->OriginComponentRange[0] = this->OriginComponentRange[1] = -1;
}

//----------------------------------------------------------------------------
int vtkDataObjectToDataSetFilter::FillInputPortInformation(int,
                                                           vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
int vtkDataObjectToDataSetFilter::RequestDataObject(
  vtkInformation *,
  vtkInformationVector **,
  vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkDataSet *output = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!output || (output->GetDataObjectType() != this->DataSetType))
  {
    switch (this->DataSetType)
    {
      case VTK_POLY_DATA:
        output = vtkPolyData::New();
        break;
      case VTK_STRUCTURED_GRID:
        output = vtkStructuredGrid::New();
        break;
      case VTK_STRUCTURED_POINTS:
        output = vtkStructuredPoints::New();
        break;
      case VTK_UNSTRUCTURED_GRID:
        output = vtkUnstructuredGrid::New();
        break;
      case VTK_RECTILINEAR_GRID:
        output = vtkRectilinearGrid::New();
        break;
      default:
        vtkWarningMacro("unknown DataSetType");
    }
    if (output)
    {
      outInfo->Set(vtkDataObject::DATA_OBJECT(), output);
      output->Delete();
    }
  }
  return 1;
}
