/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOverrideInformation.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to William A. Hoffman who developed this class

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
// .NAME vtkOverrideInformation - Factory object override information
// .SECTION Description
// vtkOverrideInformation is used to represent the information about
// a class which is overriden in a vtkObjectFactory.
//



#ifndef __vtkOverrideInformation_h
#define __vtkOverrideInformation_h


#include "vtkObject.h"
#include "vtkObjectFactory.h"


class VTK_EXPORT vtkOverrideInformation : public vtkObject
{
public: 
  static vtkOverrideInformation* New();
  vtkTypeMacro(vtkOverrideInformation,vtkObject);
  // Description:
  // Print ObjectFactor to stream.
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns the name of the class being overriden.  For example,
  // if you had a factory that provided an override for 
  // vtkVertex, then this funciton would return "vtkVertex"
  const char* GetClassOverrideName() 
    { 
      return this->ClassOverrideName; 
    }
  
  // Description:
  // Returns the name of the class that will override the class.
  // For example, if you had a factory that provided an override for
  // vtkVertex called vtkMyVertex, then this would return "vtkMyVertex"
  const char* GetClassOverrideWithName()
    { 
      return this->ClassOverrideWithName; 
    }
  
  // Description:
  // Return a human readable or GUI displayable description of this
  // override.   
  const char* GetDescription()
    { 
      return this->Description; 
    }

  // Description:
  // Return the specific object factory that this override occurs in.
  vtkObjectFactory* GetObjectFactory()
    {
      return this->ObjectFactory;
    }
  // Description:
  // Set the class override name
  vtkSetStringMacro(ClassOverrideName);
  // Set the class override with name
  vtkSetStringMacro(ClassOverrideWithName);
  // Set the description
  vtkSetStringMacro(Description);
protected:
  vtkSetObjectMacro(ObjectFactory, vtkObjectFactory);
private:
  vtkOverrideInformation();
  ~vtkOverrideInformation();
  vtkOverrideInformation(const vtkOverrideInformation&);
  void operator=(const vtkOverrideInformation&);
  // allow the object factory to set the values in this
  // class, but only the object factory
//BTX
  friend class vtkObjectFactory;
//ETX
  
  char* ClassOverrideName;
  char* ClassOverrideWithName;
  char* Description;
  vtkObjectFactory* ObjectFactory;  
};

#endif
