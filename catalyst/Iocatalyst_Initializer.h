// Copyright(C) 1999-2020, 2022 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "iocatalyst_export.h"

namespace Iocatalyst {
  /** \brief Initialization of the Catalyst database parts of the Ioss library.
   *
   *  If any input or output type is catalyst, catalyst2 or catalyst_conduit,
   *  then an object of this type must be created before using any other functions
   *  or methods in the Ioss library except Ioss::Init::Initializer().
   */
  class IOCATALYST_EXPORT Initializer
  {
  public:
    Initializer();
    ~Initializer();
    // Copy constructor
    // Assignment operator
  private:
    static int useCount;
  };
} // namespace Iocatalyst
