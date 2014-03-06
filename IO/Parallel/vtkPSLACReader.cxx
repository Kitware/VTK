// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPSLACReader.cxx

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

#include "vtkPSLACReader.h"

#include "vtkCellArray.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDummyController.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiProcessStream.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSortDataArray.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"
#include "vtkDoubleArray.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

#include "vtk_netcdf.h"

#include <vtksys/hash_map.hxx>

//=============================================================================
#define MY_MIN(x, y)    ((x) < (y) ? (x) : (y))
#define MY_MAX(x, y)    ((x) < (y) ? (y) : (x))

//=============================================================================
#define CALL_NETCDF(call)                       \
  { \
    int errorcode = call; \
    if (errorcode != NC_NOERR) \
      { \
      vtkErrorMacro(<< "netCDF Error: " << nc_strerror(errorcode)); \
      return 0; \
      } \
  }

#define WRAP_NETCDF(call) \
  { \
    int errorcode = call; \
    if (errorcode != NC_NOERR) return errorcode; \
  }

#ifdef VTK_USE_64BIT_IDS
//#ifdef NC_INT64
//// This may or may not work with the netCDF 4 library reading in netCDF 3 files.
//#define nc_get_vars_vtkIdType nc_get_vars_longlong
//#else // NC_INT64
static int nc_get_vars_vtkIdType(int ncid, int varid,
                                 const size_t start[], const size_t count[],
                                 const ptrdiff_t stride[],
                                 vtkIdType *ip)
{
  // Step 1, figure out how many entries in the given variable.
  int numdims;
  WRAP_NETCDF(nc_inq_varndims(ncid, varid, &numdims));
  vtkIdType numValues = 1;
  for (int dim = 0; dim < numdims; dim++)
    {
    numValues *= count[dim];
    }

  // Step 2, read the data in as 32 bit integers.  Recast the input buffer
  // so we do not have to create a new one.
  long *smallIp = reinterpret_cast<long*>(ip);
  WRAP_NETCDF(nc_get_vars_long(ncid, varid, start, count, stride, smallIp));

  // Step 3, recast the data from 32 bit integers to 64 bit integers.  Since we
  // are storing both in the same buffer, we need to be careful to not overwrite
  // uncopied 32 bit numbers with 64 bit numbers.  We can do that by copying
  // backwards.
  for (vtkIdType i = numValues-1; i >= 0; i--)
    {
    ip[i] = static_cast<vtkIdType>(smallIp[i]);
    }

  return NC_NOERR;
}
//#endif // NC_INT64
#else // VTK_USE_64_BIT_IDS
#define nc_get_vars_vtkIdType nc_get_vars_int
#endif // VTK_USE_64BIT_IDS

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
// In this version, indexMap points from outArray to inArray.  All the values
// of outArray get filled.
template<class T>
void vtkPSLACReaderMapValues1(const T *inArray, T *outArray, int numComponents,
                              vtkIdTypeArray *indexMap, vtkIdType offset=0)
{
  vtkIdType numVals = indexMap->GetNumberOfTuples();
  for (vtkIdType i = 0; i < numVals; i++)
    {
    vtkIdType j = indexMap->GetValue(i) - offset;
    for (int c = 0; c < numComponents; c++)
      {
      outArray[numComponents*i+c] = inArray[numComponents*j+c];
      }
    }
}

// // In this version, indexMap points from inArray to outArray.  All the values
// // of inArray get copied.
// template<class T>
// void vtkPSLACReaderMapValues2(const T *inArray, T *outArray, int numComponents,
//                               vtkIdTypeArray *indexMap)
// {
//   vtkIdType numVals = indexMap->GetNumberOfTuples();
//   for (vtkIdType i = 0; i < numVals; i++)
//     {
//     vtkIdType j = indexMap->GetValue(i);
//     for (int c = 0; c < numComponents; c++)
//       {
//       outArray[numComponents*j+c] = inArray[numComponents*i+c];
//       }
//     }
// }

//=============================================================================
// Make sure that each process has the same number of blocks in the same
// position.  Assumes that all blocks are unstructured grids.
static void SynchronizeBlocks(vtkMultiBlockDataSet *blocks,
                              vtkMultiProcessController *controller,
                              vtkInformationIntegerKey *typeKey)
{
  unsigned long localNumBlocks = blocks->GetNumberOfBlocks();
  unsigned long numBlocks;
  controller->AllReduce(&localNumBlocks, &numBlocks, 1,
                        vtkCommunicator::MAX_OP);
  if (blocks->GetNumberOfBlocks() < numBlocks)
    {
    blocks->SetNumberOfBlocks(numBlocks);
    }

  for (unsigned int blockId = 0; blockId < numBlocks; blockId++)
    {
    vtkDataObject *object = blocks->GetBlock(blockId);
    if (object && !object->IsA("vtkUnstructuredGrid"))
      {
      vtkGenericWarningMacro(<< "Sanity error: found a block that is not an unstructured grid.");
      }
    int localBlockExists = (object != NULL);
    int globalBlockExists = 0;
    controller->AllReduce(&localBlockExists, &globalBlockExists, 1,
                          vtkCommunicator::LOGICAL_OR_OP);
    if (!localBlockExists && globalBlockExists)
      {
      VTK_CREATE(vtkUnstructuredGrid, grid);
      blocks->SetBlock(blockId, grid);
      blocks->GetMetaData(blockId)->Set(typeKey, 1);
      }
    }
}

//=============================================================================
// Structures used by ReadMidpointCoordinates to store and transfer midpoint
// information.
namespace vtkPSLACReaderTypes
{
  struct EdgeEndpointsHash {
  public:
    size_t operator()(const vtkSLACReader::EdgeEndpoints &edge) const {
      return static_cast<size_t>(edge.GetMinEndPoint() + edge.GetMaxEndPoint());
    }
  };

  typedef struct {
    double coord[3];
  } midpointPositionType;
  const vtkIdType midpointPositionSize
    = sizeof(midpointPositionType)/sizeof(double);

  typedef struct {
    vtkIdType minEdgePoint;
    vtkIdType maxEdgePoint;
    vtkIdType globalId;
  } midpointTopologyType;
  const vtkIdType midpointTopologySize
    = sizeof(midpointTopologyType)/sizeof(vtkIdType);

  typedef struct {
    std::vector<midpointPositionType> position;
    std::vector<midpointTopologyType> topology;
  } midpointListsType;

  typedef struct {
    midpointPositionType *position;
    midpointTopologyType *topology;
  } midpointPointersType;
  typedef vtksys::hash_map<vtkSLACReader::EdgeEndpoints,
                           midpointPointersType,
                           EdgeEndpointsHash> MidpointsAvailableType;

//-----------------------------------------------------------------------------
// Convenience function for gathering midpoint information to a process.
  static void GatherMidpoints(vtkMultiProcessController *controller,
                              const midpointListsType &sendMidpoints,
                              midpointListsType &recvMidpoints,
                              int process)
  {
    vtkIdType sendLength = sendMidpoints.position.size();
    if (sendLength != static_cast<vtkIdType>(sendMidpoints.topology.size()))
      {
      vtkGenericWarningMacro(<< "Bad midpoint array structure.");
      return;
      }

    vtkIdType numProcesses = controller->GetNumberOfProcesses();

    // Gather the amount of data each process is going to send.
    std::vector<vtkIdType> receiveCounts(numProcesses);
    controller->Gather(&sendLength, &receiveCounts.at(0), 1, process);

    // Get ready the arrays for the receiver that determine how much data
    // to get and where to put it.
    std::vector<vtkIdType> positionLengths(numProcesses);
    std::vector<vtkIdType> positionOffsets(numProcesses);
    std::vector<vtkIdType> topologyLengths(numProcesses);
    std::vector<vtkIdType> topologyOffsets(numProcesses);

    const double *sendPositionBuffer
      = (  (sendLength > 0)
         ? reinterpret_cast<const double *>(&sendMidpoints.position.at(0))
         : NULL);
    const vtkIdType *sendTopologyBuffer
      = (  (sendLength > 0)
         ? reinterpret_cast<const vtkIdType *>(&sendMidpoints.topology.at(0))
         : NULL);
    double *recvPositionBuffer;
    vtkIdType *recvTopologyBuffer;

    if (process == controller->GetLocalProcessId())
      {
      vtkIdType numEntries = 0;
      for (int i = 0; i < numProcesses; i++)
        {
        positionLengths[i] = midpointPositionSize*receiveCounts[i];
        positionOffsets[i] = midpointPositionSize*numEntries;
        topologyLengths[i] = midpointTopologySize*receiveCounts[i];
        topologyOffsets[i] = midpointTopologySize*numEntries;
        numEntries += receiveCounts[i];
        }
      recvMidpoints.position.resize(numEntries);
      recvMidpoints.topology.resize(numEntries);

      recvPositionBuffer
        = (  (numEntries > 0)
           ? reinterpret_cast<double *>(&recvMidpoints.position.at(0))
           : NULL);
      recvTopologyBuffer
        = (  (numEntries > 0)
           ? reinterpret_cast<vtkIdType *>(&recvMidpoints.topology.at(0))
           : NULL);
      }
    else
      {
      recvPositionBuffer = NULL;
      recvTopologyBuffer = NULL;
      }

    // Gather the actual data.
    controller->GatherV(sendPositionBuffer, recvPositionBuffer,
                        midpointPositionSize*sendLength,
                        &positionLengths.at(0), &positionOffsets.at(0),
                        process);
    controller->GatherV(sendTopologyBuffer, recvTopologyBuffer,
                        midpointTopologySize*sendLength,
                        &topologyLengths.at(0), &topologyOffsets.at(0),
                        process);
  }
};
using namespace vtkPSLACReaderTypes;

//-----------------------------------------------------------------------------
// Simple hash function for vtkIdType.
struct vtkPSLACReaderIdTypeHash {
  size_t operator()(vtkIdType val) const { return static_cast<size_t>(val); }
};

//=============================================================================
vtkStandardNewMacro(vtkPSLACReader);

vtkCxxSetObjectMacro(vtkPSLACReader, Controller, vtkMultiProcessController);

//-----------------------------------------------------------------------------
class vtkPSLACReader::vtkInternal
{
public:
  typedef vtksys::hash_map<vtkIdType, vtkIdType, vtkPSLACReaderIdTypeHash>
    GlobalToLocalIdType;
  GlobalToLocalIdType GlobalToLocalIds;

  // Description:
  // A map from local point ids to global ids.  Can also be used as the
  // global point ids.
  vtkSmartPointer<vtkIdTypeArray> LocalToGlobalIds;

  // Description:
  // The point data we expect to receive from each process.
  vtkSmartPointer<vtkIdTypeArray> PointsExpectedFromProcessesLengths;
  vtkSmartPointer<vtkIdTypeArray> PointsExpectedFromProcessesOffsets;

  // Description:
  // The point data we have to send to each process.  Stored as global ids.
  vtkSmartPointer<vtkIdTypeArray> PointsToSendToProcesses;
  vtkSmartPointer<vtkIdTypeArray> PointsToSendToProcessesLengths;
  vtkSmartPointer<vtkIdTypeArray> PointsToSendToProcessesOffsets;

  // Description:
  // The edge data we expect to receive from each process.
  vtkSmartPointer<vtkIdTypeArray> EdgesExpectedFromProcessesCounts;

  // Description:
  // The edge data we have to send to each process.  Stored as global ids.
  vtkSmartPointer<vtkIdTypeArray> EdgesToSendToProcesses;
  vtkSmartPointer<vtkIdTypeArray> EdgesToSendToProcessesLengths;
  vtkSmartPointer<vtkIdTypeArray> EdgesToSendToProcessesOffsets;
};

//-----------------------------------------------------------------------------
vtkPSLACReader::vtkPSLACReader()
{
  this->Controller = NULL;
  this->SetController(vtkMultiProcessController::GetGlobalController());
  if (!this->Controller)
    {
    this->SetController(vtkSmartPointer<vtkDummyController>::New());
    }
  this->NumberOfPiecesCache = 0;
  this->RequestedPieceCache = -1;

  this->Internal = new vtkPSLACReader::vtkInternal;
}

vtkPSLACReader::~vtkPSLACReader()
{
  this->SetController(NULL);

  delete this->Internal;
}

void vtkPSLACReader::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->Controller)
    {
    os << indent << "Controller: " << this->Controller << endl;
    }
  else
    {
    os << indent << "Controller: (null)\n";
    }
}

//-----------------------------------------------------------------------------
int vtkPSLACReader::RequestInformation(vtkInformation *request,
                                       vtkInformationVector **inputVector,
                                       vtkInformationVector *outputVector)
{
  // It would be more efficient to read the meta data on just process 0 and
  // propgate to the rest.  However, this will probably have a profound effect
  // only on big jobs accessing parallel file systems.  Until we need that,
  // I'm not going to bother.
  if (!this->Superclass::RequestInformation(request, inputVector, outputVector))
    {
    return 0;
    }

  if (!this->Controller)
    {
    vtkErrorMacro(<< "I need a Controller to read the data.");
    return 0;
    }

  for (int i = 0; i < vtkPSLACReader::NUM_OUTPUTS; i++)
    {
    vtkInformation *outInfo = outputVector->GetInformationObject(i);
    outInfo->Set(CAN_HANDLE_PIECE_REQUEST(),
                 1);
    }

  return 1;
}

//-----------------------------------------------------------------------------
int vtkPSLACReader::RequestData(vtkInformation *request,
                                vtkInformationVector **inputVector,
                                vtkInformationVector *outputVector)
{
  // Check to make sure the pieces match the processes.
  this->RequestedPiece = 0;
  this->NumberOfPieces = 1;
  for (int i = 0; i < vtkSLACReader::NUM_OUTPUTS; i++)
    {
    vtkInformation *outInfo = outputVector->GetInformationObject(i);
    if (   outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER())
        && outInfo->Has(
                  vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()) )
      {
      this->RequestedPiece = outInfo->Get(
                       vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
      this->NumberOfPieces = outInfo->Get(
                   vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
      if (   (this->RequestedPiece == this->Controller->GetLocalProcessId())
          && (this->NumberOfPieces == this->Controller->GetNumberOfProcesses()))
        {
        break;
        }
      }
    }

  if (   (this->RequestedPiece != this->Controller->GetLocalProcessId())
      || (this->NumberOfPieces != this->Controller->GetNumberOfProcesses()) )
    {
    vtkErrorMacro(<< "Process numbers do not match piece numbers.");
    return 0;
    }

  // RequestData will call other methods that we have overloaded to read
  // partitioned pieces.
  int retval =this->Superclass::RequestData(request, inputVector, outputVector);

  return retval;
}

//-----------------------------------------------------------------------------
int vtkPSLACReader::ReadTetrahedronInteriorArray(int meshFD,
                                                 vtkIdTypeArray *connectivity)
{
  int tetInteriorVarId;
  CALL_NETCDF(nc_inq_varid(meshFD, "tetrahedron_interior", &tetInteriorVarId));
  vtkIdType numTets
    = this->GetNumTuplesInVariable(meshFD, tetInteriorVarId, NumPerTetInt);

  vtkIdType numTetsPerPiece = numTets/this->NumberOfPieces + 1;
  vtkIdType startTet = this->RequestedPiece*numTetsPerPiece;
  vtkIdType endTet = startTet + numTetsPerPiece;
  if (endTet > numTets) endTet = numTets;

  size_t start[2];
  size_t count[2];

  start[0] = startTet;  count[0] = endTet - startTet;
  start[1] = 0;         count[1] = NumPerTetInt;

  connectivity->Initialize();
  connectivity->SetNumberOfComponents(static_cast<int>(count[1]));
  connectivity->SetNumberOfTuples(static_cast<vtkIdType>(count[0]));
  CALL_NETCDF(nc_get_vars_vtkIdType(meshFD, tetInteriorVarId,
                                    start, count, NULL,
                                    connectivity->GetPointer(0)));

  return 1;
}

//-----------------------------------------------------------------------------
int vtkPSLACReader::ReadTetrahedronExteriorArray(int meshFD,
                                                 vtkIdTypeArray *connectivity)
{
  int tetExteriorVarId;
  CALL_NETCDF(nc_inq_varid(meshFD, "tetrahedron_exterior", &tetExteriorVarId));
  vtkIdType numTets
    = this->GetNumTuplesInVariable(meshFD, tetExteriorVarId, NumPerTetExt);

  vtkIdType numTetsPerPiece = numTets/this->NumberOfPieces + 1;
  vtkIdType startTet = this->RequestedPiece*numTetsPerPiece;
  vtkIdType endTet = startTet + numTetsPerPiece;
  if (endTet > numTets) endTet = numTets;

  size_t start[2];
  size_t count[2];

  start[0] = startTet;  count[0] = endTet - startTet;
  start[1] = 0;         count[1] = NumPerTetExt;

  connectivity->Initialize();
  connectivity->SetNumberOfComponents(static_cast<int>(count[1]));
  connectivity->SetNumberOfTuples(static_cast<vtkIdType>(count[0]));
  CALL_NETCDF(nc_get_vars_vtkIdType(meshFD, tetExteriorVarId,
                                    start, count, NULL,
                                    connectivity->GetPointer(0)));

  return 1;
}

//-----------------------------------------------------------------------------
int vtkPSLACReader::CheckTetrahedraWinding(int meshFD)
{
  // Check the file only on the first process and broadcast the result.
  int winding;
  if (this->Controller->GetLocalProcessId() == 0)
    {
    winding = this->Superclass::CheckTetrahedraWinding(meshFD);
    }
  this->Controller->Broadcast(&winding, 1, 0);
  return winding;
}

//-----------------------------------------------------------------------------
int vtkPSLACReader::ReadConnectivity(int meshFD,
                                     vtkMultiBlockDataSet *surfaceOutput,
                                     vtkMultiBlockDataSet *volumeOutput)
{
  //---------------------------------
  // Call the superclass to read the arrays from disk and assemble the
  // primitives.  The superclass will call the ReadTetrahedron*Array methods,
  // which we have overridden to read only a partition of the cells.
  if (!this->Superclass::ReadConnectivity(meshFD, surfaceOutput, volumeOutput))
    {
    return 0;
    }

  //---------------------------------
  // Right now, the output only has blocks that are defined by the local piece.
  // However, downstream components will expect the multiblock structure to be
  // uniform amongst all processes.  Thus, we correct that problem here by
  // adding empty blocks for those not in our local piece.
  SynchronizeBlocks(surfaceOutput, this->Controller, IS_EXTERNAL_SURFACE());
  SynchronizeBlocks(volumeOutput,  this->Controller, IS_INTERNAL_VOLUME());

  //---------------------------------
  // This multiblock that contains both outputs provides an easy way to iterate
  // over all cells in both output.
  VTK_CREATE(vtkMultiBlockDataSet, compositeOutput);
  compositeOutput->SetNumberOfBlocks(2);
  compositeOutput->SetBlock(SURFACE_OUTPUT, surfaceOutput);
  compositeOutput->SetBlock(VOLUME_OUTPUT, volumeOutput);

  // ---------------------------------
  // All the cells have "global" ids.  That is, an index into a global list of
  // all possible points.  We don't want to have to read in all points in all
  // processes, so here we are going to figure out what points we need to load
  // locally, make maps between local and global ids, and convert the ids in the
  // connectivity arrays from global ids to local ids.

  this->Internal->LocalToGlobalIds = vtkSmartPointer<vtkIdTypeArray>::New();
  this->Internal->LocalToGlobalIds->SetName("GlobalIds");

  // Iterate over all points of all cells and mark what points we encounter
  // in GlobalToLocalIds.
  this->Internal->GlobalToLocalIds.clear();
  vtkSmartPointer<vtkCompositeDataIterator> outputIter;
  for (outputIter.TakeReference(compositeOutput->NewIterator());
       !outputIter->IsDoneWithTraversal(); outputIter->GoToNextItem())
    {
    vtkUnstructuredGrid *ugrid = vtkUnstructuredGrid::SafeDownCast(
                                       compositeOutput->GetDataSet(outputIter));
    vtkCellArray *cells = ugrid->GetCells();

    vtkIdType npts, *pts;
    for (cells->InitTraversal(); cells->GetNextCell(npts, pts); )
      {
      for (vtkIdType i = 0; i < npts; i++)
        {
        // The following inserts an entry into the map if one does not exist.
        // We will assign actual local ids later.
        this->Internal->GlobalToLocalIds[pts[i]] = -1;
        }
      }
    }


  // If we are reading midpoints, record any edges that might require endpoints.
  std::vector<vtkSLACReader::EdgeEndpoints> edgesNeeded;

  if (this->ReadMidpoints)
    {
    for (outputIter.TakeReference(surfaceOutput->NewIterator());
         !outputIter->IsDoneWithTraversal(); outputIter->GoToNextItem())
      {
      vtkUnstructuredGrid *ugrid = vtkUnstructuredGrid::SafeDownCast(
                                         surfaceOutput->GetDataSet(outputIter));
      vtkCellArray *cells = ugrid->GetCells();

      vtkIdType npts, *pts;
      for (cells->InitTraversal(); cells->GetNextCell(npts, pts); )
        {
        for (vtkIdType i = 0; i < npts; i++)
          {
          edgesNeeded.push_back(vtkSLACReader::EdgeEndpoints(pts[i],
                                                             pts[(i+1)%npts]));
          }
        }
      }
    }

  // ---------------------------------
  // Now that we know all the global ids we have, create a map from local
  // to global ids.  First we'll just copy the global ids into the array and
  // then sort them.  Sorting them will make the global ids monotonically
  // increasing, which means that when we get data from another process we
  // can just copy it into a block of memory.  We are only calculating the
  // local to global id map for now.  We will fill the global to local id
  // later when we iterate over the local ids.
  this->Internal->LocalToGlobalIds->Allocate(
                                       this->Internal->GlobalToLocalIds.size());
  vtkInternal::GlobalToLocalIdType::iterator itr;
  for (itr = this->Internal->GlobalToLocalIds.begin();
       itr != this->Internal->GlobalToLocalIds.end(); itr++)
    {
    this->Internal->LocalToGlobalIds->InsertNextValue(itr->first);
    }
  vtkSortDataArray::Sort(this->Internal->LocalToGlobalIds);

  // ---------------------------------
  // Now that we have the local to global id maps, we can determine which
  // process will send what point data where.  This is also where we assign
  // local ids to global ids (i.e. determine locally where we store each point).
  this->Internal->PointsExpectedFromProcessesLengths = vtkSmartPointer<vtkIdTypeArray>::New();
  this->Internal->PointsExpectedFromProcessesLengths->SetNumberOfTuples(this->NumberOfPieces);
  this->Internal->PointsExpectedFromProcessesOffsets = vtkSmartPointer<vtkIdTypeArray>::New();
  this->Internal->PointsExpectedFromProcessesOffsets->SetNumberOfTuples(this->NumberOfPieces);
  this->Internal->PointsToSendToProcesses = vtkSmartPointer<vtkIdTypeArray>::New();
  this->Internal->PointsToSendToProcessesLengths = vtkSmartPointer<vtkIdTypeArray>::New();
  this->Internal->PointsToSendToProcessesLengths->SetNumberOfTuples(this->NumberOfPieces);
  this->Internal->PointsToSendToProcessesOffsets = vtkSmartPointer<vtkIdTypeArray>::New();
  this->Internal->PointsToSendToProcessesOffsets->SetNumberOfTuples(this->NumberOfPieces);

  // Record how many global points there are.
  int coordsVarId;
  CALL_NETCDF(nc_inq_varid(meshFD, "coords", &coordsVarId));
  this->NumberOfGlobalPoints
    = this->GetNumTuplesInVariable(meshFD, coordsVarId, 3);

  // Iterate over our LocalToGlobalIds map and determine which process reads
  // which points.  We also fill out GlobalToLocalIds.  Until this point we
  // only have keys and we need to set the values.
  vtkIdType localId = 0;
  vtkIdType numLocalIds = this->Internal->LocalToGlobalIds->GetNumberOfTuples();
  for (int process = 0; process < this->NumberOfPieces; process++)
    {
    VTK_CREATE(vtkIdTypeArray, pointList);
    pointList->Allocate(this->NumberOfGlobalPoints/this->NumberOfPieces,
                        this->NumberOfGlobalPoints/this->NumberOfPieces);
    vtkIdType lastId = this->EndPointRead(process);
    for ( ; (localId < numLocalIds); localId++)
      {
      vtkIdType globalId = this->Internal->LocalToGlobalIds->GetValue(localId);
      if (globalId >= lastId) break;
      this->Internal->GlobalToLocalIds[globalId] = localId;
      pointList->InsertNextValue(globalId);
      }

    // pointList now has all the global ids for points that will be loaded by
    // process.  Send those ids to process so that it knows what data to send
    // back when reading in point data.
    vtkIdType numPoints = pointList->GetNumberOfTuples();
    this->Internal->PointsExpectedFromProcessesLengths->SetValue(process, numPoints);
    this->Controller->Gather(&numPoints,
                             this->Internal->PointsToSendToProcessesLengths->WritePointer(0,this->NumberOfPieces),
                             1, process);
    vtkIdType offset = 0;
    if (process == this->RequestedPiece)
      {
      for (int i = 0; i < this->NumberOfPieces; i++)
        {
        this->Internal->PointsToSendToProcessesOffsets->SetValue(i, offset);
        offset += this->Internal->PointsToSendToProcessesLengths->GetValue(i);
        }
      this->Internal->PointsToSendToProcesses->SetNumberOfTuples(offset);
      }
    this->Controller->GatherV(
                pointList->GetPointer(0),
                this->Internal->PointsToSendToProcesses->WritePointer(0,offset),
                numPoints,
                this->Internal->PointsToSendToProcessesLengths->GetPointer(0),
                this->Internal->PointsToSendToProcessesOffsets->GetPointer(0),
                process);
    }

  // Calculate the offsets for the incoming point data into the local array.
  vtkIdType offset = 0;
  for (int process = 0; process < this->NumberOfPieces; process++)
    {
    this->Internal->PointsExpectedFromProcessesOffsets->SetValue(process,
                                                                 offset);
    offset
      += this->Internal->PointsExpectedFromProcessesLengths->GetValue(process);
    }

  // Now that we have a complete map from global to local ids, modify the
  // connectivity arrays to use local ids instead of global ids.
  for (outputIter.TakeReference(compositeOutput->NewIterator());
       !outputIter->IsDoneWithTraversal(); outputIter->GoToNextItem())
    {
    vtkUnstructuredGrid *ugrid = vtkUnstructuredGrid::SafeDownCast(
                                       compositeOutput->GetDataSet(outputIter));
    vtkCellArray *cells = ugrid->GetCells();

    vtkIdType npts, *pts;
    for (cells->InitTraversal(); cells->GetNextCell(npts, pts); )
      {
      for (vtkIdType i = 0; i < npts; i++)
        {
        pts[i] = this->Internal->GlobalToLocalIds[pts[i]];
        }
      }
    }

  if (this->ReadMidpoints)
    {
    // Setup the Edge transfers
    this->Internal->EdgesExpectedFromProcessesCounts = vtkSmartPointer<vtkIdTypeArray>::New();
    this->Internal->EdgesExpectedFromProcessesCounts->SetNumberOfTuples(this->NumberOfPieces);
    this->Internal->EdgesToSendToProcesses = vtkSmartPointer<vtkIdTypeArray>::New();
    this->Internal->EdgesToSendToProcessesLengths = vtkSmartPointer<vtkIdTypeArray>::New();
    this->Internal->EdgesToSendToProcessesLengths->SetNumberOfTuples(this->NumberOfPieces);
    this->Internal->EdgesToSendToProcessesOffsets = vtkSmartPointer<vtkIdTypeArray>::New();
    this->Internal->EdgesToSendToProcessesOffsets->SetNumberOfTuples(this->NumberOfPieces);

    std::vector< vtkSmartPointer<vtkIdTypeArray> > edgeLists (this->NumberOfPieces);
    for (int process = 0; process < this->NumberOfPieces; process ++)
      {
      edgeLists[process] = vtkSmartPointer<vtkIdTypeArray>::New ();
      edgeLists[process]->SetNumberOfComponents (2);
      }
    int pointsPerProcess = this->NumberOfGlobalPoints/this->NumberOfPieces + 1;
    for (size_t i = 0; i < edgesNeeded.size (); i ++)
      {
      int process = edgesNeeded[i].GetMinEndPoint() / pointsPerProcess;
      vtkIdType ids[2];
      ids[0] = edgesNeeded[i].GetMinEndPoint();
      ids[1] = edgesNeeded[i].GetMaxEndPoint();
      edgeLists[process]->InsertNextTupleValue(static_cast<vtkIdType*>(ids));
      }
    for (int process = 0; process < this->NumberOfPieces; process ++)
      {
      vtkIdType numEdges = edgeLists[process]->GetNumberOfTuples();
      this->Internal->EdgesExpectedFromProcessesCounts->SetValue(process,
                                                                 numEdges);
      this->Controller->Gather(&numEdges,
                               this->Internal->EdgesToSendToProcessesLengths->WritePointer(0,this->NumberOfPieces),
                               1, process);
      offset = 0;
      if (process == this->RequestedPiece)
        {
        for (int i = 0; i < this->NumberOfPieces; i++)
          {
          this->Internal->EdgesToSendToProcessesOffsets->SetValue(i, offset);
          int len
            = this->Internal->EdgesToSendToProcessesLengths->GetValue(i) * 2;
          this->Internal->EdgesToSendToProcessesLengths->SetValue (i, len);
          offset += len;
          }
        }
      this->Internal->EdgesToSendToProcesses->SetNumberOfComponents (2);
      this->Internal->EdgesToSendToProcesses->SetNumberOfTuples (offset/2);
      this->Controller->GatherV(
                 edgeLists[process]->GetPointer(0),
                 this->Internal->EdgesToSendToProcesses->WritePointer(0,offset),
                 numEdges*2,
                 this->Internal->EdgesToSendToProcessesLengths->GetPointer(0),
                 this->Internal->EdgesToSendToProcessesOffsets->GetPointer(0),
                 process);
      }
    }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkPSLACReader::RestoreMeshCache(vtkMultiBlockDataSet *surfaceOutput,
                                     vtkMultiBlockDataSet *volumeOutput,
                                     vtkMultiBlockDataSet *compositeOutput)
{
  if (!this->Superclass::RestoreMeshCache(surfaceOutput, volumeOutput,
                                          compositeOutput)) return 0;

  // Record the global ids in the point data.
  vtkPointData *pd = vtkPointData::SafeDownCast(
           compositeOutput->GetInformation()->Get(vtkSLACReader::POINT_DATA()));
  pd->SetGlobalIds(this->Internal->LocalToGlobalIds);
  pd->SetPedigreeIds(this->Internal->LocalToGlobalIds);

  return 1;
}

//-----------------------------------------------------------------------------
vtkSmartPointer<vtkDataArray> vtkPSLACReader::ReadPointDataArray(int ncFD,
                                                                 int varId)
{
  // Get the dimension info.  We should only need to worry about 1 or 2D arrays.
  int numDims;
  CALL_NETCDF(nc_inq_varndims(ncFD, varId, &numDims));
  if (numDims > 2)
    {
    vtkErrorMacro(<< "Sanity check failed.  "
                  << "Encountered array with too many dimensions.");
    return 0;
    }
  if (numDims < 1)
    {
    vtkErrorMacro(<< "Sanity check failed.  "
                  << "Encountered array with *no* dimensions.");
    return 0;
    }
  int dimIds[2];
  CALL_NETCDF(nc_inq_vardimid(ncFD, varId, dimIds));
  size_t numCoords;
  CALL_NETCDF(nc_inq_dimlen(ncFD, dimIds[0], &numCoords));
  if (numCoords != static_cast<size_t>(this->NumberOfGlobalPoints))
    {
    vtkErrorMacro(<< "Encountered inconsistent number of coordinates.");
    return 0;
    }
  size_t numComponents = 1;
  if (numDims > 1)
    {
    CALL_NETCDF(nc_inq_dimlen(ncFD, dimIds[1], &numComponents));
    }

  // Allocate an array of the right type.
  nc_type ncType;
  CALL_NETCDF(nc_inq_vartype(ncFD, varId, &ncType));
  int vtkType = NetCDFTypeToVTKType(ncType);
  if (vtkType < 1) return 0;
  vtkSmartPointer<vtkDataArray> dataArray;
  dataArray.TakeReference(vtkDataArray::CreateDataArray(vtkType));

  // Read the data from the file.
  size_t start[2], count[2];
  start[0] = this->StartPointRead(this->RequestedPiece);
  count[0] = this->EndPointRead(this->RequestedPiece) - start[0];
  start[1] = 0;  count[1] = numComponents;
  dataArray->SetNumberOfComponents(static_cast<int>(count[1]));
  dataArray->SetNumberOfTuples(static_cast<vtkIdType>(count[0]));
  CALL_NETCDF(nc_get_vars(ncFD, varId, start, count, NULL,
                          dataArray->GetVoidPointer(0)));

  // We now need to redistribute the data.  Allocate an array to store the final
  // point data and a buffer to send data to the rest of the processes.
  vtkSmartPointer<vtkDataArray> finalDataArray;
  finalDataArray.TakeReference(vtkDataArray::CreateDataArray(vtkType));
  finalDataArray->SetNumberOfComponents(static_cast<int>(numComponents));
  finalDataArray->SetNumberOfTuples(
                         this->Internal->LocalToGlobalIds->GetNumberOfTuples());

  vtkSmartPointer<vtkDataArray> sendBuffer;
  sendBuffer.TakeReference(vtkDataArray::CreateDataArray(vtkType));
  sendBuffer->SetNumberOfComponents(static_cast<int>(numComponents));
  sendBuffer->SetNumberOfTuples(
                  this->Internal->PointsToSendToProcesses->GetNumberOfTuples());
  switch (vtkType)
    {
    vtkTemplateMacro(vtkPSLACReaderMapValues1(
                                   (VTK_TT*)dataArray->GetVoidPointer(0),
                                   (VTK_TT*)sendBuffer->GetVoidPointer(0),
                                   static_cast<int>(numComponents),
                                   this->Internal->PointsToSendToProcesses,
                                   this->StartPointRead(this->RequestedPiece)));
    }

  // Scatter expects identifiers per value, not per tuple.  Thus, we (may)
  // need to adjust the lengths and offsets of what we send.
  VTK_CREATE(vtkIdTypeArray, sendLengths);
  sendLengths->SetNumberOfTuples(this->NumberOfPieces);
  VTK_CREATE(vtkIdTypeArray, sendOffsets);
  sendOffsets->SetNumberOfTuples(this->NumberOfPieces);
  for (int i = 0; i < this->NumberOfPieces; i++)
    {
    sendLengths->SetValue(i,
     this->Internal->PointsToSendToProcessesLengths->GetValue(i)*numComponents);
    sendOffsets->SetValue(i,
     this->Internal->PointsToSendToProcessesOffsets->GetValue(i)*numComponents);
    }

  // Let each process have a turn sending data to the other processes.
  // Upon receiving
  for (int proc = 0; proc < this->NumberOfPieces; proc++)
    {
    // Scatter data from source.  Note that lengths and offsets are only valid
    // on the source process.  All others are ignored.
    vtkIdType destLength = numComponents*this->Internal->PointsExpectedFromProcessesLengths->GetValue(proc);
    vtkIdType destOffset = numComponents*this->Internal->PointsExpectedFromProcessesOffsets->GetValue(proc);
    this->Controller->GetCommunicator()->ScatterVVoidArray(
                                     sendBuffer->GetVoidPointer(0),
                                     finalDataArray->GetVoidPointer(destOffset),
                                     sendLengths->GetPointer(0),
                                     sendOffsets->GetPointer(0),
                                     destLength, vtkType, proc);
    }

  return finalDataArray;
}

//-----------------------------------------------------------------------------
int vtkPSLACReader::ReadCoordinates(int meshFD, vtkMultiBlockDataSet *output)
{
  // The superclass reads everything correctly because it will call our
  // ReadPointDataArray method, which will properly redistribute points.
  if (!this->Superclass::ReadCoordinates(meshFD, output)) return 0;

  // This is a convenient place to set the global ids.  Doing this in
  // ReadFieldData is not a good idea as it might not be called if no mode
  // file is specified.
  vtkPointData *pd = vtkPointData::SafeDownCast(
                    output->GetInformation()->Get(vtkSLACReader::POINT_DATA()));
  pd->SetGlobalIds(this->Internal->LocalToGlobalIds);
  pd->SetPedigreeIds(this->Internal->LocalToGlobalIds);

  return 1;
}

//-----------------------------------------------------------------------------
int vtkPSLACReader::ReadFieldData(const int *modeFDArray,
                                  int numModeFDs,
                                  vtkMultiBlockDataSet *output)
{
  // The superclass reads everything correctly because it will call our
  // ReadPointDataArray method, which will properly redistribute points.
  return this->Superclass::ReadFieldData(modeFDArray, numModeFDs, output);
}

//-----------------------------------------------------------------------------
int vtkPSLACReader::ReadMidpointCoordinates (
                                   int meshFD,
                                   vtkMultiBlockDataSet *vtkNotUsed(output),
                                   vtkSLACReader::MidpointCoordinateMap &map)
{
  // Get the number of midpoints.
  int midpointsVar;
  CALL_NETCDF(nc_inq_varid(meshFD, "surface_midpoint", &midpointsVar));
  this->NumberOfGlobalMidpoints = this->GetNumTuplesInVariable(meshFD,midpointsVar,5);
  if (this->NumberOfGlobalMidpoints < 1) return 0;

  vtkIdType numMidpointsPerPiece = this->NumberOfGlobalMidpoints/this->NumberOfPieces + 1;
  vtkIdType startMidpoint = this->RequestedPiece*numMidpointsPerPiece;
  vtkIdType endMidpoint = startMidpoint + numMidpointsPerPiece;
  if (endMidpoint > this->NumberOfGlobalMidpoints)
    {
    endMidpoint = this->NumberOfGlobalMidpoints;
    }

  size_t starts[2];
  size_t counts[2];

  starts[0] = startMidpoint;  counts[0] = endMidpoint - startMidpoint;
  starts[1] = 0;              counts[1] = 5;

  VTK_CREATE (vtkDoubleArray, midpointData);
  midpointData->SetNumberOfComponents(static_cast<int>(counts[1]));
  midpointData->SetNumberOfTuples(static_cast<vtkIdType>(counts[0]));
  CALL_NETCDF(nc_get_vars_double(meshFD, midpointsVar,
                                 starts, counts, NULL,
                                 midpointData->GetPointer(0)));

  // Collect the midpoints we've read on the processes that originally read the
  // corresponding main points (the edge the midpoint is on).  These original
  // processes are aware of who requested hose original points.  Thus they can
  // redistribute the midpoints that correspond to those processes that
  // requested the original points.
  std::vector<midpointListsType> midpointsToDistribute(this->NumberOfPieces);

  int pointsPerProcess = this->NumberOfGlobalPoints / this->NumberOfPieces + 1;
  for (vtkIdType i = 0; i < midpointData->GetNumberOfTuples(); i ++)
    {
    double *mp = midpointData->GetPointer(i*5);

    midpointPositionType position;
    position.coord[0] = mp[2];
    position.coord[1] = mp[3];
    position.coord[2] = mp[4];

    midpointTopologyType topology;
    topology.minEdgePoint = static_cast<vtkIdType>(MY_MIN(mp[0], mp[1]));
    topology.maxEdgePoint = static_cast<vtkIdType>(MY_MAX(mp[0], mp[1]));
    topology.globalId = i + startMidpoint + this->NumberOfGlobalPoints;

    // find the processor the minimum edge point belongs to (by global id)
    vtkIdType process = topology.minEdgePoint / pointsPerProcess;

    // insert the midpoint's global point id into the data
    midpointsToDistribute[process].position.push_back(position);
    midpointsToDistribute[process].topology.push_back(topology);
    }

  midpointListsType midpointsToRedistribute;
  for (int process = 0; process < this->NumberOfPieces; process++)
    {
    GatherMidpoints(this->Controller, midpointsToDistribute[process],
                    midpointsToRedistribute, process);
    }

  // Build a map of midpoints so that as processes request midpoints we can
  // quickly find them.
  MidpointsAvailableType MidpointsAvailable;

  std::vector<midpointPositionType>::iterator posIter;
  std::vector<midpointTopologyType>::iterator topIter;
  for (posIter = midpointsToRedistribute.position.begin(),
         topIter = midpointsToRedistribute.topology.begin();
       posIter != midpointsToRedistribute.position.end();
       posIter++, topIter++)
    {
    midpointPointersType mp;
    mp.position = &(*posIter);  mp.topology = &(*topIter);
#ifdef _RWSTD_NO_MEMBER_TEMPLATES
    // Deal with Sun Studio old libCstd.
    // http://sahajtechstyle.blogspot.com/2007/11/whats-wrong-with-sun-studio-c.html
    MidpointsAvailable.insert(
      std::pair<const EdgeEndpoints,midpointPointersType>(
        EdgeEndpoints(topIter->minEdgePoint,topIter->maxEdgePoint),mp));
#else
    MidpointsAvailable.insert(
      std::make_pair(EdgeEndpoints(topIter->minEdgePoint,
                                      topIter->maxEdgePoint),
                        mp));
#endif
    }

  // For each process, find the midpoints we need to send there and then
  // send them with a gather operation.
  midpointListsType midpointsToReceive;
  for (int process = 0; process < this->NumberOfPieces; process++)
    {
    vtkIdType start
      = this->Internal->EdgesToSendToProcessesOffsets->GetValue(process);
    vtkIdType end
      = start +this->Internal->EdgesToSendToProcessesLengths->GetValue(process);

    start /= this->Internal->EdgesToSendToProcesses->GetNumberOfComponents();
    end /= this->Internal->EdgesToSendToProcesses->GetNumberOfComponents();

    midpointListsType midpointsToSend;
    for (vtkIdType i = start; i < end; i ++)
      {
      MidpointsAvailableType::const_iterator iter;
      vtkIdType e[2];
      this->Internal->EdgesToSendToProcesses->GetTupleValue(i, e);
      iter = MidpointsAvailable.find(EdgeEndpoints(e[0], e[1]));
      if (iter != MidpointsAvailable.end ())
        {
        midpointsToSend.position.push_back(*iter->second.position);
        midpointsToSend.topology.push_back(*iter->second.topology);
        }
      else // in order to have the proper length we must insert empty.
        {
        midpointPositionType position;
        position.coord[0]=-1;  position.coord[1]=-1;  position.coord[2]=-1;
        midpointTopologyType topology;
        topology.minEdgePoint = -1;  topology.maxEdgePoint = -1;
        topology.globalId = -1;
        midpointsToSend.position.push_back(position);
        midpointsToSend.topology.push_back(topology);
        }
      }

    GatherMidpoints(this->Controller, midpointsToSend,
                    midpointsToReceive, process);
    }

  // finally, we have all midpoints that correspond to edges we know about
  // convert their edge points to localId and insert into the map and return.
  typedef vtksys::hash_map<vtkIdType, vtkIdType, vtkPSLACReaderIdTypeHash> localMapType;
  localMapType localMap;
  for (posIter = midpointsToReceive.position.begin(),
         topIter = midpointsToReceive.topology.begin();
       posIter != midpointsToReceive.position.end();
       posIter++, topIter++)
    {
    if (topIter->globalId < 0) continue;

    vtkIdType local0 = this->Internal->GlobalToLocalIds[topIter->minEdgePoint];
    vtkIdType local1 = this->Internal->GlobalToLocalIds[topIter->maxEdgePoint];
    localMapType::const_iterator iter;
    iter = localMap.find(topIter->globalId);
    vtkIdType index;
    if (iter == localMap.end())
      {
      index = this->Internal->LocalToGlobalIds->InsertNextTupleValue(
                                                            &topIter->globalId);
      localMap[topIter->globalId] = index;
      }
    else
      {
      index = iter->second;
      }
    map.AddMidpoint(vtkSLACReader::EdgeEndpoints(local0, local1),
                    vtkSLACReader::MidpointCoordinates(posIter->coord, index));
    }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkPSLACReader::ReadMidpointData(int meshFD, vtkMultiBlockDataSet *output,
                                     vtkSLACReader::MidpointIdMap &map)
{
  int result = this->Superclass::ReadMidpointData(meshFD, output, map);
  if (result != 1)
    {
    return result;
    }
  // add global IDs for midpoints added that weren't in the file
  vtkPoints *points = vtkPoints::SafeDownCast(
                        output->GetInformation()->Get(vtkSLACReader::POINTS()));
  vtkIdType pointsAdded = points->GetNumberOfPoints () -
          this->Internal->LocalToGlobalIds->GetNumberOfTuples ();
  // Use the maximum number of points added so that the offsets don't overlap
  // There will be gaps and shared edges between two processes will get different ids
  // TODO: Will this cause problems?
  vtkIdType maxPointsAdded;
  this->Controller->AllReduce (&pointsAdded, &maxPointsAdded, 1, vtkCommunicator::MAX_OP);

  vtkIdType start = this->NumberOfGlobalPoints + this->NumberOfGlobalMidpoints +
          this->RequestedPiece*maxPointsAdded;
  vtkIdType end = start + pointsAdded;
  for (vtkIdType i = start; i < end; i ++)
    {
    this->Internal->LocalToGlobalIds->InsertNextTupleValue (&i);
    }

  return 1;
}

//-----------------------------------------------------------------------------
int vtkPSLACReader::MeshUpToDate()
{
  int localflag = this->Superclass::MeshUpToDate();
  localflag &= (this->NumberOfPieces != this->NumberOfPiecesCache);
  localflag &= (this->RequestedPieceCache != this->RequestedPiece);

  int globalflag;
  this->Controller->AllReduce(&localflag, &globalflag, 1,
                              vtkCommunicator::LOGICAL_AND_OP);
  return globalflag;
}
