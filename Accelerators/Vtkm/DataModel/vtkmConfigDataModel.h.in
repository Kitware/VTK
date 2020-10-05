/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ObjectFactory.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkConfigDataModel_h
#define vtkConfigDataModel_h

/*--------------------------------------------------------------------------*/
/* Other Configuration Options                                              */

#include <vtkm/internal/Configure.h>
#include "vtkAcceleratorsVTKmDataModelModule.h" //required for correct implementation


/*--------------------------------------------------------------------------*/
/* Make sure we use the same id's in VTK and VTK-m                          */
#include "vtkType.h"
#ifdef VTK_USE_64BIT_IDS
# ifndef VTKM_USE_64BIT_IDS
#  error VTK was defined with 64-bit ids but VTK-m with 32-bit ids.
# endif
#else // !VTK_USE_64BIT_IDS
# ifdef VTKM_USE_64BIT_IDS
#  error VTK was defined with 32-bit ids but VTK-m with 64-bit ids.
# endif
#endif


#ifndef VTKACCELERATORSVTKMDATAMODEL_TEMPLATE_EXPORT
# if !defined(VTKACCELERATORSVTKMDATAMODEL_STATIC_DEFINE) && defined(_MSC_VER)
  /* Warning C4910 on windows state that extern explicit template can't be
     labeled with __declspec(dllexport). So that is why we use a new custom
     define. But when other modules ( e.g. rendering ) include this header
     we need them to see that the extern template is actually being imported.
  */
    /* We are building this library with MSVC */
#   define VTKACCELERATORSVTKMDATAMODEL_TEMPLATE_EXPORT
# else
    /* Defer to the config module */
#   define VTKACCELERATORSVTKMDATAMODEL_TEMPLATE_EXPORT VTKACCELERATORSVTKMDATAMODEL_EXPORT
# endif
#endif


#endif // vtkConfigDataModel_h
