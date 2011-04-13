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
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"

#include <map>
#include <vtk_netcdfcpp.h>

vtkStandardNewMacro(vtkNetCDFCAMReader);

//============================================================================
#define CALL_NETCDF(call) \
{ \
  int errorcode = call; \
  if (errorcode != NC_NOERR) \
  { \
    vtkErrorMacro(<< "netCDF Error: " << nc_strerror(errorcode)); \
    return 0; \
  } \
}

//----------------------------------------------------------------------------
vtkNetCDFCAMReader::vtkNetCDFCAMReader()
{
  this->FileName = NULL;
  this->ConnectivityFileName = NULL;
  this->PointsFile = NULL;
  this->ConnectivityFile = NULL;
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkNetCDFCAMReader::~vtkNetCDFCAMReader()
{
  this->SetFileName(NULL);
  this->SetConnectivityFileName(NULL);
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
    return 1;
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkNetCDFCAMReader::RequestData(
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
  vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // All of the data in the first piece.
  if (outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()) > 0)
    {
    return 1;
    }

  vtkDebugMacro(<<"Reading NetCDF CAM file.");

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
    }

  // read in the points first
  NcDim* dimension = this->PointsFile->get_dim("ncol");
  NcVar* lon = this->PointsFile->get_var("lon");
  NcVar* lat = this->PointsFile->get_var("lat");
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  long numPoints = dimension->size();
  if(lat == NULL || lon == NULL)
    {
    vtkErrorMacro("Cannot find coordinates.");
    return 0;
    }
  if(lat->type() == ncDouble)
    {
    points->SetDataTypeToDouble();
    points->SetNumberOfPoints(numPoints);
    std::vector<double> array(numPoints*2);
    if(!lon->get(&array[0], numPoints))
      {
      return 0;
      }
    if(!lat->get(&array[numPoints], numPoints))
      {
      return 0;
      }
    for(long i=0;i<numPoints;i++)
      {
      points->SetPoint(i, array[i], array[i+numPoints], 0.);
      }
    }
  else
    {
    points->SetDataTypeToFloat();
    points->SetNumberOfPoints(numPoints);
    std::vector<float> array(numPoints*2);
    if(!lon->get(&array[0], numPoints))
      {
      return 0;
      }
    if(!lat->get(&array[numPoints], numPoints))
      {
      return 0;
      }
    for(long i=0;i<numPoints;i++)
      {
      points->SetPoint(i, array[i], array[i+numPoints], 0.);
      }
    for(long i=0;i<numPoints;i++)
      {
      points->SetPoint(i, array[i], array[i+numPoints], 0.);
      }
    }
  output->SetPoints(points);

  // read in any point data with dimensions (time, lev, ncol)
  NcDim* levelsDimension = this->PointsFile->get_dim("lev");
  long numLevels = levelsDimension->size();
  for(int i=0;i<this->PointsFile->num_vars();i++)
    {
    NcVar* variable = this->PointsFile->get_var(i);
    if(variable->num_dims() != 3 || variable->get_dim(0)->size() != 1 ||
       variable->get_dim(1)->size() != numLevels ||
       variable->get_dim(2)->size() != numPoints)
      { // not a field variable
      continue;
      }
    NcToken variableName = variable->name();
    // if we have a different level, that should be set here
    long level = 0;
    variable->set_cur(0, level, 0);
    vtkDataArray* dataArray = NULL;
    if(variable->type() == ncDouble)
      {
      vtkDoubleArray* doubleArray = vtkDoubleArray::New();
      doubleArray->SetNumberOfTuples(numPoints);
      if(!variable->get(doubleArray->GetPointer(0),
                        1, 1, numPoints))
        {
        return 0;
        }
      dataArray = doubleArray;
      }
    else
      {
      vtkFloatArray* floatArray = vtkFloatArray::New();
      floatArray->SetNumberOfTuples(numPoints);
      if(!variable->get(floatArray->GetPointer(0),
                        1, 1, numPoints))
        {
        return 0;
        }
      dataArray = floatArray;
      }
    dataArray->SetName(variableName);
    output->GetPointData()->AddArray(dataArray);
    dataArray->Delete();
    }

  // now read in the cell connectivity.  note that this is a periodic
  // domain and only the points on the left boundary are included in
  // the points file.  if a cell uses a point that is on the left
  // boundary and it should be on the right boundary we will have
  // to create that point.  that's what boundaryPoints is used for.
  std::map<vtkIdType, vtkIdType> boundaryPoints;
  dimension = this->ConnectivityFile->get_dim("ncenters");
  if(dimension == NULL)
    {  //supposed to be ncells but in example dataset it is ncenters
    this->ConnectivityFile->get_dim("ncenters");
    }
  NcVar* connectivity = this->ConnectivityFile->get_var("element_corners");
  long numCells = dimension->size();
  output->Allocate(numCells);
  if(connectivity == NULL)
    {
    vtkErrorMacro("Cannot find cell connectivity.");
    return 0;
    }
  std::vector<int> cellConnectivity(4*numCells);
  connectivity->get(&(cellConnectivity[0]), 4, numCells);
  double bounds[6];
  output->GetBounds(bounds);
  double leftSide = bounds[0] + .25*(bounds[1]-bounds[0]);
  double rightSide = bounds[0] + .75*(bounds[1]-bounds[0]);
  for(long i=0;i<numCells;i++)
    {
    vtkIdType pointIds[4];
    bool nearRightBoundary = false;
    bool nearLeftBoundary = false;
    double point[3];
    for(int j=0;j<4;j++)
      {
      pointIds[j] = cellConnectivity[i+j*numCells]-1;
      output->GetPoint(pointIds[j], point);
      if(point[0] > rightSide)
        {
        nearRightBoundary = true;
        }
      else if(point[0] < leftSide)
        {
        nearLeftBoundary = true;
        }
      }
    if(nearLeftBoundary == true && nearRightBoundary == true)
      {
      for(int j=0;j<4;j++)
        {
        output->GetPoint(pointIds[j], point);
        if(point[0] < leftSide)
          {
          std::map<vtkIdType, vtkIdType>::iterator otherPoint =
            boundaryPoints.find(pointIds[j]);
          if(otherPoint != boundaryPoints.end())
            { // already made point on the right boundary
            pointIds[j] = otherPoint->second;
            }
          else
            { // need to make point on the right boundary
            pointIds[j] = boundaryPoints[pointIds[j]] =
              points->InsertNextPoint(point[0]+360., point[1], point[2]);
            }
          }
        }
      }
    output->InsertNextCell(VTK_QUAD, 4, pointIds);
    }

  output->GetPointData()->CopyAllOn();
  output->GetPointData()->CopyAllocate(output->GetPointData(),
                                       output->GetNumberOfPoints());
  vtkPointData* pointData = output->GetPointData();
  for(std::map<vtkIdType, vtkIdType>::const_iterator it=
        boundaryPoints.begin();it!=boundaryPoints.end();it++)
    {
    pointData->CopyData(pointData, it->first, it->second);
    }

  vtkDebugMacro(<<"Read " <<output->GetNumberOfPoints() <<" points,"
                <<output->GetNumberOfCells() <<" cells.\n");

  return 1;
}

//----------------------------------------------------------------------------
int vtkNetCDFCAMReader::FillOutputPortInformation(int,
                                                  vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkUnstructuredGrid");
  return 1;
}

//----------------------------------------------------------------------------
void vtkNetCDFCAMReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
