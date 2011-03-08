// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNetCDFReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkNetCDFReader.h"

#include "vtkCallbackCommand.h"
#include "vtkCellData.h"
#include "vtkDataArraySelection.h"
#include "vtkDoubleArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkRectilinearGrid.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkStructuredGrid.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

#include <vtkstd/algorithm>
#include <vtkstd/set>
#include <vtksys/SystemTools.hxx>

#include "vtk_netcdf.h"

#define CALL_NETCDF(call) \
  { \
    int errorcode = call; \
    if (errorcode != NC_NOERR) \
      { \
      vtkErrorMacro(<< "netCDF Error: " << nc_strerror(errorcode)); \
      return 0; \
      } \
  }

#include <ctype.h>

//=============================================================================
static int NetCDFTypeToVTKType(nc_type type)
{
  switch (type)
    {
    case NC_BYTE: return VTK_UNSIGNED_CHAR;
    case NC_CHAR: return VTK_CHAR;
    case NC_SHORT: return VTK_SHORT;
    case NC_INT: return VTK_INT;
    case NC_FLOAT: return VTK_FLOAT;
    case NC_DOUBLE: return VTK_DOUBLE;
    default:
      vtkGenericWarningMacro(<< "Unknown netCDF variable type "
                             << type);
      return -1;
    }
}

//=============================================================================
vtkStandardNewMacro(vtkNetCDFReader);

//-----------------------------------------------------------------------------
vtkNetCDFReader::vtkNetCDFReader()
{
  this->SetNumberOfInputPorts(0);

  this->FileName = NULL;
  this->ReplaceFillValueWithNan = 0;

  this->LoadingDimensions = vtkSmartPointer<vtkIntArray>::New();

  this->VariableArraySelection = vtkSmartPointer<vtkDataArraySelection>::New();
  VTK_CREATE(vtkCallbackCommand, cbc);
  cbc->SetCallback(&vtkNetCDFReader::SelectionModifiedCallback);
  cbc->SetClientData(this);
  this->VariableArraySelection->AddObserver(vtkCommand::ModifiedEvent, cbc);

  this->AllVariableArrayNames = vtkSmartPointer<vtkStringArray>::New();

  this->VariableDimensions = vtkStringArray::New();
  this->AllDimensions = vtkStringArray::New();

  this->WholeExtent[0] = this->WholeExtent[1]
    = this->WholeExtent[2] = this->WholeExtent[3]
    = this->WholeExtent[4] = this->WholeExtent[5] = 0;
}

vtkNetCDFReader::~vtkNetCDFReader()
{
  this->SetFileName(NULL);
  this->VariableDimensions->Delete();
  this->AllDimensions->Delete();
}

void vtkNetCDFReader::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "FileName: "
     << (this->FileName ? this->FileName : "(NULL)") << endl;
  os << indent << "ReplaceFillValueWithNan: "
     << this->ReplaceFillValueWithNan << endl;

  os << indent << "VariableArraySelection:" << endl;
  this->VariableArraySelection->PrintSelf(os, indent.GetNextIndent());
  os << indent << "AllVariableArrayNames:" << endl;
  this->GetAllVariableArrayNames()->PrintSelf(os, indent.GetNextIndent());
  os << indent << "VariableDimensions: " << this->VariableDimensions << endl;
  os << indent << "AllDimensions: " << this->AllDimensions << endl;
}

//-----------------------------------------------------------------------------
int vtkNetCDFReader::RequestDataObject(
                                 vtkInformation *vtkNotUsed(request),
                                 vtkInformationVector **vtkNotUsed(inputVector),
                                 vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkDataObject *output = vtkDataObject::GetData(outInfo);

  if (!output || !output->IsA("vtkImageData"))
    {
    output = vtkImageData::New();
    output->SetPipelineInformation(outInfo);
    output->Delete();   // Not really deleted.
    }

  return 1;
}

//-----------------------------------------------------------------------------
int vtkNetCDFReader::RequestInformation(
                                 vtkInformation *vtkNotUsed(request),
                                 vtkInformationVector **vtkNotUsed(inputVector),
                                 vtkInformationVector *outputVector)
{
  if (!this->UpdateMetaData()) return 0;

  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  int ncFD;
  CALL_NETCDF(nc_open(this->FileName, NC_NOWRITE, &ncFD));

  VTK_CREATE(vtkDoubleArray, timeValues);
  VTK_CREATE(vtkIntArray, currentDimensions);
  this->LoadingDimensions->Initialize();
  int numArrays = this->VariableArraySelection->GetNumberOfArrays();
  int numDims = 0;
  for (int arrayIndex = 0; arrayIndex < numArrays; arrayIndex++)
    {
    if (!this->VariableArraySelection->GetArraySetting(arrayIndex)) continue;

    const char *name = this->VariableArraySelection->GetArrayName(arrayIndex);
    int varId;
    CALL_NETCDF(nc_inq_varid(ncFD, name, &varId));

    int currentNumDims;
    CALL_NETCDF(nc_inq_varndims(ncFD, varId, &currentNumDims));
    if (currentNumDims < 1) continue;
    currentDimensions->SetNumberOfComponents(1);
    currentDimensions->SetNumberOfTuples(currentNumDims);
    CALL_NETCDF(nc_inq_vardimid(ncFD, varId,
                                currentDimensions->GetPointer(0)));

    // Assumption: time dimension is first.
    int timeDim = currentDimensions->GetValue(0);       // Not determined yet.
    if (this->IsTimeDimension(ncFD, timeDim))
      {
      // Get time step information.
      VTK_CREATE(vtkDoubleArray, compositeTimeValues);
      vtkSmartPointer<vtkDoubleArray> currentTimeValues
        = this->GetTimeValues(ncFD, timeDim);
      double *oldTime = timeValues->GetPointer(0);
      double *newTime = currentTimeValues->GetPointer(0);
      double *oldTimeEnd = oldTime + timeValues->GetNumberOfTuples();
      double *newTimeEnd = newTime + currentTimeValues->GetNumberOfTuples();
      compositeTimeValues->Allocate(  timeValues->GetNumberOfTuples()
                                    + currentTimeValues->GetNumberOfTuples());
      compositeTimeValues->SetNumberOfComponents(1);
      while ((oldTime < oldTimeEnd) || (newTime < newTimeEnd))
        {
        if (   (newTime >= newTimeEnd)
            || ((oldTime < oldTimeEnd) && (*oldTime < *newTime)) )
          {
          compositeTimeValues->InsertNextTuple1(*oldTime);
          oldTime++;
          }
        else if ((oldTime >= oldTimeEnd) || (*newTime < *oldTime))
          {
          compositeTimeValues->InsertNextTuple1(*newTime);
          newTime++;
          }
        else // *oldTime == *newTime
          {
          compositeTimeValues->InsertNextTuple1(*oldTime);
          oldTime++;  newTime++;
          }
        }
      timeValues = compositeTimeValues;

      // Strip off time dimension from what we load (we will use it to
      // subset instead).
      currentDimensions->RemoveTuple(0);
      currentNumDims--;
      }

    // Remember the first variable we encounter.  Use it to determine extents
    // (below).
    if (numDims == 0)
      {
      numDims = currentNumDims;
      this->LoadingDimensions->DeepCopy(currentDimensions);
      }
    }

  // Capture the extent information from this->LoadingDimensions.
  bool pointData = this->DimensionsAreForPointData(this->LoadingDimensions);
  for (int i = 0 ; i < 3; i++)
    {
    this->WholeExtent[2*i] = 0;
    if (i < this->LoadingDimensions->GetNumberOfTuples())
      {
      size_t dimlength;
      // Remember that netCDF arrays are indexed backward from VTK images.
      int dim = this->LoadingDimensions->GetValue(numDims-i-1);
      CALL_NETCDF(nc_inq_dimlen(ncFD, dim, &dimlength));
      this->WholeExtent[2*i+1] = static_cast<int>(dimlength-1);
      // For cell data, add one to the extent (which is for points).
      if (!pointData) this->WholeExtent[2*i+1]++;
      }
    else
      {
      this->WholeExtent[2*i+1] = 0;
      }
    }
  vtkDebugMacro(<< "Whole extents: "
                << this->WholeExtent[0] << ", " << this->WholeExtent[1] <<", "
                << this->WholeExtent[2] << ", " << this->WholeExtent[3] <<", "
                << this->WholeExtent[4] << ", " << this->WholeExtent[5]);

  // Report extents.
  vtkDataObject *output = vtkDataObject::GetData(outInfo);
  if (output && (output->GetExtentType() == VTK_3D_EXTENT))
    {
    outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
                 this->WholeExtent, 6);
    }

  // If we have time, report that.
  if (timeValues && (timeValues->GetNumberOfTuples() > 0))
    {
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(),
                 timeValues->GetPointer(0),
                 timeValues->GetNumberOfTuples());
    double timeRange[2];
    timeRange[0] = timeValues->GetValue(0);
    timeRange[1] = timeValues->GetValue(timeValues->GetNumberOfTuples()-1);
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
    }
  else
    {
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
    }

  CALL_NETCDF(nc_close(ncFD));

  return 1;
}

//-----------------------------------------------------------------------------
int vtkNetCDFReader::RequestData(vtkInformation *vtkNotUsed(request),
                                 vtkInformationVector **vtkNotUsed(inputVector),
                                 vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  // If the output is not a vtkDataSet, then the subclass needs to override
  // this method.
  vtkDataSet *output = vtkDataSet::GetData(outInfo);
  if (!output)
    {
    vtkErrorMacro(<< "Bad output type.");
    return 0;
    }

  // Set up the extent for regular-grid type data sets.
  vtkImageData *imageOutput = vtkImageData::SafeDownCast(output);
  vtkRectilinearGrid *rectOutput = vtkRectilinearGrid::SafeDownCast(output);
  vtkStructuredGrid *structOutput = vtkStructuredGrid::SafeDownCast(output);
  if (imageOutput)
    {
    imageOutput->SetExtent(imageOutput->GetUpdateExtent());
    }
  else if (rectOutput)
    {
    rectOutput->SetExtent(rectOutput->GetUpdateExtent());
    }
  else if (structOutput)
    {
    structOutput->SetExtent(structOutput->GetUpdateExtent());
    }
  else
    {
    // Superclass should handle extent setup if necessary.
    }

  // Get requested time step.
  double time = 0.0;
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS()))
    {
    time
      = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS())[0];
    }

  int ncFD;
  CALL_NETCDF(nc_open(this->FileName, NC_NOWRITE, &ncFD));

  // Iterate over arrays and load selected ones.
  int numArrays = this->VariableArraySelection->GetNumberOfArrays();
  for (int arrayIndex = 0; arrayIndex < numArrays; arrayIndex++)
    {
    if (!this->VariableArraySelection->GetArraySetting(arrayIndex)) continue;

    const char *name = this->VariableArraySelection->GetArrayName(arrayIndex);

    if (!this->LoadVariable(ncFD, name, time, output)) return 0;
    }

  CALL_NETCDF(nc_close(ncFD));

  return 1;
}

//-----------------------------------------------------------------------------
void vtkNetCDFReader::SetFileName(const char *filename)
{
  if (this->FileName == filename) return;
  if (this->FileName && filename && (strcmp(this->FileName, filename) == 0))
    {
    return;
    }

  if (this->FileName)
    {
    delete[] this->FileName;
    this->FileName = NULL;
    }

  if (filename)
    {
    this->FileName = new char[strlen(filename)+1];
    strcpy(this->FileName, filename);
    }

  this->Modified();
  this->FileNameMTime.Modified();
}

//----------------------------------------------------------------------------
void vtkNetCDFReader::SelectionModifiedCallback(vtkObject*, unsigned long,
                                                void* clientdata, void*)
{
  static_cast<vtkNetCDFReader*>(clientdata)->Modified();
}

//-----------------------------------------------------------------------------
int vtkNetCDFReader::GetNumberOfVariableArrays()
{
  return this->VariableArraySelection->GetNumberOfArrays();
}

//-----------------------------------------------------------------------------
const char* vtkNetCDFReader::GetVariableArrayName(int index)
{
  return this->VariableArraySelection->GetArrayName(index);
}

//-----------------------------------------------------------------------------
int vtkNetCDFReader::GetVariableArrayStatus(const char* name)
{
  return this->VariableArraySelection->ArrayIsEnabled(name);
}

//-----------------------------------------------------------------------------
void vtkNetCDFReader::SetVariableArrayStatus(const char* name, int status)
{
  vtkDebugMacro("Set cell array \"" << name << "\" status to: " << status);
  if(status)
    {
    this->VariableArraySelection->EnableArray(name);
    }
  else
    {
    this->VariableArraySelection->DisableArray(name);
    }
}

//-----------------------------------------------------------------------------
vtkStringArray *vtkNetCDFReader::GetAllVariableArrayNames()
{
  int numArrays = this->GetNumberOfVariableArrays();
  this->AllVariableArrayNames->SetNumberOfValues(numArrays);
  for (int arrayIdx = 0; arrayIdx < numArrays; arrayIdx++)
    {
    const char *arrayName = this->GetVariableArrayName(arrayIdx);
    this->AllVariableArrayNames->SetValue(arrayIdx, arrayName);
    }

  return this->AllVariableArrayNames;
}

//-----------------------------------------------------------------------------
void vtkNetCDFReader::SetDimensions(const char *dimensions)
{
  this->VariableArraySelection->DisableAllArrays();

  for (vtkIdType i = 0; i < this->VariableDimensions->GetNumberOfValues(); i++)
    {
    if (this->VariableDimensions->GetValue(i) == dimensions)
      {
      const char *variableName = this->VariableArraySelection->GetArrayName(i);
      this->VariableArraySelection->EnableArray(variableName);
      }
    }
}

//-----------------------------------------------------------------------------
int vtkNetCDFReader::UpdateMetaData()
{
  if (this->MetaDataMTime < this->FileNameMTime)
    {
    if (!this->FileName)
      {
      vtkErrorMacro(<< "FileName not set.");
      return 0;
      }

    int ncFD;
    CALL_NETCDF(nc_open(this->FileName, NC_NOWRITE, &ncFD));

    int retval = this->ReadMetaData(ncFD);

    if (retval) retval = this->FillVariableDimensions(ncFD);

    if (retval) this->MetaDataMTime.Modified();

    CALL_NETCDF(nc_close(ncFD));

    return retval;
    }
  else
    {
    return 1;
    }
}

//-----------------------------------------------------------------------------
vtkStdString vtkNetCDFReader::DescribeDimensions(int ncFD,
                                                 const int *dimIds, int numDims)
{
  vtkStdString description;
  for (int i = 0; i < numDims; i++)
    {
    char name[NC_MAX_NAME+1];
    CALL_NETCDF(nc_inq_dimname(ncFD, dimIds[i], name));
    if (i > 0) description += " ";
    description += name;
    }
  return description;
}

//-----------------------------------------------------------------------------
int vtkNetCDFReader::ReadMetaData(int ncFD)
{
  vtkDebugMacro("ReadMetaData");

  // Look at all variables and record them so that the user can select which
  // ones he wants.  This oddness of adding and removing from
  // VariableArraySelection is to preserve any current settings for variables.
  typedef vtkstd::set<vtkStdString> stringSet;
  stringSet variablesToAdd;
  stringSet variablesToRemove;

  // Initialize variablesToRemove with all the variables.  Then remove them from
  // the list as we find them.
  for (int i = 0; i < this->VariableArraySelection->GetNumberOfArrays(); i++)
    {
    variablesToRemove.insert(this->VariableArraySelection->GetArrayName(i));
    }

  int numVariables;
  CALL_NETCDF(nc_inq_nvars(ncFD, &numVariables));

  for (int i = 0; i < numVariables; i++)
    {
    char name[NC_MAX_NAME+1];
    CALL_NETCDF(nc_inq_varname(ncFD, i, name));
    if (variablesToRemove.find(name) == variablesToRemove.end())
      {
      // Variable not already here.  Insert it in the variables to add.
      variablesToAdd.insert(name);
      }
    else
      {
      // Variable already exists.  Leave it be.  Remove it from the
      // variablesToRemove list.
      variablesToRemove.erase(name);
      }
    }

  // Add and remove variables.  This will be a no-op if the variables have not
  // changed.
  for (stringSet::iterator removeItr = variablesToRemove.begin();
       removeItr != variablesToRemove.end(); removeItr++)
    {
    this->VariableArraySelection->RemoveArrayByName(removeItr->c_str());
    }
  for (stringSet::iterator addItr = variablesToAdd.begin();
       addItr != variablesToAdd.end(); addItr++)
    {
    this->VariableArraySelection->AddArray(addItr->c_str());
    }

  return 1;
}

//-----------------------------------------------------------------------------
int vtkNetCDFReader::FillVariableDimensions(int ncFD)
{
  int numVar = this->GetNumberOfVariableArrays();
  this->VariableDimensions->SetNumberOfValues(numVar);
  this->AllDimensions->SetNumberOfValues(0);

  for (int i = 0; i < numVar; i++)
    {
    // Get the dimensions of this variable and encode them in a string.
    const char *varName = this->GetVariableArrayName(i);
    int varId, numDim, dimIds[NC_MAX_VAR_DIMS];
    CALL_NETCDF(nc_inq_varid(ncFD, varName, &varId));
    CALL_NETCDF(nc_inq_varndims(ncFD, varId, &numDim));
    CALL_NETCDF(nc_inq_vardimid(ncFD, varId, dimIds));
    vtkStdString dimEncoding("(");
    for (int j = 0; j < numDim; j++)
      {
      if ((j == 0) && (this->IsTimeDimension(ncFD, dimIds[j]))) continue;
      char dimName[NC_MAX_NAME+1];
      CALL_NETCDF(nc_inq_dimname(ncFD, dimIds[j], dimName));
      if (dimEncoding.size() > 1) dimEncoding += ", ";
      dimEncoding += dimName;
      }
    dimEncoding += ")";
    // Store all dimensions in the VariableDimensions array.
    this->VariableDimensions->SetValue(i, dimEncoding.c_str());
    // Store unique dimensions in the AllDimensions array.
    bool unique = true;
    for (vtkIdType j = 0; j < this->AllDimensions->GetNumberOfValues(); j++)
      {
      if (this->AllDimensions->GetValue(j) == dimEncoding)
        {
        unique = false;
        break;
        }
      }
    if (unique) this->AllDimensions->InsertNextValue(dimEncoding);
    }

  return 1;
}

//-----------------------------------------------------------------------------
int vtkNetCDFReader::IsTimeDimension(int ncFD, int dimId)
{
  char name[NC_MAX_NAME+1];
  CALL_NETCDF(nc_inq_dimname(ncFD, dimId, name));
  name[4] = '\0';       // Truncate to 4 characters.
  return (vtksys::SystemTools::Strucmp(name, "time") == 0);
}

//-----------------------------------------------------------------------------
vtkSmartPointer<vtkDoubleArray> vtkNetCDFReader::GetTimeValues(int ncFD,
                                                               int dimId)
{
  VTK_CREATE(vtkDoubleArray, timeValues);
  size_t dimLength;
  CALL_NETCDF(nc_inq_dimlen(ncFD, dimId, &dimLength));
  timeValues->SetNumberOfComponents(1);
  timeValues->SetNumberOfTuples(dimLength);
  for (size_t j = 0; j < dimLength; j++)
    {
    timeValues->SetValue(j, static_cast<double>(j));
    }
  return timeValues;
}

//-----------------------------------------------------------------------------
void vtkNetCDFReader::GetUpdateExtentForOutput(vtkDataSet *output,
                                               int extent[6])
{
  output->GetUpdateExtent(extent);
}

//-----------------------------------------------------------------------------
int vtkNetCDFReader::LoadVariable(int ncFD, const char *varName, double time,
                                  vtkDataSet *output)
{
  // Get the variable id.
  int varId;
  CALL_NETCDF(nc_inq_varid(ncFD, varName, &varId));

  // Get dimension info.
  int numDims;
  CALL_NETCDF(nc_inq_varndims(ncFD, varId, &numDims));
  if (numDims > 4)
    {
    vtkErrorMacro(<< "More than 3 dims + time not supported in variable "
                  << varName);
    return 0;
    }
  int dimIds[4];
  CALL_NETCDF(nc_inq_vardimid(ncFD, varId, dimIds));

  // Number of values to read.
  vtkIdType arraySize = 1;

  // Indices to read from.
  size_t start[4], count[4];

  // Are we using time?
  int timeIndexOffset = 0;
  if ((numDims > 0) && this->IsTimeDimension(ncFD, dimIds[0]))
    {
    vtkSmartPointer<vtkDoubleArray> timeValues
      = this->GetTimeValues(ncFD, dimIds[0]);
    timeIndexOffset = 1;
    // Find the index for the given time.
    // We could speed this up with a binary search or something.
    for (start[0] = 0;
         start[0] < static_cast<size_t>(timeValues->GetNumberOfTuples());
         start[0]++)
      {
      if (timeValues->GetValue(start[0]) >= time) break;
      }
    count[0] = 1;
    numDims--;
    }

  if (numDims > 3)
    {
    vtkErrorMacro(<< "More than 3 dims without time not supported in variable "
                  << varName);
    return 0;
    }

  bool loadingPointData = this->DimensionsAreForPointData(
                                                       this->LoadingDimensions);

  // Set up read indices.  Also check to make sure the dimensions are consistent
  // with other loaded variables.
  int extent[6];
  this->GetUpdateExtentForOutput(output, extent);
  if (numDims != this->LoadingDimensions->GetNumberOfTuples())
    {
    vtkWarningMacro(<< "Variable " << varName << " dimensions ("
                    << this->DescribeDimensions(ncFD, dimIds+timeIndexOffset,
                                                numDims).c_str()
                    << ") are different than the other variable dimensions ("
                    << this->DescribeDimensions(ncFD,
                           this->LoadingDimensions->GetPointer(0),
                           this->LoadingDimensions->GetNumberOfTuples()).c_str()
                    << ").  Skipping");
    return 1;
    }
  for (int i = 0; i < numDims; i++)
    {
    if (dimIds[i+timeIndexOffset] != this->LoadingDimensions->GetValue(i))
      {
      vtkWarningMacro(<< "Variable " << varName << " dimensions ("
                      << this->DescribeDimensions(ncFD, dimIds+timeIndexOffset,
                                                  numDims).c_str()
                      << ") are different than the other variable dimensions ("
                      << this->DescribeDimensions(ncFD,
                           this->LoadingDimensions->GetPointer(0),
                           this->LoadingDimensions->GetNumberOfTuples()).c_str()
                      << ").  Skipping");
      return 1;
      }
    // Remember that netCDF arrays are indexed backward from VTK images.
    start[i+timeIndexOffset] = extent[2*(numDims-i-1)];
    count[i+timeIndexOffset]
      = extent[2*(numDims-i-1)+1]-extent[2*(numDims-i-1)]+1;

    // If loading cell data, subtract one from the data being loaded.
    if (!loadingPointData) count[i+timeIndexOffset]--;

    arraySize *= count[i+timeIndexOffset];
    }

  // Allocate an array of the right type.
  nc_type ncType;
  CALL_NETCDF(nc_inq_vartype(ncFD, varId, &ncType));
  int vtkType = NetCDFTypeToVTKType(ncType);
  if (vtkType < 1) return 0;
  vtkSmartPointer<vtkDataArray> dataArray;
  dataArray.TakeReference(vtkDataArray::CreateDataArray(vtkType));
  dataArray->SetNumberOfComponents(1);
  dataArray->SetNumberOfTuples(arraySize);

  // Read the array from the file.
  CALL_NETCDF(nc_get_vars(ncFD, varId, start, count, NULL,
                          dataArray->GetVoidPointer(0)));

  // Check for a fill value.
  size_t attribLength;
  if (   (nc_inq_attlen(ncFD, varId, "_FillValue", &attribLength) == NC_NOERR)
      && (attribLength == 1) )
    {
    if (this->ReplaceFillValueWithNan)
      {
      // NaN only available with float and double.
      if (dataArray->GetDataType() == VTK_FLOAT)
        {
        float fillValue;
        nc_get_att_float(ncFD, varId, "_FillValue", &fillValue);
        vtkstd::replace(reinterpret_cast<float*>(dataArray->GetVoidPointer(0)),
                        reinterpret_cast<float*>(dataArray->GetVoidPointer(
                                               dataArray->GetNumberOfTuples())),
                        fillValue, static_cast<float>(vtkMath::Nan()));
        }
      else if (dataArray->GetDataType() == VTK_DOUBLE)
        {
        double fillValue;
        nc_get_att_double(ncFD, varId, "_FillValue", &fillValue);
        vtkstd::replace(reinterpret_cast<double*>(dataArray->GetVoidPointer(0)),
                        reinterpret_cast<double*>(dataArray->GetVoidPointer(
                                               dataArray->GetNumberOfTuples())),
                        fillValue, vtkMath::Nan());
        }
      else
        {
        vtkDebugMacro(<< "No NaN available for data of type "
                      << dataArray->GetDataType());
        }
      }
    }

  // Check to see if there is a scale or offset.
  double scale = 1.0;
  double offset = 0.0;
  if (   (nc_inq_attlen(ncFD, varId, "scale_factor", &attribLength) == NC_NOERR)
      && (attribLength == 1) )
    {
    CALL_NETCDF(nc_get_att_double(ncFD, varId, "scale_factor", &scale));
    }
  if (   (nc_inq_attlen(ncFD, varId, "add_offset", &attribLength) == NC_NOERR)
      && (attribLength == 1) )
    {
    CALL_NETCDF(nc_get_att_double(ncFD, varId, "add_offset", &offset));
    }

  if ((scale != 1.0) || (offset != 0.0))
    {
    VTK_CREATE(vtkDoubleArray, adjustedArray);
    adjustedArray->SetNumberOfComponents(1);
    adjustedArray->SetNumberOfTuples(arraySize);
    for (vtkIdType i = 0; i < arraySize; i++)
      {
      adjustedArray->SetValue(i, dataArray->GetTuple1(i)*scale + offset);
      }
    dataArray = adjustedArray;
    }

  // Add data to the output.
  dataArray->SetName(varName);
  if (loadingPointData)
    {
    output->GetPointData()->AddArray(dataArray);
    }
  else
    {
    output->GetCellData()->AddArray(dataArray);
    }

  return 1;
}

