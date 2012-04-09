/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOverrideInformation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOverrideInformation - Factory object override information
// .SECTION Description
// vtkOverrideInformation is used to represent the information about
// a class which is overriden in a vtkObjectFactory.
//

#ifndef __vtkOverrideInformation_h
#define __vtkOverrideInformation_h

#include "vtkObject.h"

class vtkObjectFactory;

class VTK_COMMON_EXPORT vtkOverrideInformation : public vtkObject
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
  virtual void SetObjectFactory(vtkObjectFactory*);

private:
  vtkOverrideInformation();
  ~vtkOverrideInformation();
  // allow the object factory to set the values in this
  // class, but only the object factory
//BTX
  friend class vtkObjectFactory;
//ETX
  
  char* ClassOverrideName;
  char* ClassOverrideWithName;
  char* Description;
  vtkObjectFactory* ObjectFactory;  
private:
  vtkOverrideInformation(const vtkOverrideInformation&);  // Not implemented.
  void operator=(const vtkOverrideInformation&);  // Not implemented.
};

#endif
