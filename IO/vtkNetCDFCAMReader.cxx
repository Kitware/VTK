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
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolygon.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"

#include <map>
#include <vtk_netcdfcpp.h>

namespace
{
// determine if this is a cell that wraps from 360 to 0 (i.e. if it's
// a cell that wraps from the right side of the domain to the left side)
  bool IsCellInverted(double points[4][3])
  {
    double normal[3];
    vtkPolygon::ComputeNormal(4, points[0], normal);
    if(normal[2] > 0)
      {
      return true;
      }
    return false;
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
  this->CellLayerRight = 1;
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
  if(this->PointsFile)
    {
    delete this->PointsFile;
    this->PointsFile = NULL;
    }
  if(this->ConnectivityFile)
    {
    delete this->ConnectivityFile;
    this->ConnectivityFile = NULL;
    }
  if(this->TimeSteps)
    {
    delete []this->TimeSteps;
    this->TimeSteps = NULL;
    }
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
  if(this->PointsFile)
    {
    delete this->PointsFile;
    this->PointsFile = NULL;
    }
  if (this->FileName)
    {
    delete [] this->FileName;
    this->FileName = NULL;
    }
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
  if(this->ConnectivityFile)
    {
    delete this->ConnectivityFile;
    this->ConnectivityFile = NULL;
    }
  if (this->ConnectivityFileName)
    {
    delete [] this->ConnectivityFileName;
    }
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
  vtkInformation* info = outputVector->GetInformationObject(0);

  if (this->NumberOfTimeSteps > 0)
    {
    if(this->TimeSteps)
      {
      delete []this->TimeSteps;
      }
    this->TimeSteps = new double[this->NumberOfTimeSteps];
    NcVar* timeVar = this->PointsFile->get_var("time");
    timeVar->get(this->TimeSteps, this->NumberOfTimeSteps);

    // Tell the pipeline what steps are available
    info->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(),
              this->TimeSteps, this->NumberOfTimeSteps);

    // Range is required to get GUI to show things
    double tRange[2] = {this->TimeSteps[0],
                        this->TimeSteps[this->NumberOfTimeSteps - 1]};
    info->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), tRange, 2);
    }
  else
    {
    info->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    info->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
    }

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

  // All of the data in the first piece.
  if (outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()) > 0)
    {
    return 1;
    }

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
  long numLevels = levelsDimension->size();
  NcVar* levelsVar = this->PointsFile->get_var("lev");
  if(levelsVar == NULL)
    {
    vtkErrorMacro("Cannot find the number of levels (lev variable).");
    return 0;
    }
  if(levelsVar->num_dims() != 1 ||
     levelsVar->get_dim(0)->size() != numLevels)
    {
    vtkErrorMacro("The lev variable is not consistent.");
    return 0;
    }
  if(this->SingleLevel != 0)
    {
    numLevels = 1;
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
      points->SetPoint(i, array[i], array[i+numFilePoints], numLevels);
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
      points->SetPoint(i, array[i], array[i+numFilePoints], numLevels);
      }
    }
  this->SetProgress(.25);  // educated guess for progress

  // now read in the cell connectivity.  note that this is a periodic
  // domain and only the points on the left boundary are included in
  // the points file.  if a cell uses a point that is on the left
  // boundary and it should be on the right boundary we will have
  // to create that point.  that's what boundaryPoints is used for.
  // note that if this->CellLayerRight is false then we do the opposite
  // and make the 'connecting' cell on the left side of the domain
  // we don't actually build the cells yet though.
  std::map<vtkIdType, vtkIdType> boundaryPoints;
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
  long numCells = dimension->size();
  std::vector<int> cellConnectivity(4*numCells);
  connectivity->get(&(cellConnectivity[0]), 4, numCells);
  double leftSide = 1.;
  double rightSide = 359.;
  for(long i=0;i<numCells;i++)
    {
    vtkIdType pointIds[4];
    double coords[4][3]; // assume quads here
    for(int j=0;j<4;j++)
      {
      pointIds[j] = cellConnectivity[i+j*numCells]-1;
      points->GetPoint(pointIds[j], coords[j]);
      }
    if(IsCellInverted(coords) == true)
      {
      for(int j=0;j<4;j++)
        {
        if(this->CellLayerRight && coords[j][0] < leftSide)
          {
          std::map<vtkIdType, vtkIdType>::iterator otherPoint =
            boundaryPoints.find(pointIds[j]);
          if(otherPoint != boundaryPoints.end())
            { // already made point on the right boundary
            cellConnectivity[i+j*numCells] = otherPoint->second + 1;
            }
          else
            { // need to make point on the right boundary
            vtkIdType index =
              points->InsertNextPoint(coords[j][0]+360., coords[j][1], coords[j][2]);
            cellConnectivity[i+j*numCells] = index+1;
            boundaryPoints[pointIds[j]] = index;
            //cerr << "making a copy of point " << pointIds[j] << " to " << index << endl;
            }
          }
        else if(this->CellLayerRight == 0 && coords[j][0] > rightSide)
          {
          std::map<vtkIdType, vtkIdType>::iterator otherPoint =
            boundaryPoints.find(pointIds[j]);
          if(otherPoint != boundaryPoints.end())
            { // already made point on the right boundary
            cellConnectivity[i+j*numCells] = otherPoint->second+1;
            }
          else
            { // need to make point on the right boundary
            vtkIdType index =
              points->InsertNextPoint(coords[j][0]-360., coords[j][1], coords[j][2]);
            cellConnectivity[i+j*numCells] = index+1;
            boundaryPoints[pointIds[j]] = index;
            }
          }
        }
      }
    }

  // we now have all of the points at a single level.  build them up
  // for the rest of the levels before creating the cells.
  vtkIdType numPointsPerLevel = points->GetNumberOfPoints();
  if(numLevels > 1)
    {
    // a hacky way to resize the points array without resetting the data
    points->InsertPoint(numPointsPerLevel*numLevels-1, 0, 0, 0);
    for(vtkIdType pt=0;pt<numPointsPerLevel;pt++)
      {
      double point[3];
      points->GetPoint(pt, point);
      for(long lev=1;lev<numLevels;lev++)
        {
        point[2] = numLevels - lev;
        points->SetPoint(pt+lev*numPointsPerLevel, point);
        }
      }
    }
  points->Modified();
  output->SetPoints(points);

  this->SetProgress(.5);  // educated guess for progress

  // Collect the time step requested
  vtkInformationDoubleVectorKey* timeKey =
    static_cast<vtkInformationDoubleVectorKey*>
    (vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS());

  double dTime = 0.0;
  if (outInfo->Has(timeKey))
    {
    double* requestedTimeSteps = outInfo->Get(timeKey);
    dTime = requestedTimeSteps[0];
    }

  // Actual time for the time step
  output->GetInformation()->Set(vtkDataObject::DATA_TIME_STEPS(), &dTime, 1);

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
  // memory that we'll have to brea up anyways.
  for(int i=0;i<this->PointsFile->num_vars();i++)
    {
    NcVar* variable = this->PointsFile->get_var(i);
    if(numLevels > 1 && (variable->num_dims() != 3 ||
                         strcmp(variable->get_dim(0)->name(), "time") != 0 ||
                         strcmp(variable->get_dim(1)->name(), "lev") != 0 ||
                         strcmp(variable->get_dim(2)->name(), "ncol") != 0) )
      { // not a 3D field variable
      continue;
      }
    else if(numLevels == 1 && ((variable->num_dims() != 2 ||
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
    if(numLevels > 1)
      {
      for(long lev=0;lev<numLevels;lev++)
        {
        variable->set_cur(timeStep, lev, 0);
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
    else if(numLevels == 1)
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
  // we have to copy the values from the left size to the right side
  output->GetPointData()->CopyAllOn();
  output->GetPointData()->CopyAllocate(output->GetPointData(),
                                       output->GetNumberOfPoints());
  vtkPointData* pointData = output->GetPointData();
  for(std::map<vtkIdType, vtkIdType>::const_iterator it=
        boundaryPoints.begin();it!=boundaryPoints.end();it++)
    {
    for(long lev=0;lev<numLevels;lev++)
      {
      pointData->CopyData(pointData, it->first+lev*numPointsPerLevel,
                          it->second+lev*numPointsPerLevel);
      }
    }
  // add in level data for each plane which corresponds to an average pressure
  std::vector<float> levelData(numLevels);
  levelsVar->get(&levelData[0], numLevels);
  vtkNew<vtkFloatArray> levelPointData;
  levelPointData->SetName(levelsVar->name());
  levelPointData->SetNumberOfTuples(points->GetNumberOfPoints());
  for(long j=0;j<numLevels;j++)
    {
    for(vtkIdType i=0;i<numPointsPerLevel;i++)
      {
      levelPointData->SetValue(j*numPointsPerLevel+i, levelData[j]);
      }
    }
  output->GetPointData()->AddArray(levelPointData.GetPointer());

  this->SetProgress(.75);  // educated guess for progress

  // now we actually create the cells
  output->Allocate(numCells*(levelData.size()-1));
  for(long i=0;i<numCells;i++)
    {
    vtkIdType pointIds[4];
    for(int j=0;j<4;j++)
      {
      pointIds[j] = cellConnectivity[i+j*numCells]-1;
      }
    if(numLevels > 1)
      { // volumetric grid
      for(size_t lev=0;lev<levelData.size()-1;lev++)
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
    else if(numLevels == 1)
      { // surface grid
      output->InsertNextCell(VTK_QUAD, 4, pointIds);
      }
    }

  vtkDebugMacro(<<"Read " << output->GetNumberOfPoints() <<" points,"
                << output->GetNumberOfCells() <<" cells.\n");

  return 1;
}

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
  os << indent << "CellLayerRight: " << this->CellLayerRight << endl;
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
