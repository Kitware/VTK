// Copyright(C) 1999-2021 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#ifndef IOSS_Iotr_MinMax_h
#define IOSS_Iotr_MinMax_h

#include "vtk_ioss_mangle.h"

#include "Ioss_VariableType.h" // for VariableType
#include <Ioss_Transform.h>    // for Transform, Factory
#include <string>              // for string
namespace Ioss {
  class Field;
} // namespace Ioss

namespace Iotr {

  class MinMax_Factory : public Factory
  {
  public:
    static const MinMax_Factory *factory();

  private:
    MinMax_Factory();
    Ioss::Transform *make(const std::string &type) const override;
  };

  class MinMax : public Ioss::Transform
  {
    friend class MinMax_Factory;

  public:
    const Ioss::VariableType *output_storage(const Ioss::VariableType *in) const override;
    size_t                    output_count(size_t in) const override;

  protected:
    explicit MinMax(const std::string &type);

    bool internal_execute(const Ioss::Field &field, void *data) override;

  private:
    bool doMin{false};
    bool doAbs{false};
  };
} // namespace Iotr

#endif // IOSS_Iotr_MinMax_h
