// Copyright(C) 1999-2022 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "vtk_ioss_mangle.h"

#include "Ioss_VariableType.h" // for VariableType
#include <Ioss_Transform.h>    // for Transform, Factory
#include <string>              // for string
namespace Ioss {
  class Field;
} // namespace Ioss

namespace Iotr {

  class Tensor_Factory : public Factory
  {
  public:
    static const Tensor_Factory *factory();

  private:
    Tensor_Factory();
    Ioss::Transform *make(const std::string &type) const override;
  };

  class Tensor : public Ioss::Transform
  {
    friend class Tensor_Factory;
    enum TranType {
      INVALID,
      TRACE,
      SPHERICAL,
      DEVIATOR,
      MAGNITUDE,
      INVARIANTS,
      INVARIANT1,
      INVARIANT2,
      INVARIANT3
    };

  public:
    const Ioss::VariableType *output_storage(const Ioss::VariableType *in) const override;
    size_t                    output_count(size_t in) const override;

  protected:
    explicit Tensor(const std::string &type);

    bool internal_execute(const Ioss::Field &field, void *data) override;

  private:
    TranType type_;
  };
} // namespace Iotr
