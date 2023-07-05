// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkmConfigCore_h
#define vtkmConfigCore_h

/*--------------------------------------------------------------------------*/
/* Other Configuration Options                                              */

#include <vtkm/internal/Configure.h>
#include "vtkAcceleratorsVTKmCoreModule.h" //required for correct implementation


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


#ifndef VTKACCELERATORSVTKMCORE_TEMPLATE_EXPORT
# if !defined(VTKACCELERATORSVTKMCORE_STATIC_DEFINE) && defined(_MSC_VER)
  /* Warning C4910 on windows state that extern explicit template can't be
     labeled with __declspec(dllexport). So that is why we use a new custom
     define. But when other modules ( e.g. rendering ) include this header
     we need them to see that the extern template is actually being imported.
  */
    /* We are building this library with MSVC */
#   define VTKACCELERATORSVTKMCORE_TEMPLATE_EXPORT
# else
    /* Defer to the config module */
#   define VTKACCELERATORSVTKMCORE_TEMPLATE_EXPORT VTKACCELERATORSVTKMCORE_EXPORT
# endif
#endif


#endif // vtkmConfigCore_h
