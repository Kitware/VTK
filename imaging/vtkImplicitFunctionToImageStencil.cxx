/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitFunctionToImageStencil.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G Gobbi who developed this class.

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

#include <math.h>
#include "vtkImplicitFunctionToImageStencil.h"
#include "vtkObjectFactory.h"


//----------------------------------------------------------------------------
vtkImplicitFunctionToImageStencil* vtkImplicitFunctionToImageStencil::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImplicitFunctionToImageStencil");
  if(ret)
    {
    return (vtkImplicitFunctionToImageStencil*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImplicitFunctionToImageStencil;
}

//----------------------------------------------------------------------------
vtkImplicitFunctionToImageStencil::vtkImplicitFunctionToImageStencil()
{
  this->Threshold = 0;
  this->Input = NULL;
}

//----------------------------------------------------------------------------
vtkImplicitFunctionToImageStencil::~vtkImplicitFunctionToImageStencil()
{
  this->SetInput(NULL);
}

//----------------------------------------------------------------------------
void vtkImplicitFunctionToImageStencil::PrintSelf(ostream& os,
						  vtkIndent indent)
{
  vtkImageStencilSource::PrintSelf(os,indent);

  os << indent << "Input: " << this->Input << "\n";
  os << indent << "Threshold: " << this->Threshold << "\n";
}

//----------------------------------------------------------------------------
// set up the clipping extents from an implicit function by brute force
// (i.e. by evaluating the function at each and every voxel)
void vtkImplicitFunctionToImageStencil::ThreadedExecute(vtkImageStencilData 
							              *data,
							int extent[6],
							int vtkNotUsed(id))
{
  vtkImplicitFunction *function = this->Input;
  float *spacing = data->GetSpacing();
  float *origin = data->GetOrigin();
  float threshold = this->Threshold;

  // for conversion of (idX,idY,idZ) into (x,y,z)
  float point[3];

  // loop through all voxels
  for (int idZ = extent[4]; idZ <= extent[5]; idZ++)
    {
    point[2] = idZ*spacing[2] + origin[2];

    for (int idY = extent[2]; idY <= extent[3]; idY++)
      {
      point[1] = idY*spacing[1] + origin[1];
      int state = 1; // inside or outside, start outside
      int r1 = extent[0];
      int r2 = extent[1];

      for (int idX = extent[0]; idX <= extent[1]; idX++)
	{
        point[0] = idX*spacing[0] + origin[0];
	int newstate = 1;
	if (function->FunctionValue(point) < threshold)
	  {
          newstate = -1;
	  if (newstate != state)
	    { // sub extent starts
	    r1 = idX;
	    }
	  }
	else if (newstate != state)
	  { // sub extent ends
	  r2 = idX - 1;
	  data->InsertNextExtent(r1, r2, idY, idZ);
	  }
	state = newstate;
	} // for idX
      if (state == -1)
	{ // if inside at end, cap off the sub extent
	data->InsertNextExtent(r1, extent[1], idY, idZ);
	}
      } // for idY    
    } // for idZ
}
