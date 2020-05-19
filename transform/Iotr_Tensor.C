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
#include <transform/Iotr_Tensor.h>

#include "Ioss_Transform.h"

namespace Iotr {

  const Tensor_Factory *Tensor_Factory::factory()
  {
    static Tensor_Factory registerThis;
    return &registerThis;
  }

  Tensor_Factory::Tensor_Factory() : Factory("generic_tensor")
  {
    Factory::alias("generic_tensor", "trace");      // scalar
    Factory::alias("generic_tensor", "deviator");   // tensor
    Factory::alias("generic_tensor", "spherical");  // tensor
    Factory::alias("generic_tensor", "invariants"); // vector
    Factory::alias("generic_tensor", "invariant1"); // scalar
    Factory::alias("generic_tensor", "invariant2"); // scalar
    Factory::alias("generic_tensor", "invariant3"); // scalar
    Factory::alias("generic_tensor", "magnitude");  // scalar
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

  int Tensor::output_count(int in) const
  {
    // Does not modify the entity count...
    return in;
  }

  bool Tensor::internal_execute(const Ioss::Field &field, void *data)
  {
    assert(field.get_type() == Ioss::Field::REAL);
    auto *r = static_cast<double *>(data);

    int count      = field.raw_count();
    int components = field.raw_storage()->component_count();

    bool success = false;
    switch (type_) {
    case TRACE:
    case INVARIANT1: {
      int j = 0;
      for (int i = 0; i < count * components; i += components) {
        r[j++] = r[i] + r[i + 1] + r[i + 2];
      }
    }
      success = true;
      break;
    case INVARIANT2: {
      int j = 0;
      for (int i = 0; i < count * components; i += components) {
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
