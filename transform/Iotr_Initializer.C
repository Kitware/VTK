// Copyright(C) 1999-2020 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "transform/Iotr_Initializer.h"
#include "transform/Iotr_MinMax.h"
#include "transform/Iotr_Offset.h"
#include "transform/Iotr_Offset3D.h"
#include "transform/Iotr_Scale.h"
#include "transform/Iotr_Scale3D.h"
#include "transform/Iotr_Tensor.h"
#include "transform/Iotr_VectorMagnitude.h"

namespace Iotr {
  Initializer::Initializer()
  {
    MinMax_Factory::factory();
    Offset_Factory::factory();
    Scale_Factory::factory();
    Offset3D_Factory::factory();
    Scale3D_Factory::factory();
    Tensor_Factory::factory();
    VM_Factory::factory();
  }
} // namespace Iotr
