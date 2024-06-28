// Copyright(C) 1999-2022, 2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "Ioss_Transform.h"
#include "Ioss_TransformFactory.h"
#include "Ioss_VariableType.h"
#include <stddef.h>
#include <string>

#include "iotr_export.h"
#include "vtk_ioss_mangle.h"

namespace Ioss {
  class Field;
} // namespace Ioss

namespace Iotr {

  class IOTR_EXPORT Scale_Factory : public Ioss::TransformFactory
  {
  public:
    static const Scale_Factory *factory();

  private:
    Scale_Factory();
    IOSS_NODISCARD Ioss::Transform *make(const std::string & /*unused*/) const override;
  };

  class IOTR_EXPORT Scale : public Ioss::Transform
  {
    friend class Scale_Factory;

  public:
    IOSS_NODISCARD const  Ioss::VariableType                       *
    output_storage(const Ioss::VariableType *in) const override;
    IOSS_NODISCARD size_t output_count(size_t in) const override;

    void set_property(const std::string &name, int value) override;
    void set_property(const std::string &name, double value) override;

  protected:
    Scale();

    bool internal_execute(const Ioss::Field &field, void *data) override;

  private:
    int    intMultiplier{1};
    double realMultiplier{1.0};
  };
} // namespace Iotr
