/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGaussianCubeReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGaussianCubeReader.h"

#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkTransform.h"

#include <ctype.h>

vtkStandardNewMacro(vtkGaussianCubeReader);

//----------------------------------------------------------------------------
// Construct object with merging set to true.
vtkGaussianCubeReader::vtkGaussianCubeReader()
{
  this->FileName = NULL;
  this->Transform = vtkTransform::New();
  // Add the second output for the grid data

  this->SetNumberOfOutputPorts(2);
  vtkImageData *grid;
  grid = vtkImageData::New();
  grid->ReleaseData();
  this->GetExecutive()->SetOutputData(1, grid);
  grid->Delete();
}

//----------------------------------------------------------------------------
vtkGaussianCubeReader::~vtkGaussianCubeReader()
{
  if (this->FileName)
    {
    delete [] this->FileName;
    }
  this->Transform->Delete();
  // must delete the second output added
}

//----------------------------------------------------------------------------
int vtkGaussianCubeReader::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  FILE *fp;
  char title[256];
  char data_name[256];
  double elements[16];
  int JN1, N1N2, n1, n2, n3, i, j, k;
  float tmp, *cubedata;
  bool orbitalCubeFile = false;
  int numberOfOrbitals;

  // Output 0 (the default is the polydata)
  // Output 1 will be the gridded Image data

  vtkImageData *grid = this->GetGridOutput();

  if (!this->FileName)
    {
    return 0;
    }

  if ((fp = fopen(this->FileName, "r")) == NULL)
    {
    vtkErrorMacro(<< "File " << this->FileName << " not found");
    return 0;
    }

  if (!fgets(title, 256, fp))
    {
    vtkErrorMacro ("GaussianCubeReader error reading file: " << this->FileName
                   << " Premature EOF while reading title.");
    fclose (fp);
    return 0;
    }
  if(strtok(title, ":") != NULL)
    {
    if(strtok(NULL, ":") != NULL)
      {
      strcpy(data_name, strtok(NULL, ":"));
      fprintf(stderr,"label = %s\n", data_name);
      }
    }
  if (!fgets(title, 256, fp))
    {
    vtkErrorMacro ("GaussianCubeReader error reading file: " << this->FileName
                   << " Premature EOF while reading title.");
    fclose (fp);
    return 0;
    }

  // Read in number of atoms, x-origin, y-origin z-origin
  //
  if (fscanf(fp, "%d %lf %lf %lf", &(this->NumberOfAtoms), &elements[3], &elements[7], &elements[11]) != 4)
    {
    vtkErrorMacro ("GaussianCubeReader error reading file: " << this->FileName
                   << " Premature EOF while reading atoms, x-origin y-origin z-origin.");
    fclose (fp);
    return 0;
    }
  if(this->NumberOfAtoms < 0 )
    {
    this->NumberOfAtoms = -this->NumberOfAtoms;
    orbitalCubeFile = true;
    }

  if (fscanf(fp, "%d %lf %lf %lf", &n1, &elements[0], &elements[4], &elements[8]) != 4)
    {
    vtkErrorMacro ("GaussianCubeReader error reading file: " << this->FileName
                   << " Premature EOF while reading elements.");
    fclose (fp);
    return 0;
    }
  if (fscanf(fp, "%d %lf %lf %lf", &n2, &elements[1], &elements[5], &elements[9]) != 4)
    {
    vtkErrorMacro ("GaussianCubeReader error reading file: " << this->FileName
                   << " Premature EOF while reading elements.");
    fclose (fp);
    return 0;
    }
  if (fscanf(fp, "%d %lf %lf %lf", &n3, &elements[2], &elements[6], &elements[10]) != 4)
    {
    vtkErrorMacro ("GaussianCubeReader error reading file: " << this->FileName
                   << " Premature EOF while reading elements.");
    fclose (fp);
    return 0;
    }
  elements[12] = 0;
  elements[13] = 0;
  elements[14] = 0;
  elements[15] = 1;

  vtkDebugMacro(<< "Grid Size " << n1 << " " << n2 << " " << n3);

  Transform->SetMatrix(elements);
  Transform->Inverse();

  this->ReadMolecule(fp, output);

  if(orbitalCubeFile)
    {
    if (fscanf(fp,"%d", &numberOfOrbitals) != 1)
      {
      vtkErrorMacro ("GaussianCubeReader error reading file: " << this->FileName
                     << " Premature EOF while reading number of orbitals.");
      fclose (fp);
      return 0;
      }
    for(k = 0; k < numberOfOrbitals; k++)
      {
      if (fscanf(fp,"%f", &tmp) != 1)
        {
        vtkErrorMacro ("GaussianCubeReader error reading file: " << this->FileName
                       << " Premature EOF while reading orbitals.");
        fclose (fp);
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
  N1N2 = n1*n2;

  for(i = 0; i < n1; i++)
    {
    JN1 = 0;
    for(j = 0; j < n2; j++)
      {
      for(k = 0; k < n3; k++)
        {
        if (fscanf(fp,"%f", &tmp) != 1)
          {
          vtkErrorMacro ("GaussianCubeReader error reading file: " << this->FileName
                         << " Premature EOF while reading scalars.");
          fclose (fp);
          return 0;
          }
        cubedata[k*N1N2 + JN1 + i] = tmp;
        }
      JN1 += n1;
      }
    }
  fclose(fp);

  return 1;
}

//----------------------------------------------------------------------------
void vtkGaussianCubeReader::ReadSpecificMolecule(FILE* fp)
{
  int i, j;
  float x[3];
  float dummy;

  for(i = 0; i < this->NumberOfAtoms; i++)
    {
    if (fscanf(fp, "%d %f %f %f %f", &j, &dummy, x, x+1, x+2) != 5)
      {
      vtkErrorMacro ("GaussianCubeReader error reading file: " << this->FileName
                     << " Premature EOF while reading molecule.");
      fclose (fp);
      return;
      }
    this->Transform->TransformPoint(x, x);
    this->Points->InsertNextPoint(x);
    this->AtomType->InsertNextValue(j-1);
    this->AtomTypeStrings->InsertNextValue("Xx");
    this->Residue->InsertNextValue(-1);
    this->Chain->InsertNextValue(0);
    this->SecondaryStructures->InsertNextValue(0);
    this->SecondaryStructuresBegin->InsertNextValue(0);
    this->SecondaryStructuresEnd->InsertNextValue(0);
    this->IsHetatm->InsertNextValue(0);
    }
}

//----------------------------------------------------------------------------
vtkImageData *vtkGaussianCubeReader::GetGridOutput()
{
  if (this->GetNumberOfOutputPorts() < 2)
    {
    return NULL;
    }
  return vtkImageData::SafeDownCast(
    this->GetExecutive()->GetOutputData(1));
}

//----------------------------------------------------------------------------
void vtkGaussianCubeReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << "Filename: " << (this->FileName?this->FileName:"(none)") << "\n";

  os << "Transform: ";
  if( this->Transform )
    {
    os << endl;
    this->Transform->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)\n";
    }
}

//----------------------------------------------------------------------------
// Default implementation - copy information from first input to all outputs
int vtkGaussianCubeReader::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *vtkNotUsed(outputVector))
{
  // the set the information for the imagedat output
  vtkInformation *gridInfo = this->GetExecutive()->GetOutputInformation(1);

  FILE *fp;
  char title[256];

  if (!this->FileName)
    {
    return 0;
    }

  if ((fp = fopen(this->FileName, "r")) == NULL)
    {
    vtkErrorMacro(<< "File " << this->FileName << " not found");
    return 0;
    }

  if (!fgets(title, 256, fp))
    {
    vtkErrorMacro ("GaussianCubeReader error reading file: " << this->FileName
                   << " Premature EOF while reading title.");
    fclose (fp);
    return 0;
    }
  if (!fgets(title, 256, fp))
    {
    vtkErrorMacro ("GaussianCubeReader error reading file: " << this->FileName
                   << " Premature EOF while reading title.");
    fclose (fp);
    return 0;
    }

  // Read in number of atoms, x-origin, y-origin z-origin
  double tmpd;
  int n1, n2, n3;
  if (fscanf(fp, "%d %lf %lf %lf", &n1, &tmpd, &tmpd, &tmpd) != 4)
    {
    vtkErrorMacro ("GaussianCubeReader error reading file: " << this->FileName
                   << " Premature EOF while grid size.");
    fclose (fp);
    return 0;
    }

  if (fscanf(fp, "%d %lf %lf %lf", &n1, &tmpd, &tmpd, &tmpd) != 4)
    {
    vtkErrorMacro ("GaussianCubeReader error reading file: " << this->FileName
                   << " Premature EOF while grid size.");
    fclose (fp);
    return 0;
    }
  if (fscanf(fp, "%d %lf %lf %lf", &n2, &tmpd, &tmpd, &tmpd) != 4)
    {
    vtkErrorMacro ("GaussianCubeReader error reading file: " << this->FileName
                   << " Premature EOF while grid size.");
    fclose (fp);
    return 0;
    }
  if (fscanf(fp, "%d %lf %lf %lf", &n3, &tmpd, &tmpd, &tmpd) != 4)
    {
    vtkErrorMacro ("GaussianCubeReader error reading file: " << this->FileName
                   << " Premature EOF while grid size.");
    fclose (fp);
    return 0;
    }

  vtkDebugMacro(<< "Grid Size " << n1 << " " << n2 << " " << n3);
  gridInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
                0, n1-1, 0, n2-1, 0, n3-1);
  gridInfo->Set(vtkDataObject::ORIGIN(), 0, 0, 0);
  gridInfo->Set(vtkDataObject::SPACING(), 1, 1, 1);

  fclose(fp);

  vtkDataObject::SetPointDataActiveScalarInfo(gridInfo, VTK_FLOAT, -1);
  return 1;
}

//----------------------------------------------------------------------------
int vtkGaussianCubeReader::FillOutputPortInformation(int port,
                                                     vtkInformation* info)
{
  if(port == 0)
    {
    return this->Superclass::FillOutputPortInformation(port, info);
    }
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
  return 1;
}
