/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTkRenderWindowInteractorFactory.h
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


#ifndef __vtkTkRenderWindowInteractorFactory_h
#define __vtkTkRenderWindowInteractorFactory_h


#include "vtkObjectFactory.h"
#include "vtkVersion.h"

#ifndef _WIN32
 #include "vtkXRenderWindowTclInteractor.h"
#endif

class VTK_EXPORT vtkTkRenderWindowInteractorFactory : public vtkObjectFactory
{
public:  
  static vtkTkRenderWindowInteractorFactory *New() {return new vtkTkRenderWindowInteractorFactory;};
  
  // Description:
  vtkObject* CreateObject(const char* vtkclassname);

  // Description:
  // All sub-classes of vtkObjectFactory should must return the version of 
  // VTK they were built with.  This should be implemented with the macro
  // VTK_SOURCE_VERSION and NOT a call to vtkVersion::GetVTKSourceVersion.
  // As the version needs to be compiled into the file as a string constant.
  // This is critical to determine possible incompatible dynamic factory loads.
  virtual const char* GetVTKSourceVersion() {return VTK_SOURCE_VERSION;}

  // Description:
  // Return a descriptive string describing the factory.
  virtual const char* GetDescription() {return "Creates a vtkXRenderWindowTclInteractor";}

};

vtkObject* vtkTkRenderWindowInteractorFactory::CreateObject(const char* vtkclassname)
{
  #ifndef _WIN32
  if(strcmp(vtkclassname, "vtkRenderWindowInteractor") == 0)
    {
    return vtkXRenderWindowTclInteractor::New();
    }
  else
    {
    return NULL;
    }
  #else
    return NULL;
  #endif
}

#endif

