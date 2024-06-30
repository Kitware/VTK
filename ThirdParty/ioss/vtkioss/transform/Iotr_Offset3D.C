// Copyright(C) 1999-2021, 2023, 2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_Field.h"
#include "Ioss_VariableType.h"
#include "transform/Iotr_Offset3D.h"
#include <cassert>
#include <cstddef>
#include <stdint.h>
#include <vector>

#include "Ioss_Transform.h"
#include "Ioss_TransformFactory.h"

namespace Iotr {

  const Offset3D_Factory *Offset3D_Factory::factory()
  {
    static Offset3D_Factory registerThis;
    return &registerThis;
  }

  Offset3D_Factory::Offset3D_Factory() : Ioss::TransformFactory("offset3D")
  {
    Ioss::TransformFactory::alias("offset3D", "add3D");
  }

  Ioss::Transform *Offset3D_Factory::make(const std::string & /*unused*/) const
  {
    return new Offset3D();
  }

  Offset3D::Offset3D()
  {
    intOffset[0] = intOffset[1] = intOffset[2] = 0;
    realOffset[0] = realOffset[1] = realOffset[2] = 0.0;
  }

  void Offset3D::set_properties(const std::string & /*name*/, const std::vector<int> &values)
  {
    assert(values.size() == 3);
    intOffset[0] = values[0];
    intOffset[1] = values[1];
    intOffset[2] = values[2];
  }

  void Offset3D::set_properties(const std::string & /*name*/, const std::vector<double> &values)
  {
    assert(values.size() == 3);
    realOffset[0] = values[0];
    realOffset[1] = values[1];
    realOffset[2] = values[2];
  }

  const Ioss::VariableType *Offset3D::output_storage(const Ioss::VariableType *in) const
  {
    return in;
  }

  size_t Offset3D::output_count(size_t in) const
  {
    // Does not modify the entity count...
    return in;
  }

  bool Offset3D::internal_execute(const Ioss::Field &field, void *data)
  {
    size_t count = field.transformed_count();
    assert(field.transformed_storage()->component_count() == 3);

    if (field.get_type() == Ioss::Field::REAL) {
      auto *rdata = static_cast<double *>(data);

      for (size_t i = 0; i < count * 3; i += 3) {
        rdata[i + 0] += realOffset[0];
        rdata[i + 1] += realOffset[1];
        rdata[i + 2] += realOffset[2];
      }
    }
    else if (field.get_type() == Ioss::Field::INTEGER) {
      auto *idata = static_cast<int *>(data);

      for (size_t i = 0; i < count * 3; i += 3) {
        idata[i + 0] += intOffset[0];
        idata[i + 1] += intOffset[1];
        idata[i + 2] += intOffset[2];
      }
    }
    else if (field.get_type() == Ioss::Field::INT64) {
      auto *idata = static_cast<int64_t *>(data);

      for (size_t i = 0; i < count * 3; i += 3) {
        idata[i + 0] += intOffset[0];
        idata[i + 1] += intOffset[1];
        idata[i + 2] += intOffset[2];
      }
    }
    else {
    }
    return true;
  }
} // namespace Iotr
