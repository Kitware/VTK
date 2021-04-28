//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#ifndef fides_keys_Keys_H_
#define fides_keys_Keys_H_

#include "fides_export.h"

#include <cstdint>

namespace fides
{
namespace keys
{

using KeyType = std::uintptr_t;

/// Key used for storing number of blocks meta-data.
/// Uses fides::metadata::Size
FIDES_EXPORT KeyType NUMBER_OF_BLOCKS();

/// Key used for storing number of steps meta-data.
/// Uses fides::metadata::Size
FIDES_EXPORT KeyType NUMBER_OF_STEPS();

/// Key used for selecting a set of blocks. Uses
/// fides::metadata::Vector<size_t>
FIDES_EXPORT KeyType BLOCK_SELECTION();

/// Key used for available array meta-data and array
/// selection. Uses fides::metadata::Vector<fides::metadata::FieldInformation>
FIDES_EXPORT KeyType FIELDS();

/// Key used for selecting time step.
/// Uses fides::metadata::Index
FIDES_EXPORT KeyType STEP_SELECTION();

/// Key used for selecting planes for XGC data.
/// Should only be used internally.
/// Uses fides::metadata::Set
FIDES_EXPORT KeyType PLANE_SELECTION();

FIDES_EXPORT KeyType READ_AS_MULTIBLOCK();

//Special namespace for fusion related keys.
namespace fusion
{
/// Key used for specifying planes to be inserted
/// for GTC and XGC data.
/// Uses fides::metadata::Size
FIDES_EXPORT KeyType PLANE_INSERTION();

/// Key used for specifying that the Radius field should be added.
/// for GTC and XGC data.
/// Uses fides::metadata::Bool
FIDES_EXPORT KeyType ADD_R_FIELD();

/// Key used for specifying that the Phi field should be added.
/// for GTC and XGC data.
/// Uses fides::metadata::Bool
FIDES_EXPORT KeyType ADD_PHI_FIELD();

/// Key used for specifying that the Psi field should be added.
/// for GTC and XGC data.
/// Uses fides::metadata::Bool
FIDES_EXPORT KeyType ADD_PSI_FIELD();
}

}
}

#endif
