// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkAutoInit_h
#define vtkAutoInit_h

#include "vtkABINamespace.h"
#include "vtkDebugLeaksManager.h" // DebugLeaks exists longer.
#include "vtkTimeStamp.h"         // Here so that TimeStamp Schwarz initializer works

#define VTK_MODULE_AUTOINIT VTK_AUTOINIT

#define VTK_AUTOINIT(M) VTK_AUTOINIT0(M, M##_AUTOINIT)
#define VTK_AUTOINIT0(M, T) VTK_AUTOINIT1(M, T)
#define VTK_AUTOINIT1(M, T)                                                                        \
  /* Declare every <mod>_AutoInit_Construct function.  */                                          \
  VTK_AUTOINIT_DECLARE_##T namespace                                                               \
  {                                                                                                \
    static struct M##_AutoInit                                                                     \
    {                                                                                              \
      /* Call every <mod>_AutoInit_Construct during initialization.  */                            \
      M##_AutoInit()                                                                               \
      {                                                                                            \
        VTK_AUTOINIT_CONSTRUCT_##T                                                                 \
      }                                                                                            \
    } M##_AutoInit_Instance; /* NOLINT(misc-definitions-in-headers) */                             \
  }

#define VTK_AUTOINIT_DECLARE_0()
#define VTK_AUTOINIT_DECLARE_1(t1) VTK_AUTOINIT_DECLARE_0() VTK_AUTOINIT_DECLARE(t1)
#define VTK_AUTOINIT_DECLARE_2(t1, t2) VTK_AUTOINIT_DECLARE_1(t1) VTK_AUTOINIT_DECLARE(t2)
#define VTK_AUTOINIT_DECLARE_3(t1, t2, t3) VTK_AUTOINIT_DECLARE_2(t1, t2) VTK_AUTOINIT_DECLARE(t3)
#define VTK_AUTOINIT_DECLARE_4(t1, t2, t3, t4)                                                     \
  VTK_AUTOINIT_DECLARE_3(t1, t2, t3) VTK_AUTOINIT_DECLARE(t4)
#define VTK_AUTOINIT_DECLARE_5(t1, t2, t3, t4, t5)                                                 \
  VTK_AUTOINIT_DECLARE_4(t1, t2, t3, t4) VTK_AUTOINIT_DECLARE(t5)
#define VTK_AUTOINIT_DECLARE_6(t1, t2, t3, t4, t5, t6)                                             \
  VTK_AUTOINIT_DECLARE_5(t1, t2, t3, t4, t5) VTK_AUTOINIT_DECLARE(t6)
#define VTK_AUTOINIT_DECLARE_7(t1, t2, t3, t4, t5, t6, t7)                                         \
  VTK_AUTOINIT_DECLARE_6(t1, t2, t3, t4, t5, t6) VTK_AUTOINIT_DECLARE(t7)
#define VTK_AUTOINIT_DECLARE_8(t1, t2, t3, t4, t5, t6, t7, t8)                                     \
  VTK_AUTOINIT_DECLARE_7(t1, t2, t3, t4, t5, t6, t7) VTK_AUTOINIT_DECLARE(t8)
#define VTK_AUTOINIT_DECLARE_9(t1, t2, t3, t4, t5, t6, t7, t8, t9)                                 \
  VTK_AUTOINIT_DECLARE_8(t1, t2, t3, t4, t5, t6, t7, t8) VTK_AUTOINIT_DECLARE(t9)
#define VTK_AUTOINIT_DECLARE(M)                                                                    \
  VTK_ABI_NAMESPACE_BEGIN void M##_AutoInit_Construct();                                           \
  VTK_ABI_NAMESPACE_END

#define VTK_AUTOINIT_CONSTRUCT_0()
#define VTK_AUTOINIT_CONSTRUCT_1(t1) VTK_AUTOINIT_CONSTRUCT_0() VTK_AUTOINIT_CONSTRUCT(t1)
#define VTK_AUTOINIT_CONSTRUCT_2(t1, t2) VTK_AUTOINIT_CONSTRUCT_1(t1) VTK_AUTOINIT_CONSTRUCT(t2)
#define VTK_AUTOINIT_CONSTRUCT_3(t1, t2, t3)                                                       \
  VTK_AUTOINIT_CONSTRUCT_2(t1, t2) VTK_AUTOINIT_CONSTRUCT(t3)
#define VTK_AUTOINIT_CONSTRUCT_4(t1, t2, t3, t4)                                                   \
  VTK_AUTOINIT_CONSTRUCT_3(t1, t2, t3) VTK_AUTOINIT_CONSTRUCT(t4)
#define VTK_AUTOINIT_CONSTRUCT_5(t1, t2, t3, t4, t5)                                               \
  VTK_AUTOINIT_CONSTRUCT_4(t1, t2, t3, t4) VTK_AUTOINIT_CONSTRUCT(t5)
#define VTK_AUTOINIT_CONSTRUCT_6(t1, t2, t3, t4, t5, t6)                                           \
  VTK_AUTOINIT_CONSTRUCT_5(t1, t2, t3, t4, t5) VTK_AUTOINIT_CONSTRUCT(t6)
#define VTK_AUTOINIT_CONSTRUCT_7(t1, t2, t3, t4, t5, t6, t7)                                       \
  VTK_AUTOINIT_CONSTRUCT_6(t1, t2, t3, t4, t5, t6) VTK_AUTOINIT_CONSTRUCT(t7)
#define VTK_AUTOINIT_CONSTRUCT_8(t1, t2, t3, t4, t5, t6, t7, t8)                                   \
  VTK_AUTOINIT_CONSTRUCT_7(t1, t2, t3, t4, t5, t6, t7) VTK_AUTOINIT_CONSTRUCT(t8)
#define VTK_AUTOINIT_CONSTRUCT_9(t1, t2, t3, t4, t5, t6, t7, t8, t9)                               \
  VTK_AUTOINIT_CONSTRUCT_8(t1, t2, t3, t4, t5, t6, t7, t8) VTK_AUTOINIT_CONSTRUCT(t9)
#define VTK_AUTOINIT_CONSTRUCT(M) M##_AutoInit_Construct();

// Description:
// Initialize the named module, ensuring its object factory is correctly
// registered. This call must be made in global scope in the
// translation unit of your executable (which can include a shared library, but
// will not work as expected in a static library).
//
// @code{.cpp}
// #include "vtkAutoInit.h"
// VTK_MODULE_INIT(vtkRenderingOpenGL2);
// @endcode
//
// The above snippet if included in the global scope will ensure the object
// factories for vtkRenderingOpenGL2 are correctly registered and unregistered.
#define VTK_MODULE_INIT(M)                                                                         \
  VTK_AUTOINIT_DECLARE(M)                                                                          \
  namespace                                                                                        \
  {                                                                                                \
  static struct M##_ModuleInit                                                                     \
  {                                                                                                \
    /* Call <mod>_AutoInit_Construct during initialization.  */                                    \
    M##_ModuleInit()                                                                               \
    {                                                                                              \
      VTK_AUTOINIT_CONSTRUCT(M)                                                                    \
    }                                                                                              \
  } M##_ModuleInit_Instance;                                                                       \
  }

#endif
// VTK-HeaderTest-Exclude: vtkAutoInit.h
