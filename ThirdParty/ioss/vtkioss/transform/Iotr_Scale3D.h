// Copyright(C) 1999-2021 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#ifndef IOSS_Iotr_Scale3D_h
#define IOSS_Iotr_Scale3D_h

#include "vtk_ioss_mangle.h"

#include "Ioss_VariableType.h" // for VariableType
#include <Ioss_Transform.h>    // for Transform, Factory
#include <string>              // for string
#include <vector>              // for vector
namespace Ioss {
  class Field;
} // namespace Ioss

namespace Ioss {
} // namespace Ioss

namespace Iotr {

  class Scale3D_Factory : public Factory
  {
  public:
    static const Scale3D_Factory *factory();

  private:
    Scale3D_Factory();
    Ioss::Transform *make(const std::string & /*unused*/) const override;
  };

  class Scale3D : public Ioss::Transform
  {
    friend class Scale3D_Factory;

  public:
    const Ioss::VariableType *output_storage(const Ioss::VariableType *in) const override;
    size_t                    output_count(size_t in) const override;

    void set_properties(const std::string &name, const std::vector<int> &values) override;
    void set_properties(const std::string &name, const std::vector<double> &values) override;

  protected:
    Scale3D();

    bool internal_execute(const Ioss::Field &field, void *data) override;

  private:
    int    intScale[3]{};
    double realScale[3]{};
  };
} // namespace Iotr

#endif // IOSS_Iotr_Scale3D_h
