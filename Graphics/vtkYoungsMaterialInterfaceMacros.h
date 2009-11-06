/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkYoungsMaterialInterfaceMacros.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .SECTION Thanks
// <verbatim>
//
// This file is part of the generalized Youngs material interface reconstruction algorithm contributed by
// CEA/DIF - Commissariat a l'Energie Atomique, Centre DAM Ile-De-France
// BP12, F-91297 Arpajon, France
// 
// Implementation by Thierry Carrard (CEA)
//
// </verbatim>

// .SECTION Caveats
// This file is for vtkYoungsMaterialInterface internal use only, it should never be included in other source files.

#ifndef __vtkYoungsMaterialInterfaceMacros_h
#define __vtkYoungsMaterialInterfaceMacros_h

#ifndef __CUDACC__ /* compiling with host compiler (gcc, icc, etc.) */

#ifndef FUNC_DECL
#define FUNC_DECL static inline
#endif

#ifndef KERNEL_DECL
#define KERNEL_DECL /* exported function */
#endif

#ifndef CONSTANT_DECL
#define CONSTANT_DECL static const
#endif

#ifndef REAL_PRECISION
#define REAL_PRECISION 64 /* defaults to 64 bits floating point */
#endif

#else /* compiling with cuda */

#ifndef FUNC_DECL
#define FUNC_DECL __device__
#endif

#ifndef KERNEL_DECL
#define KERNEL_DECL __global__
#endif

#ifndef CONSTANT_DECL
#define CONSTANT_DECL __constant__
#endif

#ifndef REAL_PRECISION
#define REAL_PRECISION 32 /* defaults to 32 bits floating point */
#endif

#endif /* __CUDACC__ */

#include "vtkYoungsMaterialInterfaceTypes.h"      /* definit REAL en float, double ou long double */
#include "vtkYoungsMaterialInterfaceVecMath.h"


#endif /* __vtkYoungsMaterialInterfaceMacros_h */


