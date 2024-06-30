// Copyright(C) 1999-2020, 2022, 2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_GroupingEntity.h"
#include "Ioss_NullEntity.h"

namespace Ioss {
  NullEntity::NullEntity() : Ioss::GroupingEntity(nullptr, "null_entity", 0) {}
} // namespace Ioss
