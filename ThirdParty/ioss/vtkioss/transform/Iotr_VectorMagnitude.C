// Copyright(C) 1999-2021, 2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_Field.h"
#include "Ioss_VariableType.h"
#include "transform/Iotr_VectorMagnitude.h"
#include <cmath>
#include <cstddef>

#include "Ioss_Transform.h"
#include "Ioss_TransformFactory.h"

namespace Iotr {

  const VM_Factory *VM_Factory::factory()
  {
    static VM_Factory registerThis;
    return &registerThis;
  }

  VM_Factory::VM_Factory() : Ioss::TransformFactory("vector magnitude")
  {
    Ioss::TransformFactory::alias("vector magnitude", "length");
  }

  Ioss::Transform *VM_Factory::make(const std::string & /*unused*/) const
  {
    return new VectorMagnitude();
  }

  VectorMagnitude::VectorMagnitude() = default;

  const Ioss::VariableType *VectorMagnitude::output_storage(const Ioss::VariableType *in) const
  {
    static const Ioss::VariableType *v2d = Ioss::VariableType::factory("vector_2d");
    static const Ioss::VariableType *v3d = Ioss::VariableType::factory("vector_3d");
    static const Ioss::VariableType *sca = Ioss::VariableType::factory("scalar");
    if (in == v2d || in == v3d) {
      return sca;
    }
    return nullptr;
  }

  size_t VectorMagnitude::output_count(size_t in) const
  {
    // Does not modify the entity count...
    return in;
  }

  bool VectorMagnitude::internal_execute(const Ioss::Field &field, void *data)
  {
    auto *rdata = static_cast<double *>(data);

    size_t count = field.transformed_count();
    if (field.transformed_storage()->component_count() == 3) {
      int j = 0;
      for (size_t i = 0; i < count; i++) {
        rdata[i] = std::sqrt(rdata[j] * rdata[j] + rdata[j + 1] * rdata[j + 1] +
                             rdata[j + 2] * rdata[j + 2]);
        j += 3;
      }
    }
    else {
      int j = 0;
      for (size_t i = 0; i < count; i++) {
        rdata[i] = std::sqrt(rdata[j] * rdata[j] + rdata[j + 1] * rdata[j + 1]);
        j += 2;
      }
    }
    return true;
  }
} // namespace Iotr
