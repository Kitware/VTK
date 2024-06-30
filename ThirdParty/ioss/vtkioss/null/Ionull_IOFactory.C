// Copyright(C) 2023 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "null/Ionull_DatabaseIO.h"
#include "null/Ionull_IOFactory.h"

#include "Ioss_CodeTypes.h" // for Ioss_MPI_Comm
#include "Ioss_DBUsage.h"   // for DatabaseUsage
#include "Ioss_IOFactory.h" // for IOFactory

namespace Ionull {

  const IOFactory *IOFactory::factory()
  {
    static IOFactory registerThis;
    return &registerThis;
  }

  IOFactory::IOFactory() : Ioss::IOFactory("null") {}

  Ioss::DatabaseIO *IOFactory::make_IO(const std::string &filename, Ioss::DatabaseUsage db_usage,
                                       Ioss_MPI_Comm                communicator,
                                       const Ioss::PropertyManager &properties) const
  {
    return new Ionull::DatabaseIO(nullptr, filename, db_usage, communicator, properties);
  }
} // namespace Ionull
