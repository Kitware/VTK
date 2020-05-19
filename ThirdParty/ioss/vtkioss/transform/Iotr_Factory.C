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
