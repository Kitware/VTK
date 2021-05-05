// Copyright(C) 1999-2021 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#ifndef IOSS_Iotr_Scale_h
#define IOSS_Iotr_Scale_h

#include "vtk_ioss_mangle.h"

#include "Ioss_VariableType.h" // for VariableType
#include <Ioss_Transform.h>    // for Transform, Factory
#include <string>              // for string
namespace Ioss {
  class Field;
} // namespace Ioss

namespace Iotr {

  class Scale_Factory : public Factory
  {
  public:
    static const Scale_Factory *factory();

  private:
    Scale_Factory();
    Ioss::Transform *make(const std::string & /*unused*/) const override;
  };

  class Scale : public Ioss::Transform
  {
    friend class Scale_Factory;

  public:
    const Ioss::VariableType *output_storage(const Ioss::VariableType *in) const override;
    size_t                    output_count(size_t in) const override;

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

#endif // IOSS_Iotr_Scale_h
