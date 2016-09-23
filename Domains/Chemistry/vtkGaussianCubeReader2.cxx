/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGaussianCubeReader2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
#include "vtkGaussianCubeReader2.h"

#include "vtkSimpleBondPerceiver.h"
#include "vtkDataObject.h"
#include "vtkExecutive.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkImageData.h"
#include "vtkMolecule.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPeriodicTable.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTransform.h"

#include <string>
#include <vector>
#include <fstream>
#include <iostream>

vtkStandardNewMacro(vtkGaussianCubeReader2);

//----------------------------------------------------------------------------
vtkGaussianCubeReader2::vtkGaussianCubeReader2()
  : FileName(NULL)
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(2);

  // Add the second output for the grid data

  vtkImageData *grid;
  grid = vtkImageData::New();
  grid->ReleaseData();
  this->GetExecutive()->SetOutputData(1, grid);
  grid->Delete();
}

//----------------------------------------------------------------------------
vtkGaussianCubeReader2::~vtkGaussianCubeReader2()
{
  this->SetFileName(NULL);
}

//----------------------------------------------------------------------------
vtkMolecule *vtkGaussianCubeReader2::GetOutput()
{
  return vtkMolecule::SafeDownCast(this->GetOutputDataObject(0));
}

//----------------------------------------------------------------------------
void vtkGaussianCubeReader2::SetOutput(vtkMolecule *output)
{
  this->GetExecutive()->SetOutputData(0, output);
}

//----------------------------------------------------------------------------
vtkImageData *vtkGaussianCubeReader2::GetGridOutput()
{
  if (this->GetNumberOfOutputPorts() < 2)
  {
    return NULL;
  }
  return vtkImageData::SafeDownCast(this->GetOutputDataObject(1));
}

int vtkGaussianCubeReader2::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *vtkNotUsed(outputVector))
{
  // the set the information for the imagedata output
  vtkInformation *gridInfo = this->GetExecutive()->GetOutputInformation(1);

  char title[256];

  if (!this->FileName)
  {
    return 0;
  }

  ifstream file_in(this->FileName);

  if(!file_in.is_open())
  {
    vtkErrorMacro ("GaussianCubeReader2 error opening file: " << this->FileName);
    return 0;
  }

  file_in.getline(title, 256); // first title line
  file_in.getline(title, 256); // second title line with name of scalar field?

  // Read in number of atoms, x-origin, y-origin z-origin
  double tmpd;
  int n1, n2, n3;
  if (!(file_in >> n1 >> tmpd >> tmpd >> tmpd))
  {
    vtkErrorMacro ("GaussianCubeReader error reading file: " << this->FileName
                   << " Premature EOF while grid size.");
    file_in.close();
    return 0;
  }

  if (!(file_in >> n2 >> tmpd >> tmpd >> tmpd))
  {
    vtkErrorMacro ("GaussianCubeReader error reading file: " << this->FileName
                   << " Premature EOF while grid size.");
    file_in.close();
    return 0;
  }
  if (!(file_in >> n3 >> tmpd >> tmpd >> tmpd))
  {
    vtkErrorMacro ("GaussianCubeReader error reading file: " << this->FileName
                   << " Premature EOF while grid size.");
    file_in.close();
    return 0;
  }

  vtkDebugMacro(<< "Grid Size " << n1 << " " << n2 << " " << n3);
  gridInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
                0, n1-1, 0, n2-1, 0, n3-1);
  gridInfo->Set(vtkDataObject::ORIGIN(), 0, 0, 0);
  gridInfo->Set(vtkDataObject::SPACING(), 1, 1, 1);

  file_in.close();

  vtkDataObject::SetPointDataActiveScalarInfo(gridInfo, VTK_FLOAT, -1);
  return 1;
}

int vtkGaussianCubeReader2::RequestData(
  vtkInformation *,
  vtkInformationVector **,
  vtkInformationVector *outputVector)
{
  char title[256];
  double elements[16];
  int NumberOfAtoms;
  int n1, n2, n3; // grid resolution
  float tmp, *cubedata;
  bool orbitalCubeFile = false;
  int numberOfOrbitals;

  vtkMolecule *output = vtkMolecule::SafeDownCast
    (vtkDataObject::GetData(outputVector));

  if (!output)
  {
    vtkErrorMacro(<<"vtkGaussianCubeReader2 does not have a vtkMolecule "
                  "as output.");
    return 1;
  }
  // Output 0 (the default is the vtkMolecule)
  // Output 1 will be the gridded Image data

  if (!this->FileName)
  {
    return 0;
  }

  ifstream file_in(this->FileName);

  if(!file_in.is_open())
  {
    vtkErrorMacro ("GaussianCubeReader2 error opening file: " << this->FileName);
    return 0;
  }

  file_in.getline(title, 256); // first title line
  file_in.getline(title, 256); // second title line with name of scalar field?

  // Read in number of atoms, x-origin, y-origin z-origin
  //
  if (!(file_in >> NumberOfAtoms >> elements[3] >> elements[7] >> elements[11]))
  {
    vtkErrorMacro ("GaussianCubeReader error reading file: " << this->FileName
                   << " Premature EOF while reading atoms, x-origin y-origin z-origin.");
    file_in.close();
    return 0;
  }
  if(NumberOfAtoms < 0)
  {
    NumberOfAtoms = -NumberOfAtoms;
    orbitalCubeFile = true;
  }
  if (!(file_in >> n1 >> elements[0] >> elements[4] >> elements[8]))
  {
    vtkErrorMacro ("GaussianCubeReader error reading file: " << this->FileName
                   << " Premature EOF while reading elements.");
    file_in.close();
    return 0;
  }
  if (!(file_in >> n2 >> elements[1] >> elements[5] >> elements[9]))
  {
    vtkErrorMacro ("GaussianCubeReader error reading file: " << this->FileName
                   << " Premature EOF while reading elements.");
    file_in.close();
    return 0;
  }
  if (!(file_in >> n3 >> elements[2] >> elements[6] >> elements[10]))
  {
    vtkErrorMacro ("GaussianCubeReader error reading file: " << this->FileName
                   << " Premature EOF while reading elements.");
    file_in.close();
    return 0;
  }
  elements[12] = 0;
  elements[13] = 0;
  elements[14] = 0;
  elements[15] = 1;

  vtkDebugMacro(<< "Grid Size " << n1 << " " << n2 << " " << n3);

  vtkTransform *Transform = vtkTransform::New();

  Transform->SetMatrix(elements);
  Transform->Inverse();
// construct vtkMolecule
  int atomType;
  float xyz[3];
  float dummy;

  for(int i = 0; i < NumberOfAtoms; i++)
  {
    if (!(file_in >> atomType >> dummy >> xyz[0] >> xyz[1] >> xyz[2]))
    {
      vtkErrorMacro ("GaussianCubeReader error reading file: " << this->FileName
                     << " Premature EOF while reading molecule.");
      file_in.close();
      return 0;
    }
    Transform->TransformPoint(xyz, xyz);
    output->AppendAtom(atomType, xyz[0], xyz[1], xyz[2]);
  }
  Transform->Delete();
// construct grid data

  vtkImageData *grid = this->GetGridOutput();
  if(orbitalCubeFile)
  {
    if (!(file_in >> numberOfOrbitals))
    {
      vtkErrorMacro ("GaussianCubeReader error reading file: " << this->FileName
                     << " Premature EOF while reading number of orbitals.");
      file_in.close();
      return 0;
    }
    for(int k = 0; k < numberOfOrbitals; k++)
    {
      if (!(file_in >> tmp))
      {
        vtkErrorMacro ("GaussianCubeReader error reading file: " << this->FileName
                       << " Premature EOF while reading orbitals.");
        file_in.close();
        return 0;
      }
    }
  }

  vtkInformation *gridInfo = this->GetExecutive()->GetOutputInformation(1);
  gridInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
                0, n1-1, 0, n2-1, 0, n3-1);
  gridInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
                gridInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()),
                6);
  grid->SetExtent(
    gridInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));

  grid->SetOrigin(0, 0, 0);
  grid->SetSpacing(1, 1, 1);
  grid->AllocateScalars(VTK_FLOAT, 1);

  grid->GetPointData()->GetScalars()->SetName(title);

  cubedata = (float *)grid->GetPointData()->GetScalars()->GetVoidPointer(0);
  int N1N2 = n1*n2;

  for(int i = 0; i < n1; i++)
  {
    int JN1 = 0;
    for(int j = 0; j < n2; j++)
    {
      for(int k = 0; k < n3; k++)
      {
        if (!(file_in >> tmp))
        {
          vtkErrorMacro ("GaussianCubeReader error reading file: " << this->FileName
                         << " Premature EOF while reading scalars.");
          file_in.close();
          return 0;
        }
        cubedata[k*N1N2 + JN1 + i] = tmp;
      }
      JN1 += n1;
    }
  }
  file_in.close();

  return 1;
}

int vtkGaussianCubeReader2::FillOutputPortInformation(int port, vtkInformation *info)
{
  if(port == 0)
  {
    return this->Superclass::FillOutputPortInformation(port, info);
  }
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
  return 1;
}

void vtkGaussianCubeReader2::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
