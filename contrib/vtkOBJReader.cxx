/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOBJReader.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkOBJReader.h"

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
  vtkNormals *objNormals;
  vtkTCoords *objTCoords;
  vtkPoints *pts;
  vtkNormals *normals;
  vtkTCoords *tcoords;
  vtkCellArray *polys;
  float xyz[3], n[3], tc[3];
  vtkPolyData *output = this->GetOutput();
  vtkPointData *outputPD = output->GetPointData();
  int count;
  int ptId;
  int numberOfPts, numberOfNormals, numberOfTCoords;
  int objPtId, objNormalId, objTCoordId;
  char *slash, *blank, *next, *ptr;

  vtkDebugMacro(<<"Reading file");

  // Initialize
  if ((fptr = fopen(this->FileName, "r")) == NULL)
    {
    vtkErrorMacro(<< "File " << this->FileName << " not found");
    return;
    }

  // allocate points and point data to hold obj input
  objPts = vtkPoints::New();
  objPts->Allocate(1000,5000);  

  objNormals = vtkNormals::New();
  objNormals->Allocate(1000,5000);  

  objTCoords = vtkTCoords::New();
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
      objTCoords->InsertNextTCoord(tc);
      }
    else if (strncmp (line, "vn", 2) == 0)
      {
      sscanf (line, "%*[^ ]%f %f %f", n, n + 1, n + 2);
      objNormals->InsertNextNormal(n);
      }
    }

  // allocate points and point data for vtk polydata
  pts = vtkPoints::New();
  numberOfPts = objPts->GetNumberOfPoints();
  pts->Allocate(numberOfPts, numberOfPts);

  numberOfNormals = objNormals->GetNumberOfNormals ();
  if (numberOfNormals > 0)
    {
    normals = vtkNormals::New();
    normals->Allocate(numberOfPts, numberOfPts);
    }
  else
    {
    normals = NULL;
    }

  numberOfTCoords = objTCoords->GetNumberOfTCoords ();
  if (numberOfTCoords > 0)
    {
    tcoords = vtkTCoords::New();
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
        while (*ptr == ' ') ptr++;
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
              blank = (char *) strchr (line, (int) ' ');
              slash = (char *) strchr (line, (int) '/');
              if (blank && slash && (slash < blank)) 
                {
                ptr = slash + 1;
                sscanf (ptr, "%d", &objNormalId);
                }
              }
            }
	  if (normals && objNormalId != -1)
	    {
	    normals->InsertNextNormal (objNormals->GetNormal (objNormalId - 1));
	    }
	  if (tcoords && objTCoordId != -1)
	    {
	    tcoords->InsertNextTCoord (objTCoords->GetTCoord (objTCoordId - 1));
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
    if ( normals->GetNumberOfNormals() > 0 )
      {
      outputPD->SetNormals(normals);
      }
    normals->Delete();
    }    

  if (tcoords)
    {
    if ( tcoords->GetNumberOfTCoords() > 0 )
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
  vtkPolyDataSource::PrintSelf(os,indent);

  os << indent << "File Name: " 
     << (this->FileName ? this->FileName : "(none)") << "\n";

}
