/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWarpTransform.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G. Gobbi who developed this class.

Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "vtkWarpTransform.h"

//----------------------------------------------------------------------------
void vtkWarpTransform::PrintSelf(vtkOstream& os, vtkIndent indent)
{
  vtkGeneralTransform::PrintSelf(os, indent);

  os << indent << "InverseFlag: " << this->InverseFlag << "\n";
}

//------------------------------------------------------------------------
// Check the InverseFlag, and perform a forward or reverse transform
// as appropriate.
void vtkWarpTransform::InternalTransformPoint(const float input[3],
					      float output[3])
{
  if (this->InverseFlag)
    {
    this->InverseTransformPoint(input,output);
    }
  else
    {
    this->ForwardTransformPoint(input,output);
    }
}

//------------------------------------------------------------------------
// Check the InverseFlag, and set the output point and derivative as
// appropriate.
void vtkWarpTransform::InternalTransformDerivative(const float input[3],
						   float output[3],
						   float derivative[3][3])
{
  if (this->InverseFlag)
    {
    float tmp[3];
    tmp[0] = input[0];
    tmp[1] = input[1];
    tmp[2] = input[2];
    this->ForwardTransformDerivative(tmp,output,derivative);
    this->InverseTransformPoint(tmp,output);
    vtkGeneralTransform::Invert3x3(derivative,derivative);
    }
  else
    {
    this->ForwardTransformDerivative(input,output,derivative);
    }
}

//----------------------------------------------------------------------------
// To invert the transformation, just set the InverseFlag.
void vtkWarpTransform::Inverse()
{
  this->InverseFlag = !this->InverseFlag;
  this->Modified();
}







