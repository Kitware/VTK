// Copyright(C) 1999-2020, 2022, 2023, 2023 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "cgns/Iocgns_DatabaseIO.h" // for DatabaseIO -- serial
#include "cgns/Iocgns_IOFactory.h"
#include "cgns/Iocgns_Utils.h"
#include <vtk_cgns.h>
#include VTK_CGNS(cgnstypes.h)
#include <string> // for string

#include "Ioss_CodeTypes.h"
#include "Ioss_DBUsage.h"   // for DatabaseUsage
#include "Ioss_IOFactory.h" // for IOFactory
#if CG_BUILD_PARALLEL
#include "cgns/Iocgns_ParallelDatabaseIO.h" // for DatabaseIO -- parallel
#endif

namespace Ioss {
  class PropertyManager;
} // namespace Ioss

#if CG_BUILD_PARALLEL
namespace {
  std::string check_decomposition_property(const Ioss::PropertyManager &properties,
                                           Ioss::DatabaseUsage          db_usage);
  bool        check_composition_property(const Ioss::PropertyManager &properties,
                                         Ioss::DatabaseUsage          db_usage);
} // namespace
#endif

namespace Iocgns {

  const IOFactory *IOFactory::factory()
  {
    static IOFactory registerThis;
    return &registerThis;
  }

  IOFactory::IOFactory() : Ioss::IOFactory("cgns")
  {
#if CG_BUILD_PARALLEL
    Ioss::IOFactory::alias("cgns", "dof_cgns");
    Ioss::IOFactory::alias("cgns", "par_cgns");
#endif
  }

  Ioss::DatabaseIO *IOFactory::make_IO(const std::string &filename, Ioss::DatabaseUsage db_usage,
                                       Ioss_MPI_Comm                communicator,
                                       const Ioss::PropertyManager &properties) const
  {
// The "cgns" and "parallel_cgns" databases can both be accessed from
// this factory.  The "parallel_cgns" is returned if being run on more
// than 1 processor unless the decomposition property is set and the
// value is "external" or the composition property is set with value "external"
#if CG_BUILD_PARALLEL
    Ioss::ParallelUtils pu(communicator);
    int                 proc_count = pu.parallel_size();

    bool decompose = false;

    if (proc_count > 1) {
      decompose = true; // Default to decompose instead of file-per-processor if parallel.
      if (db_usage == Ioss::READ_MODEL || db_usage == Ioss::READ_RESTART) {
        std::string method = check_decomposition_property(properties, db_usage);
        if (!method.empty() && method == "EXTERNAL") {
          decompose = false;
        }
      }
      else if (db_usage == Ioss::WRITE_RESULTS || db_usage == Ioss::WRITE_RESTART) {
        decompose = check_composition_property(properties, db_usage);
      }
    }

    if (decompose)
      return new Iocgns::ParallelDatabaseIO(nullptr, filename, db_usage, communicator, properties);
    else
#endif
      return new Iocgns::DatabaseIO(nullptr, filename, db_usage, communicator, properties);
  }

  std::string IOFactory::show_config() const { return Iocgns::Utils::show_config(); }
} // namespace Iocgns

#if CG_BUILD_PARALLEL
namespace {
  std::string check_decomposition_property(const Ioss::PropertyManager &properties,
                                           Ioss::DatabaseUsage          db_usage)
  {
    std::string decomp_method;
    std::string decomp_property;
    if (db_usage == Ioss::READ_MODEL) {
      decomp_property = "MODEL_DECOMPOSITION_METHOD";
    }
    else if (db_usage == Ioss::READ_RESTART) {
      decomp_property = "RESTART_DECOMPOSITION_METHOD";
    }

    // Applies to either read_model or read_restart
    if (properties.exists("DECOMPOSITION_METHOD")) {
      std::string method = properties.get("DECOMPOSITION_METHOD").get_string();
      return Ioss::Utils::uppercase(method);
    }

    // Check for property...
    if (properties.exists(decomp_property)) {
      std::string method = properties.get(decomp_property).get_string();
      return Ioss::Utils::uppercase(method);
    }
    return decomp_method;
  }

  bool check_composition_property(const Ioss::PropertyManager &properties,
                                  Ioss::DatabaseUsage          db_usage)
  {
    bool        compose          = true;
    std::string compose_property = "COMPOSE_INVALID";
    if (db_usage == Ioss::WRITE_RESULTS) {
      compose_property = "COMPOSE_RESULTS";
    }
    else if (db_usage == Ioss::WRITE_RESTART) {
      compose_property = "COMPOSE_RESTART";
    }

    Ioss::Utils::check_set_bool_property(properties, compose_property, compose);
    return compose;
  }
} // namespace
#endif
