/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXYZMolReader2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkXYZMolReader2.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"

#include "vtkMolecule.h"
#include "vtkPeriodicTable.h"

#include "vtkStreamingDemandDrivenPipeline.h"

#include <math.h>
#include <string.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkXYZMolReader2);

//----------------------------------------------------------------------------
vtkXYZMolReader2::vtkXYZMolReader2()
  : FileName(NULL)
{
  this->SetNumberOfInputPorts(0);
  this->NumberOfTimeSteps = 0;
  this->NumberOfAtoms = 0;
}

//----------------------------------------------------------------------------
vtkXYZMolReader2::~vtkXYZMolReader2()
{
  this->SetFileName(NULL);
}

//----------------------------------------------------------------------------
vtkMolecule *vtkXYZMolReader2::GetOutput()
{
  return vtkMolecule::SafeDownCast(this->GetOutputDataObject(0));
}

//----------------------------------------------------------------------------
void vtkXYZMolReader2::SetOutput(vtkMolecule *output)
{
  this->GetExecutive()->SetOutputData(0, output);
}

int vtkXYZMolReader2::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  char title[256];

  if (!this->FileName)
    {
    return 0;
    }

  ifstream file_in(this->FileName);

  if(!file_in.is_open())
    {
    vtkErrorMacro ("vtkXYZMolReader2 error opening file: " << this->FileName);
    return 0;
    }
  int natoms;
  char eolc;
  while(file_in)
    {
    istream::pos_type current_pos = file_in.tellg();

    file_in >> natoms >> eolc; // first title line
    if (!file_in)
      break; // reached after last timestep

    file_positions.push_back(current_pos);

    if (!this->NumberOfAtoms) // first read
      this->NumberOfAtoms = natoms;
    else
      {
// do a consistency check with previous step. Assume there should be same # of atoms
      if(this->NumberOfAtoms != natoms)
      vtkWarningMacro ("XYZMolReader2 has different number of atoms at each timestep "
                      << this->NumberOfAtoms << " " << natoms);
      }

    file_in.getline(title, 256);
    // second title line with time index, time value and E?
    // search now for an optional "time = value" field and assign it if found

    char *timeLabel = strstr(title,"time");
    double timeValue;
    if(sscanf(timeLabel, "time = %lf", &timeValue) == 1)
      {
      //std::cout << __LINE__ << "timeValue " << timeValue << std::endl;
      this->TimeSteps.push_back(timeValue);
      }
    else
      {
      this->TimeSteps.push_back(this->NumberOfTimeSteps);
      }

    this->NumberOfTimeSteps++;
    for(int i=0; i < natoms; i++)
      {
      file_in.getline(title, 256); // for each atom a line with symbol, x, y, z
      }
    }
  file_in.close();

  outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(),
    this->TimeSteps.data(), this->NumberOfTimeSteps);
  double timeRange[2];
  timeRange[0] = this->TimeSteps[0];
  timeRange[1] = this->TimeSteps[this->NumberOfTimeSteps - 1];
  outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
  return 1;
}

int vtkXYZMolReader2::RequestData(
  vtkInformation *,
  vtkInformationVector **,
  vtkInformationVector *outputVector)
{
  char title[256];
  int i;

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkMolecule *output = vtkMolecule::SafeDownCast
    (vtkDataObject::GetData(outputVector));

  if (!output)
    {
    vtkErrorMacro(<<"vtkXYZMolReader2 does not have a vtkMolecule "
                  "as output.");
    return 1;
    }

  if (!this->FileName)
    {
    return 0;
    }

  ifstream file_in(this->FileName);

  if(!file_in.is_open())
    {
    vtkErrorMacro ("vtkXYZMolReader2 error opening file: " << this->FileName);
    return 0;
    }
  int length = outInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  double* steps = outInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  int timestep=0;
  std::vector<double>::iterator it = this->TimeSteps.begin();

  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
    {
    // Get the requested time step.
    double requestedTimeStep =
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

    //find the timestep with the closest value
    if(requestedTimeStep < *it)
      {
      requestedTimeStep = *it;
      vtkWarningMacro ("XYZMolReader2 using its first timestep value of "
                      << requestedTimeStep);
      }
    for (it = this->TimeSteps.begin(); it < this->TimeSteps.end(); it++, timestep++)
      {
      if ((*it > requestedTimeStep))
        break;
      }

    if(it != this->TimeSteps.end())
      {
      it--; timestep--;
      //std::cout << __LINE__ << "comparing " << *it << ", " << *(it+1) << std::endl;
      if(fabs(*it - requestedTimeStep) > fabs(*(it+1) - requestedTimeStep))
        {
        // closer to next timestep value
        timestep++;
        it++;
        }
      }
    else
      {
      it--; timestep--;
      }
    }
  else
    {
    timestep = 0;
    }

  file_in.seekg(file_positions[timestep]);
  int nbAtoms;
  char eolc;
  file_in >> nbAtoms >> eolc; // first title line
  //std::cout << __LINE__ << "# of atoms read = " << nbAtoms << std::endl;
  if(nbAtoms != this->NumberOfAtoms)
    {
    vtkErrorMacro ("vtkXYZMolReader2 error reading file: " << this->FileName
                     << " Premature EOF while reading molecule.");
    file_in.close();
    return 0;
    }
  file_in.getline(title, 256); // second title line

// construct vtkMolecule
  output->Initialize();

  char atomType[16];
  float x, y, z;
  vtkPeriodicTable *pT = vtkPeriodicTable::New();
  for(i = 0; i < this->NumberOfAtoms; i++)
    {
    file_in.getline(title, 256);
    if (sscanf(title,"%s %f %f %f", atomType, &x, &y, &z) != 4)
      {
      vtkErrorMacro ("vtkXYZMolReader2 error reading file: " << this->FileName
                     << " Premature EOF while reading molecule.");
      file_in.close();
      return 0;
      }
    output->AppendAtom(pT->GetAtomicNumber(atomType), x, y, z);
    }
  pT->Delete();
  file_in.close();

  return 1;
}


//----------------------------------------------------------------------------
int vtkXYZMolReader2::CanReadFile(const char* fname)
{
// to be implemented further
  int valid = 1;

  return valid;
}


//----------------------------------------------------------------------------
void vtkXYZMolReader2::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Number of Atoms: " << this->NumberOfAtoms << endl;
  os << indent << "Number of TimeSteps: " << this->NumberOfTimeSteps;
}
