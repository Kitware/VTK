/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOBJReader.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOBJReader.h"
#include "vtkObjectFactory.h"
#include "vtkFloatArray.h"

vtkCxxRevisionMacro(vtkOBJReader, "1.20");
vtkStandardNewMacro(vtkOBJReader);

// Description:
// Instantiate object with NULL filename.
vtkOBJReader::vtkOBJReader()
{
  this->FileName = NULL;
}

vtkOBJReader::~vtkOBJReader()
{
  if (this->FileName)
    {
    delete [] this->FileName;
    this->FileName = NULL;
    }
}

void vtkOBJReader::Execute()
{
  FILE *fptr;
  char line[1024];
  vtkPoints *objPts;
  vtkFloatArray *objNormals;
  vtkFloatArray *objTCoords;
  vtkPoints *pts;
  vtkFloatArray *normals;
  vtkFloatArray *tcoords;
  vtkCellArray *polys;
  float xyz[3], n[3], tc[3];
  vtkPolyData *output = this->GetOutput();
  vtkPointData *outputPD = output->GetPointData();
  int count;
  vtkIdType ptId;
  vtkIdType numberOfPts, numberOfNormals, numberOfTCoords;
  int objPtId, objNormalId, objTCoordId;
  char *slash, *blank, *next, *ptr;

  vtkDebugMacro(<<"Reading file");

  if (!this->FileName)
    {
    vtkErrorMacro(<< "A FileName must be specified.");
    return;
    }

  // Initialize
  if ((fptr = fopen(this->FileName, "r")) == NULL)
    {
    vtkErrorMacro(<< "File " << this->FileName << " not found");
    return;
    }

  // allocate points and point data to hold obj input
  objPts = vtkPoints::New();
  objPts->Allocate(1000,5000);  

  objNormals = vtkFloatArray::New();
  objNormals->SetNumberOfComponents(3);
  objNormals->Allocate(1000,5000);  

  objTCoords = vtkFloatArray::New();
  objTCoords->SetNumberOfComponents(2);
  objTCoords->Allocate(1000,5000);  

  polys = vtkCellArray::New();
  polys->Allocate(1000,5000);

  // parse file twice... first time to get point data and then cells
  while (fgets (line, 1024, fptr)) 
    {
    if (strncmp (line, "v ", 2) == 0)
      {
      sscanf (line, "%*[^ ]%f %f %f", xyz, xyz + 1, xyz + 2);
      objPts->InsertNextPoint(xyz);
      }
    else if (strncmp (line, "vt", 2) == 0)
      {
      sscanf (line, "%*[^ ]%f %f %f", tc, tc + 1, tc + 2);
      objTCoords->InsertNextTuple(tc);
      }
    else if (strncmp (line, "vn", 2) == 0)
      {
      sscanf (line, "%*[^ ]%f %f %f", n, n + 1, n + 2);
      objNormals->InsertNextTuple(n);
      }
    }

  // allocate points and point data for vtk polydata
  pts = vtkPoints::New();
  numberOfPts = objPts->GetNumberOfPoints();
  pts->Allocate(numberOfPts, numberOfPts);

  numberOfNormals = objNormals->GetNumberOfTuples ();
  if (numberOfNormals > 0)
    {
    normals = vtkFloatArray::New();
    normals->SetNumberOfComponents(3);
    normals->Allocate(3*numberOfPts, 3*numberOfPts);
    }
  else
    {
    normals = NULL;
    }

  numberOfTCoords = objTCoords->GetNumberOfTuples ();
  if (numberOfTCoords > 0)
    {
    tcoords = vtkFloatArray::New();
    tcoords->SetNumberOfComponents(2);
    tcoords->Allocate(numberOfPts, numberOfPts);
    }
  else
    {
    tcoords = NULL;
    }

  ptId = 0;
  rewind (fptr);
  while (fgets (line, 1024, fptr)) 
    {
    if ( strncmp(line, "f ", 2) == 0 || strncmp(line, "fo", 2) == 0) 
      {
      count = 0;
      polys->InsertNextCell(0);
      ptr = line;
      while ((ptr = (char *) strchr (ptr, (int) ' '))) 
        {
        while (*ptr == ' ')
          {
          ptr++;
          }
        if ( sscanf(ptr, "%d", &objPtId) == 1 ) 
          {
          polys->InsertCellPoint(ptId++);
          pts->InsertNextPoint (objPts->GetPoint (objPtId-1));
          count++;
          objNormalId = -1;
          objTCoordId = -1;
          if ((next = (char *) strchr (ptr, (int) '/'))) 
            {
            ptr = next + 1;
            if (*ptr == '/') 
              {
              sscanf (ptr + 1, "%d", &objNormalId);
              }
            else 
              {
              sscanf (ptr, "%d", &objTCoordId);
              blank = (char *) strchr (ptr, (int) ' ');
              slash = (char *) strchr (ptr, (int) '/');
              if (blank && slash && (slash < blank)) 
                {
                ptr = slash + 1;
                sscanf (ptr, "%d", &objNormalId);
                }
              }
            }
          if (normals && objNormalId != -1)
            {
            normals->InsertNextTuple (objNormals->GetTuple (objNormalId - 1));
            }
          if (tcoords && objTCoordId != -1)
            {
            tcoords->InsertNextTuple (objTCoords->GetTuple (objTCoordId - 1));
            }
          }
        }
      polys->UpdateCellCount(count);
      }
    }

  // close the file
  fclose (fptr);

  // delete the obj points, normals and tcoords
  objPts->Delete();
  objNormals->Delete();
  objTCoords->Delete();

  output->SetPoints (pts);
  pts->Delete ();

  // update output
  if (normals)
    {
    if ( normals->GetNumberOfTuples() > 0 )
      {
      outputPD->SetNormals(normals);
      }
    normals->Delete();
    }    

  if (tcoords)
    {
    if ( tcoords->GetNumberOfTuples() > 0 )
      {
      outputPD->SetTCoords(tcoords);
      }
    tcoords->Delete();
    }    

  output->SetPolys(polys);
  polys->Delete();

  output->Squeeze();
}

void vtkOBJReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "File Name: " 
     << (this->FileName ? this->FileName : "(none)") << "\n";

}
