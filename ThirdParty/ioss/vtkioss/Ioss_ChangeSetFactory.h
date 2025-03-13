// Copyright(C) 2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "ioss_export.h"
#include "vtk_ioss_mangle.h"

#include "Ioss_CodeTypes.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

/** \brief The main namespace for the Ioss library.
 */
namespace Ioss {

  class Region;
  class DatabaseIO;
  class ChangeSet;
  class ChangeSetFactory;
  using NameList            = Ioss::NameList;
  using ChangeSetFactoryMap = std::map<std::string, ChangeSetFactory *, std::less<>>;

  /** \brief The main public user interface for creating Ioss::ChangeSet objects.
   */
  class IOSS_EXPORT ChangeSetFactory
  {
  public:
    virtual ~ChangeSetFactory() = default;
    IOSS_NODISCARD static std::shared_ptr<ChangeSet> create(Ioss::Region *region);
    IOSS_NODISCARD static std::shared_ptr<ChangeSet> create(Ioss::DatabaseIO  *db,
                                                            const std::string &dbName,
                                                            const std::string &dbType,
                                                            unsigned           fileCyclicCount = 0);

    static int                     describe(NameList *names);
    IOSS_NODISCARD static NameList describe();

    static const ChangeSetFactory *factory();

  protected:
    explicit ChangeSetFactory(const std::string &type);

    IOSS_NODISCARD virtual ChangeSet *make_ChangeSet(Ioss::Region *region) const;
    IOSS_NODISCARD virtual ChangeSet *make_ChangeSet(Ioss::DatabaseIO  *db,
                                                     const std::string &dbName,
                                                     const std::string &dbType,
                                                     unsigned           fileCyclicCount) const;

    static void alias(const std::string &base, const std::string &syn);

  private:
    IOSS_NODISCARD static ChangeSetFactoryMap *registry();
  };

} // namespace Ioss
