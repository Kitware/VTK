/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuantizePolyDataPoints.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to John Biddiscombe of the Rutherford Appleton Laboratory
             who developed and contributed this class.


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
#include "vtkQuantizePolyDataPoints.h"
#include "vtkMergePoints.h"
#include "vtkObjectFactory.h"

//--------------------------------------------------------------------------
vtkQuantizePolyDataPoints* vtkQuantizePolyDataPoints::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkQuantizePolyDataPoints");
  if (ret) 
    {
    return (vtkQuantizePolyDataPoints*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkQuantizePolyDataPoints;
}

//--------------------------------------------------------------------------
// Construct object with initial QFactor of 0.25
vtkQuantizePolyDataPoints::vtkQuantizePolyDataPoints()
{
  this->QFactor   = 0.25;
  this->Tolerance = 0.0;
}

//--------------------------------------------------------------------------
void vtkQuantizePolyDataPoints::OperateOnPoint(float in[3], float out[3])
{
  out[0] = floor(in[0]/this->QFactor + 0.5)*this->QFactor;
  out[1] = floor(in[1]/this->QFactor + 0.5)*this->QFactor;
  out[2] = floor(in[2]/this->QFactor + 0.5)*this->QFactor;
}

//-------------------------------------------------------------------------
void vtkQuantizePolyDataPoints::OperateOnBounds(float in[6], float out[6])
{
  out[0] = floor(in[0]/this->QFactor + 0.5)*this->QFactor;
  out[1] = floor(in[1]/this->QFactor + 0.5)*this->QFactor;
  out[2] = floor(in[2]/this->QFactor + 0.5)*this->QFactor;
  out[3] = floor(in[3]/this->QFactor + 0.5)*this->QFactor;
  out[4] = floor(in[4]/this->QFactor + 0.5)*this->QFactor;
  out[5] = floor(in[5]/this->QFactor + 0.5)*this->QFactor;
}

//--------------------------------------------------------------------------
void vtkQuantizePolyDataPoints::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkCleanPolyData::PrintSelf(os,indent);

  os << indent << "QFactor: " << this->QFactor << "\n";
}

