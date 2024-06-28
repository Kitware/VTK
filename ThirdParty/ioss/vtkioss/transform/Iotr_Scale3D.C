// Copyright(C) 1999-2021, 2023, 2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_Field.h"
#include "Ioss_VariableType.h"
#include "transform/Iotr_Scale3D.h"
#include <cassert>
#include <cstddef>
#include <stdint.h>
#include <vector>

#include "Ioss_Transform.h"
#include "Ioss_TransformFactory.h"

namespace Iotr {

  const Scale3D_Factory *Scale3D_Factory::factory()
  {
    static Scale3D_Factory registerThis;
    return &registerThis;
  }

  Scale3D_Factory::Scale3D_Factory() : Ioss::TransformFactory("scale3D")
  {
    Ioss::TransformFactory::alias("scale3D", "multiply3D");
  }

  Ioss::Transform *Scale3D_Factory::make(const std::string & /*unused*/) const
  {
    return new Scale3D();
  }

  Scale3D::Scale3D()
  {
    intScale[0] = intScale[1] = intScale[2] = 1;
    realScale[0] = realScale[1] = realScale[2] = 1.0;
  }

  void Scale3D::set_properties(const std::string & /*name*/, const std::vector<int> &values)
  {
    assert(values.size() == 3);
    intScale[0] = values[0];
    intScale[1] = values[1];
    intScale[2] = values[2];
  }

  void Scale3D::set_properties(const std::string & /*name*/, const std::vector<double> &values)
  {
    assert(values.size() == 3);
    realScale[0] = values[0];
    realScale[1] = values[1];
    realScale[2] = values[2];
  }

  const Ioss::VariableType *Scale3D::output_storage(const Ioss::VariableType *in) const
  {
    return in;
  }

  size_t Scale3D::output_count(size_t in) const
  {
    // Does not modify the entity count...
    return in;
  }

  bool Scale3D::internal_execute(const Ioss::Field &field, void *data)
  {
    size_t count = field.transformed_count();
    assert(field.transformed_storage()->component_count() == 3);

    if (field.get_type() == Ioss::Field::REAL) {
      auto *rdata = static_cast<double *>(data);

      for (size_t i = 0; i < count * 3; i += 3) {
        rdata[i + 0] *= realScale[0];
        rdata[i + 1] *= realScale[1];
        rdata[i + 2] *= realScale[2];
      }
    }
    else if (field.get_type() == Ioss::Field::INTEGER) {
      auto *idata = static_cast<int *>(data);

      for (size_t i = 0; i < count * 3; i += 3) {
        idata[i + 0] *= intScale[0];
        idata[i + 1] *= intScale[1];
        idata[i + 2] *= intScale[2];
      }
    }
    else if (field.get_type() == Ioss::Field::INT64) {
      auto *idata = static_cast<int64_t *>(data);

      for (size_t i = 0; i < count * 3; i += 3) {
        idata[i + 0] *= intScale[0];
        idata[i + 1] *= intScale[1];
        idata[i + 2] *= intScale[2];
      }
    }
    else {
    }
    return true;
  }
} // namespace Iotr
