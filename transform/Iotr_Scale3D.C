// Copyright(C) 1999-2017 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//
//     * Neither the name of NTESS nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <Ioss_Field.h>
#include <Ioss_VariableType.h>
#include <cassert>
#include <cstddef>
#include <string>
#include <transform/Iotr_Scale3D.h>
#include <vector>

#include "Ioss_Transform.h"

namespace Iotr {

  const Scale3D_Factory *Scale3D_Factory::factory()
  {
    static Scale3D_Factory registerThis;
    return &registerThis;
  }

  Scale3D_Factory::Scale3D_Factory() : Factory("scale3D")
  {
    Factory::alias("scale3D", "multiply3D");
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

  int Scale3D::output_count(int in) const
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
      int *idata = static_cast<int *>(data);

      for (size_t i = 0; i < count * 3; i += 3) {
        idata[i + 0] *= intScale[0];
        idata[i + 1] *= intScale[1];
        idata[i + 2] *= intScale[2];
      }
    }
    else if (field.get_type() == Ioss::Field::INT64) {
      int64_t *idata = static_cast<int64_t *>(data);

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
