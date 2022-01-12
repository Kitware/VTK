// Copyright(C) 1999-2022 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "vtk_ioss_mangle.h"

#include <Ioss_CodeTypes.h>
#include <functional> // for less
#include <map>        // for map, map<>::value_compare
#include <string>     // for string
#include <vector>     // for vector

namespace Ioss {
  class Field;
  class VariableType;

  class Transform
  {
  public:
    virtual ~Transform();
    virtual const Ioss::VariableType *output_storage(const Ioss::VariableType *in) const = 0;
    virtual size_t                    output_count(size_t in) const                      = 0;

    bool execute(const Ioss::Field &field, void *data);

    virtual void set_property(const std::string &name, int value);
    virtual void set_property(const std::string &name, double value);
    virtual void set_properties(const std::string &name, const std::vector<int> &values);
    virtual void set_properties(const std::string &name, const std::vector<double> &values);

  protected:
    Transform();

    virtual bool internal_execute(const Ioss::Field &field, void *data) = 0;
  };
} // namespace Ioss

namespace Iotr {
  class Factory;
  using FactoryMap = std::map<std::string, Factory *, std::less<std::string>>;

  class Factory
  {
  public:
    virtual ~Factory() = default;
    static Ioss::Transform *create(const std::string &type);

    static int            describe(Ioss::NameList *names);
    static Ioss::NameList describe();

  protected:
    explicit Factory(const std::string &type);
    virtual Ioss::Transform *make(const std::string &) const = 0;
    static void              alias(const std::string &base, const std::string &syn);

  private:
    static FactoryMap &registry();
  };
} // namespace Iotr
