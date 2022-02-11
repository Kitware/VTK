// Copyright(C) 1999-2020 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_DBUsage.h"                   // for DatabaseUsage
#include "Ioss_IOFactory.h"                 // for IOFactory
#include <catalyst/Iocatalyst_DatabaseIO.h> // for DatabaseIO
#include <catalyst/Iocatalyst_IOFactory.h>
#include <stddef.h> // for nullptr
#include <string>   // for string

#include <catalyst.hpp>
#include "vtk_fmt.h"
#include VTK_FMT(fmt/ostream.h)

namespace Ioss {
  class PropertyManager;
}

namespace Iocatalyst {

  const IOFactory *IOFactory::factory()
  {
    static IOFactory registerThis;
    return &registerThis;
  }

  IOFactory::IOFactory() : Ioss::IOFactory("catalyst")
  {
    Ioss::IOFactory::alias("catalyst", "catalyst2");
    Ioss::IOFactory::alias("catalyst", "catalyst_conduit");
  }

  Ioss::DatabaseIO *IOFactory::make_IO(const std::string &filename, Ioss::DatabaseUsage db_usage,
                                       MPI_Comm                     communicator,
                                       const Ioss::PropertyManager &properties) const
  {
    return new DatabaseIO(nullptr, filename, db_usage, communicator, properties);
  }

  std::string IOFactory::show_config() const
  {
    std::stringstream config;
    fmt::print(config, "\tCatalyst Library Version: {}\n", CATALYST_VERSION);
    fmt::print(config, "\t\tCatalyst ABI Version: {}\n", CATALYST_ABI_VERSION);

    conduit_cpp::Node node;
    catalyst_about(conduit_cpp::c_node(&node));
    auto implementation = node.has_path("catalyst/implementation") ?
      node["catalyst/implementation"].as_string() : std::string("stub");
    fmt::print(config, "\t\tImplementation: {}\n\n", implementation.c_str());
    return config.str();
  }
} // namespace Iocatalyst
