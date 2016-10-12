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

#include "vtkCellArray.h"
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

#include <vector>
#include <vtk_netcdfcpp.h>

namespace
{
// determine if this is a cell that wraps from 360 to 0 (i.e. if it's
// a cell that wraps from the right side of the domain to the left side)
  bool IsCellInverted(double points[4][3])
  {
    // We test the normal 3 points at a time. Not all grids are well-behaved
    // i.e. consistenly use 0 or 360. We've had grid where 3 points on the left
    // side, and just 1 on the right. Just checking the first 3 points (which is
    // what ComputeNormal() does, we may (and do) miss a few cells.
    // See BUG #0014897.
    double normal[3];
    vtkPolygon::ComputeNormal(3, points[0], normal);
    if(normal[2] > 0)
    {
      return true;
    }
    vtkPolygon::ComputeNormal(3, points[1], normal);
    if(normal[2] > 0)
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

vtkStandardNewMacro(vtkNetCDFCAMReader);

//----------------------------------------------------------------------------
vtkNetCDFCAMReader::vtkNetCDFCAMReader()
{
  this->FileName = NULL;
  this->CurrentFileName = NULL;
  this->ConnectivityFileName = NULL;
  this->CurrentConnectivityFileName = NULL;
  this->PointsFile = NULL;
  this->ConnectivityFile = NULL;
  this->SingleLevel = 0;
  this->TimeSteps = NULL;
  this->NumberOfTimeSteps = 0;
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkNetCDFCAMReader::~vtkNetCDFCAMReader()
{
  this->SetFileName(NULL);
  this->SetCurrentFileName(NULL);
  this->SetConnectivityFileName(NULL);
  this->SetCurrentConnectivityFileName(NULL);
  delete this->PointsFile;
  this->PointsFile = NULL;
  delete this->ConnectivityFile;
  this->ConnectivityFile = NULL;
  delete []this->TimeSteps;
  this->TimeSteps = NULL;
}

//----------------------------------------------------------------------------
int vtkNetCDFCAMReader::CanReadFile(const char* fileName)
{
  NcFile file(fileName, NcFile::ReadOnly);
  return file.is_valid();
}

//----------------------------------------------------------------------------
void vtkNetCDFCAMReader::SetFileName(const char* fileName)
{
  vtkDebugMacro(<<" setting FileName to " << (fileName?fileName:"(null)") );
  if ( this->FileName == NULL && fileName == NULL)
  {
    return;
  }
  if ( this->FileName && fileName && (!strcmp(this->FileName,fileName)))
  {
    return;
  }
  delete this->PointsFile;
  this->PointsFile = NULL;
  delete [] this->FileName;
  this->FileName = NULL;
  if (fileName)
  {
    size_t n = strlen(fileName) + 1;
    char *cp1 =  new char[n];
    const char *cp2 = (fileName);
    this->FileName = cp1;
    do
    {
      *cp1++ = *cp2++;
    }
    while ( --n );
  }
   else
   {
    this->FileName = NULL;
   }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkNetCDFCAMReader::SetConnectivityFileName(const char* fileName)
{
  vtkDebugMacro(<<" setting ConnectivityFileName to "
                << (fileName?fileName:"(null)") );
  if ( this->ConnectivityFileName == NULL && fileName == NULL)
  {
    return;
  }
  if ( this->ConnectivityFileName && fileName &&
       (!strcmp(this->ConnectivityFileName,fileName)))
  {
    return;
  }
  delete this->ConnectivityFile;
  this->ConnectivityFile = NULL;
  delete [] this->ConnectivityFileName;
  if (fileName)
  {
    size_t n = strlen(fileName) + 1;
    char *cp1 =  new char[n];
    const char *cp2 = (fileName);
    this->ConnectivityFileName = cp1;
    do
    {
      *cp1++ = *cp2++;
    }
    while ( --n );
  }
   else
   {
    this->ConnectivityFileName = NULL;
   }
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkNetCDFCAMReader::RequestInformation(
  vtkInformation* vtkNotUsed(reqInfo),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* outputVector)
{
  if(this->FileName == NULL)
  {
    vtkWarningMacro("Missing a file name.");
    return 0;
  }

  if(this->CurrentFileName != NULL &&
     strcmp(this->CurrentFileName, this->FileName) != 0)
  {
    delete this->PointsFile;
    this->PointsFile = NULL;
    this->SetCurrentFileName(NULL);
  }
  if(this->PointsFile == NULL)
  {
    this->PointsFile = new NcFile(this->FileName, NcFile::ReadOnly);
    if(this->PointsFile->is_valid() == 0)
    {
      vtkErrorMacro(<< "Can't read file " << this->FileName);
      delete this->PointsFile;
      this->PointsFile = NULL;
      return 0;
    }
    this->SetCurrentFileName(this->FileName);
  }
  NcDim* timeDimension = this->PointsFile->get_dim("time");
  if(timeDimension == NULL)
  {
    vtkErrorMacro("Cannot find the number of time steps (time dimension).");
    return 0;
  }
  this->NumberOfTimeSteps = timeDimension->size();
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  if (this->NumberOfTimeSteps > 0)
  {
    delete []this->TimeSteps;
    this->TimeSteps = new double[this->NumberOfTimeSteps];
    NcVar* timeVar = this->PointsFile->get_var("time");
    timeVar->get(this->TimeSteps, this->NumberOfTimeSteps);

    // Tell the pipeline what steps are available
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(),
              this->TimeSteps, this->NumberOfTimeSteps);

    // Range is required to get GUI to show things
    double tRange[2] = {this->TimeSteps[0],
                        this->TimeSteps[this->NumberOfTimeSteps - 1]};
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), tRange, 2);
  }
  else
  {
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
  }

  outInfo->Set(
    CAN_HANDLE_PIECE_REQUEST(), 1);

  return 1;
}

//----------------------------------------------------------------------------
int vtkNetCDFCAMReader::RequestUpdateExtent(
  vtkInformation *,
  vtkInformationVector **,
  vtkInformationVector *outputVector)
{
  if(this->FileName == NULL || this->ConnectivityFileName == NULL)
  {
    vtkWarningMacro("Missing a file name.");
    return 0;
  }
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  int piece =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int numPieces =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  // make sure piece is valid
  if (piece < 0 || piece >= numPieces)
  {
    return 0;
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkNetCDFCAMReader::RequestData(
  vtkInformation *,vtkInformationVector **,vtkInformationVector *outputVector)
{
  if(this->FileName == NULL || this->ConnectivityFileName == NULL)
  {
    vtkWarningMacro("Missing a file name.");
    return 0;
  }

  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDebugMacro(<<"Reading NetCDF CAM file.");
  this->SetProgress(0);
  if(this->CurrentConnectivityFileName != NULL &&
     strcmp(this->CurrentConnectivityFileName, this->ConnectivityFileName) != 0)
  {
    delete this->ConnectivityFile;
    this->ConnectivityFile = NULL;
    this->SetCurrentConnectivityFileName(NULL);
  }
  if(this->ConnectivityFile == NULL)
  {
    this->ConnectivityFile = new NcFile(this->ConnectivityFileName,
                                        NcFile::ReadOnly);
    if(this->ConnectivityFile->is_valid() == 0)
    {
      vtkErrorMacro(<< "Can't read file " << this->ConnectivityFileName);
      delete this->ConnectivityFile;
      this->ConnectivityFile = NULL;
      return 0;
    }
    this->SetCurrentConnectivityFileName(this->ConnectivityFileName);
  }

  // Set the NetCDF error handler to not kill the application.
  // Upon exiting this method the error handler will be restored
  // to its previous state.
  NcError ncError(NcError::verbose_nonfatal);

  // read in the points first
  NcDim* levelsDimension = this->PointsFile->get_dim("lev");
  if(levelsDimension == NULL)
  {
    vtkErrorMacro("Cannot find the number of levels (lev dimension).");
    return 0;
  }
  long numCellLevels = levelsDimension->size() - 1;
  NcVar* levelsVar = this->PointsFile->get_var("lev");
  if(levelsVar == NULL)
  {
    vtkErrorMacro("Cannot find the number of levels (lev variable).");
    return 0;
  }
  if(levelsVar->num_dims() != 1 ||
     levelsVar->get_dim(0)->size() != numCellLevels+1)
  {
    vtkErrorMacro("The lev variable is not consistent.");
    return 0;
  }
  if(this->SingleLevel != 0)
  {
    numCellLevels = 1;
  }
  NcDim* dimension = this->PointsFile->get_dim("ncol");
  if(dimension == NULL)
  {
    vtkErrorMacro("Cannot find the number of points (ncol dimension).");
    return 0;
  }
  NcVar* lon = this->PointsFile->get_var("lon");
  NcVar* lat = this->PointsFile->get_var("lat");
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  output->SetPoints(points);

  long numFilePoints = dimension->size();
  if(lat == NULL || lon == NULL)
  {
    vtkErrorMacro("Cannot find coordinates (lat or lon variable).");
    return 0;
  }
  if(lat->type() == ncDouble)
  {
    points->SetDataTypeToDouble();
    points->SetNumberOfPoints(numFilePoints);
    std::vector<double> array(numFilePoints*2);
    if(!lon->get(&array[0], numFilePoints))
    {
      return 0;
    }
    if(!lat->get(&array[numFilePoints], numFilePoints))
    {
      return 0;
    }
    for(long i=0;i<numFilePoints;i++)
    {
      points->SetPoint(i, array[i], array[i+numFilePoints], numCellLevels+1);
    }
  }
  else
  {
    points->SetDataTypeToFloat();
    points->SetNumberOfPoints(numFilePoints);
    std::vector<float> array(numFilePoints*2);
    if(!lon->get(&array[0], numFilePoints))
    {
      return 0;
    }
    if(!lat->get(&array[numFilePoints], numFilePoints))
    {
      return 0;
    }
    for(long i=0;i<numFilePoints;i++)
    {
      points->SetPoint(i, array[i], array[i+numFilePoints], numCellLevels+1);
    }
  }
  this->SetProgress(.25);  // educated guess for progress

  // now read in the cell connectivity.  note that this is a periodic
  // domain and only the points on the left boundary are included in
  // the points file.  if a cell uses a point that is on the left
  // boundary and it should be on the right boundary we will have
  // to create that point.  That's what boundaryPoints is used for.
  // The (index + numFilePoints) gives us the new point id, and the value
  // for that in this array will correspond to the original point id that the
  // boundaryPoint is duplicate of.
  std::vector<vtkIdType> boundaryPoints;

  // To avoid creating multiple duplicates, we we a
  // vtkIncrementalOctreePointLocator.
  vtkSmartPointer<vtkIncrementalOctreePointLocator> locator =
    vtkSmartPointer<vtkIncrementalOctreePointLocator>::New();
  locator->SetDataSet(output); // dataset only has points right now.
  locator->BuildLocator();

  dimension = this->ConnectivityFile->get_dim("ncells");
  if(dimension == NULL)
  {
    vtkErrorMacro("Cannot find the number of cells (ncells dimension).");
    return 0;
  }
  NcVar* connectivity =
    this->ConnectivityFile->get_var("element_corners");
  if(connectivity == NULL)
  {
    vtkErrorMacro("Cannot find cell connectivity (element_corners dimension).");
    return 0;
  }
  long numCellsPerLevel = dimension->size();


  int piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int numPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  int beginCellLevel, endCellLevel, beginCell, endCell;
  this->GetPartitioning(piece, numPieces, numCellLevels, numCellsPerLevel,
                        beginCellLevel, endCellLevel, beginCell, endCell);
  // the cells/levels assigned to this piece
  long numLocalCells = endCell-beginCell;
  int numLocalCellLevels = endCellLevel-beginCellLevel;
  std::vector<int> cellConnectivity(4*numLocalCells);
  connectivity->set_cur(0, beginCell);
  connectivity->get(&(cellConnectivity[0]), 4, numLocalCells);

  for(long i=0;i<numLocalCells;i++)
  {
    vtkIdType pointIds[4];
    double coords[4][3]; // assume quads here
    for(int j=0;j<4;j++)
    {
      pointIds[j] = cellConnectivity[i+j*numLocalCells]-1;
      points->GetPoint(pointIds[j], coords[j]);
    }
    if (IsCellInverted(coords) == true)
    {
      // First decide whether we're putting this cell on the 360 side (right) or on the
      // 0 side (left). We decide this based on which side will have the
      // smallest protrusion.
      double delta = 0.0;
      bool anchorLeft = false;
      for (int j=0; j < 4; ++j)
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
      for (int j=0; j < 4; ++j)
      {
        if (anchorLeft)
        {
          // if coords[j] is closer to right (360), move it to the left.
          if ( (360.0 - coords[j][0]) < coords[j][0] )
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
          if ( coords[j][0] < (360.0 - coords[j][0]) )
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
          assert(newPtId >= numFilePoints && pointIds[j] < newPtId);
          assert(static_cast<vtkIdType>(boundaryPoints.size()) == (newPtId-numFilePoints));
          boundaryPoints.push_back(pointIds[j]);
        }
        cellConnectivity[i+j*numLocalCells] = (newPtId + 1); // note: 1-indexed.
      }
    }
  }
  locator = NULL; // release the locator memory.

  // we now have all of the points at a single level.  build them up
  // for the rest of the levels before creating the cells.
  vtkIdType numPointsPerLevel = points->GetNumberOfPoints();
  if(!this->SingleLevel)
  {
    // a hacky way to resize the points array without resetting the data
    points->InsertPoint(numPointsPerLevel*(numLocalCellLevels+1)-1, 0, 0, 0);
    for(vtkIdType pt=0;pt<numPointsPerLevel;pt++)
    {
      double point[3];
      points->GetPoint(pt, point);
      // need to start at 0 here since for multiple process the first
      // level will need to be replaced
      for(long lev=0;lev<numLocalCellLevels+1;lev++)
      {
        point[2] = numCellLevels - lev - beginCellLevel;
        points->SetPoint(pt+lev*numPointsPerLevel, point);
      }
    }
  }

  points->Modified();
  points->Squeeze();

  this->SetProgress(.5);  // educated guess for progress

  // Collect the time step requested
  vtkInformationDoubleKey* timeKey =
    static_cast<vtkInformationDoubleKey*>
    (vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

  double dTime = 0.0;
  if (outInfo->Has(timeKey))
  {
    dTime = outInfo->Get(timeKey);
  }

  // Actual time for the time step
  output->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), dTime);

  // Index of the time step to request
  int timeStep = 0;
  while (timeStep < this->NumberOfTimeSteps &&
         this->TimeSteps[timeStep] < dTime)
  {
    timeStep++;
  }

  // now that we have the full set of points, read in any point
  // data with dimensions (time, lev, ncol) but read them in
  // by chunks of ncol since it will be a pretty big chunk of
  // memory that we'll have to break up anyways.
  for(int i=0;i<this->PointsFile->num_vars();i++)
  {
    NcVar* variable = this->PointsFile->get_var(i);
    if(this->SingleLevel == 0 && (variable->num_dims() != 3 ||
                                  strcmp(variable->get_dim(0)->name(), "time") != 0 ||
                                  strcmp(variable->get_dim(1)->name(), "lev") != 0 ||
                                  strcmp(variable->get_dim(2)->name(), "ncol") != 0) )
    { // not a 3D field variable
      continue;
    }
    else if(this->SingleLevel == 1 && ((variable->num_dims() != 2 ||
                                        strcmp(variable->get_dim(0)->name(), "time") != 0 ||
                                        strcmp(variable->get_dim(1)->name(), "ncol") != 0) ) )
    { // not a 2D field variable
      continue;
    }

    vtkDoubleArray* doubleArray = NULL;
    vtkFloatArray* floatArray = NULL;
    if(variable->type() == ncDouble)
    {
      doubleArray = vtkDoubleArray::New();
      doubleArray->SetNumberOfTuples(points->GetNumberOfPoints());
      doubleArray->SetName(variable->name());
      output->GetPointData()->AddArray(doubleArray);
      doubleArray->Delete();
    }
    else
    {
      floatArray = vtkFloatArray::New();
      floatArray->SetNumberOfTuples(points->GetNumberOfPoints());
      floatArray->SetName(variable->name());
      output->GetPointData()->AddArray(floatArray);
      floatArray->Delete();
    }
    if(this->SingleLevel == 0)
    {
      for(long lev=0;lev<numLocalCellLevels+1;lev++)
      {
        variable->set_cur(timeStep, lev+beginCellLevel, 0);
        if(doubleArray)
        {
          if(!variable->get(doubleArray->GetPointer(0)+lev*numPointsPerLevel,
                            1, 1, numFilePoints))
          {
            vtkErrorMacro("Problem getting NetCDF variable " << variable->name());
            return 0;
          }
        }
        else
        {
          if(!variable->get(floatArray->GetPointer(0)+lev*numPointsPerLevel,
                            1, 1, numFilePoints))
          {
            vtkErrorMacro("Problem getting NetCDF variable " << variable->name());
            return 0;
          }
        }
      }
    }
    else if(this->SingleLevel == 1)
    {
      variable->set_cur(timeStep, 0);
      if(doubleArray)
      {
        if(!variable->get(doubleArray->GetPointer(0), 1, numFilePoints))
        {
          vtkErrorMacro("Problem getting NetCDF variable " << variable->name());
          return 0;
        }
      }
      else
      {
        if(!variable->get(floatArray->GetPointer(0), 1, numFilePoints))
        {
          vtkErrorMacro("Problem getting NetCDF variable " << variable->name());
          return 0;
        }
      }
    }
  }

  // we have to copy the values from the left side to the right side
  vtkPointData* pointData = output->GetPointData();
  pointData->CopyAllOn();
  pointData->CopyAllocate(output->GetPointData(),
                          output->GetNumberOfPoints());

  vtkIdType newPtId=0;
  for (std::vector<vtkIdType>::const_iterator it=
        boundaryPoints.begin(); it!=boundaryPoints.end(); ++it, ++newPtId)
  {
    for(long lev=0;lev<numLocalCellLevels+1-this->SingleLevel;lev++)
    {
      vtkIdType srcId = (*it) + lev * numPointsPerLevel;
      vtkIdType destId = (newPtId + numFilePoints) + lev * numPointsPerLevel;
      pointData->CopyData(pointData, srcId, destId);
    }
  }

  // add in level data for each plane which corresponds to an average pressure
  // if we are loading a volumetric grid
  if(this->SingleLevel == 0)
  {
    std::vector<float> levelData(numLocalCellLevels+1);
    levelsVar->set_cur(beginCellLevel);
    levelsVar->get(&levelData[0], numLocalCellLevels+1);
    vtkNew<vtkFloatArray> levelPointData;
    levelPointData->SetName(levelsVar->name());
    levelPointData->SetNumberOfTuples(points->GetNumberOfPoints());
    for(long j=0;j<numLocalCellLevels+1;j++)
    {
      for(vtkIdType i=0;i<numPointsPerLevel;i++)
      {
        levelPointData->SetValue(j*numPointsPerLevel+i, levelData[j]);
      }
    }
    output->GetPointData()->AddArray(levelPointData.GetPointer());
  }

  this->SetProgress(.75);  // educated guess for progress

  // now we actually create the cells
  if(this->SingleLevel == 1)
  {
    output->Allocate(numLocalCells);
  }
  else
  {
    output->Allocate(numLocalCells*numLocalCellLevels);
  }
  for(long i=0;i<numLocalCells;i++)
  {
    vtkIdType pointIds[4];
    for(int j=0;j<4;j++)
    {
      pointIds[j] = cellConnectivity[i+j*numLocalCells]-1;
    }
    if(this->SingleLevel == 0)
    { // volumetric grid
      for(int lev=0;lev<numLocalCellLevels;lev++)
      {
        vtkIdType hexIds[8];
        for(int j=0;j<4;j++)
        {
          hexIds[j] = pointIds[j]+lev*numPointsPerLevel;
          hexIds[j+4] = pointIds[j]+(1+lev)*numPointsPerLevel;
        }
        output->InsertNextCell(VTK_HEXAHEDRON, 8, hexIds);
      }
    }
    else if(this->SingleLevel == 1)
    { // surface grid
      output->InsertNextCell(VTK_QUAD, 4, pointIds);
    }
  }

  if(numLocalCells != numCellsPerLevel)
  {
    // we have extra points that are not connected to any cells
    //vtkNew<vtkCleanUnstructuredGrid> cleanGrid;
    //cleanGrid->SetInput(output);
  }

  vtkDebugMacro(<<"Read " << output->GetNumberOfPoints() <<" points,"
                << output->GetNumberOfCells() <<" cells.\n");

  return 1;
}

//----------------------------------------------------------------------------
bool vtkNetCDFCAMReader::GetPartitioning(
  int piece, int numPieces,int numLevels, int numCellsPerLevel,
  int & beginLevel, int & endLevel, int & beginCell, int & endCell)
{
  // probably not the best way to partition the data but should
  // be sufficient for development.
  if(numPieces <= 0 || piece < 0 || piece >= numPieces)
  {
    vtkErrorMacro("Bad piece information for partitioning.");
    return false;
  }
  if(numPieces == 1)
  {
    beginLevel = 0;
    endLevel = numLevels;
    beginCell = 0;
    endCell = numCellsPerLevel;
    return true;
  }
  if(numPieces <= numLevels)
  {
    beginLevel = piece*numLevels/numPieces;
    endLevel = (piece+1)*numLevels/numPieces;
    beginCell = 0;
    endCell = numCellsPerLevel;
    return true;
  }

  int levelsPerPiece = vtkMath::Ceil(numLevels/static_cast<double>(numPieces));
  int piecesPerLevel = vtkMath::Ceil(numPieces/static_cast<double>(numLevels));
  int numOverworkedPieces = piecesPerLevel/levelsPerPiece*numLevels - numPieces;
  bool evenOverworked = (piecesPerLevel % 2 == 0 || numOverworkedPieces == 0);
  if(piece < numOverworkedPieces)
  {
    if(evenOverworked)
    {
      beginLevel = 2*piece/piecesPerLevel;
      endLevel = beginLevel + 1;
      int remainder = piece % (piecesPerLevel/2);
      beginCell = remainder * numCellsPerLevel * 2 / piecesPerLevel;
      endCell = (remainder + 1)* numCellsPerLevel * 2 / piecesPerLevel;
    }
    else
    {
      beginLevel = 2*piece/(piecesPerLevel-1);
      endLevel = beginLevel + 1;
      int remainder = piece % ((piecesPerLevel-1)/2);
      beginCell = remainder * numCellsPerLevel * 2 / piecesPerLevel;
      endCell = (remainder + 1)* numCellsPerLevel * 2 / piecesPerLevel;
    }
  }
  else // underworked pieces
  {
    if( evenOverworked == false && piece - numOverworkedPieces < 2*numOverworkedPieces/(piecesPerLevel-1) )
    { // fillers for levels that also have overworked pieces working on them
      beginLevel = piece - numOverworkedPieces;
      beginCell = numCellsPerLevel*(piecesPerLevel-1)/piecesPerLevel;
      endCell = numCellsPerLevel;
    }
    else
    {
      int fakePiece = numOverworkedPieces+piece; // take into account overworked pieces
      beginLevel = fakePiece / piecesPerLevel;
      int remainder = fakePiece % piecesPerLevel;
      beginCell = remainder * numCellsPerLevel / piecesPerLevel;
      endCell = (remainder + 1)*numCellsPerLevel / piecesPerLevel;
    }
    endLevel = beginLevel + 1;
  }

  return true;
}

//----------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
void vtkNetCDFCAMReader::SetCellLayerRight(int)
{
  VTK_LEGACY_BODY(vtkNetCDFCAMReader::SetCellLayerRight, "VTK 6.3");
}
#endif

//----------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
int vtkNetCDFCAMReader::GetCellLayerRight()
{
  VTK_LEGACY_BODY(vtkNetCDFCAMReader::GetCellLayerRight, "VTK 6.3");
  return 0;
}
#endif

//----------------------------------------------------------------------------
void vtkNetCDFCAMReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "FileName: "
     << (this->FileName ? this->FileName : "(NULL)") << endl;
  os << indent << "ConnectivityFileName: " <<
    (this->ConnectivityFileName ? this->ConnectivityFileName : "(NULL)")
     << endl;
  os << indent << "SingleLevel: " << this->SingleLevel << endl;
  if(this->PointsFile)
  {
    os << indent << "PointsFile: " << this->PointsFile << endl;
  }
  else
  {
    os << indent << "PointsFile: (NULL)" << endl;
  }
  if(this->ConnectivityFile)
  {
    os << indent << "ConnectivityFile: " << this->ConnectivityFile << endl;
  }
  else
  {
    os << indent << "ConnectivityFile: (NULL)" << endl;
  }
}
