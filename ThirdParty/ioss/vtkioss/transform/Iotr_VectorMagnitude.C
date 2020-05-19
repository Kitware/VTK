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
#include <cmath>
#include <cstddef>
#include <string>
#include <transform/Iotr_VectorMagnitude.h>

#include "Ioss_Transform.h"

namespace Iotr {

  const VM_Factory *VM_Factory::factory()
  {
    static VM_Factory registerThis;
    return &registerThis;
  }

  VM_Factory::VM_Factory() : Factory("vector magnitude")
  {
    Factory::alias("vector magnitude", "length");
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

  int VectorMagnitude::output_count(int in) const
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
