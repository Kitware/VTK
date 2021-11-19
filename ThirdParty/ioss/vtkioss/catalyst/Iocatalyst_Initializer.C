// Copyright(C) 1999-2020 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include <catalyst/Iocatalyst_IOFactory.h>
#include <catalyst/Iocatalyst_Initializer.h>

namespace Iocatalyst {
  int Initializer::useCount = 0;

  /** \brief Initialize the adios database parts of the Ioss library.
   *
   *  Calls appropriate internal functions and methods to
   *  initialize the adios database parts of the Ioss library.
   */
  Initializer::Initializer()
  {
    if (useCount == 0)
      Iocatalyst::IOFactory::factory();
    useCount++;
  }

  Initializer::~Initializer()
  {
    try {
      // Put code here that should run after sierra is finished
      // executing...
      useCount--;
    }
    catch (...) {
    }
  }
} // namespace Iocatalyst
