//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
//This header can be used by external application that are consuming Viskores
//to define if Viskores should be set to use 64bit data types. If you need to
//customize more of the viskores type system, or what Device Adapters
//need to be included look at viskores/internal/Configure.h for all defines that
//you can over-ride.
#ifdef viskores_internal_Configure_h
#error Incorrect header order. Include this header before any other Viskores headers.
#endif

#ifndef viskores_internal_Configure32_h
#define viskores_internal_Configure32_h

#define VISKORES_USE_DOUBLE_PRECISION
#define VISKORES_USE_64BIT_IDS

#include <viskores/internal/Configure.h>

#endif
