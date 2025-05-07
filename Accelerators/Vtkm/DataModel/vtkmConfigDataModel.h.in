// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkmConfigDataModel_h
#define vtkmConfigDataModel_h

/*--------------------------------------------------------------------------*/
/* Other Configuration Options                                              */

#include <viskores/internal/Configure.h>
#include "vtkAcceleratorsVTKmDataModelModule.h" //required for correct implementation


/*--------------------------------------------------------------------------*/
/* Make sure we use the same id's in VTK and Viskores                          */
#include "vtkType.h"
#ifdef VTK_USE_64BIT_IDS
# ifndef VISKORES_USE_64BIT_IDS
#  error VTK was defined with 64-bit ids but Viskores with 32-bit ids.
# endif
#else // !VTK_USE_64BIT_IDS
# ifdef VISKORES_USE_64BIT_IDS
#  error VTK was defined with 32-bit ids but Viskores with 64-bit ids.
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


#endif // vtkmConfigDataModel_h
