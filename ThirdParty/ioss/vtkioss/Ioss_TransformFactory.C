// Copyright(C) 1999-2021, 2024, 2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_TransformFactory.h"
#include "Ioss_Utils.h"
#include <map>
#include <ostream>
#include <string>

#include "Ioss_CodeTypes.h"

namespace Ioss {
  class Transform;

  Ioss::Transform *TransformFactory::create(const std::string &type)
  {
    Ioss::Transform *transform = nullptr;
    auto             iter      = registry().find(type);
    if (iter == registry().end()) {
      if (registry().empty()) {
        std::ostringstream errmsg;
        errmsg << "ERROR: No transformations have been registered.\n"
               << "       Was Iotr::Initializer::initialize() called?\n\n";
        IOSS_ERROR(errmsg);
      }
      else {
        std::ostringstream errmsg;
        errmsg << "ERROR: The transform named '" << type << "' is not supported.\n";
        IOSS_ERROR(errmsg);
      }
    }
    else {
      TransformFactory *factory = (*iter).second;
      transform                 = factory->make(type);
    }
    return transform;
  }

  Ioss::NameList TransformFactory::describe()
  {
    Ioss::NameList names;
    describe(&names);
    return names;
  }

  int TransformFactory::describe(Ioss::NameList *names)
  {
    int count = 0;
    for (const auto &entry : registry()) {
      names->push_back(entry.first);
      count++;
    }
    return count;
  }

  TransformFactory::TransformFactory(const std::string &type)
  {
    registry().insert(std::make_pair(type, this));
  }

  void TransformFactory::alias(const std::string &base, const std::string &syn)
  {
    TransformFactory *factory = (*registry().find(base)).second;
    registry().insert(std::make_pair(syn, factory));
  }

  TransformFactoryMap &TransformFactory::registry()
  {
    static TransformFactoryMap registry_;
    return registry_;
  }

} // namespace Ioss
