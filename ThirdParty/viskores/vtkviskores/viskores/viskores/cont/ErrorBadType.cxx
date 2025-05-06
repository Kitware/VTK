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

#include <viskores/cont/ErrorBadType.h>

#include <string>

namespace viskores
{
namespace cont
{

void throwFailedDynamicCast(const std::string& baseType, const std::string& derivedType)
{ //Should we support typeid() instead of className?

  const std::string msg = "Cast failed: " + baseType + " --> " + derivedType;
  throw viskores::cont::ErrorBadType(msg);
}
}
}
