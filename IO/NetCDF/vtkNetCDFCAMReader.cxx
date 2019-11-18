/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNetCDFCAMReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkNetCDFCAMReader.h"

#include "vtkCallbackCommand.h"
#include "vtkCellArray.h"
#include "vtkDataArraySelection.h"
#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkFloatArray.h"
#include "vtkIncrementalOctreePointLocator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolygon.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"

#include <set>
#include <sstream>
#include <vector>
#include <vtk_netcdf.h>

namespace
{
// determine if this is a cell that wraps from 360 to 0 (i.e. if it's
// a cell that wraps from the right side of the domain to the left side)
bool IsCellInverted(double points[4][3])
{
  // We test the normal 3 points at a time. Not all grids are well-behaved
  // i.e. consistently use 0 or 360. We've had grid where 3 points on the left
  // side, and just 1 on the right. Just checking the first 3 points (which is
  // what ComputeNormal() does, we may (and do) miss a few cells.
  // See BUG #0014897.
  double normal[3];
  vtkPolygon::ComputeNormal(3, points[0], normal);
  if (normal[2] > 0)
  {
    return true;
  }
  vtkPolygon::ComputeNormal(3, points[1], normal);
  if (normal[2] > 0)
  {
    return true;
  }
  return false;
}

template <class T>
inline bool IsZero(const T& val)
{
  return std::abs(val) < std::numeric_limits<T>::epsilon();
}
}

class vtkNetCDFCAMReader::Internal
{
public:
  Internal(vtkNetCDFCAMReader* r)
    : reader(r)
    , nc_points(-1)
    , nc_connectivity(-1)
  {
  }
  ~Internal()
  {
    closePoints();
    closeConnectivity();
  }

  bool open(const char* file, int* ncfile)
  {
    int mode = NC_NOWRITE | NC_NETCDF4 | NC_CLASSIC_MODEL;
    int ncid;
    if (nc_err(nc_open(file, mode, &ncid)))
    {
      return false;
    }
    *ncfile = ncid;
    return true;
  }
  bool openPoints(const char* file) { return open(file, &nc_points); }
  bool openConnectivity(const char* file) { return open(file, &nc_connectivity); }
  void closePoints()
  {
    if (nc_points != -1)
    {
      nc_err(nc_close(nc_points));
      nc_points = -1;
    }
  }

  void closeConnectivity()
  {
    if (nc_connectivity != -1)
    {
      nc_err(nc_close(nc_connectivity));
      nc_connectivity = -1;
    }
  }

  bool nc_err(int nc_ret, bool msg_on_err = true) const;

  std::string GetNameDimension(int ncid, int varid) const;

  vtkNetCDFCAMReader* reader;
  int nc_points;
  int nc_connectivity;
};

bool vtkNetCDFCAMReader::Internal::nc_err(int nc_ret, bool msg_on_err) const
{
  if (nc_ret == NC_NOERR)
  {
    return false;
  }

  if (msg_on_err)
  {
    vtkErrorWithObjectMacro(reader, << "NetCDF error: " << nc_strerror(nc_ret));
  }
  return true;
}

std::string vtkNetCDFCAMReader::Internal::GetNameDimension(int nc_file, int nc_var) const
{
  int ndims;
  if (nc_err(nc_inq_varndims(nc_file, nc_var, &ndims)))
  {
    return "";
  }
  if (ndims < 2)
  {
    return "";
  }

  int dims[NC_MAX_VAR_DIMS];
  if (nc_err(nc_inq_vardimid(nc_file, nc_var, dims)))
  {
    return "";
  }

  std::ostringstream postfix;

  char ncname[NC_MAX_NAME + 1];
  if (nc_err(nc_inq_dimname(nc_file, dims[0], ncname)))
  {
    return "";
  }
  postfix << "[" << ncname;
  if (nc_err(nc_inq_dimname(nc_file, dims[1], ncname)))
  {
    return "";
  }
  postfix << "," << ncname;

  if (ndims > 2)
  {
    if (nc_err(nc_inq_dimname(nc_file, dims[2], ncname)))
    {
      return "";
    }
    postfix << ", " << ncname;
  }
  postfix << "]";

  if (nc_err(nc_inq_varname(nc_file, nc_var, ncname)))
  {
    return "";
  }
  std::ostringstream name;
  name << ncname << " " << postfix.str();
  return name.str();
}

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkNetCDFCAMReader);

//----------------------------------------------------------------------------
vtkNetCDFCAMReader::vtkNetCDFCAMReader()
{
  this->FileName = nullptr;
  this->CurrentFileName = nullptr;
  this->ConnectivityFileName = nullptr;
  this->CurrentConnectivityFileName = nullptr;
  this->VerticalDimension = VERTICAL_DIMENSION_MIDPOINT_LAYERS;
  this->TimeSteps = nullptr;
  this->NumberOfTimeSteps = 0;
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  this->PointDataArraySelection = vtkDataArraySelection::New();
  this->SelectionObserver = vtkCallbackCommand::New();
  this->SelectionObserver->SetCallback(&vtkNetCDFCAMReader::SelectionCallback);
  this->SelectionObserver->SetClientData(this);
  this->PointDataArraySelection->AddObserver(vtkCommand::ModifiedEvent, this->SelectionObserver);

  this->SingleMidpointLayer = 0;
  this->MidpointLayerIndex = 0;
  this->MidpointLayersRange[0] = 0;
  this->MidpointLayersRange[1] = 1;

  this->SingleInterfaceLayer = 0;
  this->InterfaceLayerIndex = 0;
  this->InterfaceLayersRange[0] = 0;
  this->InterfaceLayersRange[1] = 1;

  this->Internals = new Internal(this);
}

//----------------------------------------------------------------------------
vtkNetCDFCAMReader::~vtkNetCDFCAMReader()
{
  this->SetFileName(nullptr);
  this->SetCurrentFileName(nullptr);
  this->SetConnectivityFileName(nullptr);
  this->SetCurrentConnectivityFileName(nullptr);
  delete[] this->TimeSteps;
  this->TimeSteps = nullptr;
  this->PointDataArraySelection->Delete();
  this->PointDataArraySelection = nullptr;
  this->SelectionObserver->Delete();
  this->SelectionObserver = nullptr;

  delete this->Internals;
}

//----------------------------------------------------------------------------
int vtkNetCDFCAMReader::CanReadFile(const char* fileName)
{
  Internal* internals = new Internal(nullptr);
  if (!internals->openPoints(fileName))
  {
    delete internals;
    return 0;
  }
  delete internals;
  return 1;
}

//----------------------------------------------------------------------------
void vtkNetCDFCAMReader::SetFileName(const char* fileName)
{
  vtkDebugMacro(<< " setting FileName to " << (fileName ? fileName : "(null)"));
  if (this->FileName == nullptr && fileName == nullptr)
  {
    return;
  }
  if (this->FileName && fileName && (!strcmp(this->FileName, fileName)))
  {
    return;
  }
  this->Internals->closePoints();
  delete[] this->FileName;
  this->FileName = nullptr;
  if (fileName && *fileName)
  {
    this->FileName = new char[strlen(fileName) + 1];
    strcpy(this->FileName, fileName);
  }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkNetCDFCAMReader::SetConnectivityFileName(const char* fileName)
{
  vtkDebugMacro(<< " setting ConnectivityFileName to " << (fileName ? fileName : "(null)"));
  if (this->ConnectivityFileName == nullptr && fileName == nullptr)
  {
    return;
  }
  if (this->ConnectivityFileName && fileName && (!strcmp(this->ConnectivityFileName, fileName)))
  {
    return;
  }
  this->Internals->closeConnectivity();
  delete[] this->ConnectivityFileName;
  this->ConnectivityFileName = nullptr;
  if (fileName && *fileName)
  {
    this->ConnectivityFileName = new char[strlen(fileName) + 1];
    strcpy(this->ConnectivityFileName, fileName);
  }
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkNetCDFCAMReader::RequestInformation(vtkInformation* vtkNotUsed(reqInfo),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  if (this->FileName == nullptr)
  {
    vtkWarningMacro("Missing a file name.");
    return 0;
  }

  if (this->CurrentFileName != nullptr && strcmp(this->CurrentFileName, this->FileName) != 0)
  {
    this->Internals->closePoints();
    this->PointDataArraySelection->RemoveAllArrays();
    this->SetCurrentFileName(nullptr);
  }
  if (this->Internals->nc_points == -1)
  {
    if (!this->Internals->openPoints(this->FileName))
    {
      vtkErrorMacro(<< "Can't read file " << this->FileName);
      return 0;
    }
    this->SetCurrentFileName(this->FileName);
    this->BuildVarArray();
    int dimid;
    if (!this->Internals->nc_err(nc_inq_dimid(this->Internals->nc_points, "lev", &dimid), false))
    {
      size_t size;
      if (this->Internals->nc_err(nc_inq_dimlen(this->Internals->nc_points, dimid, &size)))
      {
        return 0;
      }
      this->MidpointLayersRange[1] = static_cast<int>(size - 1);
    }
    if (!this->Internals->nc_err(nc_inq_dimid(this->Internals->nc_points, "ilev", &dimid), false))
    {
      size_t size;
      if (this->Internals->nc_err(nc_inq_dimlen(this->Internals->nc_points, dimid, &size)))
      {
        return 0;
      }
      this->InterfaceLayersRange[1] = static_cast<int>(size - 1);
    }
  }
  int dimid;
  if (this->Internals->nc_err(nc_inq_dimid(this->Internals->nc_points, "time", &dimid)))
  {
    vtkErrorMacro("Cannot find the number of time steps (time dimension).");
    return 0;
  }
  size_t size;
  if (this->Internals->nc_err(nc_inq_dimlen(this->Internals->nc_points, dimid, &size)))
  {
    return 0;
  }
  this->NumberOfTimeSteps = size;
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  if (this->NumberOfTimeSteps > 0)
  {
    delete[] this->TimeSteps;
    this->TimeSteps = new double[this->NumberOfTimeSteps];
    int varid;
    if (this->Internals->nc_err(nc_inq_varid(this->Internals->nc_points, "time", &varid)))
    {
      return 0;
    }
    size_t start[] = { 0 };
    size_t count[] = { this->NumberOfTimeSteps };
    if (this->Internals->nc_err(
          nc_get_vara_double(this->Internals->nc_points, varid, start, count, this->TimeSteps)))
    {
      return 0;
    }

    // Tell the pipeline what steps are available
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), this->TimeSteps,
      static_cast<int>(this->NumberOfTimeSteps));

    // Range is required to get GUI to show things
    double tRange[2] = { this->TimeSteps[0], this->TimeSteps[this->NumberOfTimeSteps - 1] };
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), tRange, 2);
  }
  else
  {
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
  }

  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);

  return 1;
}

//----------------------------------------------------------------------------
void vtkNetCDFCAMReader::BuildVarArray()
{
  std::vector<std::set<std::string> > varsnames(VERTICAL_DIMENSION_COUNT);
  int nvars;
  int vars[NC_MAX_VARS];
  if (this->Internals->nc_err(nc_inq_varids(this->Internals->nc_points, &nvars, vars)))
  {
    return;
  }
  for (int i = 0; i < nvars; i++)
  {
    bool showVar = false;
    enum VerticalDimension verticalDimension = VERTICAL_DIMENSION_SINGLE_LAYER;
    int ndims;
    if (this->Internals->nc_err(nc_inq_varndims(this->Internals->nc_points, vars[i], &ndims)))
    {
      continue;
    }
    int dims[NC_MAX_VAR_DIMS];
    if (this->Internals->nc_err(nc_inq_vardimid(this->Internals->nc_points, vars[i], dims)))
    {
      continue;
    }
    if (ndims == 3)
    {
      bool ok = true;
      char name[NC_MAX_NAME + 1];
      if (this->Internals->nc_err(nc_inq_dimname(this->Internals->nc_points, dims[0], name)))
      {
        continue;
      }
      ok = ok && (strcmp(name, "time") == 0);

      if (this->Internals->nc_err(nc_inq_dimname(this->Internals->nc_points, dims[1], name)))
      {
        continue;
      }
      ok = ok && (strcmp(name, "lev") == 0 || strcmp(name, "ilev") == 0);
      verticalDimension = (strcmp(name, "lev") == 0) ? VERTICAL_DIMENSION_MIDPOINT_LAYERS
                                                     : VERTICAL_DIMENSION_INTERFACE_LAYERS;

      if (this->Internals->nc_err(nc_inq_dimname(this->Internals->nc_points, dims[2], name)))
      {
        continue;
      }
      ok = ok && (strcmp(name, "ncol") == 0);

      if (ok)
      {
        showVar = true;
      }
    }
    else if (ndims == 2)
    {
      bool ok = true;
      char name[NC_MAX_NAME + 1];
      if (this->Internals->nc_err(nc_inq_dimname(this->Internals->nc_points, dims[0], name)))
      {
        continue;
      }
      ok = ok && (strcmp(name, "time") == 0);

      if (this->Internals->nc_err(nc_inq_dimname(this->Internals->nc_points, dims[1], name)))
      {
        continue;
      }
      ok = ok && (strcmp(name, "ncol") == 0);

      if (ok)
      {
        verticalDimension = VERTICAL_DIMENSION_SINGLE_LAYER;
        showVar = true;
      }
    }
    if (showVar)
    {
      varsnames[verticalDimension].insert(
        this->Internals->GetNameDimension(this->Internals->nc_points, vars[i]));
    }
  }
  for (int i = 0; i < VERTICAL_DIMENSION_COUNT; ++i)
  {
    for (std::set<std::string>::iterator it = varsnames[i].begin(); it != varsnames[i].end(); ++it)
    {
      this->PointDataArraySelection->EnableArray(it->c_str());
    }
  }
}

//----------------------------------------------------------------------------
int vtkNetCDFCAMReader::RequestUpdateExtent(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  if (this->FileName == nullptr || this->ConnectivityFileName == nullptr)
  {
    vtkWarningMacro("Missing a file name.");
    return 0;
  }
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  int piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int numPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  // make sure piece is valid
  if (piece < 0 || piece >= numPieces)
  {
    return 0;
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkNetCDFCAMReader::GetNumberOfPointArrays()
{
  return this->PointDataArraySelection->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
const char* vtkNetCDFCAMReader::GetPointArrayName(int index)
{
  return this->PointDataArraySelection->GetArrayName(index);
}

//----------------------------------------------------------------------------
int vtkNetCDFCAMReader::GetPointArrayStatus(const char* name)
{
  return this->PointDataArraySelection->ArrayIsEnabled(name);
}

//----------------------------------------------------------------------------
void vtkNetCDFCAMReader::SetPointArrayStatus(const char* name, int status)
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
void vtkNetCDFCAMReader::DisableAllPointArrays()
{
  this->PointDataArraySelection->DisableAllArrays();
}

//----------------------------------------------------------------------------
void vtkNetCDFCAMReader::EnableAllPointArrays()
{
  this->PointDataArraySelection->EnableAllArrays();
}

//----------------------------------------------------------------------------
int vtkNetCDFCAMReader::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  if (this->FileName == nullptr || this->ConnectivityFileName == nullptr)
  {
    vtkWarningMacro("Missing a file name.");
    return 0;
  }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkUnstructuredGrid* output =
    vtkUnstructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDebugMacro(<< "Reading NetCDF CAM file.");
  this->UpdateProgress(0);
  if (this->CurrentConnectivityFileName != nullptr &&
    strcmp(this->CurrentConnectivityFileName, this->ConnectivityFileName) != 0)
  {
    this->Internals->closeConnectivity();
    this->SetCurrentConnectivityFileName(nullptr);
  }
  if (this->Internals->nc_connectivity == -1)
  {
    if (!this->Internals->openConnectivity(this->ConnectivityFileName))
    {
      vtkErrorMacro(<< "Can't read file " << this->ConnectivityFileName);
      return 0;
    }
    this->SetCurrentConnectivityFileName(this->ConnectivityFileName);
  }

  // read in the points first
  size_t numLevels = 1; // value for single level
  const char* levName = nullptr;
  int levelsid;
  if (this->VerticalDimension == VERTICAL_DIMENSION_MIDPOINT_LAYERS ||
    this->VerticalDimension == VERTICAL_DIMENSION_INTERFACE_LAYERS)
  {
    levName = (this->VerticalDimension == VERTICAL_DIMENSION_MIDPOINT_LAYERS) ? "lev" : "ilev";
    int dimid;
    if (this->Internals->nc_err(nc_inq_dimid(this->Internals->nc_points, levName, &dimid)))
    {
      vtkErrorMacro("Cannot find the number of levels (lev dimension).");
      return 0;
    }
    if (this->Internals->nc_err(nc_inq_dimlen(this->Internals->nc_points, dimid, &numLevels)))
    {
      return 0;
    }
    if (this->Internals->nc_err(nc_inq_varid(this->Internals->nc_points, levName, &levelsid)))
    {
      vtkErrorMacro("Cannot find the number of levels (lev variable).");
      return 0;
    }
    int ndims;
    if (this->Internals->nc_err(nc_inq_varndims(this->Internals->nc_points, levelsid, &ndims)))
    {
      return 0;
    }
    int dims[NC_MAX_VAR_DIMS];
    if (this->Internals->nc_err(nc_inq_vardimid(this->Internals->nc_points, levelsid, dims)))
    {
      return 0;
    }
    size_t size;
    if (this->Internals->nc_err(nc_inq_dimlen(this->Internals->nc_points, dims[0], &size)))
    {
      return 0;
    }
    if (ndims != 1 || size != numLevels)
    {
      vtkErrorMacro("The lev variable is not consistent.");
      return 0;
    }
  }
  int dimid;
  if (this->Internals->nc_err(nc_inq_dimid(this->Internals->nc_points, "ncol", &dimid)))
  {
    vtkErrorMacro("Cannot find the number of points (ncol dimension).");
    return 0;
  }
  int lonid;
  if (this->Internals->nc_err(nc_inq_varid(this->Internals->nc_points, "lon", &lonid)))
  {
    vtkErrorMacro("Cannot find coordinates (lon variable).");
    return 0;
  }
  int latid;
  if (this->Internals->nc_err(nc_inq_varid(this->Internals->nc_points, "lat", &latid)))
  {
    vtkErrorMacro("Cannot find coordinates (lat variable).");
    return 0;
  }
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  output->SetPoints(points);

  size_t numFilePoints;
  if (this->Internals->nc_err(nc_inq_dimlen(this->Internals->nc_points, dimid, &numFilePoints)))
  {
    return 0;
  }

  nc_type var_type;
  if (this->Internals->nc_err(nc_inq_vartype(this->Internals->nc_points, latid, &var_type)))
  {
    return 0;
  }
  if (var_type == NC_DOUBLE)
  {
    points->SetDataTypeToDouble();
    points->SetNumberOfPoints(static_cast<int>(numFilePoints));
    std::vector<double> array(numFilePoints * 2);
    size_t start[] = { 0 };
    size_t count[] = { numFilePoints };
    if (this->Internals->nc_err(
          nc_get_vara_double(this->Internals->nc_points, lonid, start, count, &array[0])))
    {
      return 0;
    }
    if (this->Internals->nc_err(nc_get_vara_double(
          this->Internals->nc_points, latid, start, count, &array[numFilePoints])))
    {
      return 0;
    }
    for (size_t i = 0; i < numFilePoints; i++)
    {
      points->SetPoint(static_cast<vtkIdType>(i), array[i], array[i + numFilePoints], numLevels);
    }
  }
  else
  {
    points->SetDataTypeToFloat();
    points->SetNumberOfPoints(static_cast<int>(numFilePoints));
    std::vector<float> array(numFilePoints * 2);
    size_t start[] = { 0 };
    size_t count[] = { numFilePoints };
    if (this->Internals->nc_err(
          nc_get_vara_float(this->Internals->nc_points, lonid, start, count, &array[0])))
    {
      return 0;
    }
    if (this->Internals->nc_err(nc_get_vara_float(
          this->Internals->nc_points, latid, start, count, &array[numFilePoints])))
    {
      return 0;
    }
    for (size_t i = 0; i < numFilePoints; i++)
    {
      points->SetPoint(
        static_cast<vtkIdType>(i), array[i], array[i + numFilePoints], numLevels - 1);
    }
  }
  this->UpdateProgress(.25); // educated guess for progress

  // now read in the cell connectivity.  note that this is a periodic
  // domain and only the points on the left boundary are included in
  // the points file.  if a cell uses a point that is on the left
  // boundary and it should be on the right boundary we will have
  // to create that point.  That's what boundaryPoints is used for.
  // The (index + numFilePoints) gives us the new point id, and the value
  // for that in this array will correspond to the original point id that the
  // boundaryPoint is duplicate of.
  std::vector<vtkIdType> boundaryPoints;

  // To avoid creating multiple duplicates, we create a
  // vtkIncrementalOctreePointLocator.
  vtkSmartPointer<vtkIncrementalOctreePointLocator> locator =
    vtkSmartPointer<vtkIncrementalOctreePointLocator>::New();
  locator->SetDataSet(output); // dataset only has points right now.
  locator->BuildLocator();

  if (this->Internals->nc_err(nc_inq_dimid(this->Internals->nc_connectivity, "ncells", &dimid)))
  {
    vtkErrorMacro("Cannot find the number of cells (ncells dimension).");
    return 0;
  }
  int connid;
  if (this->Internals->nc_err(
        nc_inq_varid(this->Internals->nc_connectivity, "element_corners", &connid)))
  {
    vtkErrorMacro("Cannot find cell connectivity (element_corners dimension).");
    return 0;
  }
  size_t numCellsPerLevel;
  if (this->Internals->nc_err(
        nc_inq_dimlen(this->Internals->nc_connectivity, dimid, &numCellsPerLevel)))
  {
    return 0;
  }

  size_t piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  size_t numPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  size_t originalNumLevels = numLevels;
  if ((this->VerticalDimension == VERTICAL_DIMENSION_MIDPOINT_LAYERS &&
        this->SingleMidpointLayer) ||
    (this->VerticalDimension == VERTICAL_DIMENSION_INTERFACE_LAYERS && this->SingleInterfaceLayer))
  {
    numLevels = 1;
  }

  size_t beginLevel, endLevel, beginCell, endCell;
  if (!this->GetPartitioning(
        piece, numPieces, numLevels, numCellsPerLevel, beginLevel, endLevel, beginCell, endCell))
  {
    return 0;
  }
  // the cells/levels assigned to this piece
  size_t numLocalCells = endCell - beginCell;
  size_t numLocalLevels = endLevel - beginLevel + 1;
  std::vector<int> cellConnectivity(4 * numLocalCells);
  size_t start_conn[] = { 0, static_cast<size_t>(beginCell) };
  size_t count_conn[] = { 4, static_cast<size_t>(numLocalCells) };
  if (this->Internals->nc_err(nc_get_vara_int(
        this->Internals->nc_connectivity, connid, start_conn, count_conn, &cellConnectivity[0])))
  {
    return 0;
  }

  for (size_t i = 0; i < numLocalCells; i++)
  {
    vtkIdType pointIds[4];
    double coords[4][3]; // assume quads here
    for (int j = 0; j < 4; j++)
    {
      pointIds[j] = cellConnectivity[i + j * numLocalCells] - 1;
      points->GetPoint(pointIds[j], coords[j]);
    }
    if (IsCellInverted(coords) == true)
    {
      // First decide whether we're putting this cell on the 360 side (right) or on the
      // 0 side (left). We decide this based on which side will have the
      // smallest protrusion.
      double delta = 0.0;
      bool anchorLeft = false;
      for (int j = 0; j < 4; ++j)
      {
        // We're assured that coords[j][0] is in the range [0, 360].
        // We just that fact to avoid having to do a std::abs() here.
        double rightDelta = (360.0 - coords[j][0]);
        double leftDelta = coords[j][0]; // i.e. (coords[j][0] - 0.0).
        if (IsZero(rightDelta) || IsZero(leftDelta) || rightDelta == leftDelta)
        {
          // if the point is equidistant from both ends or is one of the ends,
          // we let the other points in this cell dictate where the cell should
          // anchor since this point can easily be anchored on either side with
          // no side effects.
          continue;
        }
        if (rightDelta < leftDelta)
        {
          if (rightDelta > delta)
          {
            delta = rightDelta;
            anchorLeft = false;
          }
        }
        else
        {
          if (leftDelta > delta)
          {
            delta = leftDelta;
            anchorLeft = true;
          }
        }
      }
      // Once we've decided where we're anchoring we adjust the points.
      for (int j = 0; j < 4; ++j)
      {
        if (anchorLeft)
        {
          // if coords[j] is closer to right (360), move it to the left.
          if ((360.0 - coords[j][0]) < coords[j][0])
          {
            coords[j][0] -= 360.0;
          }
          else
          {
            continue;
          }
        }
        else
        {
          // if coords[j] is closer to left (0), move it to the right
          if (coords[j][0] < (360.0 - coords[j][0]))
          {
            coords[j][0] += 360.0;
          }
          else
          {
            continue;
          }
        }
        // Okay, we have moved the coords. Update the boundaryPoints so which
        // original point id is this new point id a clone of.
        vtkIdType newPtId;
        if (locator->InsertUniquePoint(coords[j], newPtId) == 1)
        {
          // if a new point was indeed inserted, we need to update the
          // boundaryPoints to keep track of it.
          assert(newPtId >= static_cast<vtkIdType>(numFilePoints) && pointIds[j] < newPtId);
          assert(static_cast<size_t>(boundaryPoints.size()) == (newPtId - numFilePoints));
          boundaryPoints.push_back(pointIds[j]);
        }
        cellConnectivity[i + j * numLocalCells] = static_cast<int>(newPtId + 1); // note: 1-indexed.
      }
    }
  }
  locator = nullptr; // release the locator memory.

  // we now have all of the points at a single level.  build them up
  // for the rest of the levels before creating the cells.
  vtkIdType numPointsPerLevel = points->GetNumberOfPoints();
  if (this->VerticalDimension != VERTICAL_DIMENSION_SINGLE_LAYER &&
    // we load all levels
    originalNumLevels == numLevels)
  {
    // a hacky way to resize the points array without resetting the data
    points->InsertPoint(static_cast<vtkIdType>(numPointsPerLevel * numLocalLevels - 1), 0, 0, 0);
    for (vtkIdType pt = 0; pt < numPointsPerLevel; pt++)
    {
      double point[3];
      points->GetPoint(pt, point);
      // need to start at 0 here since for multiple process the first
      // level will need to be replaced
      for (size_t lev = 0; lev < numLocalLevels; lev++)
      {
        point[2] = numLevels - lev - beginLevel - 1;
        points->SetPoint(static_cast<vtkIdType>(pt + lev * numPointsPerLevel), point);
      }
    }
  }

  points->Modified();
  points->Squeeze();

  this->UpdateProgress(.5); // educated guess for progress

  // Collect the time step requested
  vtkInformationDoubleKey* timeKey =
    static_cast<vtkInformationDoubleKey*>(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

  double dTime = 0.0;
  if (outInfo->Has(timeKey))
  {
    dTime = outInfo->Get(timeKey);
  }

  // Actual time for the time step
  output->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), dTime);

  // Index of the time step to request
  size_t timeStep = 0;
  while (timeStep < this->NumberOfTimeSteps && this->TimeSteps[timeStep] < dTime)
  {
    timeStep++;
  }

  // now that we have the full set of points, read in any point
  // data with dimensions (time, lev, ncol) but read them in
  // by chunks of ncol since it will be a pretty big chunk of
  // memory that we'll have to break up anyways.
  int nvars;
  int vars[NC_MAX_VARS];
  if (this->Internals->nc_err(nc_inq_varids(this->Internals->nc_points, &nvars, vars)))
  {
    return 0;
  }
  for (int i = 0; i < nvars; i++)
  {
    int ndims;
    if (this->Internals->nc_err(nc_inq_varndims(this->Internals->nc_points, vars[i], &ndims)))
    {
      return 0;
    }
    int dims[NC_MAX_VAR_DIMS];
    if (this->Internals->nc_err(nc_inq_vardimid(this->Internals->nc_points, vars[i], dims)))
    {
      return 0;
    }
    if (this->VerticalDimension != VERTICAL_DIMENSION_SINGLE_LAYER)
    { // check for a 3D field variable
      if (ndims != 3)
      {
        continue;
      }

      char name[NC_MAX_NAME + 1];
      if (this->Internals->nc_err(nc_inq_dimname(this->Internals->nc_points, dims[0], name)))
      {
        return 0;
      }
      if (strcmp(name, "time") != 0)
      {
        continue;
      }
      if (this->Internals->nc_err(nc_inq_dimname(this->Internals->nc_points, dims[1], name)))
      {
        return 0;
      }
      if (strcmp(name, levName) != 0)
      {
        continue;
      }
      if (this->Internals->nc_err(nc_inq_dimname(this->Internals->nc_points, dims[2], name)))
      {
        return 0;
      }
      if (strcmp(name, "ncol") != 0)
      {
        continue;
      }
    }
    else
    { // check for a 2D field variable
      if (ndims != 2)
      {
        continue;
      }

      char name[NC_MAX_NAME + 1];
      if (this->Internals->nc_err(nc_inq_dimname(this->Internals->nc_points, dims[0], name)))
      {
        return 0;
      }
      if (strcmp(name, "time") != 0)
      {
        continue;
      }
      if (this->Internals->nc_err(nc_inq_dimname(this->Internals->nc_points, dims[1], name)))
      {
        return 0;
      }
      if (strcmp(name, "ncol") != 0)
      {
        continue;
      }
    }

    if (!this->PointDataArraySelection->GetArraySetting(
          this->Internals->GetNameDimension(this->Internals->nc_points, vars[i]).c_str()))
    {
      // not enabled
      continue;
    }

    vtkDoubleArray* doubleArray = nullptr;
    vtkFloatArray* floatArray = nullptr;
    if (this->Internals->nc_err(nc_inq_vartype(this->Internals->nc_points, vars[i], &var_type)))
    {
      return 0;
    }
    char varname[NC_MAX_NAME + 1];
    if (this->Internals->nc_err(nc_inq_varname(this->Internals->nc_points, vars[i], varname)))
    {
      return 0;
    }
    if (var_type == NC_DOUBLE)
    {
      doubleArray = vtkDoubleArray::New();
      doubleArray->SetNumberOfTuples(points->GetNumberOfPoints());
      doubleArray->SetName(varname);
      output->GetPointData()->AddArray(doubleArray);
      doubleArray->Delete();
    }
    else
    {
      floatArray = vtkFloatArray::New();
      floatArray->SetNumberOfTuples(points->GetNumberOfPoints());
      floatArray->SetName(varname);
      output->GetPointData()->AddArray(floatArray);
      floatArray->Delete();
    }
    if (this->VerticalDimension != VERTICAL_DIMENSION_SINGLE_LAYER)
    {
      for (size_t lev = 0; lev < numLocalLevels; lev++)
      {
        size_t start[] = { static_cast<size_t>(timeStep), static_cast<size_t>(lev + beginLevel),
          0 };
        size_t count[] = { 1, 1, numFilePoints };
        if (doubleArray)
        {
          if (this->Internals->nc_err(nc_get_vara_double(this->Internals->nc_points, vars[i], start,
                count, doubleArray->GetPointer(0) + lev * numPointsPerLevel)))
          {
            vtkErrorMacro("Problem getting NetCDF variable " << varname);
            return 0;
          }
        }
        else
        {
          if (this->Internals->nc_err(nc_get_vara_float(this->Internals->nc_points, vars[i], start,
                count, floatArray->GetPointer(0) + lev * numPointsPerLevel)))
          {
            vtkErrorMacro("Problem getting NetCDF variable " << varname);
            return 0;
          }
        }
      }
    }
    else
    {
      size_t start[] = { static_cast<size_t>(timeStep), 0 };
      size_t count[] = { 1, numFilePoints };
      if (doubleArray)
      {
        if (this->Internals->nc_err(nc_get_vara_double(
              this->Internals->nc_points, vars[i], start, count, doubleArray->GetPointer(0))))
        {
          vtkErrorMacro("Problem getting NetCDF variable " << varname);
          return 0;
        }
      }
      else
      {
        if (this->Internals->nc_err(nc_get_vara_float(
              this->Internals->nc_points, vars[i], start, count, floatArray->GetPointer(0))))
        {
          vtkErrorMacro("Problem getting NetCDF variable " << varname);
          return 0;
        }
      }
    }
  }

  // we have to copy the values from the left side to the right side
  vtkPointData* pointData = output->GetPointData();
  pointData->CopyAllOn();
  pointData->CopyAllocate(output->GetPointData(), output->GetNumberOfPoints());

  vtkIdType newPtId = 0;
  for (std::vector<vtkIdType>::const_iterator it = boundaryPoints.begin();
       it != boundaryPoints.end(); ++it, ++newPtId)
  {
    for (size_t lev = 0; lev < numLocalLevels; lev++)
    {
      vtkIdType srcId = static_cast<vtkIdType>((*it) + lev * numPointsPerLevel);
      vtkIdType destId =
        static_cast<vtkIdType>((newPtId + numFilePoints) + lev * numPointsPerLevel);
      pointData->CopyData(pointData, srcId, destId);
    }
  }

  // add in level data for each plane which corresponds to an average pressure
  // if we are loading a volumetric grid
  if (this->VerticalDimension != VERTICAL_DIMENSION_SINGLE_LAYER)
  {
    std::vector<float> levelData(numLocalLevels);
    size_t start[] = { static_cast<size_t>(beginLevel) };
    size_t count[] = { static_cast<size_t>(numLocalLevels) };
    if (this->Internals->nc_err(
          nc_get_vara_float(this->Internals->nc_points, lonid, start, count, &levelData[0])))
    {
      return 0;
    }
    vtkNew<vtkFloatArray> levelPointData;
    levelPointData->SetName(levName);
    levelPointData->SetNumberOfTuples(points->GetNumberOfPoints());
    for (size_t j = 0; j < numLocalLevels; j++)
    {
      for (vtkIdType i = 0; i < numPointsPerLevel; i++)
      {
        levelPointData->SetValue(static_cast<vtkIdType>(j * numPointsPerLevel + i), levelData[j]);
      }
    }
    output->GetPointData()->AddArray(levelPointData);
  }

  this->UpdateProgress(.75); // educated guess for progress

  // now we actually create the cells
  if (this->VerticalDimension == VERTICAL_DIMENSION_SINGLE_LAYER ||
    // We load only one level
    numLevels != originalNumLevels)
  {
    output->Allocate(static_cast<vtkIdType>(numLocalCells));
  }
  else
  {
    // we have numLocalLevels points so we have (numLocalLevels-1) cells.
    output->Allocate(static_cast<vtkIdType>(numLocalCells * (numLocalLevels - 1)));
  }
  for (size_t i = 0; i < numLocalCells; i++)
  {
    vtkIdType pointIds[4];
    for (int j = 0; j < 4; j++)
    {
      pointIds[j] = cellConnectivity[i + j * numLocalCells] - 1;
    }
    if (this->VerticalDimension != VERTICAL_DIMENSION_SINGLE_LAYER &&
      // we load all layers
      numLevels == originalNumLevels)
    {
      // volumetric grid
      for (size_t lev = 0; lev < (numLocalLevels - 1); lev++)
      {
        vtkIdType hexIds[8];
        for (int j = 0; j < 4; j++)
        {
          hexIds[j] = static_cast<vtkIdType>(pointIds[j] + lev * numPointsPerLevel);
          hexIds[j + 4] = static_cast<vtkIdType>(pointIds[j] + (1 + lev) * numPointsPerLevel);
        }
        output->InsertNextCell(VTK_HEXAHEDRON, 8, hexIds);
      }
    }
    else
    {
      // we load one level only
      output->InsertNextCell(VTK_QUAD, 4, pointIds);
    }
  }

  if (numLocalCells != numCellsPerLevel)
  {
    // we have extra points that are not connected to any cells
    // vtkNew<vtkCleanUnstructuredGrid> cleanGrid;
    // cleanGrid->SetInput(output);
  }

  vtkDebugMacro(<< "Read " << output->GetNumberOfPoints() << " points,"
                << output->GetNumberOfCells() << " cells.\n");

  return 1;
}

//----------------------------------------------------------------------------
bool vtkNetCDFCAMReader::GetPartitioning(size_t piece, size_t numPieces, size_t numLevels,
  size_t numCellsPerLevel, size_t& beginLevel, size_t& endLevel, size_t& beginCell, size_t& endCell)
{
  // probably not the best way to partition the data but should
  // be sufficient for development.
  if (numPieces <= 0 || piece >= numPieces)
  {
    vtkErrorMacro("Bad piece information for partitioning.");
    return false;
  }
  int inputBeginLevel = 0;
  if ((this->VerticalDimension == VERTICAL_DIMENSION_MIDPOINT_LAYERS && this->SingleMidpointLayer))
  {
    inputBeginLevel = this->MidpointLayerIndex;
  }
  else if ((this->VerticalDimension == VERTICAL_DIMENSION_INTERFACE_LAYERS &&
             this->SingleInterfaceLayer))
  {
    inputBeginLevel = this->InterfaceLayerIndex;
  }

  if (numPieces == 1)
  {
    beginLevel = inputBeginLevel;
    endLevel = beginLevel + numLevels - 1;
    beginCell = 0;
    endCell = numCellsPerLevel;
    return true;
  }
  if (numPieces <= (numLevels - 1))
  {
    // this cannot happen for numLevels == 1
    beginLevel = piece * (numLevels - 1) / numPieces;
    endLevel = (piece + 1) * (numLevels - 1) / numPieces;
    beginCell = 0;
    endCell = numCellsPerLevel;
    return true;
  }

  int levelsPerPiece = vtkMath::Ceil(numLevels / static_cast<double>(numPieces));
  int piecesPerLevel = vtkMath::Ceil(numPieces / static_cast<double>(numLevels));
  size_t numOverworkedPieces = piecesPerLevel / levelsPerPiece * numLevels - numPieces;
  bool evenOverworked = (piecesPerLevel % 2 == 0 || numOverworkedPieces == 0);
  if (piece < numOverworkedPieces)
  {
    if (evenOverworked)
    {
      beginLevel = inputBeginLevel + 2 * piece / piecesPerLevel;
      size_t remainder = piece % (piecesPerLevel / 2);
      beginCell = remainder * numCellsPerLevel * 2 / piecesPerLevel;
      endCell = (remainder + 1) * numCellsPerLevel * 2 / piecesPerLevel;
    }
    else
    {
      beginLevel = inputBeginLevel + 2 * piece / (piecesPerLevel - 1);
      size_t remainder = piece % ((piecesPerLevel - 1) / 2);
      beginCell = remainder * numCellsPerLevel * 2 / piecesPerLevel;
      endCell = (remainder + 1) * numCellsPerLevel * 2 / piecesPerLevel;
    }
  }
  else // underworked pieces
  {
    if (evenOverworked == false &&
      piece - numOverworkedPieces < 2 * numOverworkedPieces / (piecesPerLevel - 1))
    { // fillers for levels that also have overworked pieces working on them
      beginLevel = inputBeginLevel + piece - numOverworkedPieces;
      beginCell = numCellsPerLevel * (piecesPerLevel - 1) / piecesPerLevel;
      endCell = numCellsPerLevel;
    }
    else
    {
      size_t fakePiece = numOverworkedPieces + piece; // take into account overworked pieces
      beginLevel = inputBeginLevel + fakePiece / piecesPerLevel;
      size_t remainder = fakePiece % piecesPerLevel;
      beginCell = remainder * numCellsPerLevel / piecesPerLevel;
      endCell = (remainder + 1) * numCellsPerLevel / piecesPerLevel;
    }
  }
  endLevel = beginLevel + numLevels - 1;
  return true;
}

//----------------------------------------------------------------------------
void vtkNetCDFCAMReader::SelectionCallback(
  vtkObject*, unsigned long vtkNotUsed(eventid), void* clientdata, void* vtkNotUsed(calldata))
{
  static_cast<vtkNetCDFCAMReader*>(clientdata)->Modified();
}

//----------------------------------------------------------------------------
void vtkNetCDFCAMReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(nullptr)") << endl;
  os << indent << "ConnectivityFileName: "
     << (this->ConnectivityFileName ? this->ConnectivityFileName : "(nullptr)") << endl;
  os << indent << "VerticalDimension: " << this->VerticalDimension << endl;
  os << indent << "SingleMidpointLayer: " << this->SingleMidpointLayer << endl;
  os << indent << "MidpointLayerIndex: " << this->MidpointLayerIndex << endl;
  os << indent << "SingleInterfaceLayer: " << this->SingleInterfaceLayer << endl;
  os << indent << "InterfaceLayerIndex: " << this->InterfaceLayerIndex << endl;

  os << indent << "PointsFile: " << this->Internals->nc_points << endl;
  os << indent << "ConnectivityFile: " << this->Internals->nc_connectivity << endl;
}
