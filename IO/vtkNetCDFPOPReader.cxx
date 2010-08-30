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
// .NAME vtkNetCDFPOPReader - read NetCDF files
// .Author Joshua Wu 09.15.2009
// .SECTION Description
// vtkNetCDFPOPReader is a source object that reads NetCDF files.
// It should be able to read most any NetCDF file that wants to output
// rectilinear grid
//

#include "netcdf.h"
#include "string.h"

#include "vtkNetCDFPOPReader.h"
#include "vtkCallbackCommand.h"
#include "vtkDataArraySelection.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
//#include "vtkMultiThreader.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkRectilinearGrid.h"
#include "vtkRectilinearGridGeometryFilter.h"
#include "vtkStreamingDemandDrivenPipeline.h"


vtkStandardNewMacro(vtkNetCDFPOPReader);

//============================================================================
#include <iostream>
#define CALL_NETCDF(call) \
{ \
  int errorcode = call; \
  if (errorcode != NC_NOERR) \
  { \
    vtkErrorMacro(<< "netCDF Error: " << nc_strerror(errorcode)); \
    return 0; \
  } \
}

#define NDIM 4
//============================================================================
//----------------------------------------------------------------------------
//set default values
vtkNetCDFPOPReader::vtkNetCDFPOPReader()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  this->FileName = NULL;
  this->Stride[0] = this->Stride[1] = this->Stride[2] = 1;
  this->VariableArraySelection =
    vtkSmartPointer<vtkDataArraySelection>::New();
  this->SelectionObserver = vtkCallbackCommand::New();
  this->SelectionObserver->SetCallback
    (&vtkNetCDFPOPReader::SelectionModifiedCallback);
  this->SelectionObserver->SetClientData(this);
  this->VariableArraySelection->AddObserver(vtkCommand::ModifiedEvent,
                                            this->SelectionObserver);
  for(int f=0;f<NCDFPOP_MAX_ARRAYS;f++)
    {
    //records what variable to output, default is 0, to output is 1
    this->Draw[f]=0;
    }
  this->NVarspw=0;

}

//----------------------------------------------------------------------------
//delete filename and netcdf file descriptor
vtkNetCDFPOPReader::~vtkNetCDFPOPReader()
{
  if (this->FileName)
    {
    delete[] this->FileName;
    }
  nc_close(this->NCDFFD);
  this->SelectionObserver->Delete();
}

//----------------------------------------------------------------------------
void vtkNetCDFPOPReader::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "FileName: "
     << (this->FileName ? this->FileName : "(NULL)") << endl;

  os << indent << "VariableArraySelection:" << endl;

  os << indent << "Stride: {" << this->Stride[0] << ", "
     << this->Stride[1] << ", " << this->Stride[2] << ", "
     << "}" << endl;

  this->VariableArraySelection->PrintSelf(os, indent.GetNextIndent());
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
  int retval = nc_open(this->FileName, NC_NOWRITE, &this->NCDFFD);//read file
  if (retval != NC_NOERR)//checks if read file error
    {
    vtkErrorMacro(<< "can't read file " << nc_strerror(retval));
    }
  // get number of variables from file
  nc_inq_nvars(this->NCDFFD, &this->NVarsp);
  int dimidsp[NC_MAX_VAR_DIMS];
  this->NVarspw=0;
  int extent[6];
  int finaldim; //data dimension
  int Dimensions[4]; //dimension value
  // For every variable in the file
  for(int i=0;i<this->NVarsp;i++)
    {
    //get number of dimensions
    CALL_NETCDF(nc_inq_varndims(this->NCDFFD, i, &finaldim));
    //get variable name
    CALL_NETCDF(nc_inq_varname(this->NCDFFD, i, this->VariableName[i]));
    //Variable Dimension ID's containing x,y,z coords for the rectilinear
    //grid spacing
    CALL_NETCDF(nc_inq_vardimid(this->NCDFFD, i, dimidsp));
    if(finaldim == 3)
      {
      strcpy(this->VariableArrayInfo[this->NVarspw], this->VariableName[i]);
      this->NVarspw++;
      this->Draw[i] = 1;
      for(int m=0;m<finaldim;m++)
        {
        CALL_NETCDF(nc_inq_dimlen(this->NCDFFD, dimidsp[m],
                                  (size_t*) &Dimensions[m]));
        //acquire variable dimensions
        }
      extent[0] = extent[2] = extent[4] =0; //set extent
      extent[1] = Dimensions[2] -1;
      extent[1] = extent[1] / this->Stride[2];
      extent[3] = Dimensions[1] -1;
      extent[3] = extent[3] / this->Stride[1];
      extent[5] = Dimensions[0] -1;
      extent[5] = extent[5] / this->Stride[0];
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
  int varidp[NCDFPOP_MAX_ARRAYS], dimidsp[NC_MAX_VAR_DIMS];
  int subext[6];
  //vtkInformation * outInfo = output->GetInformationObject(0);
  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),subext);
  //setup extents for netcdf library to read the netcdf data file
  size_t start[]= {subext[4], subext[2], subext[0]};
  size_t count[]= {subext[5]-subext[4]+1,
                   subext[3]-subext[2]+1,
                   subext[1]-subext[0]+1};
  size_t start1[] = {subext[4]};
  size_t start2[] = {subext[2]};
  size_t start3[] = {subext[0]};
  size_t count1[] = {subext[5]-subext[4]+1};
  size_t count2[] = {subext[3]-subext[2]+1};
  size_t count3[] = {subext[1]-subext[0]+1};
  size_t bytestoallocate = (count[0])*(count[1])*(count[2]);
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
     cerr << "floats to allocate" << bytestoallocate << endl;
  */
  //initialize memory (raw data space, x y z axis space) and rectilinear grid
  vtkRectilinearGrid *rgrid = vtkRectilinearGrid::SafeDownCast(output);
  rgrid->SetExtent(subext);

  ptrdiff_t rStride[3];
  rStride[0] = (ptrdiff_t)this->Stride[2];
  rStride[1] = (ptrdiff_t)this->Stride[1];
  rStride[2] = (ptrdiff_t)this->Stride[0];

  //reads data and draws rectilinear grid
  int counter=0;
  for(int i=0;i<this->NVarsp;i++)
    {
    if(this->Draw[i]==1)
      {
      nc_inq_varid(this->NCDFFD, this->VariableName[i], &varidp[i]);
      nc_inq_vardimid(this->NCDFFD, varidp[i], dimidsp);
      if(counter==0)
        {
        vtkFloatArray *xCoords = vtkFloatArray::New();
        vtkFloatArray *yCoords = vtkFloatArray::New();
        vtkFloatArray *zCoords = vtkFloatArray::New();
        float *x=new float[count[0]]; //x axis data
        float *y=new float[count[1]]; //y axis data
        float *z=new float[count[2]]; //z axis data
        //gets data from x,y,z axis (spacing)
#if 0
        nc_get_vara_float(this->NCDFFD, dimidsp[0], start1, count1, x);
        nc_get_vara_float(this->NCDFFD, dimidsp[1], start2, count2, y);
        nc_get_vara_float(this->NCDFFD, dimidsp[2], start3, count3, z);
#else
        nc_get_vars_float(this->NCDFFD, dimidsp[0],
                          start1, count1, &rStride[0],
                          x);
        nc_get_vars_float(this->NCDFFD, dimidsp[1],
                          start2, count2, &rStride[1],
                          y);
        nc_get_vars_float(this->NCDFFD, dimidsp[2],
                          start3, count3, &rStride[2],
                          z);
#endif
        //gets data
        // sets axis values/spacing
        for (int o=0; o<subext[1]-subext[0]+1; o++)
          {
          xCoords->InsertNextValue(z[o]);
          }
        for (int p=0; p<subext[3]-subext[2]+1; p++)
          {
          yCoords->InsertNextValue(y[p]);
          }
        for (int q=0; q<subext[5]-subext[4]+1; q++)
          {
          zCoords->InsertNextValue(-x[q]);
          }
        // set dimensions and coordinates
        // rgrid->SetDimensions(xCoords->GetNumberOfTuples(),
        //     yCoords->GetNumberOfTuples(), zCoords->GetNumberOfTuples());
        rgrid->SetXCoordinates(xCoords);
        rgrid->SetYCoordinates(yCoords);
        rgrid->SetZCoordinates(zCoords);
        xCoords->Delete();
        yCoords->Delete();
        zCoords->Delete();
        delete [] x;
        delete [] y;
        delete [] z;
        counter++;
        }
      //create vtkFloatArray and get the scalars into it
      vtkFloatArray *myscalars = vtkFloatArray::New();
      float* data=new float[bytestoallocate]; //pointer to 3D data if 3D
#if 0
      nc_get_vara_float(this->NCDFFD, varidp[i], start, count, data);
#else
      nc_get_vars_float(this->NCDFFD, varidp[i], start, count, rStride,
                        data);
#endif
      myscalars->SetArray(data, bytestoallocate, 0);
      //set list of variables to display data on rectilinear grid
      myscalars->SetName(this->VariableName[i]);
      vtkDataSetAttributes *a=rgrid->GetPointData();
      a->AddArray(myscalars);
      myscalars->Delete();
      /*
      cerr << "---- Executed Data rectilinear grid dataset querries ------"
           << endl;
      cerr << "NumberOfPoints:" << rgrid->GetNumberOfPoints() << endl;
      cerr << "NumberOfCells: " << rgrid->GetNumberOfCells() << endl;
      cerr << "Dimensions:"
           << rgrid->GetDimensions()[0] << " "
           << rgrid->GetDimensions()[1] << " "
           << rgrid->GetDimensions()[2] << endl;
      cerr << "---- End Executed Data ------------------------------------"
           << endl;
      */
      }
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
  return this->NVarspw;
}

//-----------------------------------------------------------------------------
const char* vtkNetCDFPOPReader::GetVariableArrayName(int index)
{
  return this->VariableArrayInfo[index];
}

//-----------------------------------------------------------------------------
int vtkNetCDFPOPReader::GetVariableArrayStatus(const char* name)
{
  //TODO: actually use the array setting
  this->VariableArraySelection->EnableArray(name);
  return this->VariableArraySelection->ArrayIsEnabled(name);
}

//-----------------------------------------------------------------------------
void vtkNetCDFPOPReader::SetVariableArrayStatus(const char* name, int status)
{
  //TODO: exit without calling modified if state is actually unchanged
  vtkDebugMacro("Set cell array \"" << name << "\" status to: " << status);
  if(status)
    {
    this->VariableArraySelection->EnableArray(name);
    }
  else
    {
    this->VariableArraySelection->DisableArray(name);
    }

  //vtkDebugMacro("Set cell array \"" << name << "\" status to: "
  //<< status);
  for(int i=0;i<this->NVarsp;i++)
    {
    if( strcmp(name, this->VariableName[i])==0 )
      {
      //TODO: use array selection directly instead of this imposter
      this->Draw[i]=1;
      }
    else
      {
      this->Draw[i]=0;
      }
    }
}
