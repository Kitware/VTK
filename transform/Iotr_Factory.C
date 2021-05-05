// Copyright(C) 1999-2020 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include <Ioss_Transform.h>
#include <Ioss_Utils.h>
#include <cstddef>
#include <map>
#include <ostream>
#include <string>
#include <utility>

namespace Iotr {

  Ioss::Transform *Factory::create(const std::string &type)
  {
    Ioss::Transform *transform = nullptr;
    auto             iter      = registry()->find(type);
    if (iter == registry()->end()) {
      if (registry()->empty()) {
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
      Factory *factory = (*iter).second;
      transform        = factory->make(type);
    }
    return transform;
  }

  int Factory::describe(Ioss::NameList *names)
  {
    int                        count = 0;
    FactoryMap::const_iterator I;
    for (I = registry()->begin(); I != registry()->end(); ++I) {
      names->push_back((*I).first);
      count++;
    }
    return count;
  }

  Factory::Factory(const std::string &type) { registry()->insert(std::make_pair(type, this)); }

  void Factory::alias(const std::string &base, const std::string &syn)
  {
    Factory *factory = (*registry()->find(base)).second;
    registry()->insert(std::make_pair(syn, factory));
  }

  FactoryMap *Factory::registry()
  {
    static FactoryMap registry_;
    return &registry_;
  }

} // namespace Iotr
