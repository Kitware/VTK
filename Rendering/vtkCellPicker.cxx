/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellPicker.cxx
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
#include "vtkCellPicker.h"
#include "vtkObjectFactory.h"
#include "vtkVolumeMapper.h"

//----------------------------------------------------------------------------
vtkCellPicker* vtkCellPicker::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkCellPicker");
  if(ret)
    {
    return (vtkCellPicker*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkCellPicker;
}

vtkCellPicker::vtkCellPicker()
{
  this->CellId = -1;
  this->SubId = -1;
  for (int i=0; i<3; i++)
    {
    this->PCoords[i] = 0.0;
    }
  this->Cell = vtkGenericCell::New();
}

vtkCellPicker::~vtkCellPicker()
{
  this->Cell->Delete();
}

float vtkCellPicker::IntersectWithLine(float p1[3], float p2[3], float tol, 
                                       vtkAssemblyPath *path, 
                                       vtkProp3D *prop3D, 
                                       vtkAbstractMapper3D *m)
{
  vtkIdType numCells, cellId, minCellId;
  int i, minSubId, subId;
  float x[3], tMin, t, pcoords[3], minXYZ[3], minPcoords[3];
  vtkDataSet *input;
  vtkMapper *mapper;
  vtkVolumeMapper *volumeMapper;

  // Get the underlying dataset
  if ( (mapper=vtkMapper::SafeDownCast(m)) != NULL )
    {
    input = mapper->GetInput();
    }
  else if ( (volumeMapper=vtkVolumeMapper::SafeDownCast(m)) != NULL )
    {
    input = volumeMapper->GetInput();
    }
  else
    {
    return VTK_LARGE_FLOAT;
    }

  if ( (numCells = input->GetNumberOfCells()) < 1 )
    {
    return 2.0;
    }

  //  Intersect each cell with ray.  Keep track of one closest to 
  //  the eye (and within the clipping range).
  //
  minCellId = -1;
  minSubId = -1;
  pcoords[0] = pcoords[1] = pcoords[2] = 0;
  for (tMin=VTK_LARGE_FLOAT,cellId=0; cellId<numCells; cellId++) 
    {
    input->GetCell(cellId, this->Cell);

    if ( this->Cell->IntersectWithLine(p1, p2, tol, t, x, pcoords, subId) 
    && t < tMin )
      {
      minCellId = cellId;
      minSubId = subId;
      for (i=0; i<3; i++)
        {
        minXYZ[i] = x[i];
        minPcoords[i] = pcoords[i];
        }
      tMin = t;
      }
    }
  //  Now compare this against other actors.
  //
  if ( minCellId>(-1) && tMin < this->GlobalTMin ) 
    {
    this->MarkPicked(path, prop3D, m, tMin, minXYZ);
    this->CellId = minCellId;
    this->SubId = minSubId;
    for (i=0; i<3; i++)
      {
      this->PCoords[i] = minPcoords[i];
      }
    vtkDebugMacro("Picked cell id= " << minCellId);
    }
  return tMin;
}

void vtkCellPicker::Initialize()
{
  this->CellId = (-1);
  this->SubId = (-1);
  for (int i=0; i<3; i++)
    {
    this->PCoords[i] = 0.0;
    }
  this->vtkPicker::Initialize();
}

void vtkCellPicker::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkPicker::PrintSelf(os,indent);

  os << indent << "Cell Id: " << this->CellId << "\n";
  os << indent << "SubId: " << this->SubId << "\n";
  os << indent << "PCoords: (" << this->PCoords[0] << ", " 
     << this->PCoords[1] << ", " << this->PCoords[2] << ")\n";
}
