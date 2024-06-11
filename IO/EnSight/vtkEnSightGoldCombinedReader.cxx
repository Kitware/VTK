// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkEnSightGoldCombinedReader.h"
#include "core/EnSightDataSet.h"

#include "vtkDataArraySelection.h"
#include "vtkDataAssembly.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <algorithm>

VTK_ABI_NAMESPACE_BEGIN
struct vtkEnSightGoldCombinedReader::ReaderImpl
{
  ensight_gold::EnSightDataSet Reader;
  vtkNew<vtkDataArraySelection> PartSelection;
  vtkNew<vtkDataArraySelection> PointArraySelection;
  vtkNew<vtkDataArraySelection> CellArraySelection;
  vtkNew<vtkDataArraySelection> FieldArraySelection;
  std::vector<double> TimeSteps;
};

vtkStandardNewMacro(vtkEnSightGoldCombinedReader);

//----------------------------------------------------------------------------
vtkEnSightGoldCombinedReader::vtkEnSightGoldCombinedReader()
{
  this->SetNumberOfInputPorts(0);
  this->CaseFileName = nullptr;
  this->FilePath = nullptr;
  this->TimeValue = 0.0;
  this->Impl = new ReaderImpl;
}

//----------------------------------------------------------------------------
vtkEnSightGoldCombinedReader::~vtkEnSightGoldCombinedReader()
{
  this->SetCaseFileName(nullptr);
  this->SetFilePath(nullptr);
  delete this->Impl;
}

//------------------------------------------------------------------------------
int vtkEnSightGoldCombinedReader::CanReadFile(const char* casefilename)
{
  return (this->Impl->Reader.CheckVersion(casefilename) ? 1 : 0);
}

//------------------------------------------------------------------------------
int vtkEnSightGoldCombinedReader::RequestInformation(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  if (!this->CaseFileName)
  {
    vtkErrorMacro("CaseFileName is null");
    return 0;
  }

  std::string fullFileName;
  if (this->FilePath)
  {
    fullFileName = this->FilePath;
    fullFileName += "/";
  }
  fullFileName += this->CaseFileName;
  if (!this->Impl->Reader.ParseCaseFile(fullFileName.c_str()))
  {
    vtkErrorMacro("Case file " << this->CaseFileName << " could not be parsed without error");
    return 0;
  }

  // the rigid body files need to be read here because it's possible that there's no time step
  // information in the rest of the files, so we'll need to use the info in the eet file to get
  // time values.
  if (this->Impl->Reader.HasRigidBodyFile())
  {
    if (!this->Impl->Reader.ReadRigidBodyGeometryFile())
    {
      vtkErrorMacro("Error reading rigid body file. Will attempt to continue reading EnSight "
                    "files, without applying rigid body transformations.");
    }
  }

  this->Impl->TimeSteps = this->Impl->Reader.GetTimeSteps();
  if (this->Impl->TimeSteps.empty() && this->Impl->Reader.UseRigidBodyTimeSteps())
  {
    // we'll fall back on using time step info from rigid body files
    this->Impl->TimeSteps = this->Impl->Reader.GetEulerTimeSteps();
    if (this->Impl->TimeSteps.empty())
    {
      vtkErrorMacro("UseEulerTimeSteps is true, but there are no time steps saved.");
      return 0;
    }
  }

  if (!this->Impl->TimeSteps.empty())
  {
    if (!this->AllTimeSteps)
    {
      this->AllTimeSteps = vtkSmartPointer<vtkDoubleArray>::New();
    }
    this->AllTimeSteps->SetArray(this->Impl->TimeSteps.data(), this->Impl->TimeSteps.size(), 1);

    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), this->Impl->TimeSteps.data(),
      static_cast<int>(this->Impl->TimeSteps.size()));

    double timeRange[2];
    timeRange[0] = this->Impl->TimeSteps[0];
    timeRange[1] = this->Impl->TimeSteps[this->Impl->TimeSteps.size() - 1];
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
  }

  this->Impl->Reader.GetPartInfo(this->Impl->PartSelection, this->Impl->PointArraySelection,
    this->Impl->CellArraySelection, this->Impl->FieldArraySelection);

  return 1;
}

//------------------------------------------------------------------------------
int vtkEnSightGoldCombinedReader::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkPartitionedDataSetCollection* output =
    vtkPartitionedDataSetCollection::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkNew<vtkDataAssembly> assembly;
  output->SetDataAssembly(assembly);

  int tsLength = outInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  double* steps = outInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

  double actualTimeValue = this->TimeValue;
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()) && tsLength > 0)
  {
    // Get the requested time step. We only support requests of a single time
    // step in this reader right now
    double requestedTimeStep = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

    // find the first time value larger than requested time value
    // this logic could be improved
    int cnt = 0;
    while (cnt < tsLength - 1 && steps[cnt] < requestedTimeStep)
    {
      cnt++;
    }
    actualTimeValue = steps[cnt];
  }
  output->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), actualTimeValue);
  this->Impl->Reader.SetActualTimeValue(actualTimeValue);

  if (!this->Impl->Reader.ReadGeometry(output, this->Impl->PartSelection))
  {
    vtkErrorMacro("Geometry file could not be read");
    return 0;
  }

  if (!this->Impl->Reader.ReadMeasuredGeometry(output, this->Impl->PartSelection))
  {
    vtkErrorMacro("Measured geometry file could not be read");
    return 0;
  }

  if (!this->Impl->Reader.ReadVariables(output, this->Impl->PartSelection,
        this->Impl->PointArraySelection, this->Impl->CellArraySelection,
        this->Impl->FieldArraySelection))
  {
    vtkErrorMacro("Variable file(s) could not be read");
    return 0;
  }

  return 1;
}

//------------------------------------------------------------------------------
vtkDataArraySelection* vtkEnSightGoldCombinedReader::GetPartSelection()
{
  return this->Impl->PartSelection;
}

//------------------------------------------------------------------------------
vtkDataArraySelection* vtkEnSightGoldCombinedReader::GetPointArraySelection()
{
  return this->Impl->PointArraySelection;
}

//------------------------------------------------------------------------------
vtkDataArraySelection* vtkEnSightGoldCombinedReader::GetCellArraySelection()
{
  return this->Impl->CellArraySelection;
}

//------------------------------------------------------------------------------
vtkDataArraySelection* vtkEnSightGoldCombinedReader::GetFieldArraySelection()
{
  return this->Impl->FieldArraySelection;
}

//------------------------------------------------------------------------------
vtkMTimeType vtkEnSightGoldCombinedReader::GetMTime()
{
  auto maxVal = std::max(this->Superclass::GetMTime(), this->Impl->PartSelection->GetMTime());
  maxVal = std::max(maxVal, this->Impl->PointArraySelection->GetMTime());
  maxVal = std::max(maxVal, this->Impl->CellArraySelection->GetMTime());
  return std::max(maxVal, this->Impl->FieldArraySelection->GetMTime());
}

//------------------------------------------------------------------------------
void vtkEnSightGoldCombinedReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Case FileName: " << (this->CaseFileName ? this->CaseFileName : "(none)") << endl;
  os << indent << "File path: " << (this->FilePath ? this->FilePath : "(none)") << endl;
}
VTK_ABI_NAMESPACE_END
