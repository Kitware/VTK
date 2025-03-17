// Copyright(C) 2022, 2023, 2024, 2025 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "Ioss_CodeTypes.h"
#include <functional>
#include <map>
#include <string>

#include "ioss_export.h"
#include "vtk_ioss_mangle.h"

namespace Ioss {
  class Transform;

  class TransformFactory;

  using TransformFactoryMap = std::map<std::string, TransformFactory *, std::less<>>;

  class IOSS_EXPORT TransformFactory
  {
  public:
    virtual ~TransformFactory() = default;
    IOSS_NODISCARD static Ioss::Transform *create(const std::string &type);

    static int                           describe(Ioss::NameList *names);
    IOSS_NODISCARD static Ioss::NameList describe();

  protected:
    explicit TransformFactory(const std::string &type);
    IOSS_NODISCARD virtual Ioss::Transform *make(const std::string &) const = 0;
    static void                             alias(const std::string &base, const std::string &syn);

  private:
    static TransformFactoryMap &registry();
  };
} // namespace Ioss
