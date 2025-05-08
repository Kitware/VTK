//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef viskores_thirdparty_diy_diy_h
#define viskores_thirdparty_diy_diy_h


#include "pre-include.h"
// clang-format off
#include VISKORES_DIY_INCLUDE(assigner.hpp)
#include VISKORES_DIY_INCLUDE(decomposition.hpp)
#include VISKORES_DIY_INCLUDE(master.hpp)
#include VISKORES_DIY_INCLUDE(mpi.hpp)
#include VISKORES_DIY_INCLUDE(partners/all-reduce.hpp)
#include VISKORES_DIY_INCLUDE(partners/broadcast.hpp)
#include VISKORES_DIY_INCLUDE(partners/swap.hpp)
#include VISKORES_DIY_INCLUDE(reduce.hpp)
#include VISKORES_DIY_INCLUDE(reduce-operations.hpp)
#include VISKORES_DIY_INCLUDE(resolve.hpp)
#include VISKORES_DIY_INCLUDE(serialization.hpp)
// clang-format on
#include "post-include.h"

#endif // viskores_thirdparty_diy_diy_h
