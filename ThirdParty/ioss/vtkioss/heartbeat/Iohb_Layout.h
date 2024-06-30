// Copyright(C) 1999-2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "vtk_fmt.h"
#include VTK_FMT(fmt/format.h)
#include VTK_FMT(fmt/ostream.h)
#include VTK_FMT(fmt/ranges.h)
#include <sstream>
#include <string>
#include <vector>

#include "iohb_export.h"
#include "vtk_ioss_mangle.h"

namespace Iohb {
  class IOHB_EXPORT Layout
  {
  public:
    Layout(bool show_labels, int precision, std::string separator, int field_width);
    Layout(const Layout &)            = delete;
    Layout &operator=(const Layout &) = delete;

    const std::string layout() const { return layout_.str(); }

    void add_literal(const std::string &label);
    void add_legend(const std::string &label);

    template <typename T> void add(const std::string &name, const T &value);
    template <typename T> void add(const std::string &name, const std::vector<T> &value);

  private:
    void               output_common(const std::string &name);
    std::ostringstream layout_{};
    std::string        separator_{", "};

    int  precision_{5};
    int  count_{0}; // Number of fields on current line...
    int  fieldWidth_{0};
    bool showLabels{true};
    bool legendStarted{false};
  };

} // namespace Iohb
