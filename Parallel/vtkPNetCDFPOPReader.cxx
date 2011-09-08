/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkPNetCDFPOPReader.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPNetCDFPOPReader.h"
#include "vtkCallbackCommand.h"
#include "vtkDataArraySelection.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkRectilinearGrid.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include "vtkMPI.h"  // added by RGM
#include "vtkMPIController.h" // added by RGM

#include "vtk_netcdf.h"
#include <string>
#include <vector>
#include <set>

#ifdef MPI_Comm
    #error MPI_Comm is #define'd somewhere!  That's BAD!  (Try checking netcdf.h.)
#endif

vtkStandardNewMacro(vtkPNetCDFPOPReader);

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

//============================================================================
class vtkPNetCDFPOPReaderInternal
{
public:
  vtkSmartPointer<vtkDataArraySelection> VariableArraySelection;
  // a mapping from the list of all variables to the list of available
  // point-based variables
  std::vector<int> VariableMap;

  // Added by RGM
//  int numProcs, rank;  // MPI rank info

  // Pointers to the buffers where we stored the values for each depth that we
  // read.  We can't delete the buffers until we know all the send requests that
  // used them (there may be more than one send for each buffer) have completed.
  std::vector <float *> SendBufs;

  // MPI ranks of the processes that will actually do the netCDF reads
  // NOTE: I'm currently getting these from an environment variable, but
  // I'm not certain that's the best way.  (If the var doesn't exist, I've
  // added what I hope are sane defaults.  See SetReaderRanks().)
  std::vector<int> ReaderRanks;

  // Memory to hold the extents for all processes (reader processes need this,
  // others can delete it after the Allgather operation, but it's still more
  // efficient to do an Allgather than to a bunch of individual Gathers.)
  int *P_allExtents;

  // MPI_Request identifiers for all the MPI_Isend() calls.
  std::vector <MPI_Request> SendReqs;

  //////////////////////


  vtkPNetCDFPOPReaderInternal()
    {
      this->VariableArraySelection =
        vtkSmartPointer<vtkDataArraySelection>::New();
    }
};

//----------------------------------------------------------------------------
//set default values
vtkPNetCDFPOPReader::vtkPNetCDFPOPReader()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  this->FileName = NULL;
  this->OpenedFileName = NULL;
  this->Stride[0] = this->Stride[1] = this->Stride[2] = 1;
  this->SelectionObserver = vtkCallbackCommand::New();
  this->SelectionObserver->SetCallback
    (&vtkPNetCDFPOPReader::SelectionModifiedCallback);
  this->SelectionObserver->SetClientData(this);
  this->Internals = new vtkPNetCDFPOPReaderInternal;
  this->Internals->VariableArraySelection->AddObserver(
    vtkCommand::ModifiedEvent, this->SelectionObserver);
  this->Controller = NULL;
  this->SetController(vtkMPIController::SafeDownCast(
                        vtkMultiProcessController::GetGlobalController()));
  this->SetReaderRanks(NULL);
  this->NCDFFD = -1;
}

//----------------------------------------------------------------------------
//delete filename and netcdf file descriptor
vtkPNetCDFPOPReader::~vtkPNetCDFPOPReader()
{
  this->SetController(NULL);
  this->SetFileName(0);
  if(this->OpenedFileName)
    {
    nc_close(this->NCDFFD);
    }
  this->SetOpenedFileName(0);
  if(this->SelectionObserver)
    {
    this->SelectionObserver->Delete();
    this->SelectionObserver = NULL;
    }
  if(this->Internals)
    {
    delete this->Internals;
    this->Internals = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkPNetCDFPOPReader::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "FileName: "
     << (this->FileName ? this->FileName : "(NULL)") << endl;
  os << indent << "OpenedFileName: "
     << (this->OpenedFileName ? this->OpenedFileName : "(NULL)") << endl;
  os << indent << "Stride: {" << this->Stride[0] << ", "
     << this->Stride[1] << ", " << this->Stride[2] << ", "
     << "}" << endl;
  if(this->Controller)
    {
    os << indent << "Controller: " << this->Controller << endl;
    }
  else
    {
    os << indent << "Controller: (NULL)" << endl;
    }
  os << indent << "NCDFFD: " << this->NCDFFD << endl;

  this->Internals->VariableArraySelection->PrintSelf(os, indent.GetNextIndent());
}

//----------------------------------------------------------------------------
// RequestInformation supplies global meta information
// This should return the reality of what the reader is going to supply.
// This retrieve the extents for the rectilinear grid
// NC_MAX_VAR_DIMS comes from the nc library
int vtkPNetCDFPOPReader::RequestInformation(
    vtkInformation* vtkNotUsed(request),
    vtkInformationVector** vtkNotUsed(inputVector),
    vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if(this->FileName == NULL)
    {
    vtkErrorMacro("FileName not set.");
    return 0;
    }

  // every rank that is a reader process needs to open the file
  if (this->IsReaderRank())
    {
    bool openFile = true;
    if(this->OpenedFileName != NULL)
      {
      if(strcmp(this->OpenedFileName, this->FileName) == 0)
        {
        openFile = false;
        }
      else
        {
        nc_close(this->NCDFFD);
        }
      }
    if(openFile)
      {
      int retval = nc_open(this->FileName, NC_NOWRITE, &this->NCDFFD);//read file
      if (retval != NC_NOERR)//checks if read file error
        {
        vtkErrorMacro(<< "Can't read file " << nc_strerror(retval));
        this->SetOpenedFileName(0);
        return 0;
        }
      }
    this->SetOpenedFileName(this->FileName);
    }
  // first reader reads the metadata and broadcasts it to everyone else
  int extent[6];
  char variableName[NC_MAX_NAME+1];
  if (this->IsFirstReaderRank())
    {
    // get number of variables from file
    int numberOfVariables;
    nc_inq_nvars(this->NCDFFD, &numberOfVariables);
    int dimidsp[NC_MAX_VAR_DIMS];
    int dataDimension;
    size_t dimensions[4]; //dimension value
    this->Internals->VariableMap.resize(numberOfVariables);
    int actualVariableCounter = 0;
    // For every variable in the file
    for(int i=0;i<numberOfVariables;i++)
      {
      this->Internals->VariableMap[i] = -1;
      //get number of dimensions
      CALL_NETCDF(nc_inq_varndims(this->NCDFFD, i, &dataDimension));
      //Variable Dimension ID's containing x,y,z coords for the rectilinear
      //grid spacing
      CALL_NETCDF(nc_inq_vardimid(this->NCDFFD, i, dimidsp));
      if(dataDimension == 3)
        {
        this->Internals->VariableMap[i] = actualVariableCounter++;
        //get variable name
        CALL_NETCDF(nc_inq_varname(this->NCDFFD, i, variableName));
        this->Internals->VariableArraySelection->AddArray(variableName);
        for(int m=0;m<dataDimension;m++)
          {
          CALL_NETCDF(nc_inq_dimlen(this->NCDFFD, dimidsp[m], dimensions+m));
          //acquire variable dimensions
          }
        extent[0] = extent[2] = extent[4] =0; //set extent
        extent[1] = static_cast<int>((dimensions[2] -1) / this->Stride[0]);
        extent[3] = static_cast<int>((dimensions[1] -1) / this->Stride[1]);
        extent[5] = static_cast<int>((dimensions[0] -1) / this->Stride[2]);
        }
      }

    // We've read in all the metadata.  Now broadcast it to the other ranks

    // There's probably only 1 variable name, but we'll allow for more just in case
    if(this->Controller)
      {
      int numNames = this->Internals->VariableArraySelection->GetNumberOfArrays();
      this->Controller->Broadcast( &numNames, 1,
                                   this->Internals->ReaderRanks[0]);
      for (int i = 0; i < numNames; i++)
        {
        // I don't really like this extra strcpy, but I want to broadcast a string of
        // known, fixed size and the pointer retrurned by GetArrayName could point to
        // considerably less than NC_MAX_NAME chars, which means I'd risk a segfault if
        // I didn't do the copy.
        strcpy( variableName, this->Internals->VariableArraySelection->GetArrayName( i));
        this->Controller->Broadcast( variableName, NC_MAX_NAME+1,
                                     this->Internals->ReaderRanks[0]);
        }

      // Send out the variable map data
      int numVariables = static_cast<int>(this->Internals->VariableMap.size());
      this->Controller->Broadcast( &numVariables, 1,
                                   this->Internals->ReaderRanks[0]);
      this->Controller->Broadcast( &this->Internals->VariableMap[0], numVariables,
                                   this->Internals->ReaderRanks[0]);

      // send out the extents data
      this->Controller->Broadcast( extent, 6, this->Internals->ReaderRanks[0]);
      }
    }
  else  // everyone else listens for the broadcasted metadata and fills in
        // Internals->VariableMap and extent[]
    {
    // Receive the variable name(s)  (probably only 1 name)
    int numNames = 0;
    this->Controller->Broadcast( &numNames, 1,
                                 this->Internals->ReaderRanks[0]);
    for (int i=0; i < numNames; i++)
      {
      this->Controller->Broadcast( variableName, NC_MAX_NAME+1,
                                   this->Internals->ReaderRanks[0]);
      this->Internals->VariableArraySelection->AddArray(variableName);
      }

    // Receive the variable map data
    int numVariables = 0;
    this->Controller->Broadcast( &numVariables, 1,
                                 this->Internals->ReaderRanks[0]);
    this->Internals->VariableMap.resize( numVariables, 0);
    this->Controller->Broadcast( &this->Internals->VariableMap[0], numVariables,
                                 this->Internals->ReaderRanks[0]);
    // Receive the extents data
    this->Controller->Broadcast( extent, 6, this->Internals->ReaderRanks[0]);
    }

  //fill in the extent information
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),extent,6);
  return 1;
}

//----------------------------------------------------------------------------
// Setting extents of the rectilinear grid
int vtkPNetCDFPOPReader::RequestData(vtkInformation* request,
    vtkInformationVector** vtkNotUsed(inputVector),
    vtkInformationVector* outputVector  )
{
  this->UpdateProgress(0);
  // the default implementation is to do what the old pipeline did find what
  // output is requesting the data, and pass that into ExecuteData
  // which output port did the request come from
  int outputPort = request->Get(vtkDemandDrivenPipeline::FROM_OUTPUT_PORT());
  // if output port is negative then that means this filter is calling the
  // update directly, in that case just assume port 0
  if (outputPort == -1)
    {
    outputPort = 0;
    }

  // get the data object
  vtkInformation *outInfo = outputVector->GetInformationObject(outputPort);
  vtkDataObject* output = outInfo->Get(vtkDataObject::DATA_OBJECT());

  int subext[6];
  //vtkInformation * outInfo = output->GetInformationObject(0);
  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),subext);

  vtkRectilinearGrid *rgrid = vtkRectilinearGrid::SafeDownCast(output);
  rgrid->SetExtent(subext);

  size_t count[]= { subext[5]-subext[4]+1, subext[3]-subext[2]+1,
                    subext[1]-subext[0]+1};

  //initialize memory (raw data space, x y z axis space) and rectilinear grid
  bool firstPass = true;
  for(size_t i=0;i<this->Internals->VariableMap.size();i++)
    {
    if(this->Internals->VariableMap[i] != -1 &&
       this->Internals->VariableArraySelection->GetArraySetting(
         this->Internals->VariableMap[i]) != 0)
      {
      // varidp is probably i in which case nc_inq_varid isn't needed
      int varidp = -1;
      if (this->IsReaderRank())
        {
        nc_inq_varid(this->NCDFFD,
                     this->Internals->VariableArraySelection->GetArrayName(
                     this->Internals->VariableMap[i]), &varidp);
        }

      if(firstPass == true)
        {
        firstPass = false;
        // Get the latitude, longitude & depth values: the first reader process
        // does the read and broadcasts to everyone

        //set up start and stride values for this process's data (count is already set)
        size_t start[]= { subext[4], subext[2], subext[0] };

        ptrdiff_t rStride[3] = { (ptrdiff_t)this->Stride[2],
                                 (ptrdiff_t)this->Stride[1],
                                 (ptrdiff_t)this->Stride[0] };

        int wholeExtent[6];
        outInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),wholeExtent);
        // The Z & X dimensions have been swapped in wholeExtent (see the comments at the top
        // of the header file).  We want the arrays below to reflect the on-disk layout, which
        // is why the array indexes don't match up

        size_t wholeCount[3] = { wholeExtent[5] - wholeExtent[4] + 1,
                                 wholeExtent[3] - wholeExtent[2] + 1,
                                 wholeExtent[1] - wholeExtent[0] + 1};

        size_t wholeStart[3]= { wholeExtent[4]*this->Stride[2],
                                wholeExtent[2]*this->Stride[1],
                                wholeExtent[0]*this->Stride[0]};

        float *p_buff = new float[wholeCount[0] + wholeCount[1] + wholeCount[2]];
        if (this->IsFirstReaderRank())
          {
          int dimidsp[3];
          nc_inq_vardimid(this->NCDFFD, varidp, dimidsp);
          nc_get_vars_float(this->NCDFFD, dimidsp[0], wholeStart, wholeCount,
                            rStride, &p_buff[0]);
          nc_get_vars_float(this->NCDFFD, dimidsp[1], wholeStart+1, wholeCount+1,
                            rStride+1, &p_buff[wholeCount[0]]);
          nc_get_vars_float(this->NCDFFD, dimidsp[2], wholeStart+2, wholeCount+2,
                            rStride+2, &p_buff[wholeCount[0] + wholeCount[1]]);
          }

        if(this->Controller)
          {
          this->Controller->Broadcast( p_buff, (wholeCount[0] + wholeCount[1] + wholeCount[2]),
                                       this->Internals->ReaderRanks[0]);
          }

        // Extract the values we need out of p_buff and store them in the x, y & z buffers
        float* x = new float[count[0]];
        float* y = new float[count[1]];
        float* z = new float[count[2]];
        memcpy( x, &p_buff[0 + start[0]],                            count[0] * sizeof( float));
        memcpy( y, &p_buff[wholeCount[0] + start[1]],                count[1] * sizeof( float));
        memcpy( z, &p_buff[wholeCount[0] + wholeCount[1]+ start[2]], count[2] * sizeof( float));
        delete[] p_buff;  // don't need p_buff once we've copied in to x, y & z

        // Note the axis swap:  xcoords gets the z data and zcoords gets the x data
        vtkFloatArray *xCoords = vtkFloatArray::New();
        xCoords->SetArray(z, count[2], 0, 1);
        vtkFloatArray *yCoords = vtkFloatArray::New();
        yCoords->SetArray(y, count[1], 0, 1);
        for (unsigned int q=0; q<count[0]; q++)
          {
          x[q] = -x[q];
          }
        vtkFloatArray *zCoords = vtkFloatArray::New();
        zCoords->SetArray(x, count[0], 0, 1);
        rgrid->SetXCoordinates(xCoords);
        rgrid->SetYCoordinates(yCoords);
        rgrid->SetZCoordinates(zCoords);
        xCoords->Delete();
        yCoords->Delete();
        zCoords->Delete();
        }
      //create vtkFloatArray and get the scalars into it
      vtkFloatArray *scalars = vtkFloatArray::New();
      vtkIdType numberOfTuples = (count[0])*(count[1])*(count[2]);
      float* data = new float[numberOfTuples];

      if(this->Controller == NULL)
        {
        size_t start[]= {subext[4]*this->Stride[2], subext[2]*this->Stride[1],
                         subext[0]*this->Stride[0]};

        ptrdiff_t rStride[3] = { (ptrdiff_t)this->Stride[2], (ptrdiff_t)this->Stride[1],
                                 (ptrdiff_t)this->Stride[0] };

        nc_get_vars_float(this->NCDFFD, varidp, start, count, rStride, data);
        }
      else // parallel communication of point/cell data arrays
        {
        // Do a gather of all process' sub-extents so that the reader processes will know
        // who needs what data.  (An AllGather() operation is somewhat wasteful, because
        // even processes that aren't readers will still receive the data, but it's still
        // more efficient than having everyone send extents to each individual reader process.)
        int mpiNumProcs = this->Controller->GetNumberOfProcesses();
        this->Internals->P_allExtents = new int[ 6 * mpiNumProcs];
        this->Controller->AllGather(subext, this->Internals->P_allExtents, 6);

        // First, post all the receive requests
        std::vector <MPI_Request> recvReqs;  // should be 1 recv for each depth

        // Number of values stored for each depth
        //unsigned long oneDepthSize = (subext[3]-subext[2]+1) * (subext[5]-subext[4]+1);
        unsigned long oneDepthSize = static_cast<unsigned long>(count[1]*count[2]);  // should be the same value as the line above...

        for (int curDepth = subext[4]; curDepth <= subext[5]; curDepth++)
          {
          float *p_depthStart = data + ((curDepth-subext[4])*oneDepthSize);  // Where this data should start
          int sourceRank =  ReaderForDepth( curDepth);

          // We could probably use vtkMPIController::NoBlockReceive(), but but I'm not sure
          // how it will behave since we're calling MPI_Isend() directly.  So, I'm going
          // to call MPI_Irecv directly, too...

          MPI_Comm *p_comm = ((vtkMPICommunicator *)this->Controller->GetCommunicator())->GetMPIComm()->GetHandle();
          MPI_Request recvReq;
          MPI_Irecv( p_depthStart, oneDepthSize, MPI_FLOAT, sourceRank, curDepth, *p_comm, &recvReq);
          recvReqs.push_back( recvReq);
          }


        if (this->IsReaderRank())
          {
          // Reads part of the netCDF file and sends subarrays out to all the ranks
          this->ReadAndSend( outInfo, varidp);
          }

        delete[] this->Internals->P_allExtents;

        // Wait for all the sends to complete
        if (this->Internals->SendReqs.size() > 0)
          {
          MPI_Waitall( static_cast<int>(this->Internals->SendReqs.size()), &this->Internals->SendReqs[0], MPI_STATUSES_IGNORE);

          // Now that all the sends are complete, it's safe to free the buffers
          for (size_t j= 0; j < this->Internals->SendBufs.size(); j++)
            {
            delete[] this->Internals->SendBufs[j];
            }
          this->Internals->SendBufs.clear();
          this->Internals->SendReqs.clear();
          }

        MPI_Waitall( static_cast<int>(recvReqs.size()), &recvReqs[0], MPI_STATUSES_IGNORE);
        recvReqs.clear();
        }

      scalars->SetArray(data, numberOfTuples, 0, 1);
      //set list of variables to display data on rectilinear grid
      scalars->SetName(this->Internals->VariableArraySelection->GetArrayName(
                         this->Internals->VariableMap[i]));
      rgrid->GetPointData()->AddArray(scalars);
      scalars->Delete();
      }
    this->UpdateProgress((i+1.0)/this->Internals->VariableMap.size());
    }
  return 1;
}

//----------------------------------------------------------------------------
//following 5 functions are used for paraview user interface
void vtkPNetCDFPOPReader::SelectionModifiedCallback(vtkObject*, unsigned long,
                                                   void* clientdata, void*)
{
  static_cast<vtkPNetCDFPOPReader*>(clientdata)->Modified();
}

//-----------------------------------------------------------------------------
int vtkPNetCDFPOPReader::GetNumberOfVariableArrays()
{
  return this->Internals->VariableArraySelection->GetNumberOfArrays();
}

//-----------------------------------------------------------------------------
const char* vtkPNetCDFPOPReader::GetVariableArrayName(int index)
{
  if(index < 0 || index >= this->GetNumberOfVariableArrays())
    {
    return NULL;
    }
  return this->Internals->VariableArraySelection->GetArrayName(index);
}

//-----------------------------------------------------------------------------
int vtkPNetCDFPOPReader::GetVariableArrayStatus(const char* name)
{
  return this->Internals->VariableArraySelection->ArrayIsEnabled(name);
}

//-----------------------------------------------------------------------------
void vtkPNetCDFPOPReader::SetVariableArrayStatus(const char* name, int status)
{
  vtkDebugMacro("Set cell array \"" << name << "\" status to: " << status);
  if(this->Internals->VariableArraySelection->ArrayExists(name) == 0)
    {
    vtkErrorMacro(<< name << " is not available in the file.");
    return;
    }
  int enabled = this->Internals->VariableArraySelection->ArrayIsEnabled(name);
  if(status != 0 && enabled == 0)
    {
    this->Internals->VariableArraySelection->EnableArray(name);
    this->Modified();
    }
  else if(status == 0 && enabled != 0)
    {
    this->Internals->VariableArraySelection->DisableArray(name);
    this->Modified();
    }
}


// New functions added by RGM
namespace
{
  void swap( int &A, int &B) { int temp = B; B=A; A=temp;}
}

//----------------------------------------------------------------------------
// Helper function for RequestData.  Reads one or more depth arrays from the
// netCDF file and sends sub-arrays out to all ranks that need that data
int vtkPNetCDFPOPReader::ReadAndSend( vtkInformation *outInfo, int varID)
{
  int wholeExtent[6];
  outInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),wholeExtent);
  // Z & X dimensions have been swapped (see the comments at the top of the header
  // file).  All arrays in this function, however, should reflect the on-disk
  // layout, so we'll 'un-swap' the X & Z extents.
  swap( wholeExtent[0], wholeExtent[4]);
  swap( wholeExtent[1], wholeExtent[5]);

  // this->Stride is also in the in-memory layout
  ptrdiff_t rStride[3] = { this->Stride[2], this->Stride[1], this->Stride[0] };

  int rank = this->Controller->GetLocalProcessId();

  // We read one depth at a time, skipping over the depths that other reader processes will read
  for (int curDepth = wholeExtent[0]; curDepth <= wholeExtent[1]; curDepth++)
    {
    if (ReaderForDepth( curDepth) == rank)
      {
      size_t start[3] = { curDepth*rStride[0], wholeExtent[2], wholeExtent[4] };
      size_t count[3] = { 1, wholeExtent[3] - wholeExtent[2] + 1, wholeExtent[5] - wholeExtent[4] + 1 };

      float *p_buf = new float[count[1] * count[2]];
      this->Internals->SendBufs.push_back( p_buf);

      int ncErr = nc_get_vars_float( this->NCDFFD, varID, start, count, rStride, p_buf);
      if (ncErr != NC_NOERR)
        {
        vtkErrorMacro("nc_get_vars_float() returned error code " << ncErr);
        }

      // Create sub arrays and send to all processes
      for (int destRank = 0; destRank < this->Controller->GetNumberOfProcesses(); destRank++)
        {
        int destExtent[6];
        memcpy( destExtent, &this->Internals->P_allExtents[destRank*6], 6*sizeof( int));
        // Note that P_allExtents is also in in-memory layout order, so we need to swap
        // the X & Z values
        swap( destExtent[0], destExtent[4]);
        swap( destExtent[1], destExtent[5]);

        // Verify that destRank does, in fact, receive an extent at this depth
        // TODO: Don't use MPI to send data to ourself.....
        if (curDepth >= destExtent[0] && curDepth <= destExtent[1])
          {
          int subarray_sizes[2] =    { (wholeExtent[3] - wholeExtent[2]+1), (wholeExtent[5] - wholeExtent[4]+1), };
          int subarray_subsizes[2] = { (destExtent[3] -  destExtent[2]+1),  (destExtent[5]  - destExtent[4]+1) };
          int subarray_starts[2] = { destExtent[2], destExtent[4] };
          MPI_Datatype subArrayType;
          MPI_Request sendReq;
          MPI_Type_create_subarray( 2, subarray_sizes, subarray_subsizes, subarray_starts, MPI_ORDER_C, MPI_FLOAT, &subArrayType);
          MPI_Type_commit( &subArrayType);

          // vtkMPICommunicator can't handle arbitrary types, so we'll have to do it ourselves
          MPI_Comm *p_comm = ((vtkMPICommunicator *)this->Controller->GetCommunicator())->GetMPIComm()->GetHandle();
          MPI_Isend( p_buf, 1, subArrayType, destRank, curDepth, *p_comm, &sendReq);  // using the depth value as the tag
          this->Internals->SendReqs.push_back( sendReq);
          MPI_Type_free( &subArrayType);
          }
        }

      // Check to see if any of the previous sends have completed
      // (This helps to keep the number of 'in-flight' sends to a minimum.)
      if (this->Internals->SendReqs.size())
        {
        int foundOne = 1;
        int reqIndex = 0;
        MPI_Status status; // we never actually use this, but MPI_Testany() requires it

        // MPI_Testany() will deallocate the request, so we don't need to do anything
        // except call it in a loop.
        do
          {
          MPI_Testany( static_cast<int>(this->Internals->SendReqs.size()), &this->Internals->SendReqs[0],
                       &reqIndex, &foundOne, &status);
          } while (foundOne && reqIndex != MPI_UNDEFINED);
        }
      }
    }

  return 1;
}


//----------------------------------------------------------------------------
// Another helper function for RequestData.  Check an environment variable
// for list of reader ranks.  If the variable doesn't exist, picks a default
// set.  The end result is an updated readerRanks vector in the Internals
// object.
void vtkPNetCDFPOPReader::SetReaderRanks(vtkIdList* ranks)
{
  if(this->Controller == NULL)
    {
    this->Internals->ReaderRanks.clear();
    this->Internals->ReaderRanks.push_back(0);
    return;
    }

  vtkNew<vtkIdList> readerRanks;  // temporary holder for the values.
  // We use a set so that ranks are automatically ordered and duplicates
  // are skipped.  Once the set is created, it's copied over to a vector
  // because I want to be able to use operator[] to access the values.

  int numProcs = this->Controller->GetNumberOfProcesses();

  if(ranks)
    {
    for(vtkIdType i=0;i<ranks->GetNumberOfIds();i++)
      {
      vtkIdType rank = ranks->GetId(i);
      if(rank >= 0 && rank < numProcs)
        {
        readerRanks->InsertNextId(rank);
        }
      }
    }

  if (readerRanks->GetNumberOfIds() == 0)
    {
    // Either nobody set the env var or it had bogus values
    //  in it.  Try to pick a reasonable default.

    // This is somewhat arbritrary:  below 24 processes, we'll use 4 readers.
    // More than 24 processes, we'll use 8.  (>24 processes, I'm assuming we're
    // running on Jaguar where 2 readers per OSS seems to work well).
    // All readers will be evenly spread across the range of processes that
    // are working on this file

    int numReaders = numProcs < 24 ? 4 : 8;
    this->AssignRoundRobin(numReaders, readerRanks.GetPointer());
    }

  // Copy the contents of the set into its permanent location...
  this->Internals->ReaderRanks.clear();
  for(vtkIdType i=0;i<readerRanks->GetNumberOfIds();i++)
    {
    this->Internals->ReaderRanks.push_back(readerRanks->GetId(i));
    }
}

//----------------------------------------------------------------------------
// Returns the rank (relative to our controller) for the process that will
// read the specified depth
int vtkPNetCDFPOPReader::ReaderForDepth( unsigned depth)
{
  // NOTE: This is a very simple algorithm - each rank in readerRanks will
  // read single depth in a round-robbin fashion.  There might be a more
  // efficient way to do this...
  size_t numReaders = this->Internals->ReaderRanks.size();
  return this->Internals->ReaderRanks[(depth % numReaders)];
}

//----------------------------------------------------------------------------
// Returns true if the calling process should read data from the netCDF file
bool vtkPNetCDFPOPReader::IsReaderRank()
{
  if(this->Controller == NULL)
    {
    return true;
    }
  int rank = this->Controller->GetLocalProcessId();
  for (unsigned i = 0; i < this->Internals->ReaderRanks.size(); i++)
    {
    if (rank == this->Internals->ReaderRanks[i])
      return true;
    }
  return false;
}

//----------------------------------------------------------------------------
// Similar to above, but returns true only if the calling process is the first
// rank in the readerRanks vector.  (This function exists because much of the
// file metadata is read by a single rank and broadcast to all the others.)
bool vtkPNetCDFPOPReader::IsFirstReaderRank()
{
  if (this->Internals->ReaderRanks.size() == 0)
    {
    return false; // sanity check
    }
  if(this->Controller == NULL)
    {
    return true;
    }
  int rank = this->Controller->GetLocalProcessId();
  return (rank == this->Internals->ReaderRanks[0]);
}

//----------------------------------------------------------------------------
void vtkPNetCDFPOPReader::SetController(vtkMPIController *controller)
{
  if(this->Controller != controller)
    {
    this->Controller = controller;
    if (this->Controller != NULL)
      {
      this->SetReaderRanks(NULL);
      }
    }
}

//----------------------------------------------------------------------------
void vtkPNetCDFPOPReader::SetNumberOfReaderProcesses(int numReaders)
{
  vtkNew<vtkIdList> ranks;
  this->AssignRoundRobin(numReaders, ranks.GetPointer());
  this->SetReaderRanks(ranks.GetPointer());
}

//----------------------------------------------------------------------------
void vtkPNetCDFPOPReader::AssignRoundRobin(int numReaders,
                                           vtkIdList* readerRanks)
{
  int numProcs = this->Controller == NULL ? 1 : this->Controller->GetNumberOfProcesses();
  if(numReaders < 1)
    {
    numReaders = 1;
    }
  else if(numReaders > numProcs)
    {
    numReaders = numProcs;
    }
  readerRanks->SetNumberOfIds(numReaders);
  int proc = 0;
  int counter = 0;
  while (proc < numProcs)
    {
    readerRanks->SetId(counter, proc);
    proc += (numProcs / numReaders);
    counter++;
    }
  }
