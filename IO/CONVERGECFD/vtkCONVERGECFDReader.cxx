/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCONVERGECFDReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCONVERGECFDReader.h"

#include "vtkBuffer.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCommand.h"
#include "vtkDataArraySelection.h"
#include "vtkDataAssembly.h"
#include "vtkDirectory.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"

#include <vtksys/RegularExpression.hxx>

#include <algorithm>
#include <array>
#include <map>
#include <set>
#include <sstream>

#define H5_USE_16_API
#include "vtk_hdf5.h"

#include "vtkHDF5ScopedHandle.h"

vtkStandardNewMacro(vtkCONVERGECFDReader);

namespace
{

//----------------------------------------------------------------------------
/**
 *  Check existence of array defined by pathName relative to fileId.
 */
bool ArrayExists(hid_t fileId, const char* pathName)
{
  return (H5Lexists(fileId, pathName, H5P_DEFAULT) > 0);
}

//----------------------------------------------------------------------------
/**
 *  Check existence of array defined by groupName relative to fileId.
 */
bool GroupExists(hid_t fileId, const char* groupName)
{
  // Same implementation as ArrayExists, but that's okay.
  return (H5Lexists(fileId, groupName, H5P_DEFAULT) > 0);
}

//----------------------------------------------------------------------------
/**
 *  Get length of array defined by pathName relative to fileId.
 */
hsize_t GetDataLength(hid_t fileId, const char* pathName)
{
  vtkHDF::ScopedH5DHandle arrayId = H5Dopen(fileId, pathName);
  if (arrayId < 0)
  {
    vtkGenericWarningMacro("No array named " << pathName << " available");
    return 0;
  }

  vtkHDF::ScopedH5DHandle dataspace = H5Dget_space(arrayId);
  if (H5Sget_simple_extent_ndims(dataspace) != 1)
  {
    vtkGenericWarningMacro("Array " << pathName << " dimensionality is not 1");
    return 0;
  }

  hsize_t length = 0;
  int numDimensions = H5Sget_simple_extent_dims(dataspace, &length, nullptr);
  if (numDimensions < 0)
  {
    vtkGenericWarningMacro("Failed to get length of array");
    return 0;
  }

  return length;
}

//----------------------------------------------------------------------------
/**
 *  Read a typed array and into an array passed in by the caller. Checks that the
 *  number of elements in the array specified by fileId and pathName matches the length
 *  argument n.
 *
 *  Returns true if reading succeeded, false otherwise.
 */
template <typename T>
bool ReadArray(hid_t fileId, const char* pathName, T* data, hsize_t n)
{
  vtkHDF::ScopedH5DHandle arrayId = H5Dopen(fileId, pathName);
  if (arrayId < 0)
  {
    return false;
  }

  vtkHDF::ScopedH5DHandle rawType = H5Dget_type(arrayId);
  vtkHDF::ScopedH5THandle dataType = H5Tget_native_type(rawType, H5T_DIR_ASCEND);
  vtkHDF::ScopedH5DHandle dataspace = H5Dget_space(arrayId);
  if (H5Sget_simple_extent_ndims(dataspace) != 1)
  {
    vtkGenericWarningMacro("Array " << pathName << " dimensionality is not 1");
    return false;
  }

  hsize_t length = 0;
  int numDims = H5Sget_simple_extent_dims(dataspace, &length, nullptr);
  if (numDims < 0)
  {
    vtkGenericWarningMacro("Failed to get length of array");
    return false;
  }

  if (n != length)
  {
    vtkGenericWarningMacro(
      "Size of array passed in does not match length of array. Skipping array.");
    return false;
  }

  if (H5Dread(arrayId, dataType, H5S_ALL, H5S_ALL, H5P_DEFAULT, data) < 0)
  {
    vtkGenericWarningMacro("Could not read " << pathName);
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------
/**
 * Get an array of strings from a table defined by pathName relative to fileId.
 * Strings are returned in the vector of strings parameter that was passed in.
 */
bool ReadStrings(hid_t fileId, const char* path, std::vector<std::string>& strings)
{
  vtkHDF::ScopedH5DHandle stringsId = H5Dopen(fileId, path);
  if (stringsId < 0)
  {
    vtkGenericWarningMacro("Could not read " << path);
    return false;
  }

  vtkHDF::ScopedH5THandle filetype = H5Dget_type(stringsId);
  size_t sdim = H5Tget_size(filetype);
  sdim++; /* Make room for null terminator */

  vtkHDF::ScopedH5SHandle space = H5Dget_space(stringsId);
  hsize_t dim;
  int ndims = H5Sget_simple_extent_dims(space, &dim, nullptr);
  if (ndims != 1)
  {
    vtkGenericWarningMacro("String array dimension not 1");
    return false;
  }

  char** rdata = new char*[dim];
  rdata[0] = new char[dim * sdim];
  for (hsize_t i = 1; i < dim; ++i)
  {
    rdata[i] = rdata[0] + i * sdim;
  }

  vtkHDF::ScopedH5THandle memtype = H5Tcopy(H5T_C_S1);
  H5Tset_size(memtype, sdim);
  if (H5Dread(stringsId, memtype, H5S_ALL, H5S_ALL, H5P_DEFAULT, rdata[0]) < 0)
  {
    vtkGenericWarningMacro("Could not read " << path);
    return false;
  }

  strings.clear();
  for (hsize_t i = 0; i < dim; ++i)
  {
    strings.emplace_back(std::string(rdata[i]));
  }

  delete[] rdata[0];
  delete[] rdata;

  return true;
}

void SplitScalarAndVectorVariables(
  std::vector<std::string>& allVariables, std::vector<std::string>& vectorVariables)
{
  vectorVariables.clear();
  for (const auto& varName : allVariables)
  {
    // See if variable is an array.
    if (varName.find_last_of('_') == varName.size() - 2)
    {
      const char componentName = varName[varName.size() - 1];
      if (componentName == 'X')
      {
        // Check that components Y and Z exist as well
        std::string baseName = varName.substr(0, varName.size() - 2);
        if (std::find(allVariables.begin(), allVariables.end(), baseName + "_Y") !=
            allVariables.end() &&
          std::find(allVariables.begin(), allVariables.end(), baseName + "_Z") !=
            allVariables.end())
        {
          vectorVariables.emplace_back(baseName);
        }
      }
    }
  }

  // Now remove the vector variables from all variables. At the end, allVariables will contain
  // only scalar array names.
  for (const auto& varName : vectorVariables)
  {
    allVariables.erase(std::find(allVariables.begin(), allVariables.end(), varName + "_X"));
    allVariables.erase(std::find(allVariables.begin(), allVariables.end(), varName + "_Y"));
    allVariables.erase(std::find(allVariables.begin(), allVariables.end(), varName + "_Z"));
  }
}

} // anonymous namespace

//----------------------------------------------------------------------------
class vtkCONVERGECFDReader::vtkInternal
{
public:
  vtkCONVERGECFDReader* Self;
  std::vector<std::string> CellDataScalarVariables;
  std::vector<std::string> CellDataVectorVariables;
  std::vector<std::string> ParcelDataTypes;
  std::vector<std::string> ParcelDataScalarVariables;
  std::vector<std::string> ParcelDataVectorVariables;

  // Clears out variable info
  void Reset()
  {
    this->CellDataScalarVariables.clear();
    this->CellDataVectorVariables.clear();
    this->ParcelDataTypes.clear();
  }

  // Get a parcel dataset at a given path
  vtkSmartPointer<vtkPolyData> ReadParcelDataSet(hid_t streamId, const std::string& path)
  {
    vtkSmartPointer<vtkPolyData> parcels = vtkSmartPointer<vtkPolyData>::New();

    // Build PARCEL_X address string from path name
    std::string parcelXPath = path + "/PARCEL_X";

    // Read parcel point locations
    hsize_t parcelLength = GetDataLength(streamId, parcelXPath.c_str());

    vtkNew<vtkFloatArray> parcelPointArray;
    parcelPointArray->SetNumberOfComponents(3);
    parcelPointArray->SetNumberOfTuples(parcelLength);

    vtkNew<vtkBuffer<float>> floatBuffer;
    floatBuffer->Allocate(parcelLength);

    std::array<char, 3> dimensionNames = { 'X', 'Y', 'Z' };
    for (size_t c = 0; c < dimensionNames.size(); ++c)
    {
      std::stringstream name;
      name << path << "/PARCEL_" << dimensionNames[c];
      if (!ReadArray(streamId, name.str().c_str(), floatBuffer->GetBuffer(), parcelLength))
      {
        vtkGenericWarningMacro(
          "No parcel coordinate array " << name.str() << " dataset available in " << name.str());
        return nullptr;
      }

      for (hsize_t j = 0; j < parcelLength; ++j)
      {
        parcelPointArray->SetTypedComponent(j, static_cast<int>(c), floatBuffer->GetBuffer()[j]);
      }
    }

    vtkNew<vtkPoints> parcelPoints;
    parcelPoints->SetData(parcelPointArray);

    parcels->SetPoints(parcelPoints);

    // Create a vertex for each parcel point
    vtkNew<vtkCellArray> parcelCells;
    parcelCells->AllocateExact(parcelLength, 1);
    for (vtkIdType id = 0; id < static_cast<vtkIdType>(parcelLength); ++id)
    {
      parcelCells->InsertNextCell(1, &id);
    }
    parcels->SetVerts(parcelCells);

    // Read parcel data arrays
    for (int i = 0; i < this->Self->ParcelDataArraySelection->GetNumberOfArrays(); ++i)
    {
      std::string varName(this->Self->ParcelDataArraySelection->GetArrayName(i));
      if (varName == "PARCEL_X" || varName == "PARCEL_Y" || varName == "PARCEL_Z" ||
        this->Self->ParcelDataArraySelection->ArrayIsEnabled(varName.c_str()) == 0)
      {
        continue;
      }

      auto begin = this->ParcelDataVectorVariables.begin();
      auto end = this->ParcelDataVectorVariables.end();
      bool isVector = std::find(begin, end, varName) != end;

      // This would be a lot simpler using a vtkSOADataArrayTemplate<float>, but
      // until GetVoidPointer() is removed from more of the VTK code base, we
      // will use a vtkFloatArray.
      vtkNew<vtkFloatArray> dataArray;
      bool success = true;
      if (isVector)
      {
        std::string pathX = path + "/" + varName + "_X";
        std::string pathY = path + "/" + varName + "_Y";
        std::string pathZ = path + "/" + varName + "_Z";

        if (!ArrayExists(streamId, pathX.c_str()))
        {
          // This array just doesn't exist in this stream, skip it.
          continue;
        }

        // hsize_t length = GetDataLength(streamId, pathX.c_str());
        dataArray->SetNumberOfComponents(3);
        dataArray->SetNumberOfTuples(parcelLength);
        dataArray->SetName(varName.c_str());

        if (static_cast<hsize_t>(floatBuffer->GetSize()) != parcelLength)
        {
          floatBuffer->Allocate(parcelLength);
        }
        success =
          success && ReadArray(streamId, pathX.c_str(), floatBuffer->GetBuffer(), parcelLength);
        for (hsize_t j = 0; j < parcelLength; ++j)
        {
          dataArray->SetTypedComponent(j, 0, floatBuffer->GetBuffer()[j]);
        }
        success =
          success && ReadArray(streamId, pathY.c_str(), floatBuffer->GetBuffer(), parcelLength);
        for (hsize_t j = 0; j < parcelLength; ++j)
        {
          dataArray->SetTypedComponent(j, 1, floatBuffer->GetBuffer()[j]);
        }
        success =
          success && ReadArray(streamId, pathZ.c_str(), floatBuffer->GetBuffer(), parcelLength);
        for (hsize_t j = 0; j < parcelLength; ++j)
        {
          dataArray->SetTypedComponent(j, 2, floatBuffer->GetBuffer()[j]);
        }
      }
      else // !is_vector
      {
        std::string varPath(path);
        varPath += "/" + varName;

        if (!ArrayExists(streamId, varPath.c_str()))
        {
          // This array just doesn't exist in this stream, skip it.
          continue;
        }

        dataArray->SetNumberOfComponents(1);
        dataArray->SetNumberOfTuples(parcelLength);
        dataArray->SetName(varName.c_str());
        success =
          success && ReadArray(streamId, varPath.c_str(), dataArray->GetPointer(0), parcelLength);
      }

      if (success)
      {
        parcels->GetPointData()->AddArray(dataArray);
      }
    }

    return parcels;
  }
};

//----------------------------------------------------------------------------
vtkCONVERGECFDReader::vtkCONVERGECFDReader()
  : FileName(nullptr)
  , Internal(new vtkCONVERGECFDReader::vtkInternal())
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  this->Internal->Self = this;

  this->CellDataArraySelection->AddObserver(
    vtkCommand::ModifiedEvent, this, &vtkCONVERGECFDReader::Modified);
  this->ParcelDataArraySelection->AddObserver(
    vtkCommand::ModifiedEvent, this, &vtkCONVERGECFDReader::Modified);
}

//----------------------------------------------------------------------------
vtkCONVERGECFDReader::~vtkCONVERGECFDReader()
{
  delete[] this->FileName;
  this->FileName = nullptr;
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkCONVERGECFDReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkCONVERGECFDReader::RequestInformation(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outInfos)
{
  if (!this->FileName || this->FileName[0] == '\0')
  {
    return 1;
  }

  // Reset internal information
  this->Internal->Reset();

  vtkHDF::ScopedH5FHandle fileId = H5Fopen(this->FileName, H5F_ACC_RDONLY, H5P_DEFAULT);
  if (fileId < 0)
  {
    vtkErrorMacro("Could not open HDF5 file '" << this->FileName << "'");
    return 0;
  }

  // Iterate over all streams to find available cell data arrays and parcel data arrays
  std::set<std::string> cellVariables;
  std::set<std::string> parcelVariables;
  int streamCount = 0;
  do
  {
    herr_t status = 0;
    std::ostringstream streamName;
    streamName << "/STREAM_" << std::setw(2) << std::setfill('0') << streamCount;
    H5Eset_auto(nullptr, nullptr);
    status = H5Gget_objinfo(fileId, streamName.str().c_str(), false, nullptr);
    if (status < 0)
    {
      break;
    }

    // Open the group
    vtkHDF::ScopedH5GHandle streamId = H5Gopen(fileId, streamName.str().c_str());
    if (streamId < 0)
    {
      // Group exists, but could not be opened
      vtkErrorMacro("Could not open stream " << streamName.str());
      break;
    }

    std::vector<std::string> cellDataVariables;
    if (!ReadStrings(streamId, "VARIABLE_NAMES/CELL_VARIABLES", cellDataVariables))
    {
      vtkErrorMacro("Could not read cell variable names");
      return 0;
    }

    // Insert variables into set to ensure uniqueness
    for (auto& cellVariableName : cellDataVariables)
    {
      cellVariables.insert(cellVariableName);
    }

    // Pre- 3.1 format
    std::vector<std::string> parcelDataScalarVariables;
    if (ArrayExists(streamId, "VARIABLE_NAMES/PARCEL_VARIABLES"))
    {
      if (!ReadStrings(streamId, "VARIABLE_NAMES/PARCEL_VARIABLES", parcelDataScalarVariables))
      {
        vtkErrorMacro("Could not read parcel variable names");
        return 0;
      }

      // Copy to set of names to ensure uniqueness
      for (auto& parcelDataArrayName : parcelDataScalarVariables)
      {
        parcelVariables.insert(parcelDataArrayName);
      }
    }
    else
    {
      // 3.1 and later format
      vtkHDF::ScopedH5GHandle varNamesHandle = H5Gopen(streamId, "VARIABLE_NAMES");
      if (varNamesHandle < 0)
      {
        vtkErrorMacro("Cannot open /" << streamId << "/VARIABLE_NAMES");
        return 0;
      }

      // Iterate over parcel variable names
      hsize_t numVariableTypes = 0;
      herr_t err = H5Gget_num_objs(varNamesHandle, &numVariableTypes);
      if (err < 0)
      {
        vtkErrorMacro("Cannot get number of groups from file");
        return 0;
      }

      for (hsize_t i = 0; i < numVariableTypes; ++i)
      {
        status = 0;
        char groupName[256];
        status = H5Lget_name_by_idx(streamId, "VARIABLE_NAMES/", H5_INDEX_NAME, H5_ITER_NATIVE, i,
          groupName, 256, H5P_DEFAULT);
        if (status < 0)
        {
          vtkErrorMacro(<< "error reading parcel variable names");
          break;
        }

        std::string groupNameString(groupName);
        if (groupNameString == "CELL_VARIABLES")
        {
          continue;
        }

        auto const underscorePos = groupNameString.find_last_of('_');
        std::string parcelTypePrefix = groupNameString.substr(0, underscorePos);

        std::string parcelVariablesGroupName = parcelTypePrefix + "_VARIABLES";
        std::string parcelVariableTypeName = parcelTypePrefix + "_DATA";

        // Read parcel array names
        std::string parcelDataGroup("VARIABLE_NAMES/");
        parcelDataGroup += parcelVariablesGroupName;
        if (ArrayExists(streamId, parcelDataGroup.c_str()))
        {
          std::vector<std::string> parcelScalarVariables;
          if (!ReadStrings(streamId, parcelDataGroup.c_str(), parcelScalarVariables))
          {
            vtkErrorMacro("Could not read parcel variable names");
            return 0;
          }

          // Insert variable name into set to ensure uniqueness
          for (auto& var : parcelScalarVariables)
          {
            parcelVariables.insert(var);
          }
        }
      }
    }

    streamCount++;
  } while (true); // end iterating over streams

  constexpr bool defaultEnabledState = true;

  // Set up cell data array selection
  this->Internal->CellDataScalarVariables.clear();
  this->Internal->CellDataVectorVariables.clear();

  for (auto& cellArrayName : cellVariables)
  {
    this->Internal->CellDataScalarVariables.emplace_back(cellArrayName);
  }

  // Split cell variables into scalar and vector arrays
  SplitScalarAndVectorVariables(
    this->Internal->CellDataScalarVariables, this->Internal->CellDataVectorVariables);

  for (const auto& varName : this->Internal->CellDataScalarVariables)
  {
    if (!this->CellDataArraySelection->ArrayExists(varName.c_str()))
    {
      this->CellDataArraySelection->AddArray(varName.c_str(), defaultEnabledState);
    }
  }

  for (const auto& varName : this->Internal->CellDataVectorVariables)
  {
    if (!this->CellDataArraySelection->ArrayExists(varName.c_str()))
    {
      this->CellDataArraySelection->AddArray(varName.c_str(), defaultEnabledState);
    }
  }

  // Set up parcel data array selection
  this->Internal->ParcelDataScalarVariables.clear();
  this->Internal->ParcelDataVectorVariables.clear();

  for (auto& parcelArrayName : parcelVariables)
  {
    this->Internal->ParcelDataScalarVariables.emplace_back(parcelArrayName);
  }

  // Split parcel arrays into scalar and vector variables
  SplitScalarAndVectorVariables(
    this->Internal->ParcelDataScalarVariables, this->Internal->ParcelDataVectorVariables);

  // Set up data array status
  for (const auto& varName : this->Internal->ParcelDataScalarVariables)
  {
    if (!this->ParcelDataArraySelection->ArrayExists(varName.c_str()))
    {
      this->ParcelDataArraySelection->AddArray(varName.c_str(), defaultEnabledState);
    }
  }

  for (const auto& varName : this->Internal->ParcelDataVectorVariables)
  {
    // Skip X, Y, Z points
    if (varName == "PARCEL")
    {
      continue;
    }

    if (!this->ParcelDataArraySelection->ArrayExists(varName.c_str()))
    {
      this->ParcelDataArraySelection->AddArray(varName.c_str(), defaultEnabledState);
    }
  }

  // Get time information
  vtkInformation* outInfo = outInfos->GetInformationObject(0);
  this->ReadTimeSteps(outInfo);

  return 1;
}

//----------------------------------------------------------------------------
int vtkCONVERGECFDReader::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  size_t fileIndex = this->SelectTimeStepIndex(outInfo);
  std::string fileName = this->FileNames[fileIndex];

  if (fileName.empty())
  {
    vtkErrorMacro("No file sequence found");
    return 0;
  }

  vtkPartitionedDataSetCollection* outputPDC = vtkPartitionedDataSetCollection::GetData(outInfo);
  if (!outputPDC)
  {
    vtkErrorMacro("No output available!");
    return 0;
  }

  vtkNew<vtkDataAssembly> hierarchy;
  hierarchy->Initialize();
  outputPDC->SetDataAssembly(hierarchy);

  vtkHDF::ScopedH5FHandle fileId = H5Fopen(fileName.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
  if (fileId < 0)
  {
    vtkErrorMacro("Could not open HDF5 file '" << fileName << "'");
    return 0;
  }

  vtkHDF::ScopedH5GHandle boundaryHandle = H5Gopen(fileId, "/BOUNDARIES");
  if (boundaryHandle < 0)
  {
    vtkErrorMacro("Cannot open group/BOUNDARIES");
    return 0;
  }

  // Iterate over stream groups
  hsize_t numObjs = 0;
  herr_t err = H5Gget_num_objs(fileId, &numObjs);
  if (err < 0)
  {
    vtkErrorMacro("Cannot get number of groups from file");
    return 0;
  }

  int streamCount = 0;
  do
  {
    herr_t status = 0;
    std::ostringstream streamName;
    streamName << "/STREAM_" << std::setw(2) << std::setfill('0') << streamCount;
    H5Eset_auto(nullptr, nullptr);
    status = H5Gget_objinfo(fileId, streamName.str().c_str(), false, nullptr);
    if (status < 0)
    {
      break;
    }

    // Open the group
    vtkHDF::ScopedH5GHandle streamId = H5Gopen(fileId, streamName.str().c_str());
    if (streamId < 0)
    {
      vtkErrorMacro("Could not open stream " << streamName.str());
      break;
    }

    if (!ArrayExists(streamId, "VERTEX_COORDINATES/X"))
    {
      vtkErrorMacro("Could not find array VERTEX_COORDINATES/X");
      break;
    }

    std::stringstream streamss;
    streamss << "STREAM_" << std::setw(2) << std::setfill('0') << streamCount;
    int streamNodeId = hierarchy->AddNode(streamss.str().c_str(), 0 /* root */);

    hsize_t xCoordsLength = GetDataLength(streamId, "VERTEX_COORDINATES/X");

    // Temporary buffer for reading vector array components
    vtkNew<vtkBuffer<float>> floatBuffer;

    vtkNew<vtkFloatArray> pointArray;
    pointArray->SetNumberOfComponents(3);
    pointArray->SetNumberOfTuples(xCoordsLength);

    floatBuffer->Allocate(xCoordsLength);

    std::array<char, 3> dimensionNames = { 'X', 'Y', 'Z' };

    for (size_t c = 0; c < dimensionNames.size(); ++c)
    {
      std::stringstream name;
      name << "VERTEX_COORDINATES/" << dimensionNames[c];
      if (!ReadArray(streamId, name.str().c_str(), floatBuffer->GetBuffer(), xCoordsLength))
      {
        vtkErrorMacro(
          "No coordinate array " << name.str() << " dataset available in " << streamName.str());
        return 0;
      }

      for (hsize_t j = 0; j < xCoordsLength; ++j)
      {
        pointArray->SetTypedComponent(j, static_cast<int>(c), floatBuffer->GetBuffer()[j]);
      }
    }

    // ++++ POLYGON_OFFSET ++++
    hsize_t polygonOffsetsLength = GetDataLength(streamId, "CONNECTIVITY/POLYGON_OFFSET");
    std::vector<int> polygonOffsets(polygonOffsetsLength);
    if (!ReadArray(
          streamId, "CONNECTIVITY/POLYGON_OFFSET", polygonOffsets.data(), polygonOffsetsLength))
    {
      vtkErrorMacro("Could not read CONNECTIVITY/POLYGON_OFFSET");
      return 0;
    }

    // Reduce the number of polygons by one to make up for the fact that the POLYGON_OFFSETS
    // array is longer by one row.
    vtkIdType numPolygons = static_cast<vtkIdType>(polygonOffsets.size()) - 1;

    // ++++ POLYGON_TO_VERTEX ++++
    hsize_t polygonsLength = GetDataLength(streamId, "CONNECTIVITY/POLYGON_TO_VERTEX");
    std::vector<int> polygons(polygonsLength);
    if (!ReadArray(streamId, "CONNECTIVITY/POLYGON_TO_VERTEX", polygons.data(), polygonsLength))
    {
      vtkErrorMacro("Could not read CONNECTIVITY/POLYGON_TO_VERTEX");
      return 0;
    }

    // ++++ CONNECTED_CELLS ++++
    hsize_t connectedCellsLength = GetDataLength(streamId, "CONNECTIVITY/CONNECTED_CELLS");
    std::vector<int> connectedCells(connectedCellsLength);
    if (!ReadArray(
          streamId, "CONNECTIVITY/CONNECTED_CELLS", connectedCells.data(), connectedCellsLength))
    {
      vtkErrorMacro("Could not read CONNECTIVITY/CONNECTED_CELLS");
      return 0;
    }

    // ++++ CREATE VTK DATA SETS ++++
    vtkNew<vtkPoints> points;
    points->SetData(pointArray);

    // boundaryIdToIndex must be size of max id... ids are not guaranteed to be sequential,
    // i.e., 1, 3, 5, 30, 31, 32, 1001 so it's better to use map instead of array lookup
    std::map<int, int> boundaryIdToIndex;

    hsize_t numBoundaryNames = GetDataLength(boundaryHandle, "BOUNDARY_NAMES");
    std::vector<std::string> boundaryNames(numBoundaryNames);
    ReadStrings(boundaryHandle, "BOUNDARY_NAMES", boundaryNames);
    hsize_t numBoundaries = GetDataLength(boundaryHandle, "NUM_ELEMENTS");
    std::vector<int> boundaryNumElements(numBoundaries);
    ReadArray(
      boundaryHandle, "NUM_ELEMENTS", boundaryNumElements.data(), boundaryNumElements.size());
    std::vector<int> boundaryIds(numBoundaries);
    ReadArray(boundaryHandle, "BOUNDARY_IDS", boundaryIds.data(), boundaryIds.size());
    if (numBoundaries != numBoundaryNames)
    {
      vtkErrorMacro("Number of BOUNDARY_NAMES does not match NUM_ELEMENTS");
      return 0;
    }

    // Make mesh the first node in the stream and put it first in the collection
    int meshNodeId = hierarchy->AddNode("Mesh", streamNodeId);
    int meshStartId = outputPDC->GetNumberOfPartitionedDataSets();
    outputPDC->SetNumberOfPartitionedDataSets(meshStartId + 1);

    vtkNew<vtkUnstructuredGrid> ugrid;
    outputPDC->SetPartition(meshStartId, 0, ugrid);
    outputPDC->GetMetaData(meshStartId)->Set(vtkCompositeDataSet::NAME(), "Mesh");
    hierarchy->AddDataSetIndex(meshNodeId, meshStartId);

    // Multiple surfaces can exist in a single file. We create a vtkPolyData for
    // each one and store them under another group in the partitioned dataset collection.
    unsigned int streamSurfaceStartId = outputPDC->GetNumberOfPartitionedDataSets();
    outputPDC->SetNumberOfPartitionedDataSets(
      streamSurfaceStartId + static_cast<unsigned int>(boundaryIds.size()));

    int surfaceNodeId = hierarchy->AddNode("Surfaces", streamNodeId);
    for (int i = 0; i < static_cast<int>(boundaryIds.size()); ++i)
    {
      // If boundary index 0 has boundary id == 1, index 1 of boundaryIdToIndex will be 0.
      boundaryIdToIndex[boundaryIds[i]] = static_cast<int>(i);

      vtkNew<vtkPolyData> boundarySurface;
      outputPDC->SetPartition(streamSurfaceStartId + i, 0, boundarySurface);
      outputPDC->GetMetaData(streamSurfaceStartId + static_cast<unsigned int>(i))
        ->Set(vtkCompositeDataSet::NAME(), boundaryNames[i]);

      vtkNew<vtkCellArray> polys;
      polys->AllocateEstimate(boundaryNumElements[i], 4);
      boundarySurface->SetPolys(polys);
      std::string validName = vtkDataAssembly::MakeValidNodeName(boundaryNames[i].c_str());
      unsigned int boundaryNodeId = hierarchy->AddNode(validName.c_str(), surfaceNodeId);
      hierarchy->AddDataSetIndex(boundaryNodeId, streamSurfaceStartId + i);
    }

    // Create maps from surface point IDs for each block
    std::vector<std::set<vtkIdType>> blocksSurfacePointIds(boundaryIds.size());
    for (int polyId = 0; polyId < numPolygons; ++polyId)
    {
      if (connectedCells[2 * polyId + 0] >= 0 && connectedCells[2 * polyId + 1] >= 0)
      {
        // Polygon is not part of a surface, so skip.
        continue;
      }

      int boundaryId = -(connectedCells[2 * polyId + 0] + 1);
      int boundaryIndex = boundaryIdToIndex[boundaryId];
      vtkIdType numCellPts =
        static_cast<vtkIdType>(polygonOffsets[polyId + 1] - polygonOffsets[polyId]);
      for (vtkIdType id = 0; id < numCellPts; ++id)
      {
        vtkIdType ptId = polygons[polygonOffsets[polyId] + id];
        blocksSurfacePointIds[boundaryIndex].insert(ptId);
      }
    }

    // Create maps from original point IDs to surface point IDs for each block
    std::vector<std::map<vtkIdType, vtkIdType>> blocksOriginalToBlockPointId(numBoundaryNames);
    for (hsize_t boundaryIndex = 0; boundaryIndex < numBoundaryNames; ++boundaryIndex)
    {
      // Create a map from original point ID in the global points list
      vtkIdType newIndex = 0;
      for (vtkIdType id : blocksSurfacePointIds[boundaryIndex])
      {
        blocksOriginalToBlockPointId[boundaryIndex][id] = newIndex++;
      }

      // Clear some memory
      blocksSurfacePointIds[boundaryIndex].clear();

      // Create localized points for this block
      vtkNew<vtkPoints> blockPoints;
      blockPoints->SetDataType(points->GetDataType());
      blockPoints->SetNumberOfPoints(newIndex);
      vtkFloatArray* toArray = vtkFloatArray::SafeDownCast(blockPoints->GetData());
      for (auto it = blocksOriginalToBlockPointId[boundaryIndex].begin();
           it != blocksOriginalToBlockPointId[boundaryIndex].end(); ++it)
      {
        vtkIdType from = it->first;
        vtkIdType to = it->second;
        float xyz[3];
        pointArray->GetTypedTuple(from, xyz);
        toArray->SetTypedTuple(to, xyz);
      }

      vtkPolyData* boundarySurface =
        vtkPolyData::SafeDownCast(outputPDC->GetPartition(streamSurfaceStartId + boundaryIndex, 0));
      boundarySurface->SetPoints(blockPoints);
    }

    // Go through polygons again and add them to the polydata blocks
    vtkIdType numSurfacePolys = 0;
    for (int polyId = 0; polyId < numPolygons; ++polyId)
    {
      if (connectedCells[2 * polyId + 0] >= 0 && connectedCells[2 * polyId + 1] >= 0)
      {
        // Polygon is not part of a surface, so skip.
        continue;
      }

      numSurfacePolys++;

      int boundaryId = -(connectedCells[2 * polyId + 0] + 1);
      int boundaryIndex = boundaryIdToIndex[boundaryId];
      vtkPolyData* polyData =
        vtkPolyData::SafeDownCast(outputPDC->GetPartition(streamSurfaceStartId + boundaryIndex, 0));
      vtkIdType numCellPts =
        static_cast<vtkIdType>(polygonOffsets[polyId + 1] - polygonOffsets[polyId]);
      std::vector<vtkIdType> ptIds(numCellPts);
      for (vtkIdType id = 0; id < numCellPts; ++id)
      {
        vtkIdType ptId = polygons[polygonOffsets[polyId] + id];
        ptIds[id] = blocksOriginalToBlockPointId[boundaryIndex][ptId];
      }
      polyData->GetPolys()->InsertNextCell(numCellPts, ptIds.data());
    }

    // Clear some memory
    blocksOriginalToBlockPointId.clear();

    // Create a map from cell to polygons
    std::map<int, std::set<int>> cellToPoly;

    // Create a map from polygon to the volumetric cell to which it is attached
    std::vector<int> polyToCell(numSurfacePolys);

    // Create a map from polygon to boundary
    std::vector<int> polyToBoundary(numSurfacePolys);

    vtkIdType surfacePolyCount = 0;
    for (int polyId = 0; polyId < numPolygons; ++polyId)
    {
      int cell0 = connectedCells[2 * polyId + 0];
      int cell1 = connectedCells[2 * polyId + 1];
      if (cell0 >= 0)
      {
        // Add polyId to cell 0's list of polygons
        cellToPoly[cell0].insert(polyId);
      }
      if (cell1 >= 0)
      {
        // Add polyId to cell 1's list of polygons
        cellToPoly[cell1].insert(polyId);
      }

      if (cell0 < 0 || cell1 < 0)
      {
        assert(polyToBoundary.size() > static_cast<size_t>(surfacePolyCount));
        polyToBoundary[surfacePolyCount] = cell0 >= 0 ? -(cell1 + 1) : -(cell0 + 1);
        polyToCell[surfacePolyCount] = cell0 >= 0 ? cell0 : cell1;
        surfacePolyCount++;
      }
    }

    // Set the points in the unstructured grid
    ugrid->SetPoints(points);

    // Create polyhedra from their faces
    vtkNew<vtkIdList> faces;
    for (const auto& cellIdAndPolys : cellToPoly)
    {
      faces->Reset();
      // Number of faces
      faces->InsertNextId(static_cast<vtkIdType>(cellIdAndPolys.second.size()));
      for (auto polyId : cellIdAndPolys.second)
      {
        // Get polygon
        int numPts = polygonOffsets[polyId + 1] - polygonOffsets[polyId];

        // Number of points in face
        faces->InsertNextId(numPts);
        for (int i = 0; i < numPts; ++i)
        {
          // Polygon vertex
          faces->InsertNextId(polygons[polygonOffsets[polyId] + i]);
        }
      }

      ugrid->InsertNextCell(VTK_POLYHEDRON, faces);
    }

    // ++++ CELL DATA ++++
    for (int i = 0; i < this->CellDataArraySelection->GetNumberOfArrays(); ++i)
    {
      std::string varName(this->CellDataArraySelection->GetArrayName(i));
      if (this->GetCellDataArraySelection()->ArrayIsEnabled(varName.c_str()) == 0)
      {
        continue;
      }

      auto begin = this->Internal->CellDataVectorVariables.begin();
      auto end = this->Internal->CellDataVectorVariables.end();
      bool isVector = std::find(begin, end, varName) != end;

      // This would be a lot simpler using a vtkSOADataArrayTemplate<float>, but
      // until GetVoidPointer() is removed from more of the VTK code base, we
      // will use a vtkFloatArray.
      vtkNew<vtkFloatArray> dataArray;
      bool success = true;
      if (isVector)
      {
        std::string rootPath("CELL_CENTER_DATA/");
        rootPath += varName;
        std::string pathX = rootPath + "_X";
        std::string pathY = rootPath + "_Y";
        std::string pathZ = rootPath + "_Z";

        if (!ArrayExists(streamId, pathX.c_str()))
        {
          // This array just doesn't exist in this stream, skip it.
          continue;
        }

        hsize_t length = GetDataLength(streamId, pathX.c_str());
        dataArray->SetNumberOfComponents(3);
        dataArray->SetNumberOfTuples(length);
        dataArray->SetName(varName.c_str());

        if (floatBuffer->GetSize() != static_cast<vtkIdType>(length))
        {
          floatBuffer->Allocate(length);
        }

        success = success && ReadArray(streamId, pathX.c_str(), floatBuffer->GetBuffer(), length);
        for (hsize_t j = 0; j < length; ++j)
        {
          dataArray->SetTypedComponent(j, 0, floatBuffer->GetBuffer()[j]);
        }
        success = success && ReadArray(streamId, pathY.c_str(), floatBuffer->GetBuffer(), length);
        for (hsize_t j = 0; j < length; ++j)
        {
          dataArray->SetTypedComponent(j, 1, floatBuffer->GetBuffer()[j]);
        }
        success = success && ReadArray(streamId, pathZ.c_str(), floatBuffer->GetBuffer(), length);
        for (hsize_t j = 0; j < length; ++j)
        {
          dataArray->SetTypedComponent(j, 2, floatBuffer->GetBuffer()[j]);
        }
      }
      else
      {
        std::string path("CELL_CENTER_DATA/");
        path += varName;

        if (!ArrayExists(streamId, path.c_str()))
        {
          // This array just doesn't exist in this stream, skip it.
          continue;
        }

        hsize_t length = GetDataLength(streamId, path.c_str());
        dataArray->SetNumberOfComponents(1);
        dataArray->SetNumberOfTuples(length);
        dataArray->SetName(varName.c_str());
        success = success && ReadArray(streamId, path.c_str(), dataArray->GetPointer(0), length);
      }

      if (success)
      {
        ugrid->GetCellData()->AddArray(dataArray);

        // Now pull out the values needed for the surface geometry
        for (int boundaryIndex = 0; boundaryIndex < static_cast<int>(numBoundaryNames);
             ++boundaryIndex)
        {
          vtkPolyData* boundarySurface = vtkPolyData::SafeDownCast(
            outputPDC->GetPartition(streamSurfaceStartId + boundaryIndex, 0));
          int numBoundaryPolys = boundarySurface->GetNumberOfCells();
          vtkNew<vtkFloatArray> surfaceDataArray;
          surfaceDataArray->SetNumberOfComponents(dataArray->GetNumberOfComponents());
          surfaceDataArray->SetNumberOfTuples(numBoundaryPolys);
          surfaceDataArray->SetName(varName.c_str());
          vtkIdType localDataCount = 0;
          for (vtkIdType id = 0; id < numSurfacePolys; ++id)
          {
            assert(polyToBoundary.size() > static_cast<size_t>(id));
            if (boundaryIdToIndex.find(polyToBoundary[id]) == boundaryIdToIndex.end())
            {
              vtkErrorMacro(
                "polyToBoundary[id] is not found within boundaryIdToIndex" << polyToBoundary[id]);
              return 0;
            }
            const int polyBoundaryIndex = boundaryIdToIndex[polyToBoundary[id]];
            if (polyBoundaryIndex != boundaryIndex)
            {
              continue;
            }
            for (int c = 0; c < surfaceDataArray->GetNumberOfComponents(); ++c)
            {
              assert(polyToCell.size() > static_cast<size_t>(id));
              surfaceDataArray->SetTypedComponent(
                localDataCount, c, dataArray->GetTypedComponent(polyToCell[id], c));
            }
            ++localDataCount;
          }
          boundarySurface->GetCellData()->AddArray(surfaceDataArray);
        }
      }
    }

    // ++++ PARCEL DATA ++++
    bool parcelExists = GroupExists(streamId, "PARCEL_DATA");
    if (parcelExists)
    {
      std::string parcelDataRoot;

      // Branch between pre-3.1 and post-3.1 file formats
      H5O_info_t objectInfo;
      err = H5Oget_info_by_idx(
        streamId, "PARCEL_DATA", H5_INDEX_NAME, H5_ITER_NATIVE, 0, &objectInfo, 0);
      if (err < 0 || objectInfo.type == H5O_TYPE_GROUP)
      {
        // Handle 3.1 or above version

        // Get parcel data type names
        vtkHDF::ScopedH5GHandle parcelDataTypesHandle = H5Gopen(streamId, "PARCEL_DATA");
        // We already checked that the group exists, so no need to check again.

        hsize_t numParcelDataTypes = 0;
        err = H5Gget_num_objs(parcelDataTypesHandle, &numParcelDataTypes);
        if (err < 0)
        {
          vtkErrorMacro("Cannot get number of parcel data types from file");
          return 0;
        }

        int parcelsNodeId = hierarchy->AddNode("Parcels", streamNodeId);

        // Iterate over the parcel data types/data sets
        unsigned int parcelDataTypeCount = 0;
        for (hsize_t parcelDataTypeIndex = 0; parcelDataTypeIndex < numParcelDataTypes;
             ++parcelDataTypeIndex)
        {
          status = 0;
          char groupName[256];
          status = H5Lget_name_by_idx(streamId, "PARCEL_DATA", H5_INDEX_NAME, H5_ITER_NATIVE,
            parcelDataTypeIndex, groupName, 256, H5P_DEFAULT);
          if (status < 0)
          {
            vtkErrorMacro(<< "error reading parcel variable names");
            break;
          }

          std::string dataType(groupName);
          std::string dataTypeGroupName("PARCEL_DATA/");
          dataTypeGroupName += dataType;
          vtkHDF::ScopedH5GHandle dataTypeHandle = H5Gopen(streamId, dataTypeGroupName.c_str());
          if (dataTypeHandle < 0)
          {
            vtkErrorMacro("Cannot open group " << dataTypeGroupName);
            return 0;
          }

          // Handle the datasets in each datatype
          H5G_info_t groupInfo;
          err = H5Gget_info(dataTypeHandle, &groupInfo);
          if (err < 0)
          {
            vtkErrorMacro("Cannot get number of datasets from group " << dataTypeGroupName);
            return 0;
          }
          hsize_t numDataSets = groupInfo.nlinks;

          std::string dataTypeNodeName = vtkDataAssembly::MakeValidNodeName(dataType.c_str());
          int parcelDataTypeNodeId = hierarchy->AddNode(dataTypeNodeName.c_str(), parcelsNodeId);

          parcelDataTypeCount++;

          // Iterate over the datasets in the dataset type group
          for (hsize_t i = 0; i < numDataSets; ++i)
          {
            char dataSetGroupName[256];
            status = H5Lget_name_by_idx(dataTypeHandle, ".", H5_INDEX_NAME, H5_ITER_NATIVE, i,
              dataSetGroupName, 256, H5P_DEFAULT);
            if (status < 0)
            {
              continue;
            }

            vtkSmartPointer<vtkPolyData> parcels =
              this->Internal->ReadParcelDataSet(dataTypeHandle, dataSetGroupName);
            int parcelsId = outputPDC->GetNumberOfPartitionedDataSets();
            outputPDC->SetNumberOfPartitionedDataSets(parcelsId + 1);
            outputPDC->SetPartition(parcelsId, 0, parcels);
            outputPDC->GetMetaData(parcelsId)->Set(vtkCompositeDataSet::NAME(), dataSetGroupName);
            std::string validDataSetGroupName =
              vtkDataAssembly::MakeValidNodeName(dataSetGroupName);
            int parcelNodeId =
              hierarchy->AddNode(validDataSetGroupName.c_str(), parcelDataTypeNodeId);
            hierarchy->AddDataSetIndex(parcelNodeId, parcelsId);
          }
        }
      }
      else
      {
        vtkSmartPointer<vtkPolyData> parcels =
          this->Internal->ReadParcelDataSet(streamId, "PARCEL_DATA");
        int parcelsId = outputPDC->GetNumberOfPartitionedDataSets();
        outputPDC->SetNumberOfPartitionedDataSets(parcelsId + 1);
        outputPDC->SetPartition(parcelsId, 0, parcels);
        outputPDC->GetMetaData(parcelsId)->Set(vtkCompositeDataSet::NAME(), "Parcels");
        int parcelNodeId = hierarchy->AddNode("Parcels", streamNodeId);
        hierarchy->AddDataSetIndex(parcelNodeId, parcelsId);
      }
    }
    streamCount++;
  } while (true);

  // Everything succeeded
  return 1;
}

//----------------------------------------------------------------------------
void vtkCONVERGECFDReader::ReadTimeSteps(vtkInformation* outInfo)
{
  if (!this->FileName || this->FileName[0] == '\0')
  {
    return;
  }

  // Scan for other files with the same naming pattern in the same directory.
  // We are looking for files with the following convention
  //
  // <file><zero-padded index>_<time>.h5
  //
  // We load each file and extract the time from within.
  this->FileNames.clear();
  std::string originalFile = this->FileName;
  std::string path;
  std::string baseName;
  std::string::size_type dirseppos = originalFile.find_last_of("/\\");
  if (dirseppos == std::string::npos)
  {
    path = "./";
    baseName = originalFile;
  }
  else
  {
    path = originalFile.substr(0, dirseppos + 1);
    baseName = originalFile.substr(dirseppos + 1);
  }

  std::vector<std::string> fileNames;
  vtksys::RegularExpression regEx("^([^0-9]*)([0-9]*)[_]?(.*).h5$");
  if (!regEx.find(baseName))
  {
    fileNames.emplace_back(originalFile);
    return;
  }

  std::string prefix = regEx.match(1);
  std::string indexString = regEx.match(2);

  vtkNew<vtkDirectory> dir;
  if (!dir->Open(path.c_str()))
  {
    vtkWarningMacro(<< "Could not open directory " << originalFile << " is supposed to be from ("
                    << path << ")");
    fileNames.emplace_back(originalFile);
    return;
  }

  for (vtkIdType i = 0; i < dir->GetNumberOfFiles(); i++)
  {
    const char* file = dir->GetFile(i);
    if (!regEx.find(file))
    {
      continue;
    }
    if (regEx.match(1) != prefix)
    {
      continue;
    }
    fileNames.emplace_back(path + file);
  }

  std::vector<std::pair<double, std::string>> timesAndFiles;
  for (const auto& file : fileNames)
  {
    double time = 0.0;
    bool timeRead = this->ReadOutputTime(file, time);
    if (timeRead)
    {
      timesAndFiles.emplace_back(std::make_pair(time, file));
    }
  }

  // Sort files and times by time
  std::sort(timesAndFiles.begin(), timesAndFiles.end(),
    [](const std::pair<double, std::string>& left, const std::pair<double, std::string>& right) {
      return left.first < right.first;
    });

  std::vector<double> times;
  // Reset the FileNames vector in chronological order
  this->FileNames.clear();
  for (const auto& pair : timesAndFiles)
  {
    times.emplace_back(pair.first);
    this->FileNames.emplace_back(pair.second);
  }

  if (!times.empty())
  {
    double timeRange[2] = { times[0], times[times.size() - 1] };
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
    outInfo->Set(
      vtkStreamingDemandDrivenPipeline::TIME_STEPS(), times.data(), static_cast<int>(times.size()));
  }
}

//----------------------------------------------------------------------------
bool vtkCONVERGECFDReader::ReadOutputTime(const std::string& filePath, double& time)
{
  if (filePath[0] == '\0')
  {
    return false;
  }

  vtkHDF::ScopedH5FHandle fileId = H5Fopen(filePath.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
  if (fileId < 0)
  {
    return false;
  }

  if (H5Aexists(fileId, "OUTPUT_TIME"))
  {
    vtkHDF::ScopedH5AHandle outputTimeId =
      H5Aopen_by_name(fileId, ".", "OUTPUT_TIME", H5P_DEFAULT, H5P_DEFAULT);
    vtkHDF::ScopedH5THandle rawType = H5Aget_type(outputTimeId);
    vtkHDF::ScopedH5THandle dataType = H5Tget_native_type(rawType, H5T_DIR_ASCEND);

    double outputTime = 0.0;
    if (H5Aread(outputTimeId, dataType, &outputTime) >= 0)
    {
      time = outputTime;
      return true;
    }
  }

  return false;
}

//----------------------------------------------------------------------------
size_t vtkCONVERGECFDReader::SelectTimeStepIndex(vtkInformation* info)
{
  if (!info->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()) ||
    !info->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    return 0;
  }

  double* times = info->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  int nTimes = info->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  double t = info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

  double resultDiff = VTK_DOUBLE_MAX;
  size_t result = 0;
  for (int i = 0; i < nTimes; ++i)
  {
    double diff = std::fabs(times[i] - t);
    if (diff < resultDiff)
    {
      resultDiff = diff;
      result = static_cast<size_t>(i);
    }
  }

  return result;
}

//----------------------------------------------------------------------------
int vtkCONVERGECFDReader::CanReadFile(const char* fname)
{
  if (H5Fis_hdf5(fname) == 0)
  {
    return 0;
  }

  vtkHDF::ScopedH5FHandle fileId = H5Fopen(fname, H5F_ACC_RDONLY, H5P_DEFAULT);
  if (fileId < 0)
  {
    return 0;
  }

  // Require a /BOUNDARIES group and at least one STREAM_00 group
  if (H5Lexists(fileId, "/BOUNDARIES", H5P_DEFAULT) == 0 ||
    H5Lexists(fileId, "/STREAM_00", H5P_DEFAULT) == 0)
  {
    return 0;
  }

  // Everything succeeded
  return 1;
}
