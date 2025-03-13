// Copyright(C) 1999-, 20242024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "ioss_export.h"
#include "vtk_ioss_mangle.h"

#include "Ioss_CodeTypes.h"
#include <string>

#include "Ioss_DBUsage.h"
#include "Ioss_ParallelUtils.h"
#include "Ioss_PropertyManager.h"

#include <map>
#include <vector>

/** \brief The main namespace for the Ioss library.
 */
namespace Ioss {

  class IOFactory;

  using NameList     = Ioss::NameList;
  using IOFactoryMap = std::map<std::string, IOFactory *, std::less<>>;

  class DatabaseIO;

  /** \brief The main public user interface for creating Ioss::DatabaseIO objects.
   */
  class IOSS_EXPORT IOFactory
  {
  public:
    virtual ~IOFactory() = default;
    IOSS_NODISCARD static DatabaseIO *
    create(const std::string &type, const std::string &filename, DatabaseUsage db_usage,
           Ioss_MPI_Comm                communicator = Ioss::ParallelUtils::comm_world(),
           const Ioss::PropertyManager &properties   = Ioss::PropertyManager());

    static int                        describe(NameList *names);
    IOSS_NODISCARD static NameList    describe();
    static void                       clean();
    IOSS_NODISCARD static std::string show_configuration();

  protected:
    explicit IOFactory(const std::string &type);

    IOSS_NODISCARD virtual DatabaseIO *make_IO(const std::string &filename, DatabaseUsage db_usage,
                                               Ioss_MPI_Comm                communicator,
                                               const Ioss::PropertyManager &properties) const = 0;

    IOSS_NODISCARD virtual std::string show_config() const { return {""}; }

    static void alias(const std::string &base, const std::string &syn);

  private:
    IOSS_NODISCARD static IOFactoryMap *registry();
  };
} // namespace Ioss
