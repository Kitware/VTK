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
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkObjectFactory.h"
#include "vtkTransform.h"

#include <ctype.h>

vtkCxxRevisionMacro(vtkGaussianCubeReader, "1.7");
vtkStandardNewMacro(vtkGaussianCubeReader);

// Construct object with merging set to true.
vtkGaussianCubeReader::vtkGaussianCubeReader()
{
  this->FileName = NULL;
  this->Transform = vtkTransform::New();
  // Add the second output for the grid data
 
  vtkImageData *grid;
  grid = vtkImageData::New();
  grid->ReleaseData();
  this->AddOutput(grid);
  grid->Delete();
}

vtkGaussianCubeReader::~vtkGaussianCubeReader()
{
  if (this->FileName)
    {
    delete [] this->FileName;
    }
  this->Transform->Delete();
  // must delete the second output added
}

void vtkGaussianCubeReader::Execute()
{
  FILE *fp;
  char Title[256];
  char data_name[256];
  double elements[16];
  int JN1, N1N2, n1, n2, n3, i, j, k;
  float tmp, *Cube_data;

  // Output 0 (the default is the polydata)
  // Output 1 will be the gridded Image data

  vtkImageData *grid = this->GetGridOutput();

  if (!this->FileName)
    {
    return;
    }

  if ((fp = fopen(this->FileName, "r")) == NULL)
    {
    vtkErrorMacro(<< "File " << this->FileName << " not found");
    return;
    }

  fgets(Title, 256, fp);
  if(strtok(Title, ":") != NULL)
    {
    if(strtok(NULL, ":") != NULL)
      {
      strcpy(data_name, strtok(NULL, ":"));
      fprintf(stderr,"label = %s\n", data_name);
      }
    }
  fgets(Title, 256, fp);

  // Read in number of atoms, x-origin, y-origin z-origin
  //
  fscanf(fp, "%d %lf %lf %lf", &(this->NumberOfAtoms), &elements[3], 
         &elements[7], &elements[11]);
  if(this->NumberOfAtoms < 0 )
    {
    this->NumberOfAtoms = -this->NumberOfAtoms;
    }

  fscanf(fp, "%d %lf %lf %lf", &n1, &elements[0], &elements[4], &elements[8]);
  fscanf(fp, "%d %lf %lf %lf", &n2, &elements[1], &elements[5], &elements[9]);
  fscanf(fp, "%d %lf %lf %lf", &n3, &elements[2], &elements[6], &elements[10]);
  elements[12] = 0;
  elements[13] = 0;
  elements[14] = 0;
  elements[15] = 1;
  
  vtkDebugMacro(<< "Grid Size " << n1 << " " << n2 << " " << n3);

  Transform->SetMatrix(elements);
  Transform->Inverse();

  ReadMolecule(fp);

  grid->SetWholeExtent(0, n1-1,
                       0, n2-1,
                       0, n3-1);
  grid->SetUpdateExtent(grid->GetWholeExtent());
  grid->SetExtent(grid->GetWholeExtent());

  grid->SetOrigin(0, 0, 0);
  grid->SetSpacing(1, 1, 1);
  grid->SetScalarTypeToFloat();
  grid->AllocateScalars();

  grid->GetPointData()->GetScalars()->SetName("Gaussian Cube density");

  Cube_data = (float *)grid->GetPointData()->GetScalars()->GetVoidPointer(0);
  N1N2 = n1*n2;

  for(i = 0; i < n1; i++) 
    {
    JN1 = 0;
    for(j = 0; j < n2; j++) 
      {
      for(k = 0; k < n3; k++) 
        {
        fscanf(fp,"%f", &tmp);
        Cube_data[k*N1N2 + JN1 + i] = tmp;
        }
      JN1 += n1;
      }
    }
  fclose(fp);
}

void vtkGaussianCubeReader::ReadSpecificMolecule(FILE* fp)
{
  int i, j;
  float x[3];
  float dummy;

  for(i = 0; i < this->NumberOfAtoms; i++) 
    {
    fscanf(fp, "%d %f %f %f %f", &j, &dummy, x, x+1, x+2);
    this->Points->InsertNextPoint(x);
    this->AtomType->InsertNextValue(j-1);
    }
}

vtkImageData *vtkGaussianCubeReader::GetGridOutput()
{
  return vtkImageData::SafeDownCast(this->Outputs[1]);
}

void vtkGaussianCubeReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << "Filename: " << (this->FileName?this->FileName:"<null>") << "\n";
  os << indent << "Xform: ";
  //os << indent << this->Transform->PrintSelf(os, indent);
}
