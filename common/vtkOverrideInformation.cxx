/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOverrideInformation.cxx
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
#include "vtkOverrideInformation.h"



vtkOverrideInformation* vtkOverrideInformation::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkOverrideInformation");
  if(ret)
    {
    return (vtkOverrideInformation*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkOverrideInformation;
}

vtkOverrideInformation::vtkOverrideInformation()
{
  this->ClassOverrideName = 0;
  this->ClassOverrideWithName = 0;
  this->Description = 0;
  this->ObjectFactory = 0;
}

vtkOverrideInformation::~vtkOverrideInformation()
{
  delete [] this->ClassOverrideName;
  delete [] this->ClassOverrideWithName;
  delete [] this->Description;
  if(this->ObjectFactory)
    {
    this->ObjectFactory->Delete();
    }
}


void vtkOverrideInformation::PrintSelf(ostream& os,
                                       vtkIndent indent)
{
  vtkObject::PrintSelf(os, indent);
  os << indent
     << "Override: " << this->ClassOverrideName 
     << "\nWith: " << this->ClassOverrideWithName 
     << "\nDescription: " << this->Description;
  os << indent << "From Factory:\n";
  if(this->ObjectFactory)
    {
    this->ObjectFactory->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    vtkIndent n = indent.GetNextIndent();
    os << n << "(NULL)\n";
    }
}


