// Copyright(C) 2024, 2025 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_ChangeSet.h"
#include "Ioss_ChangeSetFactory.h"
#include "Ioss_Utils.h" // for IOSS_ERROR
#include "Ioss_Version.h"
#include "vtk_fmt.h"
#include VTK_FMT(fmt/format.h)
#include VTK_FMT(fmt/ostream.h)
#include VTK_FMT(fmt/ranges.h)
#include <map>     // for _Rb_tree_iterator, etc
#include <ostream> // for basic_ostream, etc
#include <set>
#include <string> // for char_traits, string, etc

#include "Ioss_CodeTypes.h"
#include "Ioss_DBUsage.h" // for DatabaseUsage
#include "Ioss_PropertyManager.h"
#include "Ioss_Region.h"

namespace {
#if defined(IOSS_THREADSAFE)
  std::mutex m_;
#endif

  int describe_nl(Ioss::ChangeSetFactoryMap *registry, Ioss::NameList *names)
  {
    int                                       count = 0;
    Ioss::ChangeSetFactoryMap::const_iterator I;
    for (I = registry->begin(); I != registry->end(); ++I) {
      names->push_back((*I).first);
      ++count;
    }
    return count;
  }
} // namespace

namespace Ioss {
  class ChangeSet;

  using ChangeSetFactoryValuePair = ChangeSetFactoryMap::value_type;
} // namespace Ioss

const Ioss::ChangeSetFactory *Ioss::ChangeSetFactory::factory()
{
  static Ioss::ChangeSetFactory registerThis("ioss");
  return &registerThis;
}

std::shared_ptr<Ioss::ChangeSet> Ioss::ChangeSetFactory::create(Ioss::Region *region)
{
  IOSS_FUNC_ENTER(m_);
  std::string dbType = region->get_property("database_type").get_string();

  auto iter = registry()->find(dbType);
  if (iter == registry()->end()) {
    if (registry()->empty()) {
      std::ostringstream errmsg;
      fmt::print(errmsg, "ERROR: No change set types have been registered.\n"
                         "       Was Ioss::Init::Initializer() called?\n\n");
      IOSS_ERROR(errmsg);
    }
    iter = registry()->find("ioss");
    if (iter == registry()->end()) {
      std::ostringstream errmsg;
      fmt::print(errmsg, "ERROR: Could not locate correct change set types.\n\n");
      IOSS_ERROR(errmsg);
    }
  }

  Ioss::ChangeSetFactory          *factory = (*iter).second;
  Ioss::ChangeSet                 *csPtr   = factory->make_ChangeSet(region);
  std::shared_ptr<Ioss::ChangeSet> cs(csPtr);
  return cs;
}

std::shared_ptr<Ioss::ChangeSet> Ioss::ChangeSetFactory::create(Ioss::DatabaseIO  *db,
                                                                const std::string &dbName,
                                                                const std::string &dbType,
                                                                unsigned           fileCyclicCount)
{
  IOSS_FUNC_ENTER(m_);

  auto iter = registry()->find(dbType);
  if (iter == registry()->end()) {
    if (registry()->empty()) {
      std::ostringstream errmsg;
      fmt::print(errmsg, "ERROR: No change set types have been registered.\n"
                         "       Was Ioss::Init::Initializer() called?\n\n");
      IOSS_ERROR(errmsg);
    }
    iter = registry()->find("ioss");
    if (iter == registry()->end()) {
      std::ostringstream errmsg;
      fmt::print(errmsg, "ERROR: Could not locate correct change set types.\n\n");
      IOSS_ERROR(errmsg);
    }
  }

  Ioss::ChangeSetFactory *factory = (*iter).second;
  Ioss::ChangeSet        *csPtr   = factory->make_ChangeSet(db, dbName, dbType, fileCyclicCount);
  std::shared_ptr<Ioss::ChangeSet> cs(csPtr);
  return cs;
}

Ioss::ChangeSet *Ioss::ChangeSetFactory::make_ChangeSet(Ioss::Region *region) const
{
  return new ChangeSet(region);
}

Ioss::ChangeSet *Ioss::ChangeSetFactory::make_ChangeSet(Ioss::DatabaseIO  *db,
                                                        const std::string &dbName,
                                                        const std::string &dbType,
                                                        unsigned           fileCyclicCount) const
{
  return new ChangeSet(db, dbName, dbType, fileCyclicCount);
}

/** \brief Get the names of change set types known to IOSS.
 *
 *  \param[out] names The list of known change set types.
 *  \returns The number of known change set types.
 */
int Ioss::ChangeSetFactory::describe(NameList *names)
{
  IOSS_FUNC_ENTER(m_);
  return ::describe_nl(registry(), names);
}

/** \brief Get the names of change set types known to IOSS.
 *
 *  \returns The list of known change set types.
 */
Ioss::NameList Ioss::ChangeSetFactory::describe()
{
  Ioss::NameList names;
  describe(&names);
  return names;
}

Ioss::ChangeSetFactory::ChangeSetFactory(const std::string &type)
{
  registry()->insert(ChangeSetFactoryValuePair(type, this));
}

void Ioss::ChangeSetFactory::alias(const std::string &base, const std::string &syn)
{
  Ioss::ChangeSetFactory *factory = (*registry()->find(base)).second;
  registry()->insert(ChangeSetFactoryValuePair(syn, factory));
}

Ioss::ChangeSetFactoryMap *Ioss::ChangeSetFactory::registry()
{
  static ChangeSetFactoryMap registry_;
  return &registry_;
}
