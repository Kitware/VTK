/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredInformation.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


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
#include "vtkStructuredInformation.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkStructuredInformation* vtkStructuredInformation::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkStructuredInformation");
  if(ret)
    {
    return (vtkStructuredInformation*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkStructuredInformation;
}




//----------------------------------------------------------------------------
// Construct a new vtkStructuredInformation 
vtkStructuredInformation::vtkStructuredInformation()
{
  this->WholeExtent[0] = this->WholeExtent[2] = this->WholeExtent[4] = 0;
  this->WholeExtent[1] = this->WholeExtent[3] = this->WholeExtent[5] = 0;
}


//----------------------------------------------------------------------------
void vtkStructuredInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataInformation::PrintSelf(os, indent);
  int idx;
  
  os << indent << "WholeExtent: (" << this->WholeExtent[0];
  for (idx = 1; idx < 6; ++idx)
    {
    os << ", " << this->WholeExtent[idx];
    }
  os << ")\n";
}

//----------------------------------------------------------------------------
void vtkStructuredInformation::SetWholeExtent(int *ext)
{
  int idx, modified = 0;
  
  for (idx = 0; idx < 6; ++idx)
    {
    if (this->WholeExtent[idx] != ext[idx])
      {
      modified = 1;
      }
    this->WholeExtent[idx] = ext[idx];
    }
  
  if (modified)
    {
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkStructuredInformation::GetWholeExtent(int *ext)
{
  int idx;
  
  for (idx = 0; idx < 6; ++idx)
    {
    ext[idx] = this->WholeExtent[idx];
    }
}



//----------------------------------------------------------------------------
int vtkStructuredInformation::GetClassCheck(char *className)
{
  if (strcmp(className, "vtkStructuredInformation") == 0)
    {
    return 1;
    }
  // check superclass
  if (this->vtkDataInformation::GetClassCheck(className))
    {
    return 1;
    }
  
  return 0;
}


//----------------------------------------------------------------------------
void vtkStructuredInformation::Copy(vtkDataInformation *in)
{
  this->vtkDataInformation::Copy(in);
  
  if (in->GetClassCheck("vtkStructuredInformation"))
    {
    vtkStructuredInformation *info = (vtkStructuredInformation*)(in);
    this->SetWholeExtent(info->GetWholeExtent());
    }
}

//----------------------------------------------------------------------------
void vtkStructuredInformation::WriteSelf(ostream& os)
{
  this->vtkDataInformation::WriteSelf(os);

  os << this->WholeExtent[0] << " " << this->WholeExtent[1] << " "
     << this->WholeExtent[2] << " " << this->WholeExtent[3] << " "
     << this->WholeExtent[4] << " " << this->WholeExtent[5] << " " ;
}

//----------------------------------------------------------------------------
void vtkStructuredInformation::ReadSelf(istream& is)
{
  this->vtkDataInformation::ReadSelf(is);

  is >> this->WholeExtent[0] >> this->WholeExtent[1]
     >> this->WholeExtent[2] >> this->WholeExtent[3]
     >> this->WholeExtent[4] >> this->WholeExtent[5] ;

}




  



