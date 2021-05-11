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
#include "vtkMolecule.h"
#include "vtkObjectFactory.h"
#include "vtkPeriodicTable.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtksys/FStream.hxx"

#include <cmath>
#include <cstring>
#include <sstream>

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkXYZMolReader2);

//------------------------------------------------------------------------------
vtkXYZMolReader2::vtkXYZMolReader2()
{
  this->SetNumberOfInputPorts(0);
}

//------------------------------------------------------------------------------
vtkMolecule* vtkXYZMolReader2::GetOutput()
{
  return vtkMolecule::SafeDownCast(this->GetOutputDataObject(0));
}

//------------------------------------------------------------------------------
void vtkXYZMolReader2::SetOutput(vtkMolecule* output)
{
  this->GetExecutive()->SetOutputData(0, output);
}

//------------------------------------------------------------------------------
int vtkXYZMolReader2::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtksys::ifstream fileInput(this->FileName.c_str());

  if (!fileInput.is_open())
  {
    vtkErrorMacro("vtkXYZMolReader2 error opening file: " << this->FileName);
    return 0;
  }

  vtkIdType nAtoms = 0;
  int timeStep = 0;

  while (fileInput)
  {
    istream::pos_type currentPos = fileInput.tellg();
    fileInput >> nAtoms;
    fileInput.get(); // end of line char
    if (!fileInput)
    {
      break; // reached after last timestep
    }

    this->FilePositions.emplace_back(currentPos);

    std::string title;
    getline(fileInput, title); // second title line
    if (!title.empty())
    {
      // second title line may have a time index, time value and E?
      // search now for an optional "time = value" field and assign it if found
      std::size_t found = title.find(std::string("time"));
      if (found != std::string::npos)
      {
        std::istringstream tmp(&title[found + 6]);
        double timeValue;
        tmp >> timeValue;
        this->TimeSteps.emplace_back(timeValue);
      }
      else
      {
        this->TimeSteps.emplace_back(timeStep);
      }
    }
    else
    {
      this->TimeSteps.emplace_back(timeStep);
    }
    timeStep++;

    for (int i = 0; i < nAtoms; i++)
    {
      getline(fileInput, title); // for each atom a line with symbol, x, y, z
    }
  }
  fileInput.close();

  outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &this->TimeSteps[0], timeStep);
  double timeRange[2];
  timeRange[0] = this->TimeSteps[0];
  timeRange[1] = this->TimeSteps[timeStep - 1];
  outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
  return 1;
}

//------------------------------------------------------------------------------
int vtkXYZMolReader2::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkMolecule* output = vtkMolecule::SafeDownCast(vtkDataObject::GetData(outputVector));

  if (!output)
  {
    vtkErrorMacro("vtkXYZMolReader2 does not have a vtkMolecule as output.");
    return 1;
  }

  vtksys::ifstream fileInput(this->FileName.c_str());

  if (!fileInput.is_open())
  {
    vtkErrorMacro("vtkXYZMolReader2 error opening file: " << this->FileName);
    return 0;
  }

  int timestep = 0;
  std::vector<double>::iterator it = this->TimeSteps.begin();

  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    // Get the requested time step.
    double requestedTimeStep = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

    // find the timestep with the closest value
    if (requestedTimeStep < *it)
    {
      requestedTimeStep = *it;
      vtkWarningMacro("XYZMolReader2 using its first timestep value of " << requestedTimeStep);
    }
    for (it = this->TimeSteps.begin(); it < this->TimeSteps.end(); ++it, ++timestep)
    {
      if ((*it > requestedTimeStep))
        break;
    }

    if (it != this->TimeSteps.end())
    {
      --it;
      --timestep;
      if (fabs(*it - requestedTimeStep) > fabs(*(it + 1) - requestedTimeStep))
      {
        // closer to next timestep value
        ++timestep;
        ++it;
      }
    }
    else
    {
      --timestep;
      --it;
    }
  }
  else
  {
    timestep = 0;
  }

  fileInput.seekg(this->FilePositions[timestep]);
  vtkIdType nAtoms;

  fileInput >> nAtoms;
  fileInput.get(); // end of line char

  std::string title;
  getline(fileInput, title); // second title line

  // construct vtkMolecule
  output->Initialize();

  vtkNew<vtkPeriodicTable> pT;
  for (int i = 0; i < nAtoms; i++)
  {
    char atomType[16];
    float x, y, z;
    fileInput >> atomType >> x >> y >> z;
    if (fileInput.fail()) // checking we are at end of line
    {
      vtkErrorMacro("vtkXYZMolReader2 error reading file: "
        << this->FileName << " Problem reading atoms' positions.");
      fileInput.close();
      return 0;
    }
    output->AppendAtom(pT->GetAtomicNumber(atomType), x, y, z);
  }
  fileInput.close();

  return 1;
}

//------------------------------------------------------------------------------
void vtkXYZMolReader2::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << this->FileName << endl;
}
