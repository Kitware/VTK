// Copyright(C) 1999-2020 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#ifndef IOSS_Iocgns_Initializer_h
#define IOSS_Iocgns_Initializer_h

namespace Iocgns {
  class Initializer
  {
  public:
    Initializer();
    ~Initializer();
    // Copy constructor
    // Assignment operator
  private:
    static int useCount;
  };
} // namespace Iocgns
#endif
