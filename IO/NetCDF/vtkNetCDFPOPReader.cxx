/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkNetCDFPOPReader.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkNetCDFPOPReader.h"
#include "vtkCallbackCommand.h"
#include "vtkDataArraySelection.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkRectilinearGrid.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include "vtk_netcdf.h"
#include <string>
#include <vector>

vtkStandardNewMacro(vtkNetCDFPOPReader);

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

class vtkNetCDFPOPReaderInternal
{
public:
  vtkSmartPointer<vtkDataArraySelection> VariableArraySelection;
  // a mapping from the list of all variables to the list of available
  // point-based variables
  std::vector<int> VariableMap;
  vtkNetCDFPOPReaderInternal()
  {
      this->VariableArraySelection =
        vtkSmartPointer<vtkDataArraySelection>::New();
  }
};

//----------------------------------------------------------------------------
//set default values
vtkNetCDFPOPReader::vtkNetCDFPOPReader()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  this->FileName = NULL;
  this->NCDFFD = 0;
  this->OpenedFileName = NULL;
  this->Stride[0] = this->Stride[1] = this->Stride[2] = 1;
  this->SelectionObserver = vtkCallbackCommand::New();
  this->SelectionObserver->SetCallback
    (&vtkNetCDFPOPReader::SelectionModifiedCallback);
  this->SelectionObserver->SetClientData(this);
  this->Internals = new vtkNetCDFPOPReaderInternal;
  this->Internals->VariableArraySelection->AddObserver(
    vtkCommand::ModifiedEvent, this->SelectionObserver);
}

//----------------------------------------------------------------------------
//delete filename and netcdf file descriptor
vtkNetCDFPOPReader::~vtkNetCDFPOPReader()
{
  this->SetFileName(0);
  if(this->OpenedFileName)
  {
    nc_close(this->NCDFFD);
    this->SetOpenedFileName(NULL);
  }
  if(this->SelectionObserver)
  {
    this->SelectionObserver->Delete();
    this->SelectionObserver = NULL;
  }
  delete this->Internals;
  this->Internals = NULL;
}

//----------------------------------------------------------------------------
void vtkNetCDFPOPReader::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "FileName: "
     << (this->FileName ? this->FileName : "(NULL)") << endl;
  os << indent << "OpenedFileName: "
     << (this->OpenedFileName ? this->OpenedFileName : "(NULL)") << endl;
  os << indent << "Stride: {" << this->Stride[0] << ", "
     << this->Stride[1] << ", " << this->Stride[2] << ", "
     << "}" << endl;
  os << indent << "NCDFFD: " << this->NCDFFD << endl;

  this->Internals->VariableArraySelection->PrintSelf(os, indent.GetNextIndent());
}

//----------------------------------------------------------------------------
// RequestInformation supplies global meta information
// This should return the reality of what the reader is going to supply.
// This retrieve the extents for the rectilinear grid
// NC_MAX_VAR_DIMS comes from the nc library
int vtkNetCDFPOPReader::RequestInformation(
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

  if(this->OpenedFileName == NULL || strcmp(this->OpenedFileName, this->FileName) != 0)
  {
    if(this->OpenedFileName)
    {
      nc_close(this->NCDFFD);
      this->SetOpenedFileName(NULL);
    }
    int retval = nc_open(this->FileName, NC_NOWRITE, &this->NCDFFD);//read file
    if (retval != NC_NOERR)//checks if read file error
    {
      // we don't need to close the file if there was an error opening the file
      vtkErrorMacro(<< "Can't read file " << nc_strerror(retval));
      return 0;
    }
    this->SetOpenedFileName(this->FileName);
  }
  // get number of variables from file
  int numberOfVariables;
  nc_inq_nvars(this->NCDFFD, &numberOfVariables);
  int dimidsp[NC_MAX_VAR_DIMS];
  int extent[6];
  int dataDimension;
  size_t dimensions[4]; //dimension value
  this->Internals->VariableMap.resize(numberOfVariables);
  char variableName[NC_MAX_NAME+1];
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
  //fill in the extent information
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),extent,6);
  return 1;
}

//----------------------------------------------------------------------------
// Setting extents of the rectilinear grid
int vtkNetCDFPOPReader::RequestData(vtkInformation* request,
    vtkInformationVector** vtkNotUsed(inputVector),
    vtkInformationVector* outputVector  )
{
  this->UpdateProgress(0);
  // the default implimentation is to do what the old pipeline did find what
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
  //setup extents for netcdf library to read the netcdf data file
  size_t start[]= { static_cast<size_t>(subext[4]*this->Stride[2]),
                    static_cast<size_t>(subext[2]*this->Stride[1]),
                    static_cast<size_t>(subext[0]*this->Stride[0]) };

  size_t count[]= { static_cast<size_t>(subext[5]-subext[4]+1),
                    static_cast<size_t>(subext[3]-subext[2]+1),
                    static_cast<size_t>(subext[1]-subext[0]+1) };

  ptrdiff_t rStride[3] = { (ptrdiff_t)this->Stride[2], (ptrdiff_t)this->Stride[1],
                           (ptrdiff_t)this->Stride[0] };

  /*
    vtkMultiThreaderIDType pid = vtkMultiThreader::GetCurrentThreadID();
    //The getpid() and getppid() functions are always successful, and no
    //return value is reserved to indicate an error. So is this check
    //necessary?
    if ((pid = vtkMultiThreader::GetCurrentThreadID()) == 0)
      {
      cerr<< "unable to get pid" << endl;
      }
    else
      {
      cerr << "The process id is " << pid << endl;
     }

     cerr << "subext " << subext[0] << " " << subext[1]
          << " " << subext[2] << " " << subext[3] << " "
           << subext[4] << " " << subext[5] << endl;
  */
  //initialize memory (raw data space, x y z axis space) and rectilinear grid
  bool firstPass = true;
  for(size_t i=0;i<this->Internals->VariableMap.size();i++)
  {
    if(this->Internals->VariableMap[i] != -1 &&
       this->Internals->VariableArraySelection->GetArraySetting(
         this->Internals->VariableMap[i]) != 0)
    {
      // varidp is probably i in which case nc_inq_varid isn't needed
      int varidp;
      nc_inq_varid(this->NCDFFD,
                   this->Internals->VariableArraySelection->GetArrayName(
                     this->Internals->VariableMap[i]), &varidp);

      if(firstPass == true)
      {
        int dimidsp[3];
        nc_inq_vardimid(this->NCDFFD, varidp, dimidsp);
        firstPass = false;
        float* x = new float[count[0]];
        float* y = new float[count[1]];
        float* z = new float[count[2]];
        //gets data from x,y,z axis (spacing)
#if 0
        nc_get_vara_float(this->NCDFFD, dimidsp[0], start, count, x);
        nc_get_vara_float(this->NCDFFD, dimidsp[1], start+1, count+1, y);
        nc_get_vara_float(this->NCDFFD, dimidsp[2], start+2, count+2, z);
#else
        nc_get_vars_float(this->NCDFFD, dimidsp[0],
                          start, count, rStride, x);
        nc_get_vars_float(this->NCDFFD, dimidsp[1],
                          start+1, count+1, rStride+1, y);
        nc_get_vars_float(this->NCDFFD, dimidsp[2],
                          start+2, count+2, rStride+2, z);
#endif
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
#if 0
      nc_get_vara_float(this->NCDFFD, varidp, start, count, data);
#else
      nc_get_vars_float(this->NCDFFD, varidp, start, count, rStride,
                        data);
#endif
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
void vtkNetCDFPOPReader::SelectionModifiedCallback(vtkObject*, unsigned long,
                                                   void* clientdata, void*)
{
  static_cast<vtkNetCDFPOPReader*>(clientdata)->Modified();
}

//-----------------------------------------------------------------------------
int vtkNetCDFPOPReader::GetNumberOfVariableArrays()
{
  return this->Internals->VariableArraySelection->GetNumberOfArrays();
}

//-----------------------------------------------------------------------------
const char* vtkNetCDFPOPReader::GetVariableArrayName(int index)
{
  if(index < 0 || index >= this->GetNumberOfVariableArrays())
  {
    return NULL;
  }
  return this->Internals->VariableArraySelection->GetArrayName(index);
}

//-----------------------------------------------------------------------------
int vtkNetCDFPOPReader::GetVariableArrayStatus(const char* name)
{
  return this->Internals->VariableArraySelection->ArrayIsEnabled(name);
}

//-----------------------------------------------------------------------------
void vtkNetCDFPOPReader::SetVariableArrayStatus(const char* name, int status)
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
