// Copyright(C) 1999-2021, 2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_Field.h"
#include "Ioss_VariableType.h"
#include "transform/Iotr_Tensor.h"
#include <cassert>
#include <cstddef>
#include <string>

#include "Ioss_Transform.h"
#include "Ioss_TransformFactory.h"

namespace Iotr {

  const Tensor_Factory *Tensor_Factory::factory()
  {
    static Tensor_Factory registerThis;
    return &registerThis;
  }

  Tensor_Factory::Tensor_Factory() : Ioss::TransformFactory("generic_tensor")
  {
    Ioss::TransformFactory::alias("generic_tensor", "trace");      // scalar
    Ioss::TransformFactory::alias("generic_tensor", "deviator");   // tensor
    Ioss::TransformFactory::alias("generic_tensor", "spherical");  // tensor
    Ioss::TransformFactory::alias("generic_tensor", "invariants"); // vector
    Ioss::TransformFactory::alias("generic_tensor", "invariant1"); // scalar
    Ioss::TransformFactory::alias("generic_tensor", "invariant2"); // scalar
    Ioss::TransformFactory::alias("generic_tensor", "invariant3"); // scalar
    Ioss::TransformFactory::alias("generic_tensor", "magnitude");  // scalar
  }

  Ioss::Transform *Tensor_Factory::make(const std::string &type) const { return new Tensor(type); }

  Tensor::Tensor(const std::string &type)
  {
    type_ = INVALID;

    if (type == "trace") {
      type_ = TRACE;
    }
    else if (type == "deviator") {
      type_ = DEVIATOR;
    }
    else if (type == "spherical") {
      type_ = SPHERICAL;
    }
    else if (type == "invariants") {
      type_ = INVARIANTS;
    }
    else if (type == "invariant1") {
      type_ = INVARIANT1;
    }
    else if (type == "invariant2") {
      type_ = INVARIANT2;
    }
    else if (type == "invariant3") {
      type_ = INVARIANT3;
    }
    else if (type == "magnitude") {
      type_ = MAGNITUDE;
    }
  }

  const Ioss::VariableType *Tensor::output_storage(const Ioss::VariableType *in) const
  {
    static const Ioss::VariableType *st33 = Ioss::VariableType::factory("sym_tensor_33");
    if (in != st33) {
      return nullptr;
    }

    switch (type_) {
    case INVARIANT1:
    case INVARIANT2:
    case INVARIANT3:
    case MAGNITUDE: return Ioss::VariableType::factory("scalar");
    case DEVIATOR:
    case SPHERICAL: return st33;
    case INVARIANTS: return Ioss::VariableType::factory("Real[3]");
    default: return nullptr;
    }
  }

  size_t Tensor::output_count(size_t in) const
  {
    // Does not modify the entity count...
    return in;
  }

  bool Tensor::internal_execute(const Ioss::Field &field, void *data)
  {
    assert(field.get_type() == Ioss::Field::REAL);
    auto *r = static_cast<double *>(data);

    auto count      = field.raw_count();
    auto components = field.raw_storage()->component_count();

    bool success = false;
    switch (type_) {
    case TRACE:
    case INVARIANT1: {
      size_t j = 0;
      for (size_t i = 0; i < count * components; i += components) {
        r[j++] = r[i] + r[i + 1] + r[i + 2];
      }
    }
      success = true;
      break;
    case INVARIANT2: {
      size_t j = 0;
      for (size_t i = 0; i < count * components; i += components) {
        r[j++] = r[i + 3] * r[i + 3] + r[i + 4] * r[i + 4] + r[i + 5] * r[i + 5] -
                 (r[i + 0] * r[i + 1] + r[i + 1] * r[i + 2] + r[i + 0] * r[i + 2]);
      }
    }
      success = true;
      break;
    case INVARIANT3:
    case MAGNITUDE:
    case DEVIATOR:
    case SPHERICAL:
    case INVARIANTS: success = false; break;
    default: success = false;
    }

    return success;
  }
} // namespace Iotr
