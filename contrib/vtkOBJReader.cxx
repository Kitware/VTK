/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOBJReader.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkOBJReader.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkOBJReader* vtkOBJReader::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkOBJReader");
  if(ret)
    {
    return (vtkOBJReader*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkOBJReader;
}




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
  vtkIdType count;
  vtkIdType ptId;
  vtkIdType numberOfPts, numberOfNormals, numberOfTCoords;
  vtkIdType objPtId, objNormalId, objTCoordId;
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
