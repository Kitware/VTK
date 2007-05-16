/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTimeSourceExample.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkTimeSourceExample.h"

#include "vtkObjectFactory.h"
#include "vtkAlgorithm.h"
#include "vtkAlgorithmOutput.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkImageData.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include <vtkstd/vector>

vtkCxxRevisionMacro(vtkTimeSourceExample, "1.1");
vtkStandardNewMacro(vtkTimeSourceExample);


//----------------------------------------------------------------------------
double vtkTimeSourceExample::ValueFunction(double t)
{
  return sin(2*M_PI*t);
}

//----------------------------------------------------------------------------
double vtkTimeSourceExample::XFunction(double t)
{
  return sin(2*M_PI*t)*this->XAmplitude;
}

//----------------------------------------------------------------------------
double vtkTimeSourceExample::YFunction(double t)
{
  return sin(2*M_PI*t)*this->YAmplitude;
}

//----------------------------------------------------------------------------
void vtkTimeSourceExample::LookupTimeAndValue(double &time, double &value)
{
  double t = time;
  if (this->Analytic)
    {
    time = t;
    //clamp within range
    if (time < this->Steps[0])
      {
      time = this->Steps[0];
      }
    if (time > this->Steps[this->NumSteps-1])
      {
      time = this->Steps[this->NumSteps-1];
      }
    value = this->ValueFunction(time);
    }
  else
    {
    int index = -2;
    for (int i = 0; i < this->NumSteps; i++)
      {
      if (this->Steps[i] == t)
        {
        index = i;
        break;
        }
      if (this->Steps[i] > t)
        {
        index = i-1;
        break;
        }
      }
    if (index == -1)
      {
      index = 0;
      }
    if (index == -2)
      {
      index = this->NumSteps-1;
      }
    time = this->Steps[index];
    value = this->Values[index];
    }
}

//----------------------------------------------------------------------------
int vtkTimeSourceExample::NumCellsFunction(double t)
{
  int numCells = 1;
  if (this->Growing)
    {
    //goes from 1 to NumSteps/2+1, adding one cell each step, and returns
    double halfSteps = this->NumSteps/2.0;
    numCells = (int)(halfSteps - (fabs(2.0*(t-0.5)*halfSteps)));
    numCells+=1; 
    }
  return numCells;
}

//----------------------------------------------------------------------------
vtkTimeSourceExample::vtkTimeSourceExample()
{
  this->SetNumberOfInputPorts(0);

  this->Analytic = 0;
  this->XAmplitude = 0.0;
  this->YAmplitude = 0.0;
  this->Growing = 0;

  this->NumSteps = 10;

  //times are regularly sampled from 0.0 to 1.0
  this->Steps = new double[this->NumSteps];
  for (int i = 0; i < this->NumSteps; i++)
    {
    this->Steps[i] = (double)i/(double)(this->NumSteps-1);
    }

  //create the table of values at those times for usee when acting as discrete
  this->Values = new double[this->NumSteps];
  for (int i = 0; i < this->NumSteps; i++)
    {
    this->Values[i] = 
      this->ValueFunction((double)i/(double)(this->NumSteps-1));
    }
}

//----------------------------------------------------------------------------
vtkTimeSourceExample::~vtkTimeSourceExample()
{
  delete[] this->Steps;
  delete[] this->Values;
}


//----------------------------------------------------------------------------
int vtkTimeSourceExample::RequestInformation(
  vtkInformation* reqInfo,
  vtkInformationVector** inVector,
  vtkInformationVector* outVector
  )
{
  if(!this->Superclass::RequestInformation(reqInfo,inVector,outVector))
    {
    return 0;
    }
  
  vtkInformation *info=outVector->GetInformationObject(0);
  
  //tell the caller that I can provide time varying data and
  //tell it what range of times I can deal with
  double tRange[2];
  tRange[0] = this->Steps[0];
  tRange[1] = this->Steps[this->NumSteps-1];
  info->Set(
    vtkStreamingDemandDrivenPipeline::TIME_RANGE(),
    tRange,
    2);
  
  //tell the caller if this filter can provide values ONLY at discrete times
  //or anywhere withing the time range
  if (!this->Analytic)
    {
    info->Set(
      vtkStreamingDemandDrivenPipeline::TIME_STEPS(),
      this->Steps,
      this->NumSteps);
    }
  else
    {
    info->Remove(
      vtkStreamingDemandDrivenPipeline::TIME_STEPS()
      );
    }

  double spacing[3] = {1,1,1};
  info->Set(vtkDataObject::SPACING(),spacing,3);

  //determine what time is being asked for
  double reqTime = 0.0;
  int reqNTS = 0;
  double *reqTS = NULL;
  if (info->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS()))
    {
    reqNTS = info->Length(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS());    
    reqTS = info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS());
    }
  if (reqTS != NULL)
    {
    //TODO: produce multiblock output when multiple time steps are asked for
    //for now just answer the first one
    reqTime = reqTS[0];
    }

  //because the output can move and grow over time
  //provide information about the output shape 
  double time = reqTime;
  double value = 0.0;
  this->LookupTimeAndValue(time, value);

  double origin[3];
  double x = this->XFunction(time);
  double y = this->YFunction(time);
  origin[0] = x;
  origin[1] = y;
  origin[2] = 0;
  info->Set(vtkDataObject::ORIGIN(),origin,3);

  int ext[6];
  int numCells = this->NumCellsFunction(time);
  ext[0] = 0;
  ext[1] = 1;
  ext[2] = 0;
  ext[3] = numCells;
  ext[4] = 0;
  ext[5] = 1;
  info->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), ext ,6);

  return 1;
}

//----------------------------------------------------------------------------
int vtkTimeSourceExample::RequestData(
  vtkInformation* reqInfo,
  vtkInformationVector** inVector,
  vtkInformationVector* outVector
  )
{  
  vtkInformation *outInfo = outVector->GetInformationObject(0);
  vtkImageData *output= vtkImageData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!output)
    {
    return 0;
    }

  //determine what time is being asked for
  double reqTime = 0.0;
  int reqNTS = 0;
  double *reqTS = NULL;
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS()))
    {
    reqNTS = outInfo->Length(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS());    
    reqTS = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS());
    }
  if (reqTS != NULL)
    {
    //TODO: produce multiblock output when multiple time steps are asked for
    //for now just answer the first one
    reqTime = reqTS[0];
    }

  //if analytic compute the value at that time
  //if discrete look up the nearest time and value from the table
  double time = reqTime;
  double value = 0.0;
  this->LookupTimeAndValue(time, value);

  output->Initialize(); 
  output->GetInformation()->Set(vtkDataObject::DATA_TIME_STEPS(), &time, 1);
  output->SetSpacing(1,1,1);

  //figure out the world space position of the output
  double x = this->XFunction(time);
  double y = this->YFunction(time);
  output->SetOrigin(x,y,0);

  //figure out the number of cells in the output
  int numCells = this->NumCellsFunction(time);
  output->SetDimensions(2,numCells+1,2);

  //allocate the rest of the output data structure
  output->SetWholeExtent(0,1,0,numCells,0,1);
  output->AllocateScalars();

  //compute values for each point and cell to test with
  vtkDoubleArray *pd = vtkDoubleArray::New();
  pd->SetNumberOfComponents(1);
  pd->SetName("Point Value");
  output->GetPointData()->AddArray(pd);
  vtkIdTypeArray *id = vtkIdTypeArray::New();
  id->SetNumberOfComponents(1);
  id->SetName("Point Label");
  output->GetPointData()->AddArray(id);
  output->GetPointData()->SetGlobalIds(id);
  vtkDoubleArray *xd = vtkDoubleArray::New();
  xd->SetNumberOfComponents(1);
  xd->SetName("Point X");
  output->GetPointData()->AddArray(xd);
  vtkDoubleArray *yd = vtkDoubleArray::New();
  yd->SetNumberOfComponents(1);
  yd->SetName("Point Y");
  output->GetPointData()->AddArray(yd);
  vtkDoubleArray *zd = vtkDoubleArray::New();
  zd->SetNumberOfComponents(1);
  zd->SetName("Point Z");
  output->GetPointData()->AddArray(zd);

  vtkIdType pid = 0;
  for (int i = 0; i < 2; i++)
    {
    for (int j = 0; j < numCells+1; j++)
      {
      for (int k = 0; k < 2; k++)
        {
        pd->InsertNextValue(value);
        id->InsertNextValue(pid);
        pid++;
        xd->InsertNextValue(x+k);
        yd->InsertNextValue(y+j);
        zd->InsertNextValue(i);
        }
      }
    }
  id->Delete();
  xd->Delete();
  yd->Delete();
  zd->Delete();
  pd->Delete();


  vtkDoubleArray *cd = vtkDoubleArray::New();
  cd->SetNumberOfComponents(1);
  cd->SetName("Cell Value");
  output->GetCellData()->AddArray(cd);
  id = vtkIdTypeArray::New();
  id->SetNumberOfComponents(1);
  id->SetName("Cell Label");
  output->GetCellData()->AddArray(id);
  output->GetCellData()->SetGlobalIds(id);
  xd = vtkDoubleArray::New();
  xd->SetNumberOfComponents(1);
  xd->SetName("Cell X");
  output->GetCellData()->AddArray(xd);
  yd = vtkDoubleArray::New();
  yd->SetNumberOfComponents(1);
  yd->SetName("Cell Y");
  output->GetCellData()->AddArray(yd);
  zd = vtkDoubleArray::New();
  zd->SetNumberOfComponents(1);
  zd->SetName("Cell Z");
  output->GetCellData()->AddArray(zd);

  vtkIdType cid = 0;
  for (int i = 0; i < 1; i++)
    {
    for (int j = 0; j < numCells; j++)
      {
      for (int k = 0; k < 1; k++)
        {
        cd->InsertNextValue(value);
        id->InsertNextValue(cid); 
        cid++;
        xd->InsertNextValue(x+k+0.5); //center of the cell
        yd->InsertNextValue(y+j+0.5);
        zd->InsertNextValue(i+0.5);
        }
      }
    }
  id->Delete();
  xd->Delete();
  yd->Delete();
  zd->Delete();
  cd->Delete();
      
  return 1;
}

//----------------------------------------------------------------------------
void vtkTimeSourceExample::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Analytic: " << (this->Analytic?"ON":"OFF") << endl;
  os << indent << "XAmplitude: " << this->XAmplitude << endl;
  os << indent << "YAmplitude: " << this->YAmplitude << endl;
  os << indent << "Growing: " << this->Growing << endl;

}

