// Copyright(C) 1999-2020 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details
#include "cgns/Iocgns_IOFactory.h"
#include "cgns/Iocgns_Initializer.h"

namespace Iocgns {

  Initializer::Initializer() { Iocgns::IOFactory::factory(); }

  Initializer::~Initializer()
  {
    try {
    }
    catch (...) {
    }
  }
} // namespace Iocgns
