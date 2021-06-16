/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCGNSReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//  Copyright 2013-2014 Mickael Philit.

#ifdef _WINDOWS
// the 4211 warning is emitted when building this file with Visual Studio 2013
// for an SDK-specific file (sys/stat.inl:57) => disable warning
#pragma warning(push)
#pragma warning(disable : 4211)
#endif

#include "vtkCGNSReader.h"
#include "vtkCGNSReaderInternal.h" // For parsing information request

#include "vtkAssume.h"
#include "vtkCGNSCache.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkCommand.h"
#include "vtkDataArraySelection.h"
#include "vtkDoubleArray.h"
#include "vtkErrorCode.h"
#include "vtkExtractGrid.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationStringKey.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkLongArray.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyhedron.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"
#include "vtkTypeInt32Array.h"
#include "vtkTypeInt64Array.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVertex.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <iterator>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string.h>
#include <string>
#include <vector>

#include <vtksys/RegularExpression.hxx>
#include <vtksys/SystemTools.hxx>

#include "cgio_helpers.h"

vtkInformationKeyMacro(vtkCGNSReader, FAMILY, String);
vtkStandardNewMacro(vtkCGNSReader);

namespace
{

/**
 * A quick function to check if vtkIdType can hold the value being
 * saved into vtkIdType
 */
template <class T>
bool IsIdTypeBigEnough(const T& val)
{
  (void)val;
  return (sizeof(vtkIdType) >= sizeof(T) || static_cast<T>(vtkTypeTraits<vtkIdType>::Max()) >= val);
}

struct duo_t
{
  duo_t()
  {
    pair[0] = 0;
    pair[1] = 0;
  }

  int& operator[](std::size_t n) { return pair[n]; }

private:
  int pair[2];
};

class SectionInformation
{
public:
  CGNSRead::char_33 name;
  CGNS_ENUMT(ElementType_t) elemType;
  cgsize_t range[2];
  int bound;
  cgsize_t eDataSize;
};

//------------------------------------------------------------------------------
/**
 *
 * let's throw this for CGNS read errors. This is currently only used by
 * BCInformation.
 */
class CGIOError : public std::runtime_error
{
public:
  CGIOError(const std::string& what_arg)
    : std::runtime_error(what_arg)
  {
  }
};

class CGIOUnsupported : public std::runtime_error
{
public:
  CGIOUnsupported(const std::string& what_arg)
    : std::runtime_error(what_arg)
  {
  }
};

#define CGIOErrorSafe(x)                                                                           \
  if (x != CG_OK)                                                                                  \
  {                                                                                                \
    char message[81];                                                                              \
    cgio_error_message(message);                                                                   \
    throw CGIOError(message);                                                                      \
  }

//------------------------------------------------------------------------------
/**
 * Class to encapsulate information provided by a BC_t node.
 * Currently, this is only use for the Structured I/O code.
 */
class BCInformation
{
public:
  char Name[CGIO_MAX_NAME_LENGTH + 1];
  std::string FamilyName;
  CGNS_ENUMT(GridLocation_t) Location;
  std::vector<vtkTypeInt64> PointRange;

  /**
   * Reads info from a BC_t node to initialize the instance.
   *
   * @param[in] cgioNum Database identifier.
   * @param[in] nodeId Node identifier. Must point to a BC_t node.
   */
  BCInformation(int cgioNum, double nodeId)
  {
    CGIOErrorSafe(cgio_get_name(cgioNum, nodeId, this->Name));

    char dtype[CGIO_MAX_DATATYPE_LENGTH + 1];
    CGIOErrorSafe(cgio_get_data_type(cgioNum, nodeId, dtype));
    dtype[CGIO_MAX_DATATYPE_LENGTH] = 0;
    if (strcmp(dtype, "C1") != 0)
    {
      throw CGIOError("Invalid data type for `BC_t` node.");
    }

    std::string bctype;
    CGNSRead::readNodeStringData(cgioNum, nodeId, bctype);
    if (bctype != "FamilySpecified")
    {
      throw CGIOUnsupported(
        std::string("BC_t type '") + bctype + std::string("' not supported yet."));
    }

    std::vector<double> childrenIds;
    CGNSRead::getNodeChildrenId(cgioNum, nodeId, childrenIds);

    for (auto iter = childrenIds.begin(); iter != childrenIds.end(); ++iter)
    {
      char nodeName[CGIO_MAX_NAME_LENGTH + 1];
      char nodeLabel[CGIO_MAX_LABEL_LENGTH + 1];
      CGIOErrorSafe(cgio_get_name(cgioNum, *iter, nodeName));
      CGIOErrorSafe(cgio_get_label(cgioNum, *iter, nodeLabel));
      if (strcmp(nodeName, "PointList") == 0)
      {
        throw CGIOUnsupported("'PointList' BC is not supported.");
      }
      else if (strcmp(nodeName, "PointRange") == 0)
      {
        CGNSRead::readNodeDataAs<vtkTypeInt64>(cgioNum, *iter, this->PointRange);
      }
      else if (strcmp(nodeLabel, "FamilyName_t") == 0)
      {
        CGNSRead::readNodeStringData(cgioNum, *iter, this->FamilyName);
        if (!this->FamilyName.empty() && this->FamilyName[0] == '/')
        {
          // This is a family path
          std::string::size_type pos = this->FamilyName.find('/', 1);
          if (pos != std::string::npos)
          {
            this->FamilyName = this->FamilyName.substr(pos + 1);
          }
        }
      }
      else if (strcmp(nodeLabel, "GridLocation_t") == 0)
      {
        std::string location;
        CGNSRead::readNodeStringData(cgioNum, *iter, location);
        if (location == "Vertex")
        {
          this->Location = CGNS_ENUMV(Vertex);
        }
        else if (location == "IFaceCenter")
        {
          this->Location = CGNS_ENUMV(IFaceCenter);
        }
        else if (location == "JFaceCenter")
        {
          this->Location = CGNS_ENUMV(JFaceCenter);
        }
        else if (location == "KFaceCenter")
        {
          this->Location = CGNS_ENUMV(KFaceCenter);
        }
        else if (location == "FaceCenter")
        {
          this->Location = CGNS_ENUMV(FaceCenter);
        }
        else if (location == "CellCenter")
        {
          this->Location = CGNS_ENUMV(CellCenter); // Volumic Boundary Condition
        }
        else
        {
          throw CGIOUnsupported("Unsupported location " + location);
        }
      }
    }
    CGNSRead::releaseIds(cgioNum, childrenIds);
  }

  ~BCInformation() = default;

  // Create a new dataset that represents the patch for the given zone.
  vtkSmartPointer<vtkDataSet> CreateDataSet(int cellDim, vtkStructuredGrid* zoneGrid) const
  {
    // We need to extract cells from zoneGrid based on this->PointRange.

    // We'll use vtkExtractGrid, which needs VOI in point extents.
    vtkNew<vtkExtractGrid> extractVOI;
    int voi[6] = { 0, 0, 0, 0, 0, 0 };
    this->GetVOI(voi, cellDim);
    extractVOI->SetInputDataObject(zoneGrid);
    extractVOI->SetVOI(voi);
    extractVOI->Update();
    return vtkSmartPointer<vtkDataSet>(extractVOI->GetOutput(0));
  }

  bool GetVOI(int voi[6], int cellDim) const
  {
    // Remember, "the default beginning vertex for the grid in a given zone is
    // (1,1,1); this means the default beginning cell center of the grid in that
    // zone is also (1,1,1)" (from CGNS docs:
    // https://cgns.github.io/CGNS_docs_current/sids/conv.html#structgrid).

    // Hint that cellDim is <= 3
    VTK_ASSUME(cellDim <= 3);

    // Hence, convert this->PointRange to 0-based values.
    int zPointRange[6];
    for (int cc = 0; cc < 2 * cellDim; ++cc)
    {
      zPointRange[cc] = this->PointRange[cc] - 1;
    }

    // It's a little unclear to me if PointRange is always a range of points,
    // irrespective of whether the this->Location is Vertex or FaceCenter. I am
    // assuming it as so since that works of the sample data I have.
    for (int cc = 0; cc < cellDim; ++cc)
    {
      voi[2 * cc] = zPointRange[cc];
      voi[2 * cc + 1] = zPointRange[cc + cellDim];
    }
    return true;
  }

private:
  BCInformation(const BCInformation&) = delete;
  BCInformation& operator=(const BCInformation&) = delete;
};

//------------------------------------------------------------------------------
/**
 * Class to encapsulate information provided by a BC_t node.
 * This is only useful for the Unstructured I/O code.
 */
class BCInformationUns
{
public:
  char Name[CGIO_MAX_NAME_LENGTH + 1];
  std::string FamilyName;
  CGNS_ENUMT(GridLocation_t) Location;
  std::vector<vtkTypeInt64> BCElementList;
  std::vector<vtkTypeInt64> BCElementRange;

  /**
   * Reads info from a BC_t node to initialize the instance.
   *
   * @param[in] cgioNum Database identifier.
   * @param[in] nodeId Node identifier. Must point to a BC_t node.
   * @param[in] cellDim 2 for 2D case and then Edge location is valid
   *                    3 for 3D case and then FaceCenter location is valid
   */
  BCInformationUns(int cgioNum, double nodeId, int cellDim)
  {
    CGIOErrorSafe(cgio_get_name(cgioNum, nodeId, this->Name));

    char dtype[CGIO_MAX_DATATYPE_LENGTH + 1];
    CGIOErrorSafe(cgio_get_data_type(cgioNum, nodeId, dtype));
    dtype[CGIO_MAX_DATATYPE_LENGTH] = 0;
    if (strcmp(dtype, "C1") != 0)
    {
      throw CGIOError("Invalid data type for `BC_t` node.");
    }
    this->Location = CGNS_ENUMV(FaceCenter);

    std::string bctype;
    CGNSRead::readNodeStringData(cgioNum, nodeId, bctype);
    if (bctype != "FamilySpecified")
    {
      // waiting for c++20 to be replaced by starts_with
      if (bctype.rfind("BCWall", 0) == 0)
      {
        // Temporary Fallback for WALL bcs for old meshes
        this->FamilyName = "Wall";
      }
      else
      {
        throw CGIOUnsupported(
          std::string("BC_t type '") + bctype + std::string("' not supported yet."));
      }
    }

    std::vector<double> childrenIds;
    CGNSRead::getNodeChildrenId(cgioNum, nodeId, childrenIds);

    for (auto iter = childrenIds.begin(); iter != childrenIds.end(); ++iter)
    {
      char nodeName[CGIO_MAX_NAME_LENGTH + 1];
      char nodeLabel[CGIO_MAX_LABEL_LENGTH + 1];
      CGIOErrorSafe(cgio_get_name(cgioNum, *iter, nodeName));
      CGIOErrorSafe(cgio_get_label(cgioNum, *iter, nodeLabel));
      if (strcmp(nodeName, "PointList") == 0)
      {
        CGNSRead::readNodeDataAs<vtkTypeInt64>(cgioNum, *iter, this->BCElementList);
      }
      else if (strcmp(nodeName, "PointRange") == 0)
      {
        CGNSRead::readNodeDataAs<vtkTypeInt64>(cgioNum, *iter, this->BCElementRange);
      }
      else if (strcmp(nodeName, "ElementRange") == 0)
      {
        // Usage of ElementRange in BC is deprecated but still found... may be add a warning
        CGNSRead::readNodeDataAs<vtkTypeInt64>(cgioNum, *iter, this->BCElementRange);
      }
      else if (strcmp(nodeLabel, "FamilyName_t") == 0)
      {
        CGNSRead::readNodeStringData(cgioNum, *iter, this->FamilyName);
        if (!this->FamilyName.empty() && this->FamilyName[0] == '/')
        {
          // This is a family path
          std::string::size_type pos = this->FamilyName.find('/', 1);
          if (pos != std::string::npos)
          {
            this->FamilyName = this->FamilyName.substr(pos + 1);
          }
        }
      }
      else if (strcmp(nodeLabel, "GridLocation_t") == 0)
      {
        std::string location;
        CGNSRead::readNodeStringData(cgioNum, *iter, location);
        if (location == "Vertex")
        {
          this->Location = CGNS_ENUMV(Vertex);
        }
        else if (location == "FaceCenter" && 3 == cellDim)
        {
          this->Location = CGNS_ENUMV(FaceCenter);
        }
        else if (location == "EdgeCenter" && 2 == cellDim)
        {
          this->Location = CGNS_ENUMV(EdgeCenter);
        }
        else
        {
          throw CGIOUnsupported("Unsupported unstrured grid location " + location);
        }
      }
    }
    CGNSRead::releaseIds(cgioNum, childrenIds);
  }

  ~BCInformationUns() = default;

private:
  BCInformationUns(const BCInformationUns&) = delete;
  BCInformationUns& operator=(const BCInformationUns&) = delete;
};
}

// vtkCGNSReader has several method that used types from CGNS
// which resulted in CGNS include being exposed to the users of this class
// causing build complications. This makes that easier.
class vtkCGNSReader::vtkPrivate
{
public:
  static bool IsVarEnabled(
    CGNS_ENUMT(GridLocation_t) varcentering, const CGNSRead::char_33 name, vtkCGNSReader* self);
  static int getGridAndSolutionNames(int base, std::string& gridCoordName,
    std::vector<std::string>& solutionNames, vtkCGNSReader* reader);
  static int getCoordsIdAndFillRind(const std::string& gridCoordName, int physicalDim,
    std::size_t& nCoordsArray, std::vector<double>& gridChildId, int* rind, vtkCGNSReader* self);
  static int getVarsIdAndFillRind(double cgioSolId, std::size_t& nVarArray,
    CGNS_ENUMT(GridLocation_t) & varCentering, std::vector<double>& solChildId, int* rind,
    vtkCGNSReader* self);

  /**
   * `voi` can be used to read a sub-extent. VOI is specified using VTK
   * conventions i.e. 0-based point extents specified as (x-min,x-max,
   * y-min,y-max, z-min, z-max).
   */
  static int readSolution(const std::string& solutionName, int cellDim, int physicalDim,
    const cgsize_t* zsize, vtkDataSet* dataset, const int* voi, vtkCGNSReader* self);

  static int fillArrayInformation(const std::vector<double>& solChildId, int physicalDim,
    std::vector<CGNSRead::CGNSVariable>& cgnsVars, std::vector<CGNSRead::CGNSVector>& cgnsVectors,
    vtkCGNSReader* self);

  static int AllocateVtkArray(int physicalDim, int requestedVectorDim, vtkIdType nVals,
    CGNS_ENUMT(GridLocation_t) varCentering, const std::vector<CGNSRead::CGNSVariable>& cgnsVars,
    const std::vector<CGNSRead::CGNSVector>& cgnsVectors, std::vector<vtkDataArray*>& vtkVars,
    vtkCGNSReader* self);

  static int AttachReferenceValue(int base, vtkDataSet* ds, vtkCGNSReader* self);

  /**
   * return -1 is num_timesteps<=0 or timesteps == nullptr, otherwise will always
   * returns an index in the range [0, num_timesteps).
   */
  static int GetTimeStepIndex(double time, const double* timesteps, int num_timesteps)
  {
    if (timesteps == nullptr || num_timesteps <= 0)
    {
      return -1;
    }

    const double* lbptr = std::lower_bound(timesteps, timesteps + num_timesteps, time);
    int index = static_cast<int>(lbptr - timesteps);

    // clamp to last timestep if beyond the range.
    index = (index >= num_timesteps) ? (num_timesteps - 1) : index;
    assert(index >= 0 && index < num_timesteps);
    return index;
  }

  static void AddIsPatchArray(vtkDataSet* ds, bool is_patch)
  {
    if (ds)
    {
      vtkNew<vtkIntArray> iarray;
      iarray->SetNumberOfTuples(1);
      iarray->SetValue(0, is_patch ? 1 : 0);
      iarray->SetName("ispatch");
      ds->GetFieldData()->AddArray(iarray.Get());
    }
  }

  // Reads a curvilinear zone along with its solution.
  // If voi is non-null, then a sub-extents (x-min, x-max, y-min,  y-max, z-min,
  // z-max) can be specified to only read a subset of the zone. Otherwise, the
  // entire zone is read in.
  static vtkSmartPointer<vtkDataObject> readCurvilinearZone(int base, int zone, int cellDim,
    int physicalDim, const cgsize_t* zsize, const int* voi, vtkCGNSReader* self);

  static vtkSmartPointer<vtkDataSet> readBCDataSet(const BCInformation& bcinfo, int base, int zone,
    int cellDim, int physicalDim, const cgsize_t* zsize, vtkCGNSReader* self)
  {
    int voi[6];
    bcinfo.GetVOI(voi, cellDim);
    vtkSmartPointer<vtkDataObject> zoneDO =
      readCurvilinearZone(base, zone, cellDim, physicalDim, zsize, voi, self);
    return vtkDataSet::SafeDownCast(zoneDO);
  }

  static int readBCData(double nodeId, int cellDim, int physicalDim,
    CGNS_ENUMT(GridLocation_t) locationParam, vtkDataSet* dataset, vtkCGNSReader* self);

  static std::string GenerateMeshKey(const char* basename, const char* zonename);

  vtkPrivate();
  ~vtkPrivate();

  CGNSRead::vtkCGNSMetaData* Internal;               // Metadata
  CGNSRead::vtkCGNSCache<vtkPoints> MeshPointsCache; // Cache for the mesh points
  CGNSRead::vtkCGNSCache<vtkUnstructuredGrid>
    ConnectivitiesCache; // Cache for the mesh connectivities
};

// Helpers for FlowSolutionxxxPointers
int EndsWithPointers(const char* s)
{
  int ret = 0;

  if (s != nullptr)
  {
    size_t size = strlen(s);
    if (size > 8 && (strncmp(s + size - 8, "Pointers", 8) == 0))
    {
      ret = 1;
    }
  }

  return ret;
}

int StartsWithFlowSolution(const char* s)
{
  int ret = 0;

  if (s != nullptr)
  {
    size_t size = strlen(s);
    if (size > 12 && (strncmp(s, "FlowSolution", 12) == 0))
    {
      ret = 1;
    }
  }

  return ret;
}
//----------------------------------------------------------------------------
// Small helper
const char* get_data_type(const CGNS_ENUMT(DataType_t) dt)
{
  const char* dataType;
  switch (dt)
  {
    case CGNS_ENUMV(Integer):
      dataType = "I4";
      break;
    case CGNS_ENUMV(LongInteger):
      dataType = "I8";
      break;
    case CGNS_ENUMV(RealSingle):
      dataType = "R4";
      break;
    case CGNS_ENUMV(RealDouble):
      dataType = "R8";
      break;
    case CGNS_ENUMV(Character):
      dataType = "C1";
      break;
    default:
      dataType = "MT";
  }
  return dataType;
}

//----------------------------------------------------------------------------
vtkCGNSReader::vtkPrivate::vtkPrivate()
  : Internal(new CGNSRead::vtkCGNSMetaData())
{
}

//----------------------------------------------------------------------------
vtkCGNSReader::vtkPrivate::~vtkPrivate()
{
  this->MeshPointsCache.ClearCache();
  this->ConnectivitiesCache.ClearCache();

  delete this->Internal;
  this->Internal = nullptr;
}

//----------------------------------------------------------------------------
vtkCGNSReader::vtkCGNSReader()
  : Internals(new vtkPrivate)
{
  this->FileName = nullptr;
  this->LoadBndPatch = false;
  this->LoadMesh = true;

  this->NumberOfBases = 0;
  this->ActualTimeStep = 0;
  this->DoublePrecisionMesh = 1;
  this->CreateEachSolutionAsBlock = 0;
  this->IgnoreFlowSolutionPointers = false;
  this->UseUnsteadyPattern = false;
  this->DistributeBlocks = true;
  this->CacheMesh = false;
  this->CacheConnectivity = false;
  this->Use3DVector = true;

  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

  this->ProcRank = 0;
  this->ProcSize = 1;
  this->Controller = nullptr;
  this->SetController(vtkMultiProcessController::GetGlobalController());

  this->PointDataArraySelection->AddObserver(
    vtkCommand::ModifiedEvent, this, &vtkCGNSReader::Modified);
  this->CellDataArraySelection->AddObserver(
    vtkCommand::ModifiedEvent, this, &vtkCGNSReader::Modified);
  this->BaseSelection->AddObserver(vtkCommand::ModifiedEvent, this, &vtkCGNSReader::Modified);
  this->FamilySelection->AddObserver(vtkCommand::ModifiedEvent, this, &vtkCGNSReader::Modified);
}

//----------------------------------------------------------------------------
vtkCGNSReader::~vtkCGNSReader()
{
  this->SetFileName(nullptr);
  this->SetController(nullptr);

  delete this->Internals;
  this->Internals = nullptr;
}

//----------------------------------------------------------------------------
void vtkCGNSReader::SetController(vtkMultiProcessController* c)
{
  if (this->Controller == c)
  {
    return;
  }

  this->Modified();

  if (this->Controller)
  {
    this->Controller->UnRegister(this);
  }

  this->Controller = c;

  if (this->Controller)
  {
    this->Controller->Register(this);
    this->ProcRank = this->Controller->GetLocalProcessId();
    this->ProcSize = this->Controller->GetNumberOfProcesses();
  }

  if (!this->Controller || this->ProcSize <= 0)
  {
    this->ProcRank = 0;
    this->ProcSize = 1;
  }
}

//------------------------------------------------------------------------------

std::string vtkCGNSReader::vtkPrivate::GenerateMeshKey(const char* basename, const char* zonename)
{
  std::ostringstream query;
  query << "/" << basename << "/" << zonename;
  return query.str();
}

//------------------------------------------------------------------------------
bool vtkCGNSReader::vtkPrivate::IsVarEnabled(
  CGNS_ENUMT(GridLocation_t) varcentering, const CGNSRead::char_33 name, vtkCGNSReader* self)
{
  vtkDataArraySelection* DataSelection = nullptr;
  if (varcentering == CGNS_ENUMV(Vertex))
  {
    DataSelection = self->PointDataArraySelection.GetPointer();
  }
  else
  {
    DataSelection = self->CellDataArraySelection.GetPointer();
  }

  return (DataSelection->ArrayIsEnabled(name) != 0);
}

//------------------------------------------------------------------------------
int vtkCGNSReader::vtkPrivate::getGridAndSolutionNames(int base, std::string& gridCoordName,
  std::vector<std::string>& solutionNames, vtkCGNSReader* self)
{
  // We encounter various ways in which solution grids are specified (standard
  // and non-standard). This code will try to handle all of them.
  const CGNSRead::BaseInformation& baseInfo = self->Internals->Internal->GetBase(base);

  //===========================================================================
  // Let's start with the easiest one, the grid coordinates.

  // Check if we have ZoneIterativeData_t/GridCoordinatesPointers present. If
  // so, use those to read grid coordinates for current timestep.
  double ziterId = 0;
  bool hasZoneIterativeData = (CGNSRead::getFirstNodeId(self->cgioNum, self->currentId,
                                 "ZoneIterativeData_t", &ziterId) == CG_OK);

  if (hasZoneIterativeData && baseInfo.useGridPointers)
  {
    double giterId = 0;
    if (CGNSRead::getFirstNodeId(
          self->cgioNum, ziterId, "DataArray_t", &giterId, "GridCoordinatesPointers") == CG_OK)
    {
      CGNSRead::char_33 gname;
      const cgsize_t offset = static_cast<cgsize_t>(self->ActualTimeStep * 32);
      cgio_read_block_data_type(
        self->cgioNum, giterId, offset + 1, offset + 32, "C1", (void*)gname);
      gname[32] = '\0';
      // NOTE: Names or identifiers contain no spaces and capitalization
      //       is used to distinguish individual words making up a name.
      //       For ill-formed CGNS files, we encounter names padded with spaces.
      //       We handle them by removing trailing spaces.
      CGNSRead::removeTrailingWhiteSpaces(gname);
      gridCoordName = gname;

      cgio_release_id(self->cgioNum, giterId);
    }
  }

  if (gridCoordName.empty())
  {
    // If ZoneIterativeData_t is not present or doesn't have
    // GridCoordinatesPointers, locate the first element of type
    // `GridCoordinates_t`. That's the coordinates array.
    double giterId;
    if (CGNSRead::getFirstNodeId(self->cgioNum, self->currentId, "GridCoordinates_t", &giterId) ==
      CG_OK)
    {
      CGNSRead::char_33 nodeName;
      if (cgio_get_name(self->cgioNum, giterId, nodeName) == CG_OK)
      {
        gridCoordName = nodeName;
      }
      cgio_release_id(self->cgioNum, giterId);
    }
  }

  if (gridCoordName.empty())
  {
    // if all fails, just say it's an array named "GridCoordinates".
    gridCoordName = "GridCoordinates";
  }

  //===========================================================================
  // Next let's determine the solution nodes.

  bool ignoreFlowSolutionPointers = self->IgnoreFlowSolutionPointers;
  bool useUnsteadyPattern = self->UseUnsteadyPattern;
  // if ZoneIterativeData_t/FlowSolutionPointers is present, they may provide us
  // some of the solution nodes for current timestep (not all).
  if (hasZoneIterativeData && baseInfo.useFlowPointers && !ignoreFlowSolutionPointers)
  {
    std::vector<double> iterChildId;
    CGNSRead::getNodeChildrenId(self->cgioNum, ziterId, iterChildId);

    std::vector<std::string> unvalidatedSolutionNames;
    for (size_t cc = 0; cc < iterChildId.size(); ++cc)
    {
      CGNSRead::char_33 nodeLabel;
      CGNSRead::char_33 nodeName;
      if (cgio_get_name(self->cgioNum, iterChildId[cc], nodeName) == CG_OK &&
        cgio_get_label(self->cgioNum, iterChildId[cc], nodeLabel) == CG_OK &&
        strcmp(nodeLabel, "DataArray_t") == 0 && StartsWithFlowSolution(nodeName) &&
        EndsWithPointers(nodeName))
      {
        CGNSRead::char_33 gname;
        cgio_read_block_data_type(self->cgioNum, iterChildId[cc],
          (cgsize_t)(self->ActualTimeStep * 32 + 1), (cgsize_t)(self->ActualTimeStep * 32 + 32),
          "C1", (void*)gname);
        gname[32] = '\0';
        CGNSRead::removeTrailingWhiteSpaces(gname);
        std::string tmpStr = std::string(gname);
        if (tmpStr != "Null" && !tmpStr.empty())
        {
          unvalidatedSolutionNames.push_back(tmpStr);
        }
      }
      cgio_release_id(self->cgioNum, iterChildId[cc]);
    }

    // Validate the names read from FlowSolutionPointers. Some exporters are known to mess up.
    for (size_t cc = 0; cc < unvalidatedSolutionNames.size(); ++cc)
    {
      double solId = 0.0;
      if (cgio_get_node_id(
            self->cgioNum, self->currentId, unvalidatedSolutionNames[cc].c_str(), &solId) == CG_OK)
      {
        solutionNames.push_back(unvalidatedSolutionNames[cc]);
      }
    }

    // If we couldn't find a single valid solution for the current timestep, we
    // should assume that FlowSolutionPointers are invalid, and we use the some
    // heuristics to decide which FlowSolution_t nodes correspond to current
    // timestep.
    ignoreFlowSolutionPointers = (solutionNames.empty() && !unvalidatedSolutionNames.empty());
    if (ignoreFlowSolutionPointers)
    {
      vtkGenericWarningMacro("`FlowSolutionPointers` in the CGNS file '"
        << self->FileName
        << "' "
           "refer to invalid solution nodes. Ignoring them.");
    }
  }

  // Case where everything is OK with standard FlowSolutionPointers
  if (hasZoneIterativeData && baseInfo.useFlowPointers && !ignoreFlowSolutionPointers &&
    !useUnsteadyPattern)
  {
    // Since we are not too careful about avoiding duplicates in solutionNames
    // array, let's clean it up here.
    std::sort(solutionNames.begin(), solutionNames.end());
    std::vector<std::string>::iterator last =
      std::unique(solutionNames.begin(), solutionNames.end());
    solutionNames.erase(last, solutionNames.end());
    cgio_release_id(self->cgioNum, ziterId);
    ziterId = 0;
    return CG_OK;
  }

  std::vector<double> childId;
  CGNSRead::getNodeChildrenId(self->cgioNum, self->currentId, childId);
  // Case where FlowSolutionPointers where not enough but there is a pattern in nodeName.
  if (useUnsteadyPattern)
  {
    // Ideally ZoneIterativeData_t/FlowSolutionPointers tell us all solution grids
    // for current timestep, but that may not be the case. Sometimes
    // ZoneIterativeData_t is missing or incomplete. So let's handle that next.

    // If we processed at least 1 FlowSolutionPointers, then we can form a pattern
    // for the names for solutions to match the current timestep.
    std::set<int> stepNumbers;
    vtksys::RegularExpression stepRe("^[^0-9]+([0-9]+)$");
    if (hasZoneIterativeData && baseInfo.useFlowPointers && !ignoreFlowSolutionPointers)
    {
      std::ostringstream str;
      for (size_t cc = 0; cc < solutionNames.size(); ++cc)
      {
        if (stepRe.find(solutionNames[cc]))
        {
          stepNumbers.insert(atoi(stepRe.match(1).c_str()));
        }
      }
    }
    else if (!baseInfo.times.empty())
    {
      // we don't have FlowSolutionPointers in the dataset
      stepNumbers.insert(self->ActualTimeStep + 1);
    }

    //  For that, we first collect a list of names for all FlowSolution_t nodes in
    // this zone.
    for (size_t cc = 0; cc < childId.size(); ++cc)
    {
      CGNSRead::char_33 nodeLabel;
      CGNSRead::char_33 nodeName;
      if (cgio_get_name(self->cgioNum, childId[cc], nodeName) == CG_OK &&
        cgio_get_label(self->cgioNum, childId[cc], nodeLabel) == CG_OK &&
        strcmp(nodeLabel, "FlowSolution_t") == 0)
      {
        if (!stepNumbers.empty())
        {
          if (stepRe.find(nodeName) == true &&
            stepNumbers.find(atoi(stepRe.match(1).c_str())) != stepNumbers.end())
          {
            // the current nodeName ends with a number that matches the current timestep
            // or timestep indicated at end of an existing nodeName.
            solutionNames.emplace_back(nodeName);
          }
        }
        else
        {
          // is stepNumbers is empty, it means the data was not temporal at all,
          // so just read all solution nodes.
          solutionNames.emplace_back(nodeName);
        }
      }
    }
  }
  if (solutionNames.empty())
  {
    // if we still have no solution nodes discovered, then we read the 1st solution node for
    // each GridLocation (see paraview/paraview#17586).
    // C'est la vie!
    std::set<CGNS_ENUMT(GridLocation_t)> handledCenterings;
    for (size_t cc = 0; cc < childId.size(); ++cc)
    {
      CGNSRead::char_33 nodeLabel;
      CGNSRead::char_33 nodeName;
      if (cgio_get_name(self->cgioNum, childId[cc], nodeName) == CG_OK &&
        cgio_get_label(self->cgioNum, childId[cc], nodeLabel) == CG_OK &&
        strcmp(nodeLabel, "FlowSolution_t") == 0)
      {
        CGNS_ENUMT(GridLocation_t) varCentering = CGNS_ENUMV(Vertex);
        double gridLocationNodeId = 0.0;
        if (CGNSRead::getFirstNodeId(
              self->cgioNum, childId[cc], "GridLocation_t", &gridLocationNodeId) == CG_OK)
        {
          std::string location;
          CGNSRead::readNodeStringData(self->cgioNum, gridLocationNodeId, location);
          if (location == "Vertex")
          {
            varCentering = CGNS_ENUMV(Vertex);
          }
          else if (location == "CellCenter")
          {
            varCentering = CGNS_ENUMV(CellCenter);
          }
          else
          {
            varCentering = CGNS_ENUMV(GridLocationNull);
          }
          cgio_release_id(self->cgioNum, gridLocationNodeId);
        }
        if (handledCenterings.find(varCentering) == handledCenterings.end())
        {
          handledCenterings.insert(varCentering);
          solutionNames.emplace_back(nodeName);
        }
        else if (self->GetCreateEachSolutionAsBlock())
        {
          solutionNames.emplace_back(nodeName);
        }
      }
    }
  }

  CGNSRead::releaseIds(self->cgioNum, childId);
  childId.clear();

  // Since we are not too careful about avoiding duplicates in solutionNames
  // array, let's clean it up here.
  std::sort(solutionNames.begin(), solutionNames.end());
  std::vector<std::string>::iterator last = std::unique(solutionNames.begin(), solutionNames.end());
  solutionNames.erase(last, solutionNames.end());
  if (hasZoneIterativeData)
  {
    cgio_release_id(self->cgioNum, ziterId);
    ziterId = 0;
  }
  return CG_OK;
}

//------------------------------------------------------------------------------
int vtkCGNSReader::vtkPrivate::getCoordsIdAndFillRind(const std::string& gridCoordNameStr,
  int physicalDim, std::size_t& nCoordsArray, std::vector<double>& gridChildId, int* rind,
  vtkCGNSReader* self)
{
  CGNSRead::char_33 GridCoordName;
  strncpy(GridCoordName, gridCoordNameStr.c_str(), 32);
  GridCoordName[32] = '\0';

  char nodeLabel[CGIO_MAX_NAME_LENGTH + 1];
  std::size_t na;

  nCoordsArray = 0;
  // Get GridCoordinate node ID for low level access
  double gridId;
  if (cgio_get_node_id(self->cgioNum, self->currentId, GridCoordName, &gridId) != CG_OK)
  {
    char message[81];
    cgio_error_message(message);
    vtkErrorWithObjectMacro(self, << "Error while reading mesh coordinates node :" << message);
    return 1;
  }

  // Get the number of Coordinates in GridCoordinates node
  CGNSRead::getNodeChildrenId(self->cgioNum, gridId, gridChildId);

  for (int n = 0; n < 6; n++)
  {
    rind[n] = 0;
  }
  for (nCoordsArray = 0, na = 0; na < gridChildId.size(); ++na)
  {
    if (cgio_get_label(self->cgioNum, gridChildId[na], nodeLabel) != CG_OK)
    {
      vtkErrorWithObjectMacro(self, << "Not enough coordinates in node " << GridCoordName << "\n");
      continue;
    }

    if (strcmp(nodeLabel, "DataArray_t") == 0)
    {
      if (nCoordsArray < na)
      {
        gridChildId[nCoordsArray] = gridChildId[na];
      }
      nCoordsArray++;
    }
    else if (strcmp(nodeLabel, "Rind_t") == 0)
    {
      // check for rind
      CGNSRead::setUpRind(self->cgioNum, gridChildId[na], rind);
    }
    else
    {
      cgio_release_id(self->cgioNum, gridChildId[na]);
    }
  }
  if (nCoordsArray < static_cast<std::size_t>(physicalDim))
  {
    vtkErrorWithObjectMacro(self, << "Not enough coordinates in node " << GridCoordName << "\n");
    return 1;
  }
  cgio_release_id(self->cgioNum, gridId);
  return 0;
}

//------------------------------------------------------------------------------
int vtkCGNSReader::vtkPrivate::getVarsIdAndFillRind(double cgioSolId, std::size_t& nVarArray,
  CGNS_ENUMT(GridLocation_t) & varCentering, std::vector<double>& solChildId, int* rind,
  vtkCGNSReader* self)
{
  char nodeLabel[CGIO_MAX_NAME_LENGTH + 1];
  std::size_t na;

  nVarArray = 0;
  for (int n = 0; n < 6; ++n)
  {
    rind[n] = 0;
  }

  CGNSRead::getNodeChildrenId(self->cgioNum, cgioSolId, solChildId);

  for (nVarArray = 0, na = 0; na < solChildId.size(); ++na)
  {
    if (cgio_get_label(self->cgioNum, solChildId[na], nodeLabel) != CG_OK)
    {
      vtkErrorWithObjectMacro(self, << "Error while reading node label in solution\n");
      continue;
    }

    if (strcmp(nodeLabel, "DataArray_t") == 0)
    {
      if (nVarArray < na)
      {
        solChildId[nVarArray] = solChildId[na];
      }
      nVarArray++;
    }
    else if (strcmp(nodeLabel, "Rind_t") == 0)
    {
      CGNSRead::setUpRind(self->cgioNum, solChildId[na], rind);
    }
    else if (strcmp(nodeLabel, "GridLocation_t") == 0)
    {
      CGNSRead::char_33 dataType;

      if (cgio_get_data_type(self->cgioNum, solChildId[na], dataType) != CG_OK)
      {
        return 1;
      }

      if (strcmp(dataType, "C1") != 0)
      {
        std::cerr << "Unexpected data type for GridLocation_t node" << std::endl;
        return 1;
      }

      std::string location;
      CGNSRead::readNodeStringData(self->cgioNum, solChildId[na], location);

      if (location == "Vertex")
      {
        varCentering = CGNS_ENUMV(Vertex);
      }
      else if (location == "CellCenter")
      {
        varCentering = CGNS_ENUMV(CellCenter);
      }
      else
      {
        varCentering = CGNS_ENUMV(GridLocationNull);
      }
    }
    else
    {
      cgio_release_id(self->cgioNum, solChildId[na]);
    }
  }
  return 0;
}

//------------------------------------------------------------------------------
int vtkCGNSReader::vtkPrivate::readSolution(const std::string& solutionNameStr, int cellDim,
  int physicalDim, const cgsize_t* zsize, vtkDataSet* dataset, const int* voi, vtkCGNSReader* self)
{
  if (solutionNameStr.empty())
  {
    return CG_OK; // should this be error?
  }

  CGNSRead::char_33 solutionName;
  strncpy(solutionName, solutionNameStr.c_str(), 32);
  solutionName[32] = '\0';

  double cgioSolId = 0.0;
  if (cgio_get_node_id(self->cgioNum, self->currentId, solutionName, &cgioSolId) != CG_OK)
  {
    char errmsg[CGIO_MAX_ERROR_LENGTH + 1];
    cgio_error_message(errmsg);
    vtkGenericWarningMacro(<< "Problem while reading Solution named '" << solutionName
                           << "', error : " << errmsg);
    return 1;
  }

  std::vector<double> solChildId;
  std::size_t nVarArray = 0;
  int rind[6];
  CGNS_ENUMT(GridLocation_t) varCentering = CGNS_ENUMV(Vertex);

  vtkPrivate::getVarsIdAndFillRind(cgioSolId, nVarArray, varCentering, solChildId, rind, self);

  if ((varCentering != CGNS_ENUMV(Vertex)) && (varCentering != CGNS_ENUMV(CellCenter)))
  {
    vtkGenericWarningMacro(<< "Solution " << solutionName << " centering is not supported\n");
    return 1;
  }

  std::vector<CGNSRead::CGNSVariable> cgnsVars(nVarArray);
  std::vector<CGNSRead::CGNSVector> cgnsVectors;
  vtkPrivate::fillArrayInformation(solChildId, physicalDim, cgnsVars, cgnsVectors, self);

  // Source
  cgsize_t fieldSrcStart[3] = { 1, 1, 1 };
  cgsize_t fieldSrcStride[3] = { 1, 1, 1 };
  cgsize_t fieldSrcEnd[3];

  // Destination Memory
  cgsize_t fieldMemStart[3] = { 1, 1, 1 };
  cgsize_t fieldMemStride[3] = { 1, 1, 1 };
  cgsize_t fieldMemEnd[3] = { 1, 1, 1 };
  cgsize_t fieldMemDims[3] = { 1, 1, 1 };

  vtkIdType nVals = 0;

  // Get solution data range
  int nsc = varCentering == CGNS_ENUMV(Vertex) ? 0 : cellDim;

  for (int n = 0; n < cellDim; ++n)
  {
    fieldSrcStart[n] = rind[2 * n] + 1;
    fieldSrcEnd[n] = rind[2 * n] + zsize[n + nsc];
    fieldMemEnd[n] = zsize[n + nsc];
    fieldMemDims[n] = zsize[n + nsc];
  }

  if (voi != nullptr)
  {
    // we are provided a sub-extent to read.
    // update src and mem pointers.
    const int* pvoi = voi;
    int cell_voi[6];
    if (varCentering == CGNS_ENUMV(CellCenter))
    {
      // need to convert pt-extents provided in VOI to cell extents.
      vtkStructuredData::GetCellExtentFromPointExtent(const_cast<int*>(voi), cell_voi);
      // if outer edge, the above method doesn't do well. so handle it.
      for (int n = 0; n < cellDim; ++n)
      {
        cell_voi[2 * n] = std::min<int>(cell_voi[2 * n], zsize[n + nsc] - 1);
        cell_voi[2 * n + 1] = std::min<int>(cell_voi[2 * n + 1], zsize[n + nsc] - 1);
      }
      pvoi = cell_voi;
    }

    // now update the source and dest regions.
    for (int n = 0; n < cellDim; ++n)
    {
      fieldSrcStart[n] += pvoi[2 * n];
      fieldSrcEnd[n] = fieldSrcStart[n] + (pvoi[2 * n + 1] - pvoi[2 * n]);
      fieldMemEnd[n] = (pvoi[2 * n + 1] - pvoi[2 * n]) + 1;
      fieldMemDims[n] = fieldMemEnd[n];
    }
  }

  // compute number of field values
  nVals = static_cast<vtkIdType>(fieldMemEnd[0] * fieldMemEnd[1] * fieldMemEnd[2]);

  // sanity check: nVals must equal num-points or num-cells.
  if (varCentering == CGNS_ENUMV(CellCenter) && nVals != dataset->GetNumberOfCells())
  {
    vtkErrorWithObjectMacro(self,
      "Mismatch in number of cells and number of values "
      "being read from Solution '"
        << solutionNameStr.c_str()
        << "'. "
           "Skipping reading. Please report as a bug.");
    return CG_ERROR;
  }
  if (varCentering == CGNS_ENUMV(Vertex) && nVals != dataset->GetNumberOfPoints())
  {
    vtkErrorWithObjectMacro(self,
      "Mismatch in number of points and number of values "
      "being read from Solution '"
        << solutionNameStr.c_str()
        << "'. "
           "Skipping reading. Please report as a bug.");
    return CG_ERROR;
  }

  //
  // VECTORS aliasing ...
  // destination
  int requestedVectorDim = physicalDim;
  if (physicalDim < 3)
  {
    requestedVectorDim = self->Use3DVector ? 3 : physicalDim;
  }
  cgsize_t fieldVectMemStart[3] = { 1, 1, 1 };
  cgsize_t fieldVectMemStride[3] = { 3, 1, 1 };
  cgsize_t fieldVectMemEnd[3] = { 1, 1, 1 };
  cgsize_t fieldVectMemDims[3] = { 1, 1, 1 };

  fieldVectMemStride[0] = static_cast<cgsize_t>(requestedVectorDim);

  fieldVectMemDims[0] = fieldMemDims[0] * fieldVectMemStride[0];
  fieldVectMemDims[1] = fieldMemDims[1];
  fieldVectMemDims[2] = fieldMemDims[2];
  fieldVectMemEnd[0] = fieldMemEnd[0] * fieldVectMemStride[0];
  fieldVectMemEnd[1] = fieldMemEnd[1];
  fieldVectMemEnd[2] = fieldMemEnd[2];

  //
  std::vector<vtkDataArray*> vtkVars(nVarArray);
  // Count number of vars and vectors
  // Assign vars and vectors to a vtkvars array
  vtkPrivate::AllocateVtkArray(
    physicalDim, requestedVectorDim, nVals, varCentering, cgnsVars, cgnsVectors, vtkVars, self);

  // Load Data
  for (std::size_t ff = 0; ff < nVarArray; ++ff)
  {
    // only read allocated fields
    if (vtkVars[ff] == nullptr)
    {
      continue;
    }
    double cgioVarId = solChildId[ff];
    const char* fieldDataType = get_data_type(cgnsVars[ff].dt);

    // quick transfer of data because data types is given by cgns database
    if (cgnsVars[ff].isComponent == false)
    {
      if (cgio_read_data_type(self->cgioNum, cgioVarId, fieldSrcStart, fieldSrcEnd, fieldSrcStride,
            fieldDataType, cellDim, fieldMemDims, fieldMemStart, fieldMemEnd, fieldMemStride,
            (void*)vtkVars[ff]->GetVoidPointer(0)) != CG_OK)
      {
        char message[81];
        cgio_error_message(message);
        vtkGenericWarningMacro(<< "cgio_read_data_type :" << message);
      }
    }
    else
    {
      if (cgio_read_data_type(self->cgioNum, cgioVarId, fieldSrcStart, fieldSrcEnd, fieldSrcStride,
            fieldDataType, cellDim, fieldVectMemDims, fieldVectMemStart, fieldVectMemEnd,
            fieldVectMemStride,
            (void*)vtkVars[ff]->GetVoidPointer(cgnsVars[ff].xyzIndex - 1)) != CG_OK)
      {
        char message[81];
        cgio_error_message(message);
        vtkGenericWarningMacro(<< "cgio_read_data_type :" << message);
      }
    }
    cgio_release_id(self->cgioNum, cgioVarId);
  }
  cgio_release_id(self->cgioNum, cgioSolId);

  // Append data to dataset
  vtkDataSetAttributes* dsa = nullptr;
  if (varCentering == CGNS_ENUMV(Vertex)) // ON_NODES
  {
    dsa = dataset->GetPointData();
  }
  if (varCentering == CGNS_ENUMV(CellCenter)) // ON_CELL
  {
    dsa = dataset->GetCellData();
  }

  // SetData in zone dataset & clean pointers
  for (std::size_t nv = 0; nv < nVarArray; ++nv)
  {
    // only transfer allocated fields
    if (vtkVars[nv] == nullptr)
    {
      continue;
    }

    if (cgnsVars[nv].isComponent == false)
    {
      dsa->AddArray(vtkVars[nv]);
      vtkVars[nv]->Delete();
    }
    else if (cgnsVars[nv].xyzIndex == 1)
    {
      dsa->AddArray(vtkVars[nv]);
      if (!dsa->GetVectors() && requestedVectorDim == 3)
      {
        dsa->SetVectors(vtkVars[nv]);
      }
      if (requestedVectorDim != physicalDim)
      {
        for (int dim = physicalDim; dim < requestedVectorDim; dim++)
        {
          vtkVars[nv]->FillComponent(dim, 0.0);
        }
      }
      vtkVars[nv]->Delete();
    }
    vtkVars[nv] = nullptr;
  }

  return CG_OK;
}

//------------------------------------------------------------------------------
int vtkCGNSReader::vtkPrivate::readBCData(double nodeId, int cellDim, int physicalDim,
  CGNS_ENUMT(GridLocation_t) locationParam, vtkDataSet* dataset, vtkCGNSReader* self)
{
  if (cellDim == 0 || physicalDim == 0)
  {
    return 1;
  }
  std::vector<double> childrenIds;
  CGNSRead::getNodeChildrenId(self->cgioNum, nodeId, childrenIds);

  for (auto& childrenId : childrenIds)
  {
    char nodeName[CGIO_MAX_NAME_LENGTH + 1];
    char nodeLabel[CGIO_MAX_LABEL_LENGTH + 1];
    CGIOErrorSafe(cgio_get_name(self->cgioNum, childrenId, nodeName));
    CGIOErrorSafe(cgio_get_label(self->cgioNum, childrenId, nodeLabel));
    if (strcmp(nodeLabel, "BCDataSet_t") == 0)
    {
      // Found a BCDataset_t and now load its data
      CGNS_ENUMT(GridLocation_t) varCentering = locationParam;
      std::vector<double> BCDataSetChildrens;
      std::vector<double> BCDataChildList; // Neumann and Dirichlet data node
      CGNSRead::getNodeChildrenId(self->cgioNum, childrenId, BCDataSetChildrens);
      for (auto& BCDataSetChild : BCDataSetChildrens)
      {
        CGIOErrorSafe(cgio_get_label(self->cgioNum, BCDataSetChild, nodeLabel));
        if (strcmp(nodeLabel, "BCData_t") == 0)
        {
          BCDataChildList.push_back(BCDataSetChild);
        }
        else if (strcmp(nodeLabel, "GridLocation_t") == 0)
        {
          std::string location;
          CGNSRead::readNodeStringData(self->cgioNum, BCDataSetChild, location);
          if (location == "FaceCenter" || location == "IFaceCenter" || location == "JFaceCenter" ||
            location == "KFaceCenter")
          {
            varCentering = CGNS_ENUMV(FaceCenter);
          }
          else if (location == "Vertex")
          {
            varCentering = CGNS_ENUMV(Vertex);
          }
          else
          {
            return 1;
          }
        }
      }
      // Now read Neumann and Dirichlet arrays
      for (auto& BCDataChild : BCDataChildList)
      {
        std::vector<double> BCDataArrayIds;
        CGNSRead::getNodeChildrenId(self->cgioNum, BCDataChild, BCDataArrayIds);
        // number Of Values to load per Array for the BCData
        vtkIdType numValues = dataset->GetNumberOfCells();
        if (varCentering == CGNS_ENUMV(Vertex))
        {
          numValues = dataset->GetNumberOfPoints();
        }
        std::vector<double> varIds;
        std::vector<CGNSRead::CGNSVariable> cgnsVars;
        std::vector<CGNSRead::CGNSVector> cgnsVectors;
        std::vector<vtkDataArray*> vtkVars;
        // Array Creation
        // Do not care about IsVarEnable right now
        // Maybe use AllocateVtkArray later
        for (auto& ArrayId : BCDataArrayIds)
        {
          CGIOErrorSafe(cgio_get_label(self->cgioNum, ArrayId, nodeLabel));

          if (strcmp(nodeLabel, "DataArray_t") == 0)
          {
            CGNSRead::CGNSVariable curVar;
            CGIOErrorSafe(cgio_get_name(self->cgioNum, ArrayId, curVar.name));
            curVar.isComponent = false;
            curVar.xyzIndex = 0;

            // read node data type
            CGNSRead::char_33 dataType;
            if (cgio_get_data_type(self->cgioNum, ArrayId, dataType))
            {
              continue;
            }
            if (strcmp(dataType, "R8") == 0)
            {
              curVar.dt = CGNS_ENUMV(RealDouble);
            }
            else if (strcmp(dataType, "R4") == 0)
            {
              curVar.dt = CGNS_ENUMV(RealSingle);
            }
            else if (strcmp(dataType, "I4") == 0)
            {
              curVar.dt = CGNS_ENUMV(Integer);
            }
            else if (strcmp(dataType, "I8") == 0)
            {
              curVar.dt = CGNS_ENUMV(LongInteger);
            }
            else
            {
              continue;
            }
            cgnsVars.push_back(curVar);
            varIds.push_back(ArrayId);
          }
        }
        CGNSRead::fillVectorsFromVars(cgnsVars, cgnsVectors, physicalDim);
        vtkVars.resize(cgnsVars.size());
        for (std::size_t var = 0; var < cgnsVars.size(); var++)
        {
          vtkVars[var] = nullptr;
          if (cgnsVars[var].isComponent == false)
          {
            switch (cgnsVars[var].dt)
            {
              case CGNS_ENUMV(Integer):
                vtkVars[var] = vtkIntArray::New();
                break;
              case CGNS_ENUMV(LongInteger):
                vtkVars[var] = vtkLongArray::New();
                break;
              case CGNS_ENUMV(RealSingle):
                vtkVars[var] = vtkFloatArray::New();
                break;
              case CGNS_ENUMV(RealDouble):
                vtkVars[var] = vtkDoubleArray::New();
                break;
              default:
                continue;
            }
            vtkVars[var]->SetName(cgnsVars[var].name);
            vtkVars[var]->SetNumberOfComponents(1);
            vtkVars[var]->SetNumberOfTuples(numValues);
          }
        }

        for (std::vector<CGNSRead::CGNSVector>::const_iterator iter = cgnsVectors.begin();
             iter != cgnsVectors.end(); ++iter)
        {
          vtkDataArray* arr = nullptr;

          int nv = iter->xyzIndex[0];
          switch (cgnsVars[nv].dt)
          {
            case CGNS_ENUMV(Integer):
              arr = vtkIntArray::New();
              break;
            case CGNS_ENUMV(LongInteger):
              arr = vtkLongArray::New();
              break;
            case CGNS_ENUMV(RealSingle):
              arr = vtkFloatArray::New();
              break;
            case CGNS_ENUMV(RealDouble):
              arr = vtkDoubleArray::New();
              break;
            default:
              continue;
          }

          arr->SetName(iter->name);
          arr->SetNumberOfComponents(physicalDim);
          arr->SetNumberOfTuples(numValues);

          for (int dim = 0; dim < physicalDim; ++dim)
          {
            arr->SetComponentName(static_cast<vtkIdType>(dim), cgnsVars[iter->xyzIndex[dim]].name);
            vtkVars[iter->xyzIndex[dim]] = arr;
          }
        }
        // Now Load Boundary Values
        for (std::size_t ff = 0; ff < cgnsVars.size(); ++ff)
        {
          // only read allocated fields
          if (vtkVars[ff] == nullptr)
          {
            continue;
          }
          double cgioVarId = varIds[ff];
          const char* fieldDataType = get_data_type(cgnsVars[ff].dt);

          cgsize_t dataSize = 1;
          cgsize_t dimVals[12];
          int n, ndim;

          if (cgio_get_dimensions(self->cgioNum, cgioVarId, &ndim, dimVals) != CG_OK)
          {
            cgio_error_exit("cgio_get_dimensions");
            return 1;
          }

          // allocate data
          for (n = 0; n < ndim; n++)
          {
            dataSize *= dimVals[n];
          }
          if (dataSize <= 0)
          {
            continue;
          }
          //
          if (dataSize == 1 || dataSize == numValues)
          {
            // quick transfer of data because data types is given by cgns database
            if (cgnsVars[ff].isComponent == false)
            {
              if (cgio_read_all_data_type(self->cgioNum, cgioVarId, fieldDataType,
                    (void*)vtkVars[ff]->GetVoidPointer(0)) != CG_OK)
              {
                char message[81];
                cgio_error_message(message);
                vtkGenericWarningMacro(<< "cgio_read_all_data_type :" << message);
              }
              if (dataSize == 1)
              {
                // This is an uniform boundary condition value
                for (vtkIdType idx = 1; idx < numValues; ++idx)
                {
                  vtkVars[ff]->SetTuple(idx, vtkVars[ff]->GetTuple(0));
                }
              }
            }
            else
            {
              //
              // VECTORS aliasing ...
              // Source
              cgsize_t fieldSrcStart[3] = { 1, 1, 1 };
              cgsize_t fieldSrcStride[3] = { 1, 1, 1 };
              cgsize_t fieldSrcEnd[3];

              // Destination Memory
              cgsize_t fieldVectMemStart[3] = { 1, 1, 1 };
              cgsize_t fieldVectMemStride[3] = { 3, 1, 1 };
              cgsize_t fieldVectMemEnd[3] = { 1, 1, 1 };
              cgsize_t fieldVectMemDims[3] = { 1, 1, 1 };

              fieldSrcEnd[0] = static_cast<cgsize_t>(dataSize);
              fieldVectMemStride[0] = static_cast<cgsize_t>(physicalDim);
              fieldVectMemDims[0] = fieldSrcEnd[0] * fieldVectMemStride[0];
              fieldVectMemEnd[0] = fieldSrcEnd[0] * fieldVectMemStride[0];

              if (cgio_read_data_type(self->cgioNum, cgioVarId, fieldSrcStart, fieldSrcEnd,
                    fieldSrcStride, fieldDataType, 1, fieldVectMemDims, fieldVectMemStart,
                    fieldVectMemEnd, fieldVectMemStride,
                    (void*)vtkVars[ff]->GetVoidPointer(cgnsVars[ff].xyzIndex - 1)) != CG_OK)
              {
                char message[81];
                cgio_error_message(message);
                vtkGenericWarningMacro(<< "cgio_read_data_type :" << message);
              }
              if (dataSize == 1)
              {
                // This is an uniform boundary condition value
                for (vtkIdType idx = 1; idx < numValues; ++idx)
                {
                  vtkVars[ff]->SetComponent(idx, cgnsVars[ff].xyzIndex - 1,
                    vtkVars[ff]->GetComponent(0, cgnsVars[ff].xyzIndex - 1));
                }
              }
            }
          }
        }

        // Append data to dataset
        vtkDataSetAttributes* dsa = nullptr;
        if (varCentering == CGNS_ENUMV(Vertex)) // Vertex
        {
          dsa = dataset->GetPointData();
        }
        if (varCentering == CGNS_ENUMV(FaceCenter)) // Face Center
        {
          dsa = dataset->GetCellData();
        }

        // SetData in zone dataset & clean pointers
        for (std::size_t nv = 0; nv < vtkVars.size(); ++nv)
        {
          // only transfer allocated fields
          if (vtkVars[nv] == nullptr)
          {
            continue;
          }

          if (cgnsVars[nv].isComponent == false)
          {
            dsa->AddArray(vtkVars[nv]);
            vtkVars[nv]->Delete();
          }
          else if (cgnsVars[nv].xyzIndex == 1)
          {
            dsa->AddArray(vtkVars[nv]);
            if (!dsa->GetVectors() && physicalDim == 3)
            {
              dsa->SetVectors(vtkVars[nv]);
            }
            vtkVars[nv]->Delete();
          }
          vtkVars[nv] = nullptr;
        }
        CGNSRead::releaseIds(self->cgioNum, BCDataArrayIds);
      }
      CGNSRead::releaseIds(self->cgioNum, BCDataSetChildrens);
    }
  }
  CGNSRead::releaseIds(self->cgioNum, childrenIds);
  return 0;
}

//------------------------------------------------------------------------------
int vtkCGNSReader::vtkPrivate::fillArrayInformation(const std::vector<double>& solChildId,
  int physicalDim, std::vector<CGNSRead::CGNSVariable>& cgnsVars,
  std::vector<CGNSRead::CGNSVector>& cgnsVectors, vtkCGNSReader* self)
{
  // Read variable names
  for (std::size_t ff = 0; ff < cgnsVars.size(); ++ff)
  {
    cgio_get_name(self->cgioNum, solChildId[ff], cgnsVars[ff].name);
    cgnsVars[ff].isComponent = false;
    cgnsVars[ff].xyzIndex = 0;

    // read node data type
    CGNSRead::char_33 dataType;
    cgio_get_data_type(self->cgioNum, solChildId[ff], dataType);
    if (strcmp(dataType, "R8") == 0)
    {
      cgnsVars[ff].dt = CGNS_ENUMV(RealDouble);
    }
    else if (strcmp(dataType, "R4") == 0)
    {
      cgnsVars[ff].dt = CGNS_ENUMV(RealSingle);
    }
    else if (strcmp(dataType, "I4") == 0)
    {
      cgnsVars[ff].dt = CGNS_ENUMV(Integer);
    }
    else if (strcmp(dataType, "I8") == 0)
    {
      cgnsVars[ff].dt = CGNS_ENUMV(LongInteger);
    }
    else
    {
      continue;
    }
  }
  // Create vector name from available variable
  // when VarX, VarY, VarZ is detected
  CGNSRead::fillVectorsFromVars(cgnsVars, cgnsVectors, physicalDim);
  return 0;
}

//------------------------------------------------------------------------------
int vtkCGNSReader::vtkPrivate::AllocateVtkArray(int physicalDim, int requestedVectorDim,
  vtkIdType nVals, CGNS_ENUMT(GridLocation_t) varCentering,
  const std::vector<CGNSRead::CGNSVariable>& cgnsVars,
  const std::vector<CGNSRead::CGNSVector>& cgnsVectors, std::vector<vtkDataArray*>& vtkVars,
  vtkCGNSReader* self)
{
  for (std::size_t ff = 0; ff < cgnsVars.size(); ff++)
  {
    vtkVars[ff] = nullptr;

    if (cgnsVars[ff].isComponent == false)
    {
      if (vtkPrivate::IsVarEnabled(varCentering, cgnsVars[ff].name, self) == false)
      {
        continue;
      }

      switch (cgnsVars[ff].dt)
      {
        // Other case to handle
        case CGNS_ENUMV(Integer):
          vtkVars[ff] = vtkIntArray::New();
          break;
        case CGNS_ENUMV(LongInteger):
          vtkVars[ff] = vtkLongArray::New();
          break;
        case CGNS_ENUMV(RealSingle):
          vtkVars[ff] = vtkFloatArray::New();
          break;
        case CGNS_ENUMV(RealDouble):
          vtkVars[ff] = vtkDoubleArray::New();
          break;
        case CGNS_ENUMV(Character):
          vtkVars[ff] = vtkCharArray::New();
          break;
        default:
          continue;
      }
      vtkVars[ff]->SetName(cgnsVars[ff].name);
      vtkVars[ff]->SetNumberOfComponents(1);
      vtkVars[ff]->SetNumberOfTuples(nVals);
    }
  }

  for (std::vector<CGNSRead::CGNSVector>::const_iterator iter = cgnsVectors.begin();
       iter != cgnsVectors.end(); ++iter)
  {
    vtkDataArray* arr = nullptr;

    if (vtkPrivate::IsVarEnabled(varCentering, iter->name, self) == false)
    {
      continue;
    }

    int nv = iter->xyzIndex[0];
    switch (cgnsVars[nv].dt)
    {
      // TODO: other cases
      case CGNS_ENUMV(Integer):
        arr = vtkIntArray::New();
        break;
      case CGNS_ENUMV(LongInteger):
        arr = vtkLongArray::New();
        break;
      case CGNS_ENUMV(RealSingle):
        arr = vtkFloatArray::New();
        break;
      case CGNS_ENUMV(RealDouble):
        arr = vtkDoubleArray::New();
        break;
      case CGNS_ENUMV(Character):
        arr = vtkCharArray::New();
        break;
      default:
        continue;
    }

    arr->SetName(iter->name);
    arr->SetNumberOfComponents(requestedVectorDim);
    arr->SetNumberOfTuples(nVals);

    for (int dim = 0; dim < physicalDim; ++dim)
    {
      arr->SetComponentName(static_cast<vtkIdType>(dim), cgnsVars[iter->xyzIndex[dim]].name);
      vtkVars[iter->xyzIndex[dim]] = arr;
    }
    for (int dim = physicalDim; dim < requestedVectorDim; ++dim)
    {
      arr->SetComponentName(static_cast<vtkIdType>(dim), "dummy");
    }
  }
  return 0;
}

//------------------------------------------------------------------------------
int vtkCGNSReader::vtkPrivate::AttachReferenceValue(int base, vtkDataSet* ds, vtkCGNSReader* self)
{
  // Handle Reference Values (Mach Number, ...)
  const std::map<std::string, double>& arrState =
    self->Internals->Internal->GetBase(base).referenceState;
  std::map<std::string, double>::const_iterator iteRef = arrState.begin();
  for (iteRef = arrState.begin(); iteRef != arrState.end(); iteRef++)
  {
    vtkDoubleArray* refValArray = vtkDoubleArray::New();
    refValArray->SetNumberOfComponents(1);
    refValArray->SetName(iteRef->first.c_str());
    refValArray->InsertNextValue(iteRef->second);
    ds->GetFieldData()->AddArray(refValArray);
    refValArray->Delete();
  }
  return 0;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkDataObject> vtkCGNSReader::vtkPrivate::readCurvilinearZone(int base, int zone,
  int cellDim, int physicalDim, const cgsize_t* zsize, const int* voi, vtkCGNSReader* self)
{
  int rind[6];
  int n;
  // int ier;

  // Source Layout
  cgsize_t srcStart[3] = { 1, 1, 1 };
  cgsize_t srcStride[3] = { 1, 1, 1 };
  cgsize_t srcEnd[3];

  // Memory Destination Layout
  cgsize_t memStart[3] = { 1, 1, 1 };
  cgsize_t memStride[3] = { 3, 1, 1 };
  cgsize_t memEnd[3] = { 1, 1, 1 };
  cgsize_t memDims[3] = { 1, 1, 1 };

  int extent[6] = { 0, 0, 0, 0, 0, 0 };

  // Get Coordinates and FlowSolution node names
  std::string gridCoordName;
  std::vector<std::string> solutionNames;
  std::string keyMesh;

  std::vector<double> gridChildId;
  std::size_t nCoordsArray = 0;
  vtkSmartPointer<vtkPoints> points;

  vtkPrivate::getGridAndSolutionNames(base, gridCoordName, solutionNames, self);
  if (gridCoordName == "Null")
  {
    return vtkSmartPointer<vtkDataObject>();
  }

  // If it is not a deforming mesh, gridCoordName keep the standard name
  // Only Volume mesh points, not subset are cached
  bool caching = (gridCoordName == "GridCoordinates" && voi == nullptr && self->CacheMesh);
  if (caching)
  {
    // Try to get from cache
    const char* basename = self->Internals->Internal->GetBase(base).name;
    const char* zonename = self->Internals->Internal->GetBase(base).zones[zone].name;
    // build a key /basename/zonename
    keyMesh = vtkPrivate::GenerateMeshKey(basename, zonename);

    points = self->Internals->MeshPointsCache.Find(keyMesh);
    if (points.Get() != nullptr)
    {
      // check storage data type
      if ((self->GetDoublePrecisionMesh() != 0) != (points->GetDataType() == VTK_DOUBLE))
      {
        points = nullptr;
      }
      for (n = 0; n < cellDim; n++)
      {
        extent[1 + 2 * n] = zsize[n] - 1;
      }
    }
  }

  // Reading points in file since cache was not hit
  if (points.Get() == nullptr)
  {
    vtkPrivate::getCoordsIdAndFillRind(
      gridCoordName, physicalDim, nCoordsArray, gridChildId, rind, self);

    // Rind was parsed (or not) then populate dimensions :
    // Compute structured grid coordinate range
    for (n = 0; n < cellDim; n++)
    {
      srcStart[n] = rind[2 * n] + 1;
      srcEnd[n] = rind[2 * n] + zsize[n];
      memEnd[n] = zsize[n];
      memDims[n] = zsize[n];
    }

    if (voi != nullptr)
    {
      // we are provided a sub-extent to read.
      // First let's assert that the subextent is valid.
      bool valid = true;
      for (n = 0; n < cellDim; ++n)
      {
        valid &= (voi[2 * n] >= 0 && voi[2 * n] <= memEnd[n] && voi[2 * n + 1] >= 0 &&
          voi[2 * n + 1] <= memEnd[n] && voi[2 * n] <= voi[2 * n + 1]);
      }
      if (!valid)
      {
        vtkGenericWarningMacro("Invalid sub-extent specified. Ignoring.");
      }
      else
      {
        // update src and mem pointers.
        for (n = 0; n < cellDim; ++n)
        {
          srcStart[n] += voi[2 * n];
          srcEnd[n] = srcStart[n] + (voi[2 * n + 1] - voi[2 * n]);
          memEnd[n] = (voi[2 * n + 1] - voi[2 * n]) + 1;
          memDims[n] = memEnd[n];
        }
      }
    }

    // Compute number of points
    const vtkIdType nPts = static_cast<vtkIdType>(memEnd[0] * memEnd[1] * memEnd[2]);

    // Populate the extent array
    // int extent[6] = { 0, 0, 0, 0, 0, 0 };
    extent[1] = memEnd[0] - 1;
    extent[3] = memEnd[1] - 1;
    extent[5] = memEnd[2] - 1;

    // wacky hack ...
    // memory aliasing is done
    // since in vtk points array stores XYZ contiguously
    // and they are stored separately in cgns file
    // the memory layout is set so that one cgns file array
    // will be filling every 3 chunks in memory
    memEnd[0] *= 3;

    // Set up points
    points = vtkSmartPointer<vtkPoints>::New();
    //
    // vtkPoints assumes float data type
    //
    if (self->GetDoublePrecisionMesh() != 0)
    {
      points->SetDataTypeToDouble();
    }
    //
    // Resize vtkPoints to fit data
    //
    points->SetNumberOfPoints(nPts);

    //
    // Populate the coordinates.  Put in 3D points with z=0 if the mesh is 2D.
    //
    if (self->GetDoublePrecisionMesh() != 0) // DOUBLE PRECISION MESHPOINTS
    {
      CGNSRead::get_XYZ_mesh<double, float>(self->cgioNum, gridChildId, nCoordsArray, cellDim, nPts,
        srcStart, srcEnd, srcStride, memStart, memEnd, memStride, memDims, points.Get());
    }
    else // SINGLE PRECISION MESHPOINTS
    {
      CGNSRead::get_XYZ_mesh<float, double>(self->cgioNum, gridChildId, nCoordsArray, cellDim, nPts,
        srcStart, srcEnd, srcStride, memStart, memEnd, memStride, memDims, points.Get());
    }
    // Add points to cache
    if (caching)
    {
      self->Internals->MeshPointsCache.Insert(keyMesh, points);
    }
  }

  //----------------------------------------------------------------------------
  // Handle solutions
  //----------------------------------------------------------------------------
  if (self->GetCreateEachSolutionAsBlock())
  {
    // Create separate grid for each solution === debugging mode
    vtkNew<vtkMultiBlockDataSet> mzone;

    unsigned int cc = 0;
    for (std::vector<std::string>::const_iterator sniter = solutionNames.begin();
         sniter != solutionNames.end(); ++sniter, ++cc)
    {
      // read the solution node.
      vtkNew<vtkStructuredGrid> sgrid;
      sgrid->SetExtent(extent);
      sgrid->SetPoints(points.Get());
      if (vtkPrivate::readSolution(*sniter, cellDim, physicalDim, zsize, sgrid.Get(), voi, self) ==
        CG_OK)
      {
        vtkPrivate::AttachReferenceValue(base, sgrid.Get(), self);
        mzone->SetBlock(cc, sgrid.Get());
        mzone->GetMetaData(cc)->Set(vtkCompositeDataSet::NAME(), sniter->c_str());
      }
    }
    if (!solutionNames.empty())
    {
      return mzone.Get();
    }
  }

  // normal case where we great a vtkStructuredGrid for the entire zone.
  vtkNew<vtkStructuredGrid> sgrid;
  sgrid->SetExtent(extent);
  sgrid->SetPoints(points.Get());
  for (const std::string& sniter : solutionNames)
  {
    vtkPrivate::readSolution(sniter, cellDim, physicalDim, zsize, sgrid.Get(), voi, self);
  }

  vtkPrivate::AttachReferenceValue(base, sgrid.Get(), self);
  return sgrid.Get();
}

//------------------------------------------------------------------------------
int vtkCGNSReader::GetCurvilinearZone(
  int base, int zone, int cellDim, int physicalDim, void* v_zsize, vtkMultiBlockDataSet* mbase)
{
  cgsize_t* zsize = reinterpret_cast<cgsize_t*>(v_zsize);
  auto& baseInfo = this->Internals->Internal->GetBase(base);
  auto& zoneInfo = baseInfo.zones[zone];

  vtkSmartPointer<vtkDataObject> zoneDO = CGNSRead::ReadGridForZone(this, baseInfo, zoneInfo)
    ? vtkPrivate::readCurvilinearZone(base, zone, cellDim, physicalDim, zsize, nullptr, this)
    : vtkSmartPointer<vtkDataObject>();
  mbase->SetBlock(zone, zoneDO.Get());

  //----------------------------------------------------------------------------
  // Handle boundary conditions (BC) patches
  //----------------------------------------------------------------------------
  if (!this->CreateEachSolutionAsBlock && CGNSRead::ReadPatchesForBase(this, baseInfo))
  {
    vtkNew<vtkMultiBlockDataSet> newZoneMB;

    vtkSmartPointer<vtkStructuredGrid> zoneGrid = vtkStructuredGrid::SafeDownCast(zoneDO);
    newZoneMB->SetBlock(0u, zoneGrid);
    newZoneMB->GetMetaData(0u)->Set(vtkCompositeDataSet::NAME(), "Internal");
    vtkPrivate::AddIsPatchArray(zoneGrid, false);

    vtkNew<vtkMultiBlockDataSet> patchesMB;
    newZoneMB->SetBlock(1, patchesMB.Get());
    newZoneMB->GetMetaData(1)->Set(vtkCompositeDataSet::NAME(), "Patches");

    std::vector<double> zoneChildren;
    CGNSRead::getNodeChildrenId(this->cgioNum, this->currentId, zoneChildren);
    for (auto iter = zoneChildren.begin(); iter != zoneChildren.end(); ++iter)
    {
      CGNSRead::char_33 nodeLabel;
      cgio_get_label(cgioNum, (*iter), nodeLabel);
      if (strcmp(nodeLabel, "ZoneBC_t") != 0)
      {
        continue;
      }

      const double zoneBCId = (*iter);

      // iterate over all children and read supported BC_t nodes.
      std::vector<double> zoneBCChildren;
      CGNSRead::getNodeChildrenId(this->cgioNum, zoneBCId, zoneBCChildren);
      for (auto bciter = zoneBCChildren.begin(); bciter != zoneBCChildren.end(); ++bciter)
      {
        char label[CGIO_MAX_LABEL_LENGTH + 1];
        cgio_get_label(this->cgioNum, *bciter, label);
        if (strcmp(label, "BC_t") == 0)
        {
          try
          {
            BCInformation binfo(this->cgioNum, *bciter);
            if (CGNSRead::ReadPatch(this, baseInfo, zoneInfo, binfo.FamilyName))
            {
              const unsigned int idx = patchesMB->GetNumberOfBlocks();
              vtkSmartPointer<vtkDataSet> ds = zoneGrid
                ? binfo.CreateDataSet(cellDim, zoneGrid)
                : vtkPrivate::readBCDataSet(binfo, base, zone, cellDim, physicalDim, zsize, this);
              vtkPrivate::AddIsPatchArray(ds, true);
              vtkCGNSReader::vtkPrivate::readBCData(
                *bciter, cellDim, physicalDim, binfo.Location, ds.Get(), this);
              patchesMB->SetBlock(idx, ds);

              if (!binfo.FamilyName.empty())
              {
                vtkInformationStringKey* bcfamily = vtkCGNSReader::FAMILY();
                patchesMB->GetMetaData(idx)->Set(bcfamily, binfo.FamilyName.c_str());
              }
              patchesMB->GetMetaData(idx)->Set(vtkCompositeDataSet::NAME(), binfo.Name);
            }
          }
          catch (const CGIOUnsupported& ue)
          {
            vtkWarningMacro("Skipping BC_t node: " << ue.what());
          }
          catch (const CGIOError& e)
          {
            vtkErrorMacro("Failed to read BC_t node: " << e.what());
          }
        }
      }
    }
    CGNSRead::releaseIds(this->cgioNum, zoneChildren);
    zoneChildren.clear();

    if (newZoneMB->GetNumberOfBlocks() > 1)
    {
      mbase->SetBlock(zone, newZoneMB.Get());
    }
  }
  return 0;
}

//------------------------------------------------------------------------------
int vtkCGNSReader::GetUnstructuredZone(
  int base, int zone, int cellDim, int physicalDim, void* v_zsize, vtkMultiBlockDataSet* mbase)
{
  cgsize_t* zsize = reinterpret_cast<cgsize_t*>(v_zsize);

  ////=========================================================================
  const bool warningIdTypeSize = sizeof(cgsize_t) > sizeof(vtkIdType);
  if (warningIdTypeSize == true)
  {
    vtkWarningMacro(<< "Warning cgsize_t is larger than the size as vtkIdType\n"
                    << "  sizeof vtkIdType = " << sizeof(vtkIdType) << "\n"
                    << "  sizeof cgsize_t = " << sizeof(cgsize_t) << "\n"
                    << "This may cause unexpected issues. If so, please recompile with "
                    << "VTK_USE_64BIT_IDS=ON.");
  }
////========================================================================
#if !defined(VTK_LEGACY_REMOVE)
  if (this->LoadMesh == false)
  {
    vtkWarningMacro(<< "Ability to not load mesh is currently only supported"
                    << "for curvilinear grids and will be ignored.");
  }
#endif
  ////========================================================================

  int rind[6];
  // source layout
  cgsize_t srcStart[3] = { 1, 1, 1 };
  cgsize_t srcStride[3] = { 1, 1, 1 };
  cgsize_t srcEnd[3];

  // memory destination layout
  cgsize_t memStart[3] = { 1, 1, 1 };
  cgsize_t memStride[3] = { 3, 1, 1 };
  cgsize_t memEnd[3] = { 1, 1, 1 };
  cgsize_t memDims[3] = { 1, 1, 1 };

  vtkIdType nPts = 0;

  // Get Coordinates and FlowSolution node names
  std::string gridCoordName;
  std::vector<std::string> solutionNames;
  std::string keyMesh;

  std::vector<double> gridChildId;
  std::size_t nCoordsArray = 0;

  vtkPrivate::getGridAndSolutionNames(base, gridCoordName, solutionNames, this);
  if (gridCoordName == "Null")
  {
    mbase->SetBlock(zone, vtkSmartPointer<vtkDataObject>());
    return 0;
  }

  vtkPrivate::getCoordsIdAndFillRind(
    gridCoordName, physicalDim, nCoordsArray, gridChildId, rind, this);

  // Rind was parsed or not then populate dimensions :
  // get grid coordinate range
  srcStart[0] = rind[0] + 1;
  srcEnd[0] = rind[0] + zsize[0];
  memEnd[0] = zsize[0];
  memDims[0] = zsize[0];

  // Compute number of points
  if (!IsIdTypeBigEnough(zsize[0]))
  {
    // overflow! cannot open the file in current configuration.
    vtkErrorMacro("vtkIdType overflow. Please compile with VTK_USE_64BIT_IDS:BOOL=ON.");
    return 1;
  }

  nPts = static_cast<vtkIdType>(zsize[0]);
  assert(nPts == zsize[0]);

  vtkSmartPointer<vtkPoints> points;

  // If it is not a deforming mesh, gridCoordName keep the standard name
  // Only Volume mesh points, not subset are cached
  bool caching = (gridCoordName == "GridCoordinates" && this->CacheMesh);
  if (caching)
  {
    // Try to get from cache
    const char* basename = this->Internals->Internal->GetBase(base).name;
    const char* zonename = this->Internals->Internal->GetBase(base).zones[zone].name;
    // build a key /basename/zonename
    keyMesh = vtkPrivate::GenerateMeshKey(basename, zonename);

    points = this->Internals->MeshPointsCache.Find(keyMesh);
    if (points.Get() != nullptr)
    {
      // check storage data type
      if ((this->GetDoublePrecisionMesh() != 0) != (points->GetDataType() == VTK_DOUBLE))
      {
        points = nullptr;
      }
    }
  }

  // Reading points from file instead of cache
  if (points.Get() == nullptr)
  {
    // Set up points
    points = vtkSmartPointer<vtkPoints>::New();

    //
    // wacky hack ...
    memEnd[0] *= 3; // for memory aliasing
    //
    // vtkPoints assumes float data type
    //
    if (this->DoublePrecisionMesh != 0)
    {
      points->SetDataTypeToDouble();
    }
    //
    // Resize vtkPoints to fit data
    //
    points->SetNumberOfPoints(nPts);

    //
    // Populate the coordinates. Put in 3D points with z=0 if the mesh is 2D.
    //
    if (this->DoublePrecisionMesh != 0) // DOUBLE PRECISION MESHPOINTS
    {
      CGNSRead::get_XYZ_mesh<double, float>(this->cgioNum, gridChildId, nCoordsArray, cellDim, nPts,
        srcStart, srcEnd, srcStride, memStart, memEnd, memStride, memDims, points.Get());
    }
    else // SINGLE PRECISION MESHPOINTS
    {
      CGNSRead::get_XYZ_mesh<float, double>(this->cgioNum, gridChildId, nCoordsArray, cellDim, nPts,
        srcStart, srcEnd, srcStride, memStart, memEnd, memStride, memDims, points.Get());
    }
    // Add points to cache
    if (caching)
    {
      this->Internals->MeshPointsCache.Insert(keyMesh, points);
    }
  }

  this->UpdateProgress(0.2);
  // points are now loaded
  //----------------------
  // Read List of zone children ids
  // and Get connectivities and solutions
  std::vector<double> zoneChildId;
  CGNSRead::getNodeChildrenId(this->cgioNum, this->currentId, zoneChildId);
  //
  std::vector<double> elemIdList;

  for (std::size_t nn = 0; nn < zoneChildId.size(); nn++)
  {
    char nodeLabel[CGIO_MAX_NAME_LENGTH + 1];
    cgio_get_label(cgioNum, zoneChildId[nn], nodeLabel);
    if (strcmp(nodeLabel, "Elements_t") == 0)
    {
      elemIdList.push_back(zoneChildId[nn]);
    }
    else
    {
      cgio_release_id(this->cgioNum, zoneChildId[nn]);
    }
  }

  //---------------------------------------------------------------------
  //  Handle connectivities
  //---------------------------------------------------------------------
  // Read the number of sections, for the zone.
  int nsections = 0;
  nsections = static_cast<int>(elemIdList.size());
  std::string keyConnect; // key to store connectivity.

  std::vector<SectionInformation> sectionInfoList(nsections);

  // Find section layout
  // Section is composed of => 1 Volume + bnd surfaces
  //                        => multi-Volume + Bnd surfaces
  // bnd surfaces are pointed to by bc_t nodes !
  // determine dim to allocate for connectivity reading
  cgsize_t elementCoreSize = 0;
  vtkIdType numCoreCells = 0;
  //
  std::vector<int> coreSec;
  std::vector<int> bndSec;
  std::vector<int> sizeSec;
  std::vector<int> startSec;
  //
  numCoreCells = 0; // force initialize
  for (int sec = 0; sec < nsections; ++sec)
  {

    CGNS_ENUMT(ElementType_t) elemType = CGNS_ENUMV(ElementTypeNull);
    cgsize_t elementSize = 0;

    //
    sectionInfoList[sec].elemType = CGNS_ENUMV(ElementTypeNull);
    sectionInfoList[sec].range[0] = 1;
    sectionInfoList[sec].range[1] = 1;
    sectionInfoList[sec].bound = 0;
    sectionInfoList[sec].eDataSize = 0;

    //
    CGNSRead::char_33 dataType;
    std::vector<vtkTypeInt32> mdata;

    if (cgio_get_name(this->cgioNum, elemIdList[sec], sectionInfoList[sec].name) != CG_OK)
    {
      vtkErrorMacro(<< "Error while getting section node name\n");
    }
    if (cgio_get_data_type(cgioNum, elemIdList[sec], dataType) != CG_OK)
    {
      vtkErrorMacro(<< "Error in cgio_get_data_type for section node\n");
    }
    if (strcmp(dataType, "I4") != 0)
    {
      vtkErrorMacro(<< "Unexpected data type for dimension data of Element\n");
    }

    CGNSRead::readNodeData<vtkTypeInt32>(cgioNum, elemIdList[sec], mdata);
    if (mdata.size() != 2)
    {
      vtkErrorMacro(<< "Unexpected data for Elements_t node\n");
    }
    sectionInfoList[sec].elemType = static_cast<CGNS_ENUMT(ElementType_t)>(mdata[0]);
    sectionInfoList[sec].bound = mdata[1];

    // ElementRange
    double elemRangeId;
    double elemConnectId;
    cgio_get_node_id(this->cgioNum, elemIdList[sec], "ElementRange", &elemRangeId);
    // read node data type
    if (cgio_get_data_type(this->cgioNum, elemRangeId, dataType) != CG_OK)
    {
      std::cerr << "Error in cgio_get_data_type for ElementRange" << std::endl;
      continue;
    }

    if (strcmp(dataType, "I4") == 0)
    {
      std::vector<vtkTypeInt32> mdata2;
      CGNSRead::readNodeData<vtkTypeInt32>(this->cgioNum, elemRangeId, mdata2);
      if (mdata2.size() != 2)
      {
        vtkErrorMacro(<< "Unexpected data for ElementRange node\n");
      }
      sectionInfoList[sec].range[0] = static_cast<cgsize_t>(mdata2[0]);
      sectionInfoList[sec].range[1] = static_cast<cgsize_t>(mdata2[1]);
    }
    else if (strcmp(dataType, "I8") == 0)
    {
      std::vector<vtkTypeInt64> mdata2;
      CGNSRead::readNodeData<vtkTypeInt64>(this->cgioNum, elemRangeId, mdata2);
      if (mdata2.size() != 2)
      {
        vtkErrorMacro(<< "Unexpected data for ElementRange node\n");
      }
      sectionInfoList[sec].range[0] = static_cast<cgsize_t>(mdata2[0]);
      sectionInfoList[sec].range[1] = static_cast<cgsize_t>(mdata2[1]);
    }
    else
    {
      std::cerr << "Unexpected data type for dimension data of Element" << std::endl;
      continue;
    }

    elementSize =
      sectionInfoList[sec].range[1] - sectionInfoList[sec].range[0] + 1; // Interior Volume + Bnd
    elemType = sectionInfoList[sec].elemType;

    cgio_get_node_id(this->cgioNum, elemIdList[sec], "ElementConnectivity", &elemConnectId);
    cgsize_t dimVals[12];
    int ndim;
    if (cgio_get_dimensions(cgioNum, elemConnectId, &ndim, dimVals) != CG_OK)
    {
      cgio_error_exit("cgio_get_dimensions");
      vtkErrorMacro(<< "Could not determine ElementDataSize\n");
      continue;
    }
    if (ndim != 1)
    {
      vtkErrorMacro(<< "ElementConnectivity wrong dimension\n");
      continue;
    }
    sectionInfoList[sec].eDataSize = dimVals[0];

    // Skip if it is a boundary
    if (sectionInfoList[sec].range[0] > zsize[1])
    {
      vtkDebugMacro(<< "@@ Boundary Section not accounted\n");
      bndSec.push_back(sec);
      continue;
    }

    cgsize_t eDataSize = 0;
    eDataSize = dimVals[0];
    if (elemType != CGNS_ENUMV(MIXED))
    {
      eDataSize += elementSize;
    }
    //
    sizeSec.push_back(eDataSize);
    startSec.push_back(sectionInfoList[sec].range[0] - 1);
    elementCoreSize += (eDataSize);

    if (!IsIdTypeBigEnough(elementSize + numCoreCells))
    {
      vtkErrorMacro("vtkIdType overflow. Please compile with VTK_USE_64BIT_IDS:BOOL=ON.");
      return 1;
    }
    numCoreCells += elementSize;
    coreSec.push_back(sec);
    //
  }
  //
  // Detect type of zone elements definition
  // By Elements --> quad, tri ... mixed
  // Or by Face Connectivity --> NGON_n, NFACE_n
  //
  std::vector<int> ngonSec;
  std::vector<int> nfaceSec;
  bool hasNFace = false;
  bool hasNGon = false;
  bool hasElemDefinition = false;
  for (int sec = 0; sec < nsections; ++sec)
  {
    if (sectionInfoList[sec].elemType == CGNS_ENUMV(NFACE_n))
    {
      hasNFace = true;
      nfaceSec.push_back(sec);
    }
    else if (sectionInfoList[sec].elemType == CGNS_ENUMV(NGON_n))
    {
      hasNGon = true;
      ngonSec.push_back(sec);
    }
    else
    {
      hasElemDefinition = true;
    }
  }
  if (hasNFace && !hasNGon)
  {
    vtkErrorMacro("NFace_n requires NGon_n definition");
    return 1;
  }
  if (hasElemDefinition && hasNGon)
  {
    vtkErrorMacro("Mixed definition of unstructured zone by elements and by faces is not valid.");
    return 1;
  }

  // Set up ugrid - we need to refer to it if we're building an NFACE_n or NGON_n grid
  // Create an unstructured grid to contain the points.
  vtkSmartPointer<vtkUnstructuredGrid> ugrid;

  caching = this->CacheConnectivity;
  if (caching)
  {
    // Try to get the Grid Connectivity from cache
    // else create new grid
    const char* basename = this->Internals->Internal->GetBase(base).name;
    const char* zonename = this->Internals->Internal->GetBase(base).zones[zone].name;
    // build a key /basename/zonename
    std::ostringstream query;
    query << "/" << basename << "/" << zonename << "/core";
    keyConnect = query.str();

    ugrid = this->Internals->ConnectivitiesCache.Find(keyConnect);
    if (ugrid.Get() != nullptr)
    {
      if ((ugrid->GetNumberOfCells() != numCoreCells && !hasNGon) ||
        (ugrid->GetNumberOfCells() != zsize[1] && hasNGon))
      {
        vtkWarningMacro(<< "Connectivities from the cache have"
                           " a different number of cells from"
                           " those being read, ditching the cache");
        ugrid = nullptr;
      }
      else
      {
        ugrid->SetPoints(points.Get());
      }
    }
  }
  if (ugrid.Get() == nullptr)
  {
    ugrid = vtkSmartPointer<vtkUnstructuredGrid>::New();
    ugrid->SetPoints(points.Get());
    //
    if (hasNGon)
    {
      // READ NGON CONNECTIVITY
      //
      // Define start of Ngon Connectivity Array for each section
      std::vector<vtkIdType> startArraySec(ngonSec.size());
      std::vector<vtkIdType> startRangeSec(ngonSec.size());
      std::size_t faceElementsSize = 0;
      vtkIdType numFaces(0);
      for (std::size_t sec = 0; sec < ngonSec.size(); sec++)
      {
        int curSec = ngonSec[sec];
        int curStart = sectionInfoList[curSec].range[0] - 1;
        numFaces += 1 + sectionInfoList[curSec].range[1] - sectionInfoList[curSec].range[0];
        vtkIdType curArrayStart = 0;
        vtkIdType curRangeStart = 0;
        for (std::size_t lse = 0; lse < ngonSec.size(); lse++)
        {
          int lseSec = ngonSec[lse];
          if (sectionInfoList[lseSec].range[0] - 1 < curStart)
          {
            curArrayStart += sectionInfoList[lseSec].eDataSize;
            curRangeStart +=
              sectionInfoList[lseSec].range[1] - sectionInfoList[lseSec].range[0] + 1;
          }
        }
        startArraySec[sec] = curArrayStart;
        startRangeSec[sec] = curRangeStart;
        faceElementsSize += sectionInfoList[curSec].eDataSize;
      }

      std::vector<vtkIdType> faceElementsIdx;
      std::vector<vtkIdType> faceElementsArr;
      bool old_polygonal_layout = false;

      faceElementsArr.resize(faceElementsSize);
      faceElementsIdx.resize(numFaces + 1);
      faceElementsIdx[0] = 0;
      // Now load the faces that are in NGON_n format.
      for (std::size_t sec = 0; sec < ngonSec.size(); sec++)
      {
        cgsize_t fDataSize(0);
        cgsize_t offsetDataSize(0);

        std::size_t osec = ngonSec[sec];
        fDataSize = sectionInfoList[osec].eDataSize;
        vtkIdType* localFaceElementsArr = &(faceElementsArr[startArraySec[sec]]);
        vtkIdType* localFaceElementsIdx = &(faceElementsIdx[startRangeSec[sec]]);
        offsetDataSize = sectionInfoList[osec].range[1] - sectionInfoList[osec].range[0] + 2;

        cgsize_t memDim[2];

        srcStart[0] = 1;
        srcEnd[0] = offsetDataSize;
        srcStride[0] = 1;

        memStart[0] = 1;
        memStart[1] = 1;
        memEnd[0] = offsetDataSize;
        memEnd[1] = 1;
        memStride[0] = 1;
        memStride[1] = 1;
        memDim[0] = offsetDataSize;
        memDim[1] = 1;

        if (0 !=
          CGNSRead::get_section_start_offset(this->cgioNum, elemIdList[osec], 1, srcStart, srcEnd,
            srcStride, memStart, memEnd, memStride, memDim, localFaceElementsIdx))
        {
          // NOTE: the old polygonal layout was replaced in CGNS version 4.0
          // NOTE: support for the old NFACE_n/NGON_n array layout may be deprecated in
          // NOTE: a future version of ParaView.
          old_polygonal_layout = true;
        }

        if (startArraySec[sec] != 0)
        {
          // Add offset since it is not the first section
          for (vtkIdType idx = 0; idx < offsetDataSize; idx++)
          {
            localFaceElementsIdx[idx] += startArraySec[sec];
          }
        }

        srcStart[0] = 1;
        srcEnd[0] = fDataSize;
        srcStride[0] = 1;

        memStart[0] = 1;
        memStart[1] = 1;
        memEnd[0] = fDataSize;
        memEnd[1] = 1;
        memStride[0] = 1;
        memStride[1] = 1;
        memDim[0] = fDataSize;
        memDim[1] = 1;

        if (0 !=
          CGNSRead::get_section_connectivity(this->cgioNum, elemIdList[osec], 1, srcStart, srcEnd,
            srcStride, memStart, memEnd, memStride, memDim, localFaceElementsArr))
        {
          vtkErrorMacro("FAILED to read NGON_n cells\n");
          return 1;
        }
      }
      // Loading Done
      //
      if (old_polygonal_layout)
      {
        // Regenerate a faceElementIdx lookupTable
        vtkIdType curFace = 0;
        vtkIdType curNodeInFace = 0;

        faceElementsIdx[0] = 0;

        for (vtkIdType idxFace = 0; idxFace < static_cast<vtkIdType>(faceElementsIdx.size() - 1);
             ++idxFace)
        {
          vtkIdType nVertexOnCurFace = faceElementsArr[curFace];

          faceElementsIdx[idxFace + 1] = faceElementsIdx[idxFace] + nVertexOnCurFace;

          for (vtkIdType idxVertex = 0; idxVertex < nVertexOnCurFace; idxVertex++)
          {
            faceElementsArr[curNodeInFace] = faceElementsArr[curFace + idxVertex + 1];
            curNodeInFace++;
          }
          curFace += nVertexOnCurFace + 1;
        }
      }
      // Now take care of NFACE_n properly
      //
      // In case of unordered section :
      std::vector<vtkIdType> startNFaceArraySec(nfaceSec.size());
      std::vector<vtkIdType> startNFaceRangeSec(nfaceSec.size());
      std::size_t cellElementsSize = 0;
      vtkIdType numCells(0);
      for (std::size_t sec = 0; sec < nfaceSec.size(); sec++)
      {
        int curSec = nfaceSec[sec];
        int curStart = sectionInfoList[curSec].range[0] - 1;
        numCells += 1 + sectionInfoList[curSec].range[1] - sectionInfoList[curSec].range[0];
        vtkIdType curNFaceArrayStart = 0;
        vtkIdType curRangeStart = 0;
        for (std::size_t lse = 0; lse < nfaceSec.size(); lse++)
        {
          int lseSec = nfaceSec[lse];
          if (sectionInfoList[lseSec].range[0] - 1 < curStart)
          {
            curNFaceArrayStart += sectionInfoList[lseSec].eDataSize;
            curRangeStart +=
              sectionInfoList[lseSec].range[1] - sectionInfoList[lseSec].range[0] + 1;
          }
        }
        startNFaceArraySec[sec] = curNFaceArrayStart;
        startNFaceRangeSec[sec] = curRangeStart;
        cellElementsSize += sectionInfoList[curSec].eDataSize;
      }
      //
      std::vector<vtkIdType> cellElementsArr;
      std::vector<vtkIdType> cellElementsIdx;
      cellElementsArr.resize(cellElementsSize);
      cellElementsIdx.resize(numCells + 1);

      if (hasNFace && numCells < zsize[1])
      {
        vtkErrorMacro("number of NFACE_n cells is not coherent with Zone_t declaration \n");
        return 1;
      }
      // Load NFace_n connectivities
      for (std::size_t sec = 0; sec < nfaceSec.size(); sec++)
      {
        cgsize_t eDataSize(0);
        cgsize_t offsetDataSize(0);
        std::size_t osec = nfaceSec[sec];
        double cgioSectionId;
        cgioSectionId = elemIdList[osec];
        eDataSize = sectionInfoList[osec].eDataSize;
        offsetDataSize = sectionInfoList[osec].range[1] - sectionInfoList[osec].range[0] + 2;
        vtkIdType* localCellElementsArr = &(cellElementsArr[startNFaceArraySec[sec]]);
        vtkIdType* localCellElementsIdx = &(cellElementsIdx[startNFaceRangeSec[sec]]);

        cgsize_t memDim[2];

        srcStart[0] = 1;
        srcEnd[0] = offsetDataSize;
        srcStride[0] = 1;

        memStart[0] = 1;
        memStart[1] = 1;
        memEnd[0] = offsetDataSize;
        memEnd[1] = 1;
        memStride[0] = 1;
        memStride[1] = 1;
        memDim[0] = offsetDataSize;
        memDim[1] = 1;

        if (0 !=
          CGNSRead::get_section_start_offset(this->cgioNum, cgioSectionId, 1, srcStart, srcEnd,
            srcStride, memStart, memEnd, memStride, memDim, localCellElementsIdx))
        {
          // NOTE: the old polygonal layout was replaced in CGNS version 4.0
          // NOTE: support for the old NFACE_n/NGON_n array layout may be deprecated in
          // NOTE: a future version of ParaView.
          old_polygonal_layout = true;
        }
        if (startNFaceArraySec[sec] != 0)
        {
          // Add offset since it is not the first section
          for (vtkIdType idx = 0; idx < offsetDataSize; idx++)
          {
            localCellElementsIdx[idx] += startNFaceArraySec[sec];
          }
        }

        srcStart[0] = 1;
        srcEnd[0] = eDataSize;
        srcStride[0] = 1;

        memStart[0] = 1;
        memStart[1] = 1;
        memEnd[0] = eDataSize;
        memEnd[1] = 1;
        memStride[0] = 1;
        memStride[1] = 1;
        memDim[0] = eDataSize;
        memDim[1] = 1;

        if (0 !=
          CGNSRead::get_section_connectivity(this->cgioNum, cgioSectionId, 1, srcStart, srcEnd,
            srcStride, memStart, memEnd, memStride, memDim, localCellElementsArr))
        {
          vtkErrorMacro("FAILED to read NFACE_n cells\n");
          return 1;
        }
        cgio_release_id(this->cgioNum, cgioSectionId);
      }

      // ok, now we have the face-to-node connectivity array and the cell-to-face connectivity
      // array.
      // VTK, however, has no concept of faces, and uses cell-to-node connectivity, so the
      // intermediate faces
      // need to be taken out of the description.
      // Basic CGNS 3.4 support

      if (old_polygonal_layout)
      {
        // Regenerate cellElementIdx lookupTable
        vtkIdType curCell = 0;
        vtkIdType curFaceInCell = 0;

        cellElementsIdx[0] = 0;

        for (vtkIdType idxCell = 0; idxCell < static_cast<vtkIdType>(cellElementsIdx.size() - 1);
             ++idxCell)
        {
          vtkIdType nFaceInCell = cellElementsArr[curCell];

          cellElementsIdx[idxCell + 1] = cellElementsIdx[idxCell] + nFaceInCell;

          for (vtkIdType idxFace = 0; idxFace < nFaceInCell; idxFace++)
          {
            cellElementsArr[curFaceInCell] = cellElementsArr[curCell + idxFace + 1];
            curFaceInCell++;
          }
          curCell += nFaceInCell + 1;
        }
      }

      for (vtkIdType nc = 0; nc < numCells; nc++)
      {
        int numCellFaces = cellElementsIdx[nc + 1] - cellElementsIdx[nc];
        vtkNew<vtkIdList> faces;
        faces->InsertNextId(numCellFaces);
        for (vtkIdType nf = 0; nf < numCellFaces; ++nf)
        {
          vtkIdType faceId = cellElementsArr[cellElementsIdx[nc] + nf];
          bool mustReverse = faceId > 0;
          faceId = std::abs(faceId);

          // the following is needed because when the NGON_n face data do not precedes the
          // NFACE_n cell data, the indices are continuous, so a "global-to-local" mapping must be
          // done.
          for (std::size_t sec = 0; sec < ngonSec.size(); sec++)
          {
            int curSec = ngonSec[sec];
            //
            if (faceId <= sectionInfoList[curSec].range[1] &&
              faceId >= sectionInfoList[curSec].range[0])
            {
              faceId = faceId - sectionInfoList[curSec].range[0] + 1 + startRangeSec[sec];
              break;
            }
          }
          faceId -= 1; // CGNS uses FORTRAN ID style, starting at 1

          vtkIdType startNode = faceElementsIdx[faceId];
          vtkIdType endNode = faceElementsIdx[faceId + 1];
          vtkIdType numNodes = endNode - startNode;
          faces->InsertNextId(numNodes);
          /* Each face is composed of multiple vertex */
          if (mustReverse)
          {
            for (vtkIdType nn = numNodes - 1; nn >= 0; --nn)
            {
              vtkIdType nodeID =
                faceElementsArr[startNode + nn] - 1; // AGAIN subtract 1 from node ID

              faces->InsertNextId(nodeID);
            }
          }
          else
          {
            for (vtkIdType nn = 0; nn < numNodes; ++nn)
            {
              vtkIdType nodeID =
                faceElementsArr[startNode + nn] - 1; // AGAIN subtract 1 from node ID
              faces->InsertNextId(nodeID);
            }
          }
        }
        ugrid->InsertNextCell(VTK_POLYHEDRON, faces.GetPointer());
      }

      // If NGon_n but no NFace_n load POLYGONS
      if (!hasNFace)
      {

        for (vtkIdType nf = 0; nf < numFaces; ++nf)
        {

          vtkIdType startNode = faceElementsIdx[nf];
          vtkIdType endNode = faceElementsIdx[nf + 1];
          vtkIdType numNodes = endNode - startNode;
          vtkNew<vtkIdList> nodes;
          // nodes->InsertNextId(numNodes);
          for (vtkIdType nn = 0; nn < numNodes; ++nn)
          {
            vtkIdType nodeID = faceElementsArr[startNode + nn] - 1;
            nodes->InsertNextId(nodeID);
          }
          ugrid->InsertNextCell(VTK_POLYGON, nodes.GetPointer());
        }
      }
    }
    else
    {
      // READ ELEMENT CONNECTIVITY
      //
      std::vector<vtkIdType> startArraySec(coreSec.size());
      for (std::size_t sec = 0; sec < coreSec.size(); sec++)
      {
        int curStart = startSec[sec];
        vtkIdType curArrayStart = 0;
        for (std::size_t lse = 0; lse < coreSec.size(); lse++)
        {
          if (startSec[lse] < curStart)
          {
            curArrayStart += sizeSec[lse];
          }
        }
        startArraySec[sec] = curArrayStart;
      }

      // Create Cell Array
      vtkNew<vtkCellArray> cells;
      // Modification for memory reliability
      vtkNew<vtkIdTypeArray> cellLocations;
      cellLocations->SetNumberOfValues(elementCoreSize);
      vtkIdType* elements = cellLocations->GetPointer(0);

      if (elements == nullptr)
      {
        vtkErrorMacro(<< "Could not allocate memory for connectivity\n");
        return 1;
      }

      int* cellsTypes = new int[numCoreCells];
      if (cellsTypes == nullptr)
      {
        vtkErrorMacro(<< "Could not allocate memory for connectivity\n");
        return 1;
      }

      // Iterate over core sections.
      for (std::vector<int>::iterator iter = coreSec.begin(); iter != coreSec.end(); ++iter)
      {
        size_t sec = *iter;
        CGNS_ENUMT(ElementType_t) elemType = CGNS_ENUMV(ElementTypeNull);
        cgsize_t start = 1, end = 1;
        cgsize_t elementSize = 0;

        start = sectionInfoList[sec].range[0];
        end = sectionInfoList[sec].range[1];
        elemType = sectionInfoList[sec].elemType;

        elementSize = end - start + 1; // Interior Volume + Bnd

        double cgioSectionId;
        cgioSectionId = elemIdList[sec];

        if (elemType != CGNS_ENUMV(MIXED))
        {
          // All cells are of the same type.
          int numPointsPerCell = 0;
          int cellType;
          bool higherOrderWarning;
          bool reOrderElements;
          //
          if (cg_npe(elemType, &numPointsPerCell) || numPointsPerCell == 0)
          {
            vtkErrorMacro(<< "Invalid numPointsPerCell\n");
          }

          cellType = CGNSRead::GetVTKElemType(elemType, higherOrderWarning, reOrderElements);
          //
          for (vtkIdType i = start - 1; i < end; i++)
          {
            cellsTypes[i] = cellType;
          }
          //
          cgsize_t eDataSize = 0;
          cgsize_t EltsEnd = elementSize + start - 1;
          eDataSize = sectionInfoList[sec].eDataSize;
          vtkDebugMacro(<< "Element data size for sec " << sec << " is: " << eDataSize << "\n");

          if (eDataSize != numPointsPerCell * elementSize)
          {
            vtkErrorMacro(<< "FATAL wrong elements dimensions\n");
          }

          // pointer on start !!
          vtkIdType* localElements = &(elements[startArraySec[sec]]);

          cgsize_t memDim[2];
          cgsize_t npe = numPointsPerCell;
          // How to handle per process reading for unstructured mesh
          // + npe* ( wantedstartperprocess-start ) ; startoffset
          srcStart[0] = 1;
          srcStart[1] = 1;

          srcEnd[0] = (EltsEnd - start + 1) * npe;
          srcEnd[1] = 1;
          srcStride[0] = 1;
          srcStride[1] = 1;

          memStart[0] = 2;
          memStart[1] = 1;
          memEnd[0] = npe + 1;
          memEnd[1] = EltsEnd - start + 1;
          memStride[0] = 1;
          memStride[1] = 1;
          memDim[0] = npe + 1;
          memDim[1] = EltsEnd - start + 1;

          memset(localElements, 1, sizeof(vtkIdType) * (npe + 1) * (EltsEnd - start + 1));

          CGNSRead::get_section_connectivity(this->cgioNum, cgioSectionId, 2, srcStart, srcEnd,
            srcStride, memStart, memEnd, memStride, memDim, localElements);

          // Add numptspercell and do -1 on indexes
          for (vtkIdType icell = 0; icell < elementSize; ++icell)
          {
            vtkIdType pos = icell * (numPointsPerCell + 1);
            localElements[pos] = static_cast<vtkIdType>(numPointsPerCell);
            for (vtkIdType ip = 0; ip < numPointsPerCell; ++ip)
            {
              pos++;
              localElements[pos] = localElements[pos] - 1;
            }
          }
          if (reOrderElements == true)
          {
            CGNSRead::CGNS2VTKorderMonoElem(elementSize, cellType, localElements);
          }
        }
        else if (elemType == CGNS_ENUMV(MIXED))
        {
          //
          int numPointsPerCell = 0;
          int cellType;
          bool higherOrderWarning;
          bool reOrderElements;
          // pointer on start !!
          vtkIdType* localElements = &(elements[startArraySec[sec]]);

          cgsize_t eDataSize = 0;
          eDataSize = sectionInfoList[sec].eDataSize;

          cgsize_t memDim[2];

          srcStart[0] = 1;
          srcEnd[0] = eDataSize;
          srcStride[0] = 1;

          memStart[0] = 1;
          memStart[1] = 1;
          memEnd[0] = eDataSize;
          memEnd[1] = 1;
          memStride[0] = 1;
          memStride[1] = 1;
          memDim[0] = eDataSize;
          memDim[1] = 1;

          CGNSRead::get_section_connectivity(this->cgioNum, cgioSectionId, 1, srcStart, srcEnd,
            srcStride, memStart, memEnd, memStride, memDim, localElements);

          vtkIdType pos = 0;
          reOrderElements = false;
          for (vtkIdType icell = 0, i = start - 1; icell < elementSize; ++icell, ++i)
          {
            bool orderFlag;
            elemType = static_cast<CGNS_ENUMT(ElementType_t)>(localElements[pos]);
            cg_npe(elemType, &numPointsPerCell);
            cellType = CGNSRead::GetVTKElemType(elemType, higherOrderWarning, orderFlag);
            reOrderElements = reOrderElements | orderFlag;
            cellsTypes[i] = cellType;
            localElements[pos] = static_cast<vtkIdType>(numPointsPerCell);
            pos++;
            for (vtkIdType ip = 0; ip < numPointsPerCell; ip++)
            {
              localElements[ip + pos] = localElements[ip + pos] - 1;
            }
            pos += numPointsPerCell;
          }

          if (reOrderElements == true)
          {
            CGNSRead::CGNS2VTKorder(elementSize, &cellsTypes[start - 1], localElements);
          }
        }
        else
        {
          vtkErrorMacro(<< "Unsupported element Type\n");
          return 1;
        }

        cgio_release_id(this->cgioNum, cgioSectionId);
      }

      cells->SetCells(numCoreCells, cellLocations.GetPointer());

      ugrid->SetCells(cellsTypes, cells.GetPointer());

      delete[] cellsTypes;
    }
    if (caching)
    {
      this->Internals->ConnectivitiesCache.Insert(keyConnect, ugrid);
    }
  }
  //
  auto& baseInfo = this->Internals->Internal->GetBase(base);
  auto& zoneInfo = baseInfo.zones[zone];
  const bool requiredPatch = CGNSRead::ReadPatchesForBase(this, baseInfo);

  // SetUp zone Blocks
  vtkMultiBlockDataSet* mzone = vtkMultiBlockDataSet::New();
  if (!bndSec.empty() && requiredPatch)
  {
    mzone->SetNumberOfBlocks(2);
  }
  else
  {
    mzone->SetNumberOfBlocks(1);
  }
  mzone->GetMetaData((unsigned int)0)->Set(vtkCompositeDataSet::NAME(), "Internal");

  //----------------------------------------------------------------------------
  // Handle solutions
  //----------------------------------------------------------------------------
  for (const std::string& sniter : solutionNames)
  {
    // cellDim=1 is based on the code that was previously here. With cellDim=1, I was
    // able to share the code between Curlinear and Unstructured grids for reading
    // solutions.
    vtkPrivate::readSolution(
      sniter, /*cellDim=*/1, physicalDim, zsize, ugrid.Get(), /*voi=*/nullptr, this);
  }

  // Handle Reference Values (Mach Number, ...)
  vtkPrivate::AttachReferenceValue(base, ugrid.Get(), this);

  //--------------------------------------------------
  // Read patch boundary Sections
  //--------------------------------------------------
  // Iterate over bnd sections.
  vtkPrivate::AddIsPatchArray(ugrid.Get(), false);

  if (hasNFace && requiredPatch)
  {
    //----------------------------------------------------------------------------
    // Handle boundary conditions (BC) patches for polyhedral grid
    //----------------------------------------------------------------------------
    mzone->SetBlock(0u, ugrid.Get());
    mzone->GetMetaData(0u)->Set(vtkCompositeDataSet::NAME(), "Internal");
    //
    vtkNew<vtkMultiBlockDataSet> patchesMB;
    mzone->SetBlock(1, patchesMB.Get());
    mzone->GetMetaData(1)->Set(vtkCompositeDataSet::NAME(), "Patches");
    // multi patch build
    //
    std::vector<double> zoneChildren;
    CGNSRead::getNodeChildrenId(this->cgioNum, this->currentId, zoneChildren);
    for (auto iter = zoneChildren.begin(); iter != zoneChildren.end(); ++iter)
    {
      CGNSRead::char_33 nodeLabel;
      cgio_get_label(cgioNum, (*iter), nodeLabel);
      if (strcmp(nodeLabel, "ZoneBC_t") != 0)
      {
        continue;
      }

      const double zoneBCId = (*iter);

      // iterate over all children and read supported BC_t nodes.
      std::vector<double> zoneBCChildren;
      CGNSRead::getNodeChildrenId(this->cgioNum, zoneBCId, zoneBCChildren);
      for (auto bciter = zoneBCChildren.begin(); bciter != zoneBCChildren.end(); ++bciter)
      {
        char label[CGIO_MAX_LABEL_LENGTH + 1];
        cgio_get_label(this->cgioNum, *bciter, label);
        if (strcmp(label, "BC_t") == 0)
        {
          try
          {
            BCInformationUns binfo(this->cgioNum, *bciter, cellDim);
            if (CGNSRead::ReadPatch(this, baseInfo, zoneInfo, binfo.FamilyName))
            {
              std::vector<vtkIdList*> bndFaceList;
              //
              // Read Polygons ...
              //------------------
              if (binfo.BCElementRange.size() == 2)
              {
                vtkIdType bcStartFaceId = binfo.BCElementRange[0];
                vtkIdType bcEndFaceId = binfo.BCElementRange[1];
                vtkIdType residualNumFacesToRead = bcEndFaceId - bcStartFaceId + 1;

                bndFaceList.resize(residualNumFacesToRead, nullptr);
                for (vtkIdType faceId = 0; faceId < residualNumFacesToRead; faceId++)
                {
                  bndFaceList[faceId] = vtkIdList::New();
                }

                for (std::size_t sec = 0; sec < ngonSec.size(); sec++)
                {
                  int curSec = ngonSec[sec];
                  //
                  // Compute range intersection with current section
                  //------------------------------------------------
                  cgsize_t startFaceId = std::max(sectionInfoList[curSec].range[0],
                    static_cast<cgsize_t>(binfo.BCElementRange[0]));
                  cgsize_t endFaceId = std::min(sectionInfoList[curSec].range[1],
                    static_cast<cgsize_t>(binfo.BCElementRange[1]));
                  cgsize_t numFacesToRead = endFaceId - startFaceId + 1;

                  if (numFacesToRead <= 0)
                    continue;

                  // Do a partial read of Faces in current Section
                  //----------------------------------------------
                  std::vector<vtkIdType> bcFaceElementsIdx;
                  std::vector<vtkIdType> bcFaceElementsArr;
                  bcFaceElementsIdx.resize(numFacesToRead + 1);

                  cgsize_t memDim[2];

                  srcStart[0] = startFaceId - sectionInfoList[curSec].range[0] + 1;
                  srcEnd[0] = srcStart[0] + numFacesToRead;
                  srcStride[0] = 1;

                  memStart[0] = 1;
                  memStart[1] = 1;
                  memEnd[0] = numFacesToRead + 1;
                  memEnd[1] = 1;
                  memStride[0] = 1;
                  memStride[1] = 1;
                  memDim[0] = numFacesToRead + 1;
                  memDim[1] = 1;

                  if (0 !=
                    CGNSRead::get_section_start_offset(this->cgioNum, elemIdList[curSec], 1,
                      srcStart, srcEnd, srcStride, memStart, memEnd, memStride, memDim,
                      bcFaceElementsIdx.data()))
                  {
                    vtkErrorMacro("Partial read of NGON_n ElementStartOffset array for BC FAILED.");
                    return 1;
                  }

                  bcFaceElementsArr.resize(
                    bcFaceElementsIdx[numFacesToRead] - bcFaceElementsIdx[0]);

                  srcStart[0] = bcFaceElementsIdx[0] + 1;
                  srcEnd[0] = bcFaceElementsIdx[numFacesToRead];
                  srcStride[0] = 1;

                  memStart[0] = 1;
                  memStart[1] = 1;
                  memEnd[0] = bcFaceElementsIdx[numFacesToRead] - bcFaceElementsIdx[0];
                  memEnd[1] = 1;
                  memStride[0] = 1;
                  memStride[1] = 1;
                  memDim[0] = bcFaceElementsIdx[numFacesToRead] - bcFaceElementsIdx[0];
                  memDim[1] = 1;

                  if (0 !=
                    CGNSRead::get_section_connectivity(this->cgioNum, elemIdList[curSec], 1,
                      srcStart, srcEnd, srcStride, memStart, memEnd, memStride, memDim,
                      bcFaceElementsArr.data()))
                  {
                    vtkErrorMacro("Partial read of BC NGON_n faces FAILED\n");
                    return 1;
                  }

                  // Prepare nodes to generate polygons
                  for (vtkIdType nf = 0; nf < numFacesToRead; ++nf)
                  {
                    vtkIdType startNode = bcFaceElementsIdx[nf] - bcFaceElementsIdx[0];
                    vtkIdType numNodes = bcFaceElementsIdx[nf + 1] - bcFaceElementsIdx[nf];
                    vtkIdList* nodes = bndFaceList[nf + startFaceId - bcStartFaceId];
                    // nodes->InsertNextId(numNodes);
                    for (vtkIdType nn = 0; nn < numNodes; ++nn)
                    {
                      vtkIdType nodeID = bcFaceElementsArr[startNode + nn] - 1;
                      nodes->InsertNextId(nodeID);
                    }
                  }

                  residualNumFacesToRead -= numFacesToRead;
                  if (residualNumFacesToRead <= 0)
                    break;
                }
              }
              else if (!binfo.BCElementList.empty())
              {
                vtkIdType residualNumFacesToRead =
                  static_cast<vtkIdType>(binfo.BCElementList.size());

                std::vector<bool> BCElementRead(binfo.BCElementList.size(), false);

                const auto bcminmax = std::minmax_element(
                  std::begin(binfo.BCElementList), std::end(binfo.BCElementList));

                bndFaceList.resize(residualNumFacesToRead, nullptr);
                for (vtkIdType faceId = 0; faceId < residualNumFacesToRead; faceId++)
                {
                  bndFaceList[faceId] = vtkIdList::New();
                }

                for (std::size_t sec = 0; sec < ngonSec.size(); sec++)
                {
                  int curSec = ngonSec[sec];
                  std::vector<std::pair<vtkIdType, vtkIdType>> faceElemToRead;
                  //
                  // Compute list of face in current section
                  //------------------------------------------------
                  // Quick skip useless section
                  if ((*bcminmax.first > sectionInfoList[curSec].range[1]) ||
                    (*bcminmax.second < sectionInfoList[curSec].range[0]))
                  {
                    continue;
                  }

                  for (std::size_t idx = 0; idx < BCElementRead.size(); idx++)
                  {
                    if (BCElementRead[idx] == true)
                    {
                      continue;
                    }
                    if (binfo.BCElementList[idx] >= sectionInfoList[curSec].range[0] &&
                      binfo.BCElementList[idx] <= sectionInfoList[curSec].range[1])
                    {
                      faceElemToRead.emplace_back(binfo.BCElementList[idx], idx);
                      BCElementRead[idx] = true;
                    }
                  }
                  //  Nothing to read in this section
                  if (faceElemToRead.empty())
                  {
                    continue;
                  }

                  // sort face Bnd Element to Read
                  std::sort(faceElemToRead.begin(), faceElemToRead.end());
                  // Generate partial contiguous chuncks to read
                  vtkIdType curFaceId = faceElemToRead[0].first;
                  std::vector<vtkIdType> rangeIdx;
                  rangeIdx.push_back(0);
                  vtkIdType sizeFaceElemToRead = static_cast<vtkIdType>(faceElemToRead.size());
                  for (vtkIdType ii = 1; ii < sizeFaceElemToRead; ii++)
                  {
                    if (faceElemToRead[ii].first != curFaceId + 1)
                    {
                      rangeIdx.push_back(ii);
                    }
                    curFaceId = faceElemToRead[ii].first;
                  }
                  rangeIdx.push_back(sizeFaceElemToRead);

                  // Do each partial range read
                  for (size_t ii = 1; ii < rangeIdx.size(); ii++)
                  {
                    vtkIdType startFaceId = faceElemToRead[rangeIdx[ii - 1]].first;
                    vtkIdType endFaceId = faceElemToRead[rangeIdx[ii] - 1].first;
                    vtkIdType numFacesToRead = endFaceId - startFaceId + 1;
                    // do partial read
                    //----------------
                    std::vector<vtkIdType> bcFaceElementsIdx;
                    std::vector<vtkIdType> bcFaceElementsArr;
                    bcFaceElementsIdx.resize(numFacesToRead + 1);

                    cgsize_t memDim[2];

                    srcStart[0] = startFaceId - sectionInfoList[curSec].range[0] + 1;
                    srcEnd[0] = endFaceId - sectionInfoList[curSec].range[0] + 2;
                    srcStride[0] = 1;

                    memStart[0] = 1;
                    memStart[1] = 1;
                    memEnd[0] = numFacesToRead + 1;
                    memEnd[1] = 1;
                    memStride[0] = 1;
                    memStride[1] = 1;
                    memDim[0] = numFacesToRead + 1;
                    memDim[1] = 1;

                    if (0 !=
                      CGNSRead::get_section_start_offset(this->cgioNum, elemIdList[curSec], 1,
                        srcStart, srcEnd, srcStride, memStart, memEnd, memStride, memDim,
                        bcFaceElementsIdx.data()))
                    {
                      vtkErrorMacro(
                        "Partial read of NGON_n ElementStartOffset array for BC FAILED.");
                      return 1;
                    }

                    bcFaceElementsArr.resize(
                      bcFaceElementsIdx[numFacesToRead] - bcFaceElementsIdx[0]);

                    srcStart[0] = bcFaceElementsIdx[0] + 1;
                    srcEnd[0] = bcFaceElementsIdx[numFacesToRead];
                    srcStride[0] = 1;

                    memStart[0] = 1;
                    memStart[1] = 1;
                    memEnd[0] = bcFaceElementsIdx[numFacesToRead] - bcFaceElementsIdx[0];
                    memEnd[1] = 1;
                    memStride[0] = 1;
                    memStride[1] = 1;
                    memDim[0] = bcFaceElementsIdx[numFacesToRead] - bcFaceElementsIdx[0];
                    memDim[1] = 1;

                    if (0 !=
                      CGNSRead::get_section_connectivity(this->cgioNum, elemIdList[curSec], 1,
                        srcStart, srcEnd, srcStride, memStart, memEnd, memStride, memDim,
                        bcFaceElementsArr.data()))
                    {
                      vtkErrorMacro("Partial read of BC NGON_n faces FAILED\n");
                      return 1;
                    }

                    // Now append
                    for (vtkIdType nf = 0; nf < numFacesToRead; ++nf)
                    {
                      vtkIdType startNode = bcFaceElementsIdx[nf] - bcFaceElementsIdx[0];
                      vtkIdType numNodes = bcFaceElementsIdx[nf + 1] - bcFaceElementsIdx[nf];

                      vtkIdList* nodes = bndFaceList[faceElemToRead[rangeIdx[ii - 1] + nf].second];
                      // nodes->InsertNextId(numNodes);
                      for (vtkIdType nn = 0; nn < numNodes; ++nn)
                      {
                        vtkIdType nodeID = bcFaceElementsArr[startNode + nn] - 1;
                        nodes->InsertNextId(nodeID);
                      }
                    }
                  }

                  residualNumFacesToRead -= sizeFaceElemToRead;
                  if (residualNumFacesToRead <= 0)
                    break;
                }
              }
              else
              {
                continue;
              }
              // Generate support unstructured grid
              vtkSmartPointer<vtkUnstructuredGrid> bcGrid =
                vtkSmartPointer<vtkUnstructuredGrid>::New();
              vtkNew<vtkIdList> sortedBCPtIds;
              for (auto nodes : bndFaceList)
              {
                for (vtkIdType ii = 0; ii < nodes->GetNumberOfIds(); ii++)
                {
                  sortedBCPtIds->InsertUniqueId(nodes->GetId(ii));
                }
              }

              sortedBCPtIds->Sort();
              // Renumbering //
              std::unordered_map<vtkIdType, vtkIdType> translateIds;
              for (vtkIdType newId = 0; newId < sortedBCPtIds->GetNumberOfIds(); newId++)
              {
                vtkIdType oldId = sortedBCPtIds->GetId(newId);
                translateIds[oldId] = newId;
              }

              // Create dedicated vtkPoints for BC
              vtkSmartPointer<vtkPoints> bcPoints = vtkSmartPointer<vtkPoints>::New();
              bcPoints->SetDataType(points->GetDataType());
              bcPoints->SetNumberOfPoints(sortedBCPtIds->GetNumberOfIds());
              points->GetPoints(sortedBCPtIds, bcPoints.Get());
              bcGrid->SetPoints(bcPoints.Get());

              // set new ids in bndFaceList
              for (auto nodes : bndFaceList)
              {
                for (vtkIdType ii = 0; ii < nodes->GetNumberOfIds(); ii++)
                {
                  vtkIdType curId = nodes->GetId(ii);
                  nodes->SetId(ii, translateIds[curId]);
                }
              }

              // Now transfer bndFaceList to the VTK POLYGONS
              for (auto nodes : bndFaceList)
              {
                bcGrid->InsertNextCell(VTK_POLYGON, nodes);
                nodes->Delete();
              }
              //
              // Parse BCDataSet CGNS arrays
              //
              // TODO: Improve read of BCDataSet_t nodes to get DirichletData, NeumannData arrays at
              // FaceCenter
              // Inherit Centering from BC_t node
              // Fill the bcGrid with these boundary values.
              vtkCGNSReader::vtkPrivate::readBCData(
                *bciter, cellDim, physicalDim, binfo.Location, bcGrid.Get(), this);
              // For Pointdata, it can be extracted from the unstructured Volume.
              //
              const unsigned int idx = patchesMB->GetNumberOfBlocks();
              vtkPrivate::AddIsPatchArray(bcGrid, true);
              patchesMB->SetBlock(idx, bcGrid.Get());

              if (!binfo.FamilyName.empty())
              {
                vtkInformationStringKey* bcfamily = vtkCGNSReader::FAMILY();
                patchesMB->GetMetaData(idx)->Set(bcfamily, binfo.FamilyName.c_str());
              }
              patchesMB->GetMetaData(idx)->Set(vtkCompositeDataSet::NAME(), binfo.Name);
            }
          }
          catch (const CGIOUnsupported& ue)
          {
            vtkWarningMacro("Skipping BC_t node: " << ue.what());
          }
          catch (const CGIOError& e)
          {
            vtkErrorMacro("Failed to read BC_t node: " << e.what());
          }
        }
      }
    }
    CGNSRead::releaseIds(this->cgioNum, zoneChildren);
    zoneChildren.clear();
  }
  else if (!bndSec.empty() && requiredPatch)
  {
    //----------------------------------------------------------------------------
    // Handle boundary conditions (BC) patches for unstructured grid
    //----------------------------------------------------------------------------
    mzone->SetBlock(0u, ugrid.Get());
    mzone->GetMetaData(0u)->Set(vtkCompositeDataSet::NAME(), "Internal");
    //
    vtkNew<vtkMultiBlockDataSet> patchesMB;
    mzone->SetBlock(1, patchesMB.Get());
    mzone->GetMetaData(1)->Set(vtkCompositeDataSet::NAME(), "Patches");
    //
    // Build Multi Patches
    //
    std::vector<double> zoneChildren;
    CGNSRead::getNodeChildrenId(this->cgioNum, this->currentId, zoneChildren);
    for (auto iter = zoneChildren.begin(); iter != zoneChildren.end(); ++iter)
    {
      CGNSRead::char_33 nodeLabel;
      cgio_get_label(cgioNum, (*iter), nodeLabel);
      if (strcmp(nodeLabel, "ZoneBC_t") != 0)
      {
        continue;
      }

      const double zoneBCId = (*iter);

      // iterate over all children and read supported BC_t nodes.
      std::vector<double> zoneBCChildren;
      CGNSRead::getNodeChildrenId(this->cgioNum, zoneBCId, zoneBCChildren);
      for (auto bciter = zoneBCChildren.begin(); bciter != zoneBCChildren.end(); ++bciter)
      {
        char label[CGIO_MAX_LABEL_LENGTH + 1];
        cgio_get_label(this->cgioNum, *bciter, label);
        if (strcmp(label, "BC_t") == 0)
        {
          try
          {
            BCInformationUns binfo(this->cgioNum, *bciter, cellDim);
            if (CGNSRead::ReadPatch(this, baseInfo, zoneInfo, binfo.FamilyName))
            {
              // Common struct to read from BCElementList or BCElementRange
              vtkNew<vtkCellArray> bcCells;
              int* bcCellsTypes = nullptr;
              // Read unstructured grids ...
              //------------------------------------
              if (binfo.BCElementRange.size() == 2)
              {
                vtkIdType bcStartElemId = binfo.BCElementRange[0];
                vtkIdType bcEndElemId = binfo.BCElementRange[1];
                vtkIdType numElemToRead = bcEndElemId - bcStartElemId + 1;
                vtkIdType residualNumElemToRead = bcEndElemId - bcStartElemId + 1;
                //
                // compute number of bnd section to read to allocate element type array
                // and connectivity array
                std::vector<int> bndSecToUse;
                std::vector<cgsize_t> numElemToReadBndSec;
                std::vector<cgsize_t> sizeAllocForEachSection;
                std::vector<cgsize_t> startReadingPos;

                for (std::vector<int>::iterator bndIter = bndSec.begin(); bndIter != bndSec.end();
                     ++bndIter)
                {
                  int curSec = *bndIter;
                  CGNS_ENUMT(ElementType_t) elemType = CGNS_ENUMV(ElementTypeNull);
                  // Compute range intersection with current section
                  //------------------------------------------------
                  cgsize_t startBndElemId = std::max(sectionInfoList[curSec].range[0],
                    static_cast<cgsize_t>(binfo.BCElementRange[0]));
                  cgsize_t endBndElemId = std::min(sectionInfoList[curSec].range[1],
                    static_cast<cgsize_t>(binfo.BCElementRange[1]));
                  cgsize_t numBndElemToRead = endBndElemId - startBndElemId + 1;

                  if (numBndElemToRead <= 0)
                    continue;

                  elemType = sectionInfoList[curSec].elemType;

                  if (numBndElemToRead ==
                    (sectionInfoList[curSec].range[1] - sectionInfoList[curSec].range[0] + 1))
                  {
                    // Read whole section
                    if (elemType != CGNS_ENUMV(MIXED))
                    {
                      sizeAllocForEachSection.push_back(
                        sectionInfoList[curSec].eDataSize + numBndElemToRead);
                    }
                    else
                    {
                      sizeAllocForEachSection.push_back(sectionInfoList[curSec].eDataSize);
                    }
                    startReadingPos.push_back(0);
                  }
                  else
                  {
                    // Partial read of section
                    if (elemType != CGNS_ENUMV(MIXED))
                    {
                      // All cells are of the same type.
                      int numPointsPerCell = 0;
                      cgsize_t startPosIdx = startBndElemId - sectionInfoList[curSec].range[0];
                      //
                      if (cg_npe(elemType, &numPointsPerCell) || numPointsPerCell == 0)
                      {
                        vtkErrorMacro(<< "Invalid numPointsPerCell\n");
                      }
                      sizeAllocForEachSection.push_back((numPointsPerCell + 1) * numBndElemToRead);
                      startPosIdx *= static_cast<cgsize_t>(numPointsPerCell);
                      startReadingPos.push_back(startPosIdx);
                    }
                    else if (elemType == CGNS_ENUMV(MIXED))
                    {
                      cgsize_t memDim[2];
                      cgsize_t offsetDataSize(0);
                      cgsize_t startPosIdx(0);
                      cgsize_t endPosIdx(0);
                      // Maybe bndElementsIdx should use cgsize_t but since get_section_start_offset
                      // already exists and use vtkIdType...
                      std::vector<vtkIdType> bndElementsIdx;
                      offsetDataSize =
                        sectionInfoList[curSec].range[1] - sectionInfoList[curSec].range[0] + 2;
                      bndElementsIdx.resize(offsetDataSize);

                      srcStart[0] = 1;
                      srcEnd[0] = offsetDataSize;
                      srcStride[0] = 1;

                      memStart[0] = 1;
                      memStart[1] = 1;
                      memEnd[0] = offsetDataSize;
                      memEnd[1] = 1;
                      memStride[0] = 1;
                      memStride[1] = 1;
                      memDim[0] = offsetDataSize;
                      memDim[1] = 1;

                      if (0 !=
                        CGNSRead::get_section_start_offset(this->cgioNum, elemIdList[curSec], 1,
                          srcStart, srcEnd, srcStride, memStart, memEnd, memStride, memDim,
                          &(bndElementsIdx[0])))
                      {
                        // no bndElementsIdx read so create it
                        // This is the worst case situation
                        cgsize_t fDataSize(0);
                        int numPointsPerCell = 0;
                        std::vector<vtkIdType> bndElements;

                        fDataSize = sectionInfoList[curSec].eDataSize;
                        bndElements.resize(fDataSize);

                        srcStart[0] = 1;
                        srcEnd[0] = fDataSize;
                        srcStride[0] = 1;

                        memStart[0] = 1;
                        memStart[1] = 1;
                        memEnd[0] = fDataSize;
                        memEnd[1] = 1;
                        memStride[0] = 1;
                        memStride[1] = 1;

                        memDim[0] = fDataSize;
                        memDim[1] = 1;

                        if (0 !=
                          CGNSRead::get_section_connectivity(this->cgioNum, elemIdList[curSec], 1,
                            srcStart, srcEnd, srcStride, memStart, memEnd, memStride, memDim,
                            &(bndElements[0])))
                        {
                          vtkErrorMacro("FAILED to read MIXED boundary cells\n");
                          return 1;
                        }
                        bndElementsIdx[0] = 0;

                        for (std::size_t idxElem = 0; idxElem < bndElementsIdx.size(); ++idxElem)
                        {
                          vtkIdType pos = bndElementsIdx[idxElem];
                          elemType = static_cast<CGNS_ENUMT(ElementType_t)>(bndElements[pos]);
                          cg_npe(elemType, &numPointsPerCell);
                          bndElementsIdx[idxElem + 1] =
                            bndElementsIdx[idxElem] + static_cast<vtkIdType>(numPointsPerCell + 1);
                        }
                      }
                      // Partial Size determination through bndElementsIdx
                      startPosIdx = startBndElemId - sectionInfoList[curSec].range[0];
                      endPosIdx = startPosIdx + numBndElemToRead;
                      vtkIdType partialSize =
                        bndElementsIdx[endPosIdx] - bndElementsIdx[startPosIdx];
                      startReadingPos.push_back(static_cast<cgsize_t>(bndElementsIdx[startPosIdx]));
                      sizeAllocForEachSection.push_back(static_cast<cgsize_t>(partialSize));
                    }
                    else
                    {
                      vtkErrorMacro("Unexpected element type for boundary\n");
                      return 1;
                    }
                  }
                  bndSecToUse.push_back(curSec);
                  numElemToReadBndSec.push_back(numBndElemToRead);
                  residualNumElemToRead -= numBndElemToRead;
                  if (residualNumElemToRead <= 0)
                  {
                    break;
                  }
                }

                if (residualNumElemToRead > 0)
                {
                  vtkWarningMacro("Not enough elements to generate BC " << binfo.Name);
                  numElemToRead -= residualNumElemToRead;
                }

                // Now allocate arrays to read connectivities

                bcCellsTypes = new int[numElemToRead];
                if (bcCellsTypes == nullptr)
                {
                  vtkErrorMacro("Could not allocate memory for boundary cell types\n");
                  return 1;
                }

                cgsize_t elemBcSize = 0;
                for (cgsize_t& sizeValue : sizeAllocForEachSection)
                {
                  elemBcSize += sizeValue;
                }
                std::vector<cgsize_t> startArrayBndSec(bndSecToUse.size());
                std::vector<cgsize_t> startIdBndSec(bndSecToUse.size());
                for (std::size_t sec = 0; sec < bndSecToUse.size(); sec++)
                {
                  cgsize_t curStart = sectionInfoList[bndSecToUse[sec]].range[0];
                  cgsize_t curArrayStart = 0;
                  cgsize_t curIdStart = 0;
                  for (std::size_t lse = 0; lse < bndSecToUse.size(); lse++)
                  {
                    if (sectionInfoList[bndSecToUse[lse]].range[0] < curStart)
                    {
                      curArrayStart += sizeAllocForEachSection[lse];
                      curIdStart += numElemToReadBndSec[lse];
                    }
                  }
                  startArrayBndSec[sec] = curArrayStart;
                  startIdBndSec[sec] = curIdStart;
                }

                // Create Cell Array
                vtkNew<vtkIdTypeArray> cellBcLocations;
                cellBcLocations->SetNumberOfValues(elemBcSize);
                vtkIdType* bcGlobalElements = cellBcLocations->GetPointer(0);

                if (bcGlobalElements == nullptr)
                {
                  vtkErrorMacro("Could not allocate memory for BC connectivity\n");
                  return 1;
                }
                memset(bcGlobalElements, 0, sizeof(vtkIdType) * elemBcSize);
                // Memory is ready now load the data
                for (std::size_t idx = 0; idx < bndSecToUse.size(); ++idx)
                {
                  int curSec = bndSecToUse[idx];
                  CGNS_ENUMT(ElementType_t) elemType = sectionInfoList[curSec].elemType;
                  vtkIdType* localBndElements = &(bcGlobalElements[startArrayBndSec[idx]]);

                  if (elemType != CGNS_ENUMV(MIXED))
                  {
                    int cellType;
                    bool higherOrderWarning;
                    bool reOrderElements;
                    cgsize_t memDim[2];
                    cgsize_t npe = 0;
                    int numPointsPerCell(0);

                    cellType =
                      CGNSRead::GetVTKElemType(elemType, higherOrderWarning, reOrderElements);
                    if (cellType == VTK_EMPTY_CELL)
                    {
                      vtkErrorMacro("Unsupported cell type\n");
                      return 1;
                    }
                    for (cgsize_t ii = 0; ii < numElemToReadBndSec[idx]; ++ii)
                    {
                      bcCellsTypes[ii + startIdBndSec[idx]] = cellType;
                    }

                    cg_npe(elemType, &numPointsPerCell);
                    npe = static_cast<cgsize_t>(numPointsPerCell);

                    srcStart[0] = 1 + startReadingPos[idx];
                    srcStart[1] = 1;

                    srcEnd[0] = startReadingPos[idx] + numElemToReadBndSec[idx] * npe;
                    srcEnd[1] = 1;
                    srcStride[0] = 1;
                    srcStride[1] = 1;

                    memStart[0] = 2;
                    memStart[1] = 1;
                    memEnd[0] = npe + 1;
                    memEnd[1] = numElemToReadBndSec[idx];
                    memStride[0] = 1;
                    memStride[1] = 1;
                    memDim[0] = npe + 1;
                    memDim[1] = numElemToReadBndSec[idx];

                    CGNSRead::get_section_connectivity(this->cgioNum, elemIdList[curSec], 2,
                      srcStart, srcEnd, srcStride, memStart, memEnd, memStride, memDim,
                      localBndElements);
                    // Add numptspercell and do -1 on indexes
                    for (vtkIdType icell = 0; icell < numElemToReadBndSec[idx]; ++icell)
                    {
                      vtkIdType pos = icell * (npe + 1);
                      localBndElements[pos] = static_cast<vtkIdType>(npe);
                      for (vtkIdType ip = 0; ip < npe; ip++)
                      {
                        pos++;
                        localBndElements[pos] = localBndElements[pos] - 1;
                      }
                    }
                  }
                  else if (elemType == CGNS_ENUMV(MIXED))
                  {
                    cgsize_t memDim[2];
                    int numPointsPerCell(0);
                    int cellType;
                    bool higherOrderWarning;
                    bool reOrderElements;

                    srcStart[0] = 1 + startReadingPos[idx];
                    srcEnd[0] = startReadingPos[idx] + sizeAllocForEachSection[idx];
                    srcStride[0] = 1;

                    memStart[0] = 1;
                    memStart[1] = 1;
                    memEnd[0] = sizeAllocForEachSection[idx];
                    memEnd[1] = 1;
                    memStride[0] = 1;
                    memStride[1] = 1;
                    memDim[0] = sizeAllocForEachSection[idx];
                    memDim[1] = 1;

                    CGNSRead::get_section_connectivity(this->cgioNum, elemIdList[curSec], 1,
                      srcStart, srcEnd, srcStride, memStart, memEnd, memStride, memDim,
                      localBndElements);
                    vtkIdType pos = 0;
                    for (vtkIdType icell = 0; icell < numElemToReadBndSec[idx]; ++icell)
                    {
                      elemType = static_cast<CGNS_ENUMT(ElementType_t)>(localBndElements[pos]);
                      cg_npe(elemType, &numPointsPerCell);
                      cellType =
                        CGNSRead::GetVTKElemType(elemType, higherOrderWarning, reOrderElements);
                      bcCellsTypes[icell + startIdBndSec[idx]] = cellType;
                      localBndElements[pos] = static_cast<vtkIdType>(numPointsPerCell);
                      pos++;
                      for (vtkIdType ip = 0; ip < numPointsPerCell; ip++)
                      {
                        localBndElements[ip + pos] = localBndElements[ip + pos] - 1;
                      }
                      pos += numPointsPerCell;
                    }
                  }
                }
                bcCells->SetCells(numElemToRead, cellBcLocations.GetPointer());
              }
              else if (!binfo.BCElementList.empty())
              {
                // This a bit more tricky to implement because it generate lot of small IO

                vtkIdType residualNumElemToRead =
                  static_cast<vtkIdType>(binfo.BCElementList.size());
                std::vector<bool> BCElementRead(binfo.BCElementList.size(), false);

                const auto bcminmax = std::minmax_element(
                  std::begin(binfo.BCElementList), std::end(binfo.BCElementList));

                std::vector<vtkIdList*> bndElemList;
                bndElemList.resize(residualNumElemToRead, nullptr);
                for (vtkIdType elemId = 0; elemId < residualNumElemToRead; elemId++)
                {
                  bndElemList[elemId] = vtkIdList::New();
                }

                bcCellsTypes = new int[residualNumElemToRead];
                if (bcCellsTypes == nullptr)
                {
                  vtkErrorMacro("Could not allocate memory for boundary cell types\n");
                  return 1;
                }

                for (std::size_t sec = 0; sec < bndSec.size(); sec++)
                {
                  int curSec = bndSec[sec];
                  std::vector<std::pair<vtkIdType, vtkIdType>> elemToRead;
                  CGNS_ENUMT(ElementType_t) elemType = CGNS_ENUMV(ElementTypeNull);
                  //
                  // Compute list of boundary elements in current section
                  //------------------------------------------------
                  // Quick skip useless section
                  if ((*bcminmax.first > sectionInfoList[curSec].range[1]) ||
                    (*bcminmax.second < sectionInfoList[curSec].range[0]))
                  {
                    continue;
                  }

                  for (std::size_t idx = 0; idx < BCElementRead.size(); idx++)
                  {
                    if (BCElementRead[idx] == true)
                    {
                      continue;
                    }
                    if (binfo.BCElementList[idx] >= sectionInfoList[curSec].range[0] &&
                      binfo.BCElementList[idx] <= sectionInfoList[curSec].range[1])
                    {
                      elemToRead.emplace_back(binfo.BCElementList[idx], idx);
                      BCElementRead[idx] = true;
                    }
                  }
                  //  Nothing to read in this section
                  if (elemToRead.empty())
                  {
                    continue;
                  }
                  elemType = sectionInfoList[curSec].elemType;

                  // sort face Bnd Element to Read
                  std::sort(elemToRead.begin(), elemToRead.end());
                  // Generate partial contiguous chuncks to read
                  vtkIdType curElemId = elemToRead[0].first;
                  std::vector<vtkIdType> rangeIdx;
                  rangeIdx.push_back(0);
                  vtkIdType sizeElemToRead = static_cast<vtkIdType>(elemToRead.size());
                  for (vtkIdType ii = 1; ii < sizeElemToRead; ii++)
                  {
                    if (elemToRead[ii].first != curElemId + 1)
                    {
                      rangeIdx.push_back(ii);
                    }
                    curElemId = elemToRead[ii].first;
                  }
                  rangeIdx.push_back(sizeElemToRead);

                  // Do each partial range read
                  for (size_t ii = 1; ii < rangeIdx.size(); ii++)
                  {
                    vtkIdType startElemId = elemToRead[rangeIdx[ii - 1]].first;
                    vtkIdType endElemId = elemToRead[rangeIdx[ii] - 1].first;
                    vtkIdType numElemToRead = endElemId - startElemId + 1;
                    // do partial read
                    //----------------
                    std::vector<vtkIdType> bcElementsArr;
                    //
                    // Partial read of section chunck
                    if (elemType != CGNS_ENUMV(MIXED))
                    {
                      // All cells are of the same type.
                      cgsize_t memDim[2];
                      cgsize_t startPosIdx = startElemId - sectionInfoList[curSec].range[0];
                      int numPointsPerCell = 0;
                      int cellType;
                      bool higherOrderWarning;
                      bool reOrderElements;

                      cellType =
                        CGNSRead::GetVTKElemType(elemType, higherOrderWarning, reOrderElements);
                      if (cellType == VTK_EMPTY_CELL)
                      {
                        vtkErrorMacro("Unsupported cellType found in BC\n");
                      }
                      //
                      if (cg_npe(elemType, &numPointsPerCell) || numPointsPerCell == 0)
                      {
                        vtkErrorMacro("Invalid numPointsPerCell\n");
                      }
                      bcElementsArr.resize((numPointsPerCell + 1) * numElemToRead);
                      startPosIdx *= static_cast<cgsize_t>(numPointsPerCell);

                      srcStart[0] = 1 + startPosIdx;
                      srcStart[1] = 1;

                      srcEnd[0] = startPosIdx + numElemToRead * numPointsPerCell;
                      srcEnd[1] = 1;
                      srcStride[0] = 1;
                      srcStride[1] = 1;

                      memStart[0] = 2;
                      memStart[1] = 1;
                      memEnd[0] = numPointsPerCell + 1;
                      memEnd[1] = numElemToRead;
                      memStride[0] = 1;
                      memStride[1] = 1;
                      memDim[0] = numPointsPerCell + 1;
                      memDim[1] = numElemToRead;

                      CGNSRead::get_section_connectivity(this->cgioNum, elemIdList[curSec], 2,
                        srcStart, srcEnd, srcStride, memStart, memEnd, memStride, memDim,
                        &(bcElementsArr[0]));
                      // Add numptspercell and do -1 on indexes
                      for (vtkIdType icell = 0; icell < numElemToRead; ++icell)
                      {
                        vtkIdType pos = icell * (numPointsPerCell + 1);
                        bcElementsArr[pos] = static_cast<vtkIdType>(numPointsPerCell);
                        for (vtkIdType ip = 0; ip < numPointsPerCell; ip++)
                        {
                          pos++;
                          bcElementsArr[pos] = bcElementsArr[pos] - 1;
                        }
                      }
                      // Now append
                      for (vtkIdType nelem = 0; nelem < numElemToRead; ++nelem)
                      {
                        bcCellsTypes[elemToRead[rangeIdx[ii - 1] + nelem].second] = cellType;
                        vtkIdList* nodes = bndElemList[elemToRead[rangeIdx[ii - 1] + nelem].second];

                        for (vtkIdType nn = 0; nn < numPointsPerCell; ++nn)
                        {
                          vtkIdType nodeID = bcElementsArr[nelem * (numPointsPerCell + 1) + 1 + nn];
                          nodes->InsertNextId(nodeID);
                        }
                      }
                    }
                    else if (elemType == CGNS_ENUMV(MIXED))
                    {
                      cgsize_t memDim[2];
                      int cellType;
                      bool higherOrderWarning;
                      bool reOrderElements;

                      // Maybe bndElementsIdx should use cgsize_t but since get_section_start_offset
                      // already exists and use vtkIdType...
                      std::vector<vtkIdType> bndElementsIdx;
                      bndElementsIdx.resize(numElemToRead + 1);

                      srcStart[0] = startElemId - sectionInfoList[curSec].range[0] + 1;
                      srcEnd[0] = endElemId - sectionInfoList[curSec].range[0] + 2;
                      srcStride[0] = 1;

                      memStart[0] = 1;
                      memStart[1] = 1;
                      memEnd[0] = numElemToRead + 1;
                      memEnd[1] = 1;
                      memStride[0] = 1;
                      memStride[1] = 1;
                      memDim[0] = numElemToRead + 1;
                      memDim[1] = 1;

                      if (0 !=
                        CGNSRead::get_section_start_offset(this->cgioNum, elemIdList[curSec], 1,
                          srcStart, srcEnd, srcStride, memStart, memEnd, memStride, memDim,
                          &(bndElementsIdx[0])))
                      {
                        // Fallback to old way because no Offset found
                        cgsize_t fDataSize(0);
                        int numPointsPerCell = 0;

                        fDataSize = sectionInfoList[curSec].eDataSize;
                        bcElementsArr.resize(fDataSize);

                        srcStart[0] = 1;
                        srcEnd[0] = fDataSize;
                        srcStride[0] = 1;

                        memStart[0] = 1;
                        memStart[1] = 1;
                        memEnd[0] = fDataSize;
                        memEnd[1] = 1;
                        memStride[0] = 1;
                        memStride[1] = 1;

                        memDim[0] = fDataSize;
                        memDim[1] = 1;

                        if (0 !=
                          CGNSRead::get_section_connectivity(this->cgioNum, elemIdList[curSec], 1,
                            srcStart, srcEnd, srcStride, memStart, memEnd, memStride, memDim,
                            &(bcElementsArr[0])))
                        {
                          vtkErrorMacro("FAILED to read MIXED boundary cells\n");
                          return 1;
                        }
                        vtkIdType pos = 0;
                        vtkIdType nelem = 0;
                        for (vtkIdType idxElem = sectionInfoList[curSec].range[0];
                             idxElem < sectionInfoList[curSec].range[1] + 1; ++idxElem)
                        {
                          CGNS_ENUMT(ElementType_t)
                          localElemType =
                            static_cast<CGNS_ENUMT(ElementType_t)>(bcElementsArr[pos]);
                          cg_npe(localElemType, &numPointsPerCell);
                          if ((startElemId - idxElem) > 0)
                          {
                            pos += numPointsPerCell + 1;
                            continue;
                          }
                          if ((endElemId - idxElem) < 0)
                          {
                            break;
                          }
                          vtkIdType numNodes = static_cast<vtkIdType>(numPointsPerCell);
                          cellType = CGNSRead::GetVTKElemType(
                            localElemType, higherOrderWarning, reOrderElements);
                          if (cellType == VTK_EMPTY_CELL)
                          {
                            vtkErrorMacro("Unsupported cellType found in BC\n");
                          }
                          bcCellsTypes[elemToRead[rangeIdx[ii - 1] + nelem].second] = cellType;
                          vtkIdList* nodes =
                            bndElemList[elemToRead[rangeIdx[ii - 1] + nelem].second];
                          pos++;
                          for (vtkIdType nn = 0; nn < numNodes; ++nn)
                          {
                            vtkIdType nodeID = bcElementsArr[pos] - 1;
                            nodes->InsertNextId(nodeID);
                            pos++;
                          }
                          nelem++;
                        }
                        // end old way
                      }
                      else
                      {
                        // modern way
                        bcElementsArr.resize(bndElementsIdx[numElemToRead] - bndElementsIdx[0]);

                        srcStart[0] = bndElementsIdx[0] + 1;
                        srcEnd[0] = bndElementsIdx[numElemToRead];
                        srcStride[0] = 1;

                        memStart[0] = 1;
                        memStart[1] = 1;
                        memEnd[0] = bndElementsIdx[numElemToRead] - bndElementsIdx[0];
                        memEnd[1] = 1;
                        memStride[0] = 1;
                        memStride[1] = 1;
                        memDim[0] = bndElementsIdx[numElemToRead] - bndElementsIdx[0];
                        memDim[1] = 1;

                        if (0 !=
                          CGNSRead::get_section_connectivity(this->cgioNum, elemIdList[curSec], 1,
                            srcStart, srcEnd, srcStride, memStart, memEnd, memStride, memDim,
                            bcElementsArr.data()))
                        {
                          vtkErrorMacro("Partial read of MIXED elements FAILED\n");
                          return 1;
                        }

                        // Now append
                        for (vtkIdType nelem = 0; nelem < numElemToRead; ++nelem)
                        {
                          vtkIdType startNode = bndElementsIdx[nelem] - bndElementsIdx[0];
                          vtkIdType numNodes =
                            bndElementsIdx[nelem + 1] - bndElementsIdx[nelem] - 1;
                          CGNS_ENUMT(ElementType_t)
                          localElemType =
                            static_cast<CGNS_ENUMT(ElementType_t)>(bcElementsArr[startNode]);
                          cellType = CGNSRead::GetVTKElemType(
                            localElemType, higherOrderWarning, reOrderElements);
                          if (cellType == VTK_EMPTY_CELL)
                          {
                            vtkErrorMacro("Unsupported cellType found in BC\n");
                          }
                          bcCellsTypes[elemToRead[rangeIdx[ii - 1] + nelem].second] = cellType;
                          vtkIdList* nodes =
                            bndElemList[elemToRead[rangeIdx[ii - 1] + nelem].second];

                          for (vtkIdType nn = 0; nn < numNodes; ++nn)
                          {
                            vtkIdType nodeID = bcElementsArr[startNode + nn + 1] - 1;
                            nodes->InsertNextId(nodeID);
                          }
                        }
                      }
                    }
                  }

                  residualNumElemToRead -= sizeElemToRead;
                  if (residualNumElemToRead <= 0)
                    break;
                }
                // Transfer to bcCells
                for (std::size_t bcCellId = 0; bcCellId < bndElemList.size(); ++bcCellId)
                {
                  if (bndElemList[bcCellId]->GetNumberOfIds() > 0)
                  {
                    bcCells->InsertNextCell(bndElemList[bcCellId]);
                  }
                }
              }
              else
              {
                continue;
              }
              // Set up ugrid
              // Create an unstructured grid to contain the points.
              // Generate support unstructured grid
              vtkSmartPointer<vtkUnstructuredGrid> bcGrid =
                vtkSmartPointer<vtkUnstructuredGrid>::New();
              // Directly use Global Volume points
              // Renumbering and reducing of points should theoretically be done
              bcGrid->SetPoints(points);
              bcGrid->SetCells(bcCellsTypes, bcCells.GetPointer());
              delete[] bcCellsTypes;

              //
              // Add ispatch 0=false/1=true as field data
              //
              vtkPrivate::AddIsPatchArray(bcGrid, true);

              // Handle Ref Values
              vtkPrivate::AttachReferenceValue(base, bcGrid, this);

              // Copy PointData if exists
              // Dirty way to get data
              vtkPointData* temp = ugrid->GetPointData();
              if (temp != nullptr)
              {
                int NumArray = temp->GetNumberOfArrays();
                for (int i = 0; i < NumArray; ++i)
                {
                  vtkDataArray* dataTmp = temp->GetArray(i);
                  bcGrid->GetPointData()->AddArray(dataTmp);
                }
              }
              // At least should read from Neumann and Dirichlet nodes
              // for face centered values
              //
              // Try to Parse BCDataSet CGNS arrays
              //
              vtkCGNSReader::vtkPrivate::readBCData(
                *bciter, cellDim, physicalDim, binfo.Location, bcGrid.Get(), this);
              //
              const unsigned int idx = patchesMB->GetNumberOfBlocks();
              patchesMB->SetBlock(idx, bcGrid.Get());
              if (!binfo.FamilyName.empty())
              {
                vtkInformationStringKey* bcfamily = vtkCGNSReader::FAMILY();
                patchesMB->GetMetaData(idx)->Set(bcfamily, binfo.FamilyName.c_str());
              }
              patchesMB->GetMetaData(idx)->Set(vtkCompositeDataSet::NAME(), binfo.Name);
            }
          }
          catch (const CGIOUnsupported& ue)
          {
            vtkWarningMacro("Skipping BC_t node: " << ue.what());
          }
          catch (const CGIOError& e)
          {
            vtkErrorMacro("Failed to read BC_t node: " << e.what());
          }
        }
      }
    }
    CGNSRead::releaseIds(this->cgioNum, zoneChildren);
    zoneChildren.clear();
  }
  //
  if ((!bndSec.empty() || hasNFace) && requiredPatch)
  {
    mbase->SetBlock(zone, mzone);
  }
  else
  {
    mbase->SetBlock(zone, ugrid.Get());
  }
  mzone->Delete();
  return 0;
}

//----------------------------------------------------------------------------
class WithinTolerance : public std::binary_function<double, double, bool>
{
public:
  result_type operator()(first_argument_type a, second_argument_type b) const
  {
    bool result = (std::fabs(a - b) <= (a * 1E-6));
    return (result_type)result;
  }
};

//----------------------------------------------------------------------------
int vtkCGNSReader::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  int ier;
  int nzones;
  unsigned int blockIndex = 0;

  int processNumber;
  int numProcessors;
  int startRange, endRange;

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  // get the output
  vtkMultiBlockDataSet* output =
    vtkMultiBlockDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // The whole notion of pieces for this reader is really
  // just a division of zones between processors
  processNumber = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numProcessors = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  if (!this->DistributeBlocks)
  {
    processNumber = 0;
    numProcessors = 1;
  }

  int numBases = this->Internals->Internal->GetNumberOfBaseNodes();
  int numZones = 0;
  for (int bb = 0; bb < numBases; bb++)
  {
    numZones += this->Internals->Internal->GetBase(bb).nzones;
  }

  // Divide the files evenly between processors
  int num_zones_per_process = numZones / numProcessors;

  // This if/else logic is for when you don't have a nice even division of files
  // Each process computes which sequence of files it needs to read in
  int left_over_zones = numZones - (num_zones_per_process * numProcessors);
  // base --> startZone,endZone
  std::map<int, duo_t> baseToZoneRange;

  // REDO this part !!!!
  if (processNumber < left_over_zones)
  {
    int accumulated = 0;
    startRange = (num_zones_per_process + 1) * processNumber;
    endRange = startRange + (num_zones_per_process + 1);
    for (int bb = 0; bb < numBases; bb++)
    {
      duo_t zoneRange;
      startRange = startRange - accumulated;
      endRange = endRange - accumulated;
      int startInterZone = std::max(startRange, 0);
      int endInterZone = std::min(endRange, this->Internals->Internal->GetBase(bb).nzones);

      if ((endInterZone - startInterZone) > 0)
      {
        zoneRange[0] = startInterZone;
        zoneRange[1] = endInterZone;
      }
      accumulated = this->Internals->Internal->GetBase(bb).nzones;
      baseToZoneRange[bb] = zoneRange;
    }
  }
  else
  {
    int accumulated = 0;
    startRange = num_zones_per_process * processNumber + left_over_zones;
    endRange = startRange + num_zones_per_process;
    for (int bb = 0; bb < numBases; bb++)
    {
      duo_t zoneRange;
      startRange = startRange - accumulated;
      endRange = endRange - accumulated;
      int startInterZone = std::max(startRange, 0);
      int endInterZone = std::min(endRange, this->Internals->Internal->GetBase(bb).nzones);
      if ((endInterZone - startInterZone) > 0)
      {
        zoneRange[0] = startInterZone;
        zoneRange[1] = endInterZone;
      }
      accumulated = this->Internals->Internal->GetBase(bb).nzones;
      baseToZoneRange[bb] = zoneRange;
    }
  }

  // Bnd Sections Not implemented yet for parallel
  if (numProcessors > 1)
  {
    this->LoadBndPatch = false;
    this->CreateEachSolutionAsBlock = 0;
  }

  if (!this->Internals->Internal->Parse(this->FileName))
  {
    return 0;
  }

  vtkMultiBlockDataSet* rootNode = output;

  vtkDebugMacro(<< "Start Loading CGNS data");

  this->UpdateProgress(0.0);

  // Setup Global Time Information
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    // Get the requested time step. We only support requests of a single time
    // step in this reader right now
    double requestedTimeValue = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

    // Adjust requested time based on available timesteps.
    std::vector<double>& ts = this->Internals->Internal->GetTimes();

    if (!ts.empty())
    {
      int tsIndex =
        vtkPrivate::GetTimeStepIndex(requestedTimeValue, &ts[0], static_cast<int>(ts.size()));
      requestedTimeValue = ts[tsIndex];
      output->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), requestedTimeValue);
    }
  }
  else
  {
    output->GetInformation()->Remove(vtkDataObject::DATA_TIME_STEP());
  }

  vtkDebugMacro(<< "CGNSReader::RequestData: Reading from file <" << this->FileName << ">...");

  // Opening with cgio layer
  ier = cgio_open_file(this->FileName, CGIO_MODE_READ, 0, &(this->cgioNum));
  if (ier != CG_OK)
  {
    vtkErrorMacro(<< "Error Reading file with cgio");
    return 0;
  }
  cgio_get_root_id(this->cgioNum, &(this->rootId));

  // Get base id list :
  std::vector<double> baseIds;
  ier = CGNSRead::readBaseIds(this->cgioNum, this->rootId, baseIds);
  if (ier != 0)
  {
    vtkErrorMacro(<< "Error Reading Base Ids");
    goto errorData;
  }

  blockIndex = 0;
  for (int numBase = 0; numBase < static_cast<int>(baseIds.size()); numBase++)
  {
    int cellDim = 0;
    int physicalDim = 0;
    const CGNSRead::BaseInformation& curBaseInfo = this->Internals->Internal->GetBase(numBase);

    // skip unselected base
    if (!CGNSRead::ReadBase(this, curBaseInfo))
    {
      continue;
    }

    cellDim = curBaseInfo.cellDim;
    physicalDim = curBaseInfo.physicalDim;

    // Get timesteps here !!
    // Operate on Global time scale :
    // clamp requestedTimeValue to available time range
    // if < timemin --> timemin
    // if > timemax --> timemax
    // Then for each base get Index for TimeStep
    // if useFlowSolution read flowSolution and take name with index
    // same for use
    // Setup Global Time Information
    this->ActualTimeStep = 0;
    bool skipBase = false;

    if (output->GetInformation()->Has(vtkDataObject::DATA_TIME_STEP()))
    {
      // Get the requested time step. We only support requests of a single time
      // step in this reader right now
      double requestedTimeValue = output->GetInformation()->Get(vtkDataObject::DATA_TIME_STEP());

      vtkDebugMacro(<< "RequestData: requested time value: " << requestedTimeValue);

      // Check if requestedTimeValue is available in base time range.
      if ((requestedTimeValue < curBaseInfo.times.front()) ||
        (requestedTimeValue > curBaseInfo.times.back()))
      {
        skipBase = true;
        requestedTimeValue = this->Internals->Internal->GetTimes().front();
      }

      std::vector<double>::const_iterator iter;
      iter =
        std::upper_bound(curBaseInfo.times.begin(), curBaseInfo.times.end(), requestedTimeValue);

      if (iter == curBaseInfo.times.begin())
      {
        // The requested time step is before any time
        this->ActualTimeStep = 0;
      }
      else
      {
        iter--;
        this->ActualTimeStep = static_cast<int>(iter - curBaseInfo.times.begin());
      }
    }
    if (skipBase == true)
    {
      continue;
    }
    vtkMultiBlockDataSet* mbase = vtkMultiBlockDataSet::New();
    nzones = curBaseInfo.nzones;
    if (nzones == 0)
    {
      vtkWarningMacro(<< "No zones in base " << curBaseInfo.name);
    }
    else
    {
      mbase->SetNumberOfBlocks(nzones);
    }

    std::vector<double> baseChildId;
    CGNSRead::getNodeChildrenId(this->cgioNum, baseIds[numBase], baseChildId);

    std::size_t nz;
    std::size_t nn;
    CGNSRead::char_33 nodeLabel;
    for (nz = 0, nn = 0; nn < baseChildId.size(); ++nn)
    {
      if (cgio_get_label(this->cgioNum, baseChildId[nn], nodeLabel) != CG_OK)
      {
        return false;
      }

      if (strcmp(nodeLabel, "Zone_t") == 0)
      {
        if (nz < nn)
        {
          baseChildId[nz] = baseChildId[nn];
        }
        nz++;
      }
      else
      {
        cgio_release_id(this->cgioNum, baseChildId[nn]);
      }
    }
    // so we don't keep ids for released nodes.
    baseChildId.resize(nz);

    int zonemin = baseToZoneRange[numBase][0];
    int zonemax = baseToZoneRange[numBase][1];
    for (int zone = zonemin; zone < zonemax; ++zone)
    {
      CGNSRead::char_33 zoneName;
      cgsize_t zsize[9];
      CGNS_ENUMT(ZoneType_t) zt = CGNS_ENUMV(ZoneTypeNull);
      memset(zoneName, 0, 33);
      memset(zsize, 0, 9 * sizeof(cgsize_t));

      if (cgio_get_name(this->cgioNum, baseChildId[zone], zoneName) != CG_OK)
      {
        char errmsg[CGIO_MAX_ERROR_LENGTH + 1];
        cgio_error_message(errmsg);
        vtkErrorMacro(<< "Problem while reading name of zone number " << zone
                      << ", error : " << errmsg);
        return 1;
      }

      CGNSRead::char_33 dataType;
      if (cgio_get_data_type(this->cgioNum, baseChildId[zone], dataType) != CG_OK)
      {
        char errmsg[CGIO_MAX_ERROR_LENGTH + 1];
        cgio_error_message(errmsg);
        vtkErrorMacro(<< "Problem while reading data_type of zone number " << zone << " "
                      << errmsg);
        return 1;
      }

      if (strcmp(dataType, "I4") == 0)
      {
        std::vector<vtkTypeInt32> mdata;
        CGNSRead::readNodeData<vtkTypeInt32>(this->cgioNum, baseChildId[zone], mdata);
        for (std::size_t index = 0; index < mdata.size(); index++)
        {
          zsize[index] = static_cast<cgsize_t>(mdata[index]);
        }
      }
      else if (strcmp(dataType, "I8") == 0)
      {
        std::vector<vtkTypeInt64> mdata;
        CGNSRead::readNodeData<vtkTypeInt64>(this->cgioNum, baseChildId[zone], mdata);
        for (std::size_t index = 0; index < mdata.size(); index++)
        {
          zsize[index] = static_cast<cgsize_t>(mdata[index]);
        }
      }
      else
      {
        vtkErrorMacro(<< "Problem while reading dimension in zone number " << zone);
        return 1;
      }

      mbase->GetMetaData(zone)->Set(vtkCompositeDataSet::NAME(), zoneName);

      std::string familyName;
      double famId;
      if (CGNSRead::getFirstNodeId(this->cgioNum, baseChildId[zone], "FamilyName_t", &famId) ==
        CG_OK)
      {
        CGNSRead::readNodeStringData(this->cgioNum, famId, familyName);
        cgio_release_id(cgioNum, famId);
        famId = 0;
      }

      if (familyName.empty() == false)
      {
        vtkInformationStringKey* zonefamily = vtkCGNSReader::FAMILY();
        mbase->GetMetaData(zone)->Set(zonefamily, familyName.c_str());
      }

      this->currentId = baseChildId[zone];

      double zoneTypeId;
      zt = CGNS_ENUMV(Structured);
      if (CGNSRead::getFirstNodeId(this->cgioNum, baseChildId[zone], "ZoneType_t", &zoneTypeId) ==
        CG_OK)
      {
        std::string zoneType;
        CGNSRead::readNodeStringData(this->cgioNum, zoneTypeId, zoneType);
        cgio_release_id(cgioNum, zoneTypeId);
        zoneTypeId = 0;

        if (zoneType == "Structured")
        {
          zt = CGNS_ENUMV(Structured);
        }
        else if (zoneType == "Unstructured")
        {
          zt = CGNS_ENUMV(Unstructured);
        }
        else if (zoneType == "Null")
        {
          zt = CGNS_ENUMV(ZoneTypeNull);
        }
        else if (zoneType == "UserDefined")
        {
          zt = CGNS_ENUMV(ZoneTypeUserDefined);
        }
      }

      switch (zt)
      {
        case CGNS_ENUMV(ZoneTypeNull):
          break;
        case CGNS_ENUMV(ZoneTypeUserDefined):
          break;
        case CGNS_ENUMV(Structured):
        {
          ier = GetCurvilinearZone(numBase, zone, cellDim, physicalDim, zsize, mbase);
          if (ier != CG_OK)
          {
            vtkErrorMacro(<< "Error Reading file");
            return 0;
          }

          break;
        }
        case CGNS_ENUMV(Unstructured):
          ier = GetUnstructuredZone(numBase, zone, cellDim, physicalDim, zsize, mbase);
          if (ier != CG_OK)
          {
            vtkErrorMacro(<< "Error Reading file");
            return 0;
          }
          break;
      }
      this->UpdateProgress(0.5);
    }
    rootNode->SetBlock(blockIndex, mbase);
    rootNode->GetMetaData(blockIndex)->Set(vtkCompositeDataSet::NAME(), curBaseInfo.name);
    mbase->Delete();
    blockIndex++;

    // release
    CGNSRead::releaseIds(this->cgioNum, baseChildId);
  }

errorData:
  cgio_close_file(this->cgioNum);

  this->UpdateProgress(1.0);
  return 1;
}

//------------------------------------------------------------------------------
int vtkCGNSReader::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{

  // Setting CAN_HANDLE_PIECE_REQUEST to 1 indicates to the
  // upstream consumer that I can provide the same number of pieces
  // as there are number of processors
  // get the info object
  {
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);
  }

  if (this->ProcRank == 0)
  {
    if (!this->FileName)
    {
      vtkErrorMacro(<< "File name not set\n");
      return 0;
    }

    // First make sure the file exists.  This prevents an empty file
    // from being created on older compilers.
    if (!vtksys::SystemTools::FileExists(this->FileName))
    {
      vtkErrorMacro(<< "Error opening file " << this->FileName);
      return false;
    }

    vtkDebugMacro(<< "CGNSReader::RequestInformation: Parsing file " << this->FileName
                  << " for fields and time steps");

    // Parse the file...
    if (!this->Internals->Internal->Parse(this->FileName))
    {
      vtkErrorMacro(<< "Failed to parse cgns file: " << this->FileName);
      return false;
    }
  } // End_ProcRank_0

  if (this->ProcSize > 1)
  {
    this->Broadcast(this->Controller);
  }

  this->NumberOfBases = this->Internals->Internal->GetNumberOfBaseNodes();

  // Set up time information
  if (!this->Internals->Internal->GetTimes().empty())
  {
    std::vector<double> timeSteps(
      this->Internals->Internal->GetTimes().begin(), this->Internals->Internal->GetTimes().end());

    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &timeSteps.front(),
      static_cast<int>(timeSteps.size()));
    double timeRange[2];
    timeRange[0] = timeSteps.front();
    timeRange[1] = timeSteps.back();
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
  }

  for (int base = 0; base < this->Internals->Internal->GetNumberOfBaseNodes(); ++base)
  {
    const CGNSRead::BaseInformation& curBase = this->Internals->Internal->GetBase(base);
    this->BaseSelection->AddArray(curBase.name, base == 0 ? true : false);

    // add families.
    for (auto& finfo : curBase.family)
    {
      this->FamilySelection->AddArray(finfo.name.c_str());
    }

    // Fill Variable Vertex/Cell names ... perhaps should be improved
    for (const auto& pair : curBase.PointDataArraySelection)
    {
      this->PointDataArraySelection->AddArray(pair.first.c_str(), false);
    }
    for (const auto& pair : curBase.CellDataArraySelection)
    {
      this->CellDataArraySelection->AddArray(pair.first.c_str(), false);
    }
  }
  return 1;
}

//------------------------------------------------------------------------------
void vtkCGNSReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "File Name: " << (this->FileName ? this->FileName : "(none)") << "\n";
  os << indent << "LoadBndPatch: " << this->LoadBndPatch << endl;
  os << indent << "LoadMesh: " << this->LoadMesh << endl;
  os << indent << "CreateEachSolutionAsBlock: " << this->CreateEachSolutionAsBlock << endl;
  os << indent << "IgnoreFlowSolutionPointers: " << this->IgnoreFlowSolutionPointers << endl;
  os << indent << "DistributeBlocks: " << this->DistributeBlocks << endl;
  os << indent << "Controller: " << this->Controller << endl;
}

//------------------------------------------------------------------------------
int vtkCGNSReader::CanReadFile(const char* name)
{
  // return value 0: can not read
  // return value 1: can read
  int cgioFile;
  int ierr = 1;
  double rootNodeId;
  double childId;
  float FileVersion = 0.0;
  int intFileVersion = 0;
  char dataType[CGIO_MAX_DATATYPE_LENGTH + 1];
  char errmsg[CGIO_MAX_ERROR_LENGTH + 1];
  int ndim = 0;
  cgsize_t dimVals[12];
  int fileType = CG_FILE_NONE;

  if (cgio_open_file(name, CG_MODE_READ, CG_FILE_NONE, &cgioFile) != CG_OK)
  {
    cgio_error_message(errmsg);
    vtkErrorMacro(<< "vtkCGNSReader::CanReadFile : " << errmsg);
    return 0;
  }

  cgio_get_root_id(cgioFile, &rootNodeId);
  cgio_get_file_type(cgioFile, &fileType);

  if (cgio_get_node_id(cgioFile, rootNodeId, "CGNSLibraryVersion", &childId))
  {
    cgio_error_message(errmsg);
    vtkErrorMacro(<< "vtkCGNSReader::CanReadFile : " << errmsg);
    ierr = 0;
    goto CanReadError;
  }

  if (cgio_get_data_type(cgioFile, childId, dataType))
  {
    vtkErrorMacro(<< "CGNS Version data type");
    ierr = 0;
    goto CanReadError;
  }

  if (cgio_get_dimensions(cgioFile, childId, &ndim, dimVals))
  {
    vtkErrorMacro(<< "cgio_get_dimensions");
    ierr = 0;
    goto CanReadError;
  }

  // check data type
  if (strcmp(dataType, "R4") != 0)
  {
    vtkErrorMacro(<< "Unexpected data type for CGNS-Library-Version=" << dataType);
    ierr = 0;
    goto CanReadError;
  }

  // check data dim
  if ((ndim != 1) || (dimVals[0] != 1))
  {
    vtkDebugMacro(<< "Wrong data dimension for CGNS-Library-Version");
    ierr = 0;
    goto CanReadError;
  }

  // read data
  if (cgio_read_all_data_type(cgioFile, childId, "R4", &FileVersion))
  {
    vtkErrorMacro(<< "read CGNS version number");
    ierr = 0;
    goto CanReadError;
  }

  // Check that the library version is at least as recent as the one used
  //   to create the file being read

  intFileVersion = static_cast<int>(FileVersion * 1000 + 0.5);

  if (intFileVersion > CGNS_VERSION)
  {
    // This code allows reading version newer than the lib,
    // as long as the 1st digit of the versions are equal
    if ((intFileVersion / 1000) > (CGNS_VERSION / 1000))
    {
      vtkErrorMacro(<< "The file " << name
                    << " was written with a more recent version"
                       "of the CGNS library.  You must update your CGNS"
                       "library before trying to read this file.");
      ierr = 0;
    }
    // warn only if different in second digit
    if ((intFileVersion / 100) > (CGNS_VERSION / 100))
    {
      vtkWarningMacro(<< "The file being read is more recent"
                         "than the CGNS library used");
    }
  }
  if ((intFileVersion / 10) < 255)
  {
    vtkWarningMacro(<< "The file being read was written with an old version"
                       "of the CGNS library. Please update your file"
                       "to a more recent version.");
  }
  vtkDebugMacro(<< "FileVersion=" << FileVersion << "\n");

CanReadError:
  cgio_close_file(cgioFile);
  return ierr ? 1 : 0;
}

//------------------------------------------------------------------------------
int vtkCGNSReader::FillOutputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkMultiBlockDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void vtkCGNSReader::DisableAllPointArrays()
{
  this->PointDataArraySelection->DisableAllArrays();
}

//----------------------------------------------------------------------------
void vtkCGNSReader::EnableAllPointArrays()
{
  this->PointDataArraySelection->EnableAllArrays();
}

//----------------------------------------------------------------------------
int vtkCGNSReader::GetNumberOfPointArrays()
{
  return this->PointDataArraySelection->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
const char* vtkCGNSReader::GetPointArrayName(int index)
{
  if (index >= (int)this->GetNumberOfPointArrays() || index < 0)
  {
    return nullptr;
  }
  else
  {
    return this->PointDataArraySelection->GetArrayName(index);
  }
}

//----------------------------------------------------------------------------
int vtkCGNSReader::GetPointArrayStatus(const char* name)
{
  return this->PointDataArraySelection->ArrayIsEnabled(name);
}

//----------------------------------------------------------------------------
void vtkCGNSReader::SetPointArrayStatus(const char* name, int status)
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

//----------------------------------------------------------------------------
void vtkCGNSReader::DisableAllCellArrays()
{
  this->CellDataArraySelection->DisableAllArrays();
}

//----------------------------------------------------------------------------
void vtkCGNSReader::EnableAllCellArrays()
{
  this->CellDataArraySelection->EnableAllArrays();
}

//----------------------------------------------------------------------------
int vtkCGNSReader::GetNumberOfCellArrays()
{
  return this->CellDataArraySelection->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
const char* vtkCGNSReader::GetCellArrayName(int index)
{
  if (index >= (int)this->GetNumberOfCellArrays() || index < 0)
  {
    return nullptr;
  }
  else
  {
    return this->CellDataArraySelection->GetArrayName(index);
  }
}

//----------------------------------------------------------------------------
int vtkCGNSReader::GetCellArrayStatus(const char* name)
{
  return this->CellDataArraySelection->ArrayIsEnabled(name);
}

//----------------------------------------------------------------------------
void vtkCGNSReader::SetCellArrayStatus(const char* name, int status)
{
  if (status)
  {
    this->CellDataArraySelection->EnableArray(name);
  }
  else
  {
    this->CellDataArraySelection->DisableArray(name);
  }
}

//------------------------------------------------------------------------------
void vtkCGNSReader::Broadcast(vtkMultiProcessController* ctrl)
{
  if (ctrl)
  {
    int rank = ctrl->GetLocalProcessId();
    this->Internals->Internal->Broadcast(ctrl, rank);
  }
}

//----------------------------------------------------------------------------
void vtkCGNSReader::DisableAllBases()
{
  this->BaseSelection->DisableAllArrays();
}

//----------------------------------------------------------------------------
void vtkCGNSReader::EnableAllBases()
{
  this->BaseSelection->EnableAllArrays();
}

//----------------------------------------------------------------------------
int vtkCGNSReader::GetNumberOfBaseArrays()
{
  return this->BaseSelection->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
int vtkCGNSReader::GetBaseArrayStatus(const char* name)
{
  return this->BaseSelection->GetArraySetting(name);
}

//----------------------------------------------------------------------------
void vtkCGNSReader::SetBaseArrayStatus(const char* name, int status)
{
  this->BaseSelection->SetArraySetting(name, status);
}

//----------------------------------------------------------------------------
const char* vtkCGNSReader::GetBaseArrayName(int index)
{
  return this->BaseSelection->GetArrayName(index);
}

//----------------------------------------------------------------------------
vtkDataArraySelection* vtkCGNSReader::GetBaseSelection()
{
  return this->BaseSelection;
}

//----------------------------------------------------------------------------
int vtkCGNSReader::GetNumberOfFamilyArrays()
{
  return this->FamilySelection->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
const char* vtkCGNSReader::GetFamilyArrayName(int index)
{
  return this->FamilySelection->GetArrayName(index);
}

//----------------------------------------------------------------------------
void vtkCGNSReader::SetFamilyArrayStatus(const char* name, int status)
{
  this->FamilySelection->SetArraySetting(name, status);
}

//----------------------------------------------------------------------------
int vtkCGNSReader::GetFamilyArrayStatus(const char* name)
{
  return this->FamilySelection->GetArraySetting(name);
}

//----------------------------------------------------------------------------
void vtkCGNSReader::EnableAllFamilies()
{
  this->FamilySelection->EnableAllArrays();
}

//----------------------------------------------------------------------------
void vtkCGNSReader::DisableAllFamilies()
{
  this->FamilySelection->DisableAllArrays();
}

//----------------------------------------------------------------------------
vtkDataArraySelection* vtkCGNSReader::GetFamilySelection()
{
  return this->FamilySelection;
}

//----------------------------------------------------------------------------
void vtkCGNSReader::SetCacheMesh(bool enable)
{
  this->CacheMesh = enable;
  if (!enable)
  {
    this->Internals->MeshPointsCache.ClearCache();
  }
}

//----------------------------------------------------------------------------
void vtkCGNSReader::SetCacheConnectivity(bool enable)
{
  this->CacheConnectivity = enable;
  if (!enable)
  {
    this->Internals->ConnectivitiesCache.ClearCache();
  }
}

//==============================================================================
#ifdef _WINDOWS
#pragma warning(pop)
#endif
