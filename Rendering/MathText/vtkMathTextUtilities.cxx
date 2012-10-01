/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMathTextUtilities.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMathTextUtilities.h"

#include "vtkObjectFactory.h"

#ifdef VTK_DEBUG_LEAKS
#include "vtkDebugLeaks.h"
#endif

//----------------------------------------------------------------------------
vtkInstantiatorNewMacro(vtkMathTextUtilities)

//----------------------------------------------------------------------------
// The singleton, and the singleton cleanup
vtkMathTextUtilities* vtkMathTextUtilities::Instance = NULL;
vtkMathTextUtilitiesCleanup vtkMathTextUtilities::Cleanup;

//----------------------------------------------------------------------------
// Create the singleton cleanup
// Register our singleton cleanup callback against the FTLibrary so that
// it might be called before the FTLibrary singleton is destroyed.
vtkMathTextUtilitiesCleanup::vtkMathTextUtilitiesCleanup()
{
}

//----------------------------------------------------------------------------
// Delete the singleton cleanup
vtkMathTextUtilitiesCleanup::~vtkMathTextUtilitiesCleanup()
{
  vtkMathTextUtilities::SetInstance(NULL);
}

//----------------------------------------------------------------------------
vtkMathTextUtilities* vtkMathTextUtilities::GetInstance()
{
  if (!vtkMathTextUtilities::Instance)
    {
    vtkMathTextUtilities::Instance = static_cast<vtkMathTextUtilities *>(
      vtkObjectFactory::CreateInstance("vtkMathTextUtilities"));
    // Clean up any leaked references from vtkDebugLeaks if needed
#ifdef VTK_DEBUG_LEAKS
    if (!vtkMathTextUtilities::Instance)
      {
      vtkDebugLeaks::DestructClass("vtkMathTextUtilities");
      }
#endif
    }

  return vtkMathTextUtilities::Instance;
}

//----------------------------------------------------------------------------
void vtkMathTextUtilities::SetInstance(vtkMathTextUtilities* instance)
{
  if (vtkMathTextUtilities::Instance == instance)
    {
    return;
    }

  if (vtkMathTextUtilities::Instance)
    {
    vtkMathTextUtilities::Instance->Delete();
    }

  vtkMathTextUtilities::Instance = instance;

  // User will call ->Delete() after setting instance
  if (instance)
    {
    instance->Register(NULL);
    }
}

//----------------------------------------------------------------------------
vtkMathTextUtilities* vtkMathTextUtilities::New()
{
  vtkMathTextUtilities* ret = vtkMathTextUtilities::GetInstance();
  if (ret)
    {
    ret->Register(NULL);
    }
  return ret;
}

//----------------------------------------------------------------------------
vtkMathTextUtilities::vtkMathTextUtilities()
{
}

//----------------------------------------------------------------------------
vtkMathTextUtilities::~vtkMathTextUtilities()
{
}

//----------------------------------------------------------------------------
void vtkMathTextUtilities::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Instance: " << this->Instance << endl;
}
