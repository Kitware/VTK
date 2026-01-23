// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (C) CSCS - Swiss National Supercomputing Centre
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkH5PartReader.h"

#include "vtkCellArray.h"
#include "vtkDataArray.h"
#include "vtkDataArraySelection.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMathUtilities.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSOATypeFloat32Array.h"
#include "vtkSOATypeFloat64Array.h"
#include "vtkSOATypeInt32Array.h"
#include "vtkSOATypeInt64Array.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringScanner.h"
#include "vtkTypeFloat32Array.h"
#include "vtkTypeFloat64Array.h"
#include "vtkTypeInt32Array.h"
#include "vtkTypeInt64Array.h"

#include <vtksys/RegularExpression.hxx>
#include <vtksys/SystemTools.hxx>

#include <algorithm>
#include <vector>

#include "vtk_h5hut.h"
// clang-format off
#include VTK_H5HUT(H5hut.h)
// clang-format on

VTK_ABI_NAMESPACE_BEGIN
static void vtkPickArray(char*& arrayPtr, const std::initializer_list<const char*>& values,
  vtkDataArraySelection* selection)
{
  if (arrayPtr != nullptr && arrayPtr[0] != '\0')
  {
    return;
  }

  for (int cc = 0, max = selection->GetNumberOfArrays(); cc < max; ++cc)
  {
    const char* aname = selection->GetArrayName(cc);
    for (const char* value : values)
    {
      if (vtksys::SystemTools::Strucmp(aname, value) == 0)
      {
        arrayPtr = vtksys::SystemTools::DuplicateString(aname);
        return;
      }
    }
  }
}

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkH5PartReader);
//------------------------------------------------------------------------------
vtkH5PartReader::vtkH5PartReader()
{
  this->SetNumberOfInputPorts(0);
  //
  this->NumberOfTimeSteps = 0;
  this->TimeStep = 0;
  this->ActualTimeStep = 0;
  this->TimeStepTolerance = 1E-6;
  this->CombineVectorComponents = 1;
  this->GenerateVertexCells = 0;
  this->FileName = nullptr;
  this->H5FileId = 0;
  this->Xarray = nullptr;
  this->Yarray = nullptr;
  this->Zarray = nullptr;
  this->TimeOutOfRange = 0;
  this->MaskOutOfTimeRangeOutput = 0;
  this->PointDataArraySelection = vtkDataArraySelection::New();
}

//------------------------------------------------------------------------------
vtkH5PartReader::~vtkH5PartReader()
{
  this->CloseFile();
  delete[] this->FileName;
  this->FileName = nullptr;

  delete[] this->Xarray;
  this->Xarray = nullptr;

  delete[] this->Yarray;
  this->Yarray = nullptr;

  delete[] this->Zarray;
  this->Zarray = nullptr;

  this->PointDataArraySelection->Delete();
  this->PointDataArraySelection = nullptr;
}

//------------------------------------------------------------------------------
void vtkH5PartReader::SetFileName(char* filename)
{
  if (this->FileName == nullptr && filename == nullptr)
  {
    return;
  }
  if (this->FileName && filename && (!strcmp(this->FileName, filename)))
  {
    return;
  }
  delete[] this->FileName;
  this->FileName = nullptr;

  if (filename)
  {
    this->FileName = vtksys::SystemTools::DuplicateString(filename);
    this->FileModifiedTime.Modified();
  }
  this->Modified();
}
//------------------------------------------------------------------------------
void vtkH5PartReader::CloseFile()
{
  if (this->H5FileId != 0)
  {
    H5CloseFile(this->H5FileId);
    this->H5FileId = 0;
  }
}
//------------------------------------------------------------------------------
int vtkH5PartReader::OpenFile()
{
  if (!this->FileName)
  {
    vtkErrorMacro(<< "FileName must be specified.");
    return 0;
  }

  if (FileModifiedTime > FileOpenedTime)
  {
    this->CloseFile();
  }

  if (!this->H5FileId)
  {
    this->H5FileId = H5OpenFile(this->FileName, H5_O_RDONLY, H5_PROP_DEFAULT);
    this->FileOpenedTime.Modified();
  }

  if (!this->H5FileId)
  {
    vtkErrorMacro(<< "Initialize: Could not open file " << this->FileName);
    return 0;
  }

  return 1;
}
//------------------------------------------------------------------------------
int vtkH5PartReader::IndexOfVectorComponent(const char* name)
{
  if (!this->CombineVectorComponents)
  {
    return 0;
  }
  //
  vtksys::RegularExpression re1(".*_([0-9]+)");
  if (re1.find(name))
  {
    int index;
    VTK_FROM_CHARS_IF_ERROR_RETURN(re1.match(1), index, 0);
    return index + 1;
  }
  return 0;
}
//------------------------------------------------------------------------------
std::string vtkH5PartReader::NameOfVectorComponent(const char* name)
{
  if (!this->CombineVectorComponents)
  {
    return name;
  }
  //
  vtksys::RegularExpression re1("(.*)_[0-9]+");
  if (re1.find(name))
  {
    return re1.match(1);
  }
  return name;
}
//------------------------------------------------------------------------------
int vtkH5PartReader::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);

  if (!this->OpenFile())
  {
    return 0;
  }

  // This block is, and always has been, unconditional. It's previous
  // "condition" is commented here in case it is needed at some point.
  // bool NeedToReadInformation = (FileModifiedTime > FileOpenedTime || !this->H5FileId);
  // if (1 || NeedToReadInformation)
  {
    this->NumberOfTimeSteps = H5GetNumSteps(this->H5FileId);
    H5SetStep(this->H5FileId, 0);
    int nds = H5PartGetNumDatasets(this->H5FileId);
    char name[512];
    for (int i = 0; i < nds; i++)
    {
      // return 0 for no, 1,2,3,4,5 etc for index (1 based offset)
      H5PartGetDatasetName(this->H5FileId, i, name, 512);
      this->PointDataArraySelection->AddArray(name);
    }

    this->TimeStepValues.assign(this->NumberOfTimeSteps, 0.0);
    int validTimes = 0;
    for (int i = 0; i < this->NumberOfTimeSteps; ++i)
    {
      H5SetStep(this->H5FileId, i);
      // Get the time value if it exists
      h5_int64_t numAttribs = H5GetNumStepAttribs(this->H5FileId);
      if (numAttribs > 0)
      {
        char attribName[128];
        h5_int64_t attribNameLength = 128;
        h5_int64_t attribType = 0;
        h5_size_t attribNelem = 0;
        for (h5_int64_t a = 0; a < numAttribs; a++)
        {
          h5_int64_t status = H5GetStepAttribInfo(
            this->H5FileId, a, attribName, attribNameLength, &attribType, &attribNelem);
          if (status == H5_SUCCESS && !strncmp("TimeValue", attribName, attribNameLength))
          {
            if (H5Tequal(attribType, H5T_NATIVE_DOUBLE) > 0 && attribNelem == 1)
            {
              status =
                H5ReadStepAttribFloat64(this->H5FileId, attribName, &this->TimeStepValues[i]);
              if (status == H5_SUCCESS)
              {
                validTimes++;
              }
            }
          }
        }
      }
    }
    H5SetStep(this->H5FileId, 0);

    if (this->NumberOfTimeSteps == 0)
    {
      vtkErrorMacro(<< "No time steps in data");
      return 0;
    }

    // if TIME information was either not present ot not consistent, then
    // set something so that consumers of this data can iterate sensibly
    if (this->NumberOfTimeSteps > 0 && this->NumberOfTimeSteps != validTimes)
    {
      for (int i = 0; i < this->NumberOfTimeSteps; i++)
      {
        // insert read of Time array here
        this->TimeStepValues[i] = i;
      }
    }
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), this->TimeStepValues.data(),
      static_cast<int>(this->TimeStepValues.size()));
    double timeRange[2];
    timeRange[0] = this->TimeStepValues.front();
    timeRange[1] = this->TimeStepValues.back();
    if (this->TimeStepValues.size() > 1)
    {
      this->TimeStepTolerance = 0.01 * (this->TimeStepValues[1] - this->TimeStepValues[0]);
    }
    else
    {
      this->TimeStepTolerance = 1E-3;
    }
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
  }

  vtkPickArray(this->Xarray, { "x", "coords_0", "coords0" }, this->PointDataArraySelection);
  vtkPickArray(this->Yarray, { "y", "coords_1", "coords1" }, this->PointDataArraySelection);
  vtkPickArray(this->Zarray, { "z", "coords_2", "coords2" }, this->PointDataArraySelection);
  return 1;
}

//------------------------------------------------------------------------------
int GetVTKDataType(h5_int64_t h5hut_datatype)
{
  switch (h5hut_datatype)
  {
    case H5_FLOAT32_T:
      return VTK_TYPE_FLOAT32;
    case H5_FLOAT64_T:
      return VTK_TYPE_FLOAT64;
    case H5_INT32_T:
      return VTK_TYPE_INT32;
    case H5_INT64_T:
      return VTK_TYPE_INT64;
    default:
      return VTK_VOID;
  }
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkDataArray> CreateDataArray(
  int dataType, std::vector<void*>& compPtrs, vtkIdType numTuples, const std::string& rootname)
{
#define HANDLE_TYPE(VTK_TYPE, SOA_CLASS, AOS_CLASS)                                                \
  case VTK_TYPE:                                                                                   \
  {                                                                                                \
    const int numComponents = static_cast<int>(compPtrs.size());                                   \
    if (numComponents != 1)                                                                        \
    {                                                                                              \
      using ValueType = typename SOA_CLASS::ValueType;                                             \
      auto soaArray = vtkSmartPointer<SOA_CLASS>::New();                                           \
      soaArray->SetNumberOfComponents(numComponents);                                              \
      soaArray->SetName(rootname.c_str());                                                         \
                                                                                                   \
      soaArray->SetArray(0, static_cast<ValueType*>(compPtrs[0]), numTuples, true, false,          \
        vtkAbstractArray::VTK_DATA_ARRAY_DELETE);                                                  \
      for (int c = 1; c < numComponents; ++c)                                                      \
      {                                                                                            \
        soaArray->SetArray(c, static_cast<ValueType*>(compPtrs[c]), numTuples, false, false,       \
          vtkAbstractArray::VTK_DATA_ARRAY_DELETE);                                                \
      }                                                                                            \
                                                                                                   \
      return soaArray;                                                                             \
    }                                                                                              \
    else                                                                                           \
    {                                                                                              \
      using ValueType = typename AOS_CLASS::ValueType;                                             \
      auto array = vtkSmartPointer<AOS_CLASS>::New();                                              \
      array->SetNumberOfComponents(1);                                                             \
      array->SetName(rootname.c_str());                                                            \
      array->SetArray(static_cast<ValueType*>(compPtrs[0]), numTuples, false,                      \
        vtkAbstractArray::VTK_DATA_ARRAY_DELETE);                                                  \
                                                                                                   \
      return array;                                                                                \
    }                                                                                              \
  }
  switch (dataType)
  {
    HANDLE_TYPE(VTK_TYPE_FLOAT32, vtkSOATypeFloat32Array, vtkTypeFloat32Array)
    HANDLE_TYPE(VTK_TYPE_FLOAT64, vtkSOATypeFloat64Array, vtkTypeFloat64Array)
    HANDLE_TYPE(VTK_TYPE_INT32, vtkSOATypeInt32Array, vtkTypeInt32Array)
    HANDLE_TYPE(VTK_TYPE_INT64, vtkSOATypeInt64Array, vtkTypeInt64Array)

    default:
      return nullptr;
  }
#undef HANDLE_TYPE
}

//------------------------------------------------------------------------------
int vtkH5PartReader::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  using SDDP = vtkStreamingDemandDrivenPipeline;

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkPolyData* output = vtkPolyData::GetData(outInfo);

  const int piece =
    outInfo->Has(SDDP::UPDATE_PIECE_NUMBER()) ? outInfo->Get(SDDP::UPDATE_PIECE_NUMBER()) : 0;
  const int numPieces = outInfo->Has(SDDP::UPDATE_NUMBER_OF_PIECES())
    ? outInfo->Get(SDDP::UPDATE_NUMBER_OF_PIECES())
    : 1;

  typedef std::map<std::string, std::vector<std::string>> FieldMap;
  FieldMap scalarFields;
  //
  if (this->TimeStepValues.empty())
  {
    return 0;
  }

  //
  // Make sure that the user selected arrays for coordinates are represented
  //
  std::vector<std::string> coordarrays(3, "");
  //
  int N = this->PointDataArraySelection->GetNumberOfArrays();
  for (int i = 0; i < N; i++)
  {
    const char* name = this->PointDataArraySelection->GetArrayName(i);
    // Do we want to load this array
    bool processarray = false;
    if (!vtksys::SystemTools::Strucmp(name, this->Xarray))
    {
      processarray = true;
      coordarrays[0] = name;
    }
    if (!vtksys::SystemTools::Strucmp(name, this->Yarray))
    {
      processarray = true;
      coordarrays[1] = name;
    }
    if (!vtksys::SystemTools::Strucmp(name, this->Zarray))
    {
      processarray = true;
      coordarrays[2] = name;
    }
    if (this->PointDataArraySelection->ArrayIsEnabled(name))
    {
      processarray = true;
    }
    if (!processarray)
    {
      continue;
    }

    // make sure we cater for multi-component vector fields
    int vectorcomponent;
    if ((vectorcomponent = this->IndexOfVectorComponent(name)) > 0)
    {
      std::string vectorname = this->NameOfVectorComponent(name) + "_v";
      FieldMap::iterator pos = scalarFields.find(vectorname);
      if (pos == scalarFields.end())
      {
        std::vector<std::string> arraylist(1, name);
        FieldMap::value_type element(vectorname, arraylist);
        scalarFields.insert(element);
      }
      else
      {
        pos->second.reserve(vectorcomponent);
        pos->second.resize(std::max((int)(pos->second.size()), vectorcomponent));
        pos->second[vectorcomponent - 1] = name;
      }
    }
    else
    {
      std::vector<std::string> arraylist(1, name);
      FieldMap::value_type element(name, arraylist);
      scalarFields.insert(element);
    }
  }
  //
  FieldMap::iterator coordvector = scalarFields.end();
  for (FieldMap::iterator pos = scalarFields.begin(); pos != scalarFields.end(); ++pos)
  {
    if (pos->second.size() == 3 && (pos->second[0] == coordarrays[0]) &&
      (pos->second[1] == coordarrays[1]) && (pos->second[2] == coordarrays[2]))
    {
      // change the name of this entry to "coords" to ensure we use it as such
      FieldMap::value_type element("Coords", pos->second);
      scalarFields.erase(pos);
      coordvector = scalarFields.insert(element).first;
      break;
    }
  }

  if (coordvector == scalarFields.end())
  {
    FieldMap::value_type element("Coords", coordarrays);
    scalarFields.insert(element);
  }

  //
  // Get the TimeStep Requested from the information if present
  //
  this->TimeOutOfRange = 0;
  this->ActualTimeStep = this->TimeStep;
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    double requestedTimeValue = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    this->ActualTimeStep = std::find_if(this->TimeStepValues.begin(), this->TimeStepValues.end(),
                             [this, &requestedTimeValue](double timeStepValue)
                             {
                               return vtkMathUtilities::FuzzyCompare(
                                 timeStepValue, requestedTimeValue, this->TimeStepTolerance);
                             }) -
      this->TimeStepValues.begin();
    //
    if (requestedTimeValue < this->TimeStepValues.front() ||
      requestedTimeValue > this->TimeStepValues.back())
    {
      this->TimeOutOfRange = 1;
    }
    output->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), requestedTimeValue);
  }
  else
  {
    double timevalue[1];
    unsigned int index = this->ActualTimeStep;
    if (index < this->TimeStepValues.size())
    {
      timevalue[0] = this->TimeStepValues[index];
    }
    else
    {
      timevalue[0] = this->TimeStepValues[0];
    }
    output->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), timevalue[0]);
  }

  if (this->TimeOutOfRange && this->MaskOutOfTimeRangeOutput)
  {
    // don't do anything, just return success
    return 1;
  }

  // Set the TimeStep on the H5 file
  H5SetStep(this->H5FileId, this->ActualTimeStep);
  // first we unset any previous view so that we get the global number of points
  H5PartSetView(this->H5FileId, -1, -1);
  // Get the number of points for this step
  vtkIdType Nt = H5PartGetNumItems(this->H5FileId);
  if (piece < Nt)
  {
    if (numPieces > 1)
    {
      vtkIdType div = Nt / numPieces;
      vtkIdType rem = Nt % numPieces;

      vtkIdType myNt = piece < rem ? div + 1 : div;
      vtkIdType myOffset = piece < rem ? (div + 1) * piece : (div + 1) * rem + div * (piece - rem);
      H5PartSetView(this->H5FileId, myOffset, myOffset + myNt - 1);
      Nt = myNt;
    }
  }
  else
  {
    // don't do anything.
    return 1;
  }

  // Setup arrays for reading data
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkDataArray> coords = nullptr;
  for (const auto& [scalarName, arraylist] : scalarFields)
  {
    // use the type of the first array for all if it is a vector field
    const char* array_name = arraylist[0].c_str();
    std::string rootname = this->NameOfVectorComponent(array_name);
    int Nc = static_cast<int>(arraylist.size());

    h5_int64_t datatype, datatype_comp;
    h5_size_t nelem, nelem_comp;
    if (H5PartGetDatasetInfoByName(this->H5FileId, array_name, &datatype, &nelem) != H5_SUCCESS)
    {
      vtkErrorMacro("Could not get dataset info for array " << array_name);
      return 0;
    }
    int vtk_datatype = GetVTKDataType(datatype);

    if (vtk_datatype != VTK_VOID)
    {
      // now read the data components.
      std::vector<void*> componentPtrs(Nc, nullptr);
      for (int c = 0; c < Nc; c++)
      {
        const char* name_comp = arraylist[c].c_str();
        if (H5PartGetDatasetInfoByName(this->H5FileId, name_comp, &datatype_comp, &nelem_comp) !=
          H5_SUCCESS)
        {
          vtkErrorMacro("Could not get dataset info for array " << array_name);
          return 0;
        }
        if (datatype_comp != datatype)
        {
          vtkErrorMacro("Inconsistent data types for vector components of " << rootname);
          return 0;
        }
        // Read using H5hut's high-level API based on type
        h5_err_t status;
        switch (datatype)
        {
          case H5_FLOAT64_T:
            componentPtrs[c] = new h5_float64_t[Nt];
            status = H5PartReadDataFloat64(
              this->H5FileId, name_comp, static_cast<h5_float64_t*>(componentPtrs[c]));
            break;
          case H5_FLOAT32_T:
            componentPtrs[c] = new h5_float32_t[Nt];
            status = H5PartReadDataFloat32(
              this->H5FileId, name_comp, static_cast<h5_float32_t*>(componentPtrs[c]));
            break;
          case H5_INT64_T:
            componentPtrs[c] = new h5_int64_t[Nt];
            status = H5PartReadDataInt64(
              this->H5FileId, name_comp, static_cast<h5_int64_t*>(componentPtrs[c]));
            break;
          case H5_INT32_T:
            componentPtrs[c] = new h5_int32_t[Nt];
            status = H5PartReadDataInt32(
              this->H5FileId, name_comp, static_cast<h5_int32_t*>(componentPtrs[c]));
            break;
          default:
            vtkErrorMacro("Unsupported data type for component " << name_comp);
            return 0;
        }
        if (status != H5_SUCCESS)
        {
          vtkErrorMacro("Failed to read component " << name_comp);
          return 0;
        }
      }
      vtkSmartPointer<vtkDataArray> dataArray =
        CreateDataArray(vtk_datatype, componentPtrs, Nt, rootname);
      if (scalarName == "Coords")
      {
        coords = dataArray;
      }
      else
      {
        output->GetPointData()->AddArray(dataArray);
        if (!output->GetPointData()->GetScalars())
        {
          output->GetPointData()->SetActiveScalars(dataArray->GetName());
        }
      }
    }
  }

  if (this->GenerateVertexCells)
  {
    vtkNew<vtkTypeInt64Array> connectivity;
    connectivity->SetNumberOfValues(Nt);
    std::iota(connectivity->Begin(), connectivity->End(), 0);
    vtkNew<vtkCellArray> vertices;
    vertices->SetData(1, connectivity);
    output->SetVerts(vertices);
  }
  coords->SetName("Points");
  points->SetData(coords);
  output->SetPoints(points);
  return 1;
}

//------------------------------------------------------------------------------
int vtkH5PartReader::GetCoordinateArrayStatus(const char* name)
{
  return this->PointDataArraySelection->ArrayIsEnabled(name);
}
//------------------------------------------------------------------------------
void vtkH5PartReader::SetCoordinateArrayStatus(const char* name, int status)
{
  if (status)
  {
    this->PointDataArraySelection->EnableArray(name);
  }
  else
  {
    this->PointDataArraySelection->DisableArray(name);
  }
}

//------------------------------------------------------------------------------
const char* vtkH5PartReader::GetPointArrayName(int index)
{
  return this->PointDataArraySelection->GetArrayName(index);
}
//------------------------------------------------------------------------------
int vtkH5PartReader::GetPointArrayStatus(const char* name)
{
  return this->PointDataArraySelection->ArrayIsEnabled(name);
}
//------------------------------------------------------------------------------
void vtkH5PartReader::SetPointArrayStatus(const char* name, int status)
{
  if (status != this->GetPointArrayStatus(name))
  {
    if (status)
    {
      this->PointDataArraySelection->EnableArray(name);
    }
    else
    {
      this->PointDataArraySelection->DisableArray(name);
    }
    this->Modified();
  }
}
//------------------------------------------------------------------------------
void vtkH5PartReader::Enable(const char* name)
{
  this->SetPointArrayStatus(name, 1);
}
//------------------------------------------------------------------------------
void vtkH5PartReader::Disable(const char* name)
{
  this->SetPointArrayStatus(name, 0);
}
//------------------------------------------------------------------------------
void vtkH5PartReader::EnableAll()
{
  this->PointDataArraySelection->EnableAllArrays();
}
//------------------------------------------------------------------------------
void vtkH5PartReader::DisableAll()
{
  this->PointDataArraySelection->DisableAllArrays();
}
//------------------------------------------------------------------------------
int vtkH5PartReader::GetNumberOfPointArrays()
{
  return this->PointDataArraySelection->GetNumberOfArrays();
}
//------------------------------------------------------------------------------
void vtkH5PartReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "FileName: " << (this->FileName ? this->FileName : "(none)") << "\n";

  os << indent << "NumberOfSteps: " << this->NumberOfTimeSteps << "\n";
}
VTK_ABI_NAMESPACE_END
