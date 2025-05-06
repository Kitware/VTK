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

#define viskores_cont_CellSetStructured_cxx
#include <viskores/cont/CellSetStructured.h>

namespace viskores
{
namespace cont
{

template class VISKORES_CONT_EXPORT CellSetStructured<1>;
template class VISKORES_CONT_EXPORT CellSetStructured<2>;
template class VISKORES_CONT_EXPORT CellSetStructured<3>;
}
}
