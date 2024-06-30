// Copyright(C) 1999-2021, 2023, 2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_Field.h"
#include "Ioss_VariableType.h"
#include "transform/Iotr_Offset.h"
#include <cstddef>
#include <stdint.h>

#include "Ioss_Transform.h"
#include "Ioss_TransformFactory.h"

namespace Iotr {

  const Offset_Factory *Offset_Factory::factory()
  {
    static Offset_Factory registerThis;
    return &registerThis;
  }

  Offset_Factory::Offset_Factory() : Ioss::TransformFactory("offset")
  {
    Ioss::TransformFactory::alias("offset", "add");
  }

  Ioss::Transform *Offset_Factory::make(const std::string & /*unused*/) const
  {
    return new Offset();
  }

  Offset::Offset() = default;

  void Offset::set_property(const std::string & /*name*/, int value) { intOffset = value; }

  void Offset::set_property(const std::string & /*name*/, double value) { realOffset = value; }

  const Ioss::VariableType *Offset::output_storage(const Ioss::VariableType *in) const
  {
    return in;
  }

  size_t Offset::output_count(size_t in) const
  {
    // Does not modify the entity count...
    return in;
  }

  bool Offset::internal_execute(const Ioss::Field &field, void *data)
  {
    size_t count      = field.transformed_count();
    int    components = field.transformed_storage()->component_count();

    if (field.get_type() == Ioss::Field::REAL) {
      auto *rdata = static_cast<double *>(data);

      for (size_t i = 0; i < count * components; i++) {
        rdata[i] += realOffset;
      }
    }
    else if (field.get_type() == Ioss::Field::INTEGER) {
      auto *idata = static_cast<int *>(data);

      for (size_t i = 0; i < count * components; i++) {
        idata[i] += intOffset;
      }
    }
    else if (field.get_type() == Ioss::Field::INT64) {
      auto *idata = static_cast<int64_t *>(data);

      for (size_t i = 0; i < count * components; i++) {
        idata[i] += intOffset;
      }
    }
    else {
    }
    return true;
  }
} // namespace Iotr
