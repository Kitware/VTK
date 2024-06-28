// Copyright(C) 1999-2021, 2023, 2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_Field.h"
#include "Ioss_VariableType.h"
#include "transform/Iotr_Scale.h"
#include <cstddef>
#include <stdint.h>

#include "Ioss_Transform.h"
#include "Ioss_TransformFactory.h"

namespace Iotr {

  const Scale_Factory *Scale_Factory::factory()
  {
    static Scale_Factory registerThis;
    return &registerThis;
  }

  Scale_Factory::Scale_Factory() : Ioss::TransformFactory("scale")
  {
    Ioss::TransformFactory::alias("scale", "multiply");
  }

  Ioss::Transform *Scale_Factory::make(const std::string & /*unused*/) const { return new Scale(); }

  Scale::Scale() = default;

  void Scale::set_property(const std::string & /*name*/, int value) { intMultiplier = value; }

  void Scale::set_property(const std::string & /*name*/, double value) { realMultiplier = value; }

  const Ioss::VariableType *Scale::output_storage(const Ioss::VariableType *in) const { return in; }

  size_t Scale::output_count(size_t in) const
  {
    // Does not modify the entity count...
    return in;
  }

  bool Scale::internal_execute(const Ioss::Field &field, void *data)
  {
    size_t count      = field.transformed_count();
    int    components = field.transformed_storage()->component_count();

    if (field.get_type() == Ioss::Field::REAL) {
      auto *rdata = static_cast<double *>(data);

      for (size_t i = 0; i < count * components; i++) {
        rdata[i] *= realMultiplier;
      }
    }
    else if (field.get_type() == Ioss::Field::INTEGER) {
      auto *idata = static_cast<int *>(data);

      for (size_t i = 0; i < count * components; i++) {
        idata[i] *= intMultiplier;
      }
    }
    else if (field.get_type() == Ioss::Field::INT64) {
      auto *idata = static_cast<int64_t *>(data);

      for (size_t i = 0; i < count * components; i++) {
        idata[i] *= intMultiplier;
      }
    }
    else {
    }
    return true;
  }
} // namespace Iotr
