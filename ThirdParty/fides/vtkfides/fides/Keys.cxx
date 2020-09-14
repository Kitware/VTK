//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================
#include <fides/Keys.h>

namespace fides
{
namespace keys
{

KeyType NUMBER_OF_BLOCKS()
{
  return reinterpret_cast<KeyType>(&NUMBER_OF_BLOCKS);
}

KeyType NUMBER_OF_STEPS()
{
  return reinterpret_cast<KeyType>(&NUMBER_OF_STEPS);
}

KeyType BLOCK_SELECTION()
{
  return reinterpret_cast<KeyType>(&BLOCK_SELECTION);
}

KeyType FIELDS()
{
  return reinterpret_cast<KeyType>(&FIELDS);
}

KeyType STEP_SELECTION()
{
  return reinterpret_cast<KeyType>(&STEP_SELECTION);
}

KeyType PLANE_SELECTION()
{
  return reinterpret_cast<KeyType>(&PLANE_SELECTION);
}

}
}
