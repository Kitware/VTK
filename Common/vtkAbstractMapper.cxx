/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractMapper.cxx
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
#include "vtkAbstractMapper.h"
#include "vtkDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPlanes.h"

// Construct object.
vtkAbstractMapper::vtkAbstractMapper()
{
  this->TimeToDraw = 0.0;
  this->LastWindow = NULL;
  this->ClippingPlanes = NULL;
  this->Timer = vtkTimerLog::New();
}

vtkAbstractMapper::~vtkAbstractMapper()
{
  this->Timer->Delete();
  if (this->ClippingPlanes)
    {
    this->ClippingPlanes->UnRegister(this);
    }
}

// Description:
// Override Modifiedtime as we have added Clipping planes
unsigned long vtkAbstractMapper::GetMTime()
{
  unsigned long mTime = vtkProcessObject::GetMTime();
  unsigned long clipMTime;

  if ( this->ClippingPlanes != NULL )
    {
    clipMTime = this->ClippingPlanes->GetMTime();
    mTime = ( clipMTime > mTime ? clipMTime : mTime );
    }

  return mTime;
}

void vtkAbstractMapper::AddClippingPlane(vtkPlane *plane)
{
  if (this->ClippingPlanes == NULL)
    {
    this->ClippingPlanes = vtkPlaneCollection::New();
    this->ClippingPlanes->Register(this);
    this->ClippingPlanes->Delete();
    }

  this->ClippingPlanes->AddItem(plane);
}

void vtkAbstractMapper::RemoveClippingPlane(vtkPlane *plane)
{
  if (this->ClippingPlanes == NULL)
    {
    vtkErrorMacro(<< "Cannot remove clipping plane: mapper has none");
    }
  this->ClippingPlanes->RemoveItem(plane);
}

void vtkAbstractMapper::RemoveAllClippingPlanes()
{
  if ( this->ClippingPlanes )
    {
    this->ClippingPlanes->RemoveAllItems();
    }
}

void vtkAbstractMapper::SetClippingPlanes(vtkPlanes *planes)
{
  vtkPlane *plane;
  int numPlanes = planes->GetNumberOfPlanes();

  this->RemoveAllClippingPlanes();
  for (int i=0; i<numPlanes && i<6; i++)
    {
    plane = planes->GetPlane(i);
    this->AddClippingPlane(plane);
    }
}

vtkDataArray *vtkAbstractMapper::GetScalars(vtkDataSet *input,
                                            int scalarMode,
											int arrayAccessMode,
                                            int arrayId, 
                                            const char *arrayName,
                                            int& offset)
{
  vtkDataArray *scalars=NULL;
  vtkPointData *pd;
  vtkCellData *cd;
  
  // make sure we have an input
  if ( !input )
    {
    return NULL;
    }
    
  // get and scalar data according to scalar mode
  if ( scalarMode == VTK_SCALAR_MODE_DEFAULT )
    {
    scalars = input->GetPointData()->GetScalars();
    if (!scalars)
      {
      scalars = input->GetCellData()->GetScalars();
      }
    }
  else if ( scalarMode == VTK_SCALAR_MODE_USE_POINT_DATA )
    {
    scalars = input->GetPointData()->GetScalars();
    }
  else if ( scalarMode == VTK_SCALAR_MODE_USE_CELL_DATA )
    {
    scalars = input->GetCellData()->GetScalars();
    }
  else if ( scalarMode == VTK_SCALAR_MODE_USE_POINT_FIELD_DATA )
    {
    pd = input->GetPointData();
    if (arrayAccessMode == VTK_GET_ARRAY_BY_ID)
      {
      scalars = pd->GetArray(arrayId);
      }
    else
      {
      scalars = pd->GetArray(arrayName);
      }
    
    if ( !scalars ||
         !(offset < scalars->GetNumberOfComponents()) )
      {
      offset=0;
      //vtkGenericWarningMacro(<<"Data array (used for coloring) not found");
      }
    }
  else if ( scalarMode == VTK_SCALAR_MODE_USE_CELL_FIELD_DATA )
    {
    cd = input->GetCellData();
    if (arrayAccessMode == VTK_GET_ARRAY_BY_ID)
      {
      scalars = cd->GetArray(arrayId);
      }
    else
      {
      scalars = cd->GetArray(arrayName);
      }

    if ( !scalars ||
         !(offset < scalars->GetNumberOfComponents()) )
      {
      offset=0;
      //vtkGenericWarningMacro(<<"Data array (used for coloring) not found");
      }
    }
  
  return scalars;
}


// Shallow copy of vtkProp.
void vtkAbstractMapper::ShallowCopy(vtkAbstractMapper *mapper)
{
  this->SetClippingPlanes( mapper->GetClippingPlanes() );
}

void vtkAbstractMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkProcessObject::PrintSelf(os,indent);

  os << indent << "TimeToDraw: " << this->TimeToDraw << "\n";

  if ( this->ClippingPlanes )
    {
    os << indent << "ClippingPlanes:\n";
    this->ClippingPlanes->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "ClippingPlanes: (none)\n";
    }
}

