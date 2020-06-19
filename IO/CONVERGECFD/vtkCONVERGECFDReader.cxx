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
#include "vtkDirectory.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"

#include <vtksys/RegularExpression.hxx>

#include <array>
#include <map>
#include <set>
#include <sstream>

#define H5_USE_16_API
#include "vtk_hdf5.h"

vtkStandardNewMacro(vtkCONVERGECFDReader);

//----------------------------------------------------------------------------
class vtkCONVERGECFDReader::vtkInternal
{
public:
  std::vector<std::string> CellDataScalarVariables;
  std::vector<std::string> CellDataVectorVariables;
  std::vector<std::string> ParcelDataScalarVariables;
  std::vector<std::string> ParcelDataVectorVariables;
};

//----------------------------------------------------------------------------
vtkCONVERGECFDReader::vtkCONVERGECFDReader()
  : FileName(nullptr)
  , Internal(new vtkCONVERGECFDReader::vtkInternal())
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

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
namespace
{

//----------------------------------------------------------------------------
/**
 * RAII class for automatically closing H5 handles.
 */
#define DefineScopedHandle(name)                                                                   \
  class ScopedH5##name##Handle                                                                     \
  {                                                                                                \
  public:                                                                                          \
    ScopedH5##name##Handle(const ScopedH5##name##Handle& other) { this->Handle = other.Handle; }   \
    ScopedH5##name##Handle(hid_t handle)                                                           \
      : Handle(handle)                                                                             \
    {                                                                                              \
    }                                                                                              \
    virtual ~ScopedH5##name##Handle()                                                              \
    {                                                                                              \
      if (this->Handle >= 0)                                                                       \
      {                                                                                            \
        H5##name##close(this->Handle);                                                             \
      }                                                                                            \
    }                                                                                              \
                                                                                                   \
    operator hid_t() const { return this->Handle; }                                                \
                                                                                                   \
  private:                                                                                         \
    hid_t Handle;                                                                                  \
  };

// Defines ScopedH5AHandle closed with H5Aclose
DefineScopedHandle(A);

// Defines ScopedH5DHandle closed with H5Dclose
DefineScopedHandle(D);

// Defines ScopedH5FHandle closed with H5Fclose
DefineScopedHandle(F);

// Defines ScopedH5GHandle closed with H5Gclose
DefineScopedHandle(G);

// Defines ScopedH5SHandle closed with H5Sclose
DefineScopedHandle(S);

// Defines ScopedH5THandle closed with H5Tclose
DefineScopedHandle(T);

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
 *  Get length of array defined by pathName relative to fileId.
 */
hsize_t GetDataLength(hid_t fileId, const char* pathName)
{
  ScopedH5DHandle arrayId = H5Dopen(fileId, pathName);
  if (arrayId < 0)
  {
    vtkGenericWarningMacro("No array named " << pathName << " available");
    return 0;
  }

  ScopedH5DHandle dataspace = H5Dget_space(arrayId);
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
  ScopedH5DHandle arrayId = H5Dopen(fileId, pathName);
  if (arrayId < 0)
  {
    vtkGenericWarningMacro("No array named " << pathName << " available");
    return false;
  }

  ScopedH5DHandle rawType = H5Dget_type(arrayId);
  ScopedH5THandle dataType = H5Tget_native_type(rawType, H5T_DIR_ASCEND);
  ScopedH5DHandle dataspace = H5Dget_space(arrayId);
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
  ScopedH5DHandle stringsId = H5Dopen(fileId, path);
  if (stringsId < 0)
  {
    vtkGenericWarningMacro("Could not read " << path);
    return false;
  }

  ScopedH5THandle filetype = H5Dget_type(stringsId);
  size_t sdim = H5Tget_size(filetype);
  sdim++; /* Make room for null terminator */

  ScopedH5SHandle space = H5Dget_space(stringsId);
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

  ScopedH5THandle memtype = H5Tcopy(H5T_C_S1);
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
int vtkCONVERGECFDReader::RequestInformation(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outInfos)
{
  if (!this->FileName || this->FileName[0] == '\0')
  {
    return 1;
  }

  ScopedH5FHandle fileId = H5Fopen(this->FileName, H5F_ACC_RDONLY, H5P_DEFAULT);
  if (fileId < 0)
  {
    vtkErrorMacro("Could not open HDF5 file '" << this->FileName << "'");
    return 0;
  }

  // Assume that the variables are the same in all streams
  ScopedH5GHandle streamId = H5Gopen(fileId, "/STREAM_00");
  if (streamId < 0)
  {
    vtkErrorMacro("Cannot open group /STREAM_00");
    return 0;
  }

  if (!ReadStrings(
        streamId, "VARIABLE_NAMES/CELL_VARIABLES", this->Internal->CellDataScalarVariables))
  {
    vtkErrorMacro("Could not read cell variable names");
    return 0;
  }

  bool defaultEnabledState = true;

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

  if (ArrayExists(streamId, "VARIABLE_NAMES/PARCEL_VARIABLES"))
  {
    if (!ReadStrings(
          streamId, "VARIABLE_NAMES/PARCEL_VARIABLES", this->Internal->ParcelDataScalarVariables))
    {
      vtkErrorMacro("Could not read parcel variable names");
      return 0;
    }

    // Split parcel arrays into scalar and vector variables
    SplitScalarAndVectorVariables(
      this->Internal->ParcelDataScalarVariables, this->Internal->ParcelDataVectorVariables);

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

  if (fileName.size() < 1 || fileName[0] == '\0')
  {
    vtkErrorMacro("No file sequence found");
    return 0;
  }

  vtkMultiBlockDataSet* outputMB = vtkMultiBlockDataSet::GetData(outInfo);

  if (!outputMB)
  {
    vtkErrorMacro("No output available!");
    return 0;
  }

  ScopedH5FHandle fileId = H5Fopen(fileName.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
  if (fileId < 0)
  {
    vtkErrorMacro("Could not open HDF5 file '" << fileName << "'");
    return 0;
  }

  ScopedH5GHandle boundaryId = H5Gopen(fileId, "/BOUNDARIES");
  if (boundaryId < 0)
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
    ScopedH5GHandle streamId = H5Gopen(fileId, streamName.str().c_str());
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

    // Create surface
    std::set<vtkIdType> surfacePointIds;
    vtkIdType numSurfacePolys = 0;
    for (int polyId = 0; polyId < numPolygons; ++polyId)
    {
      if (connectedCells[2 * polyId + 0] >= 0 && connectedCells[2 * polyId + 1] >= 0)
      {
        // Polygon is not part of a surface, so skip.
        continue;
      }

      vtkIdType numCellPts =
        static_cast<vtkIdType>(polygonOffsets[polyId + 1] - polygonOffsets[polyId]);
      for (vtkIdType id = 0; id < numCellPts; ++id)
      {
        surfacePointIds.insert(polygons[polygonOffsets[polyId] + id]);
      }
      numSurfacePolys++;
    }

    // Remap original point indices in stream to surface points
    std::map<vtkIdType, vtkIdType> originalToSurfacePointId;
    vtkIdType newIndex = 0;
    for (vtkIdType id : surfacePointIds)
    {
      originalToSurfacePointId[id] = newIndex++;
    }

    vtkNew<vtkPoints> surfacePoints;
    surfacePoints->SetDataType(points->GetDataType());
    surfacePoints->SetNumberOfPoints(newIndex);
    vtkFloatArray* toArray = vtkFloatArray::SafeDownCast(surfacePoints->GetData());
    for (auto it = originalToSurfacePointId.begin(); it != originalToSurfacePointId.end(); ++it)
    {
      vtkIdType from = it->first;
      vtkIdType to = it->second;
      float xyz[3];
      pointArray->GetTypedTuple(from, xyz);
      toArray->SetTypedTuple(to, xyz);
    }

    vtkNew<vtkPolyData> surface;
    surface->SetPoints(surfacePoints);

    vtkNew<vtkCellArray> polys;
    polys->AllocateEstimate(numSurfacePolys, 4);
    std::vector<vtkIdType> ptIds(3);
    for (int polyId = 0; polyId < numPolygons; ++polyId)
    {
      if (connectedCells[2 * polyId + 0] >= 0 && connectedCells[2 * polyId + 1] >= 0)
      {
        // Polygon is not part of a surface, so skip.
        continue;
      }

      vtkIdType numCellPts =
        static_cast<vtkIdType>(polygonOffsets[polyId + 1] - polygonOffsets[polyId]);
      if (ptIds.size() < static_cast<size_t>(numCellPts))
      {
        ptIds.resize(numCellPts);
      }
      for (vtkIdType id = 0; id < numCellPts; ++id)
      {
        ptIds[id] = originalToSurfacePointId[polygons[polygonOffsets[polyId] + id]];
      }

      polys->InsertNextCell(numCellPts, &ptIds[0]);
    }

    surface->SetPolys(polys);

    // Create a new unstructured grid
    vtkNew<vtkUnstructuredGrid> ugrid;
    ugrid->SetPoints(points);

    // Create a map from cell to polygon and from surface polygon to volumetric polygon
    std::map<int, std::set<int>> cellToPoly;
    std::vector<int> polyToCell(numSurfacePolys);
    vtkIdType surfacePolyCount = 0;
    for (int polyId = 0; polyId < numPolygons; ++polyId)
    {
      int cell0 = connectedCells[2 * polyId + 0];
      int cell1 = connectedCells[2 * polyId + 1];
      if (cell0 >= 0)
      {
        cellToPoly[cell0].insert(polyId);
      }
      if (cell1 >= 0)
      {
        cellToPoly[cell1].insert(polyId);
      }

      if (cell0 < 0 || cell1 < 0)
      {
        polyToCell[surfacePolyCount++] = cell0 >= 0 ? cell0 : cell1;
      }
    }

    // Create polyhedra from their faces
    vtkNew<vtkIdList> faces;
    for (auto cellIdAndPolys : cellToPoly)
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
        vtkNew<vtkFloatArray> surfaceDataArray;
        surfaceDataArray->SetNumberOfComponents(dataArray->GetNumberOfComponents());
        surfaceDataArray->SetNumberOfTuples(numSurfacePolys);
        surfaceDataArray->SetName(varName.c_str());
        for (vtkIdType id = 0; id < numSurfacePolys; ++id)
        {
          for (int c = 0; c < surfaceDataArray->GetNumberOfComponents(); ++c)
          {
            surfaceDataArray->SetTypedComponent(
              id, c, dataArray->GetTypedComponent(polyToCell[id], c));
          }
        }
        surface->GetCellData()->AddArray(surfaceDataArray);
      }
    }

    // Set the mesh and surface in the multiblock output
    vtkNew<vtkMultiBlockDataSet> streamMB;
    streamMB->SetNumberOfBlocks(2);
    streamMB->SetBlock(0, ugrid);
    streamMB->GetMetaData(0u)->Set(vtkCompositeDataSet::NAME(), "Mesh");
    streamMB->SetBlock(1, surface);
    streamMB->GetMetaData(1u)->Set(vtkCompositeDataSet::NAME(), "Surface");

    // ++++ PARCEL DATA ++++
    bool parcelExists = ArrayExists(streamId, "PARCEL_DATA/PARCEL_X");
    if (parcelExists)
    {
      // Read parcel point locations
      hsize_t parcelLength = GetDataLength(streamId, "PARCEL_DATA/PARCEL_X");

      vtkNew<vtkFloatArray> parcelPointArray;
      parcelPointArray->SetNumberOfComponents(3);
      parcelPointArray->SetNumberOfTuples(parcelLength);

      floatBuffer->Allocate(parcelLength);

      for (size_t c = 0; c < dimensionNames.size(); ++c)
      {
        std::stringstream name;
        name << "PARCEL_DATA/PARCEL_" << dimensionNames[c];
        if (!ReadArray(streamId, name.str().c_str(), floatBuffer->GetBuffer(), parcelLength))
        {
          vtkErrorMacro("No parcel coordinate array " << name.str() << " dataset available in "
                                                      << streamName.str());
          return 0;
        }

        for (hsize_t j = 0; j < parcelLength; ++j)
        {
          parcelPointArray->SetTypedComponent(j, static_cast<int>(c), floatBuffer->GetBuffer()[j]);
        }
      }

      vtkNew<vtkPoints> parcelPoints;
      parcelPoints->SetData(parcelPointArray);

      vtkNew<vtkPolyData> parcels;
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
      for (int i = 0; i < this->ParcelDataArraySelection->GetNumberOfArrays(); ++i)
      {
        std::string varName(this->ParcelDataArraySelection->GetArrayName(i));
        if (varName == "PARCEL_X" || varName == "PARCEL_Y" || varName == "PARCEL_Z" ||
          this->ParcelDataArraySelection->ArrayIsEnabled(varName.c_str()) == 0)
        {
          continue;
        }

        auto begin = this->Internal->ParcelDataVectorVariables.begin();
        auto end = this->Internal->ParcelDataVectorVariables.end();
        bool isVector = std::find(begin, end, varName) != end;

        // This would be a lot simpler using a vtkSOADataArrayTemplate<float>, but
        // until GetVoidPointer() is removed from more of the VTK code base, we
        // will use a vtkFloatArray.
        vtkNew<vtkFloatArray> dataArray;
        bool success = true;
        if (isVector)
        {
          std::string rootPath("PARCEL_DATA/");
          rootPath += varName;
          std::string pathX = rootPath + "_X";
          std::string pathY = rootPath + "_Y";
          std::string pathZ = rootPath + "_Z";

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
        else
        {
          std::string path("PARCEL_DATA/");
          path += varName;

          dataArray->SetNumberOfComponents(1);
          dataArray->SetNumberOfTuples(parcelLength);
          dataArray->SetName(varName.c_str());
          success =
            success && ReadArray(streamId, path.c_str(), dataArray->GetPointer(0), parcelLength);
        }

        if (success)
        {
          parcels->GetPointData()->AddArray(dataArray);
        }
      }

      streamMB->SetNumberOfBlocks(3);
      streamMB->SetBlock(2, parcels);
      streamMB->GetMetaData(2u)->Set(vtkCompositeDataSet::NAME(), "Parcels");
    }

    std::stringstream streamss;
    streamss << "STREAM_" << std::setw(2) << std::setfill('0') << streamCount;
    outputMB->SetNumberOfBlocks(streamCount + 1);
    outputMB->GetMetaData(streamCount)->Set(vtkCompositeDataSet::NAME(), streamss.str().c_str());
    outputMB->SetBlock(streamCount, streamMB);

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

  vtksys::RegularExpression regEx("^([^0-9]*)([0-9]*)[_]?(.*).h5$");
  if (!regEx.find(baseName))
  {
    this->FileNames.emplace_back(originalFile);
    return;
  }

  std::string prefix = regEx.match(1);
  std::string indexString = regEx.match(2);

  vtkNew<vtkDirectory> dir;
  if (!dir->Open(path.c_str()))
  {
    vtkWarningMacro(<< "Could not open directory " << originalFile.c_str()
                    << " is supposed to be from (" << path.c_str() << ")");
    this->FileNames.emplace_back(originalFile);
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
    this->FileNames.emplace_back(path + file);
  }

  std::vector<double> times;
  double timeRange[2] = { VTK_DOUBLE_MAX, VTK_DOUBLE_MIN };
  for (auto file : this->FileNames)
  {
    double time = 0.0;
    bool timeRead = this->ReadOutputTime(file, time);
    if (timeRead)
    {
      timeRange[0] = std::min(timeRange[0], time);
      timeRange[1] = std::max(timeRange[1], time);
      times.emplace_back(time);
    }
  }

  if (times.size() > 0)
  {
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
    outInfo->Set(
      vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &times[0], static_cast<int>(times.size()));
  }
}

//----------------------------------------------------------------------------
bool vtkCONVERGECFDReader::ReadOutputTime(const std::string& filePath, double& time)
{
  if (filePath[0] == '\0')
  {
    return false;
  }

  ScopedH5FHandle fileId = H5Fopen(filePath.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
  if (fileId < 0)
  {
    return false;
  }

  if (H5Aexists(fileId, "OUTPUT_TIME"))
  {
    ScopedH5AHandle outputTimeId =
      H5Aopen_by_name(fileId, ".", "OUTPUT_TIME", H5P_DEFAULT, H5P_DEFAULT);
    ScopedH5THandle rawType = H5Aget_type(outputTimeId);
    ScopedH5THandle dataType = H5Tget_native_type(rawType, H5T_DIR_ASCEND);

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

  ScopedH5FHandle fileId = H5Fopen(fname, H5F_ACC_RDONLY, H5P_DEFAULT);
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
