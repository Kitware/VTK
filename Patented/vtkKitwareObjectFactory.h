/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKitwareObjectFactory.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKitwareObjectFactory - Object Factory for Kitware patented objects.
// .SECTION Description
// This is an object factory used to create Kitware patented objects.
// There is a KitwareFactory.dsp and KitwareFactory.dsw file to create
// the factory dll with the microsoft compiler.  Once the Factory is 
// created, put the resulting dll in VTK_AUTOLOAD_PATH.  
//
// .SECTION See Also
// vtkObjectFactory

#ifndef __vtkKitwareObjectFactory_h
#define __vtkKitwareObjectFactory_h

#include "vtkObjectFactory.h"

class VTK_PATENTED_EXPORT vtkKitwareObjectFactory : public vtkObjectFactory
{
public:
  static vtkKitwareObjectFactory *New() {return new vtkKitwareObjectFactory;};
  vtkTypeRevisionMacro(vtkKitwareObjectFactory,vtkObjectFactory);
  void PrintSelf(ostream& os, vtkIndent indent);  
  virtual const char* GetVTKSourceVersion();
protected:
  vtkKitwareObjectFactory() {};
  virtual vtkObject* CreateObject(const char* vtkclassname );
private:
  vtkKitwareObjectFactory(const vtkKitwareObjectFactory&);  // Not implemented.
  void operator=(const vtkKitwareObjectFactory&);  // Not implemented.
};

extern "C" VTK_PATENTED_EXPORT vtkObjectFactory* vtkLoad();
#endif
