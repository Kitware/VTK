// Copyright(C) 1999-2022, 2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "heartbeat/Iohb_Layout.h"
#include "vtk_fmt.h"
#include VTK_FMT(fmt/format.h)
#include VTK_FMT(fmt/ostream.h)
#include VTK_FMT(fmt/ranges.h)
#include <string>

namespace Iohb {
  Layout::Layout(bool show_labels, int precision, std::string separator, int field_width)
      : layout_(), separator_(std::move(separator)), precision_(precision),
        fieldWidth_(field_width), showLabels(show_labels)
  {
  }

  void Layout::add_literal(const std::string &label) { fmt::print(layout_, "{}", label); }

  void Layout::add_legend(const std::string &label)
  {
    fmt::print(layout_, "{}{:>{}}", legendStarted ? separator_ : "", label, fieldWidth_);
    legendStarted = true;
  }

  void Layout::output_common(const std::string &name)
  {
    if (count_++ > 0 && !separator_.empty()) {
      fmt::print(layout_, "{}", separator_);
    }

    if (showLabels && !name.empty()) {
      fmt::print(layout_, "{}=", name);
    }
  }

  template void Layout::add(const std::string &name, const std::string &value);
  template void Layout::add(const std::string &name, const int &value);
  template void Layout::add(const std::string &name, const int64_t &value);
  template void Layout::add(const std::string &name, const size_t &value);

  // Ideally, this would be in the include file, but when building in Sierra, we
  // need to keep all `fmt` includes out of the include file due to some TPLs
  // having an embedded fmt which is a different version than used by IOSS.
  template <typename T> void Layout::add(const std::string &name, const T &value)
  {
    output_common(name);
    if (!showLabels && fieldWidth_ > 0) {
      fmt::print(layout_, "{0:{1}}", value, fieldWidth_);
    }
    else {
      fmt::print(layout_, "{}", value);
    }
  }

  template <> void Layout::add(const std::string &name, const double &value)
  {
    output_common(name);
    if (precision_ == -1) {
      // Use lib::fmt full precision output -- as many digits as needed to fully represent the
      // double
      fmt::print(layout_, "{}", value);
    }
    else if (!showLabels && fieldWidth_ > 0) {
      fmt::print(layout_, "{0:{1}.{2}e}", value, fieldWidth_, precision_);
    }
    else {
      fmt::print(layout_, "{0:.{1}e}", value, precision_);
    }
  }

  template void Layout::add(const std::string &name, const std::vector<int> &value);
  template void Layout::add(const std::string &name, const std::vector<int64_t> &value);
  template void Layout::add(const std::string &name, const std::vector<size_t> &value);

  template <typename T> void Layout::add(const std::string &name, const std::vector<T> &value)
  {
    if (value.size() == 1) {
      add(name, value[0]);
    }
    else {
      output_common(name);
      if (!showLabels && fieldWidth_ > 0) {
        fmt::print(layout_, "{0:{1}}", fmt::join(value, separator_), fieldWidth_);
      }
      else {
        fmt::print(layout_, "{}", fmt::join(value, separator_));
      }
    }
  }

  template <> void Layout::add(const std::string &name, const std::vector<double> &value)
  {
    if (value.size() == 1) {
      add(name, value[0]);
    }
    else {
      output_common(name);
      if (precision_ == -1) {
        // Use lib::fmt full precision output -- as many digits as needed to fully represent the
        // double
        fmt::print(layout_, "{}", fmt::join(value, separator_));
      }
      else if (!showLabels && fieldWidth_ > 0) {
        fmt::print(layout_, "{0:{2}.{1}e}", fmt::join(value, separator_), precision_, fieldWidth_);
      }
      else {
        fmt::print(layout_, "{0:.{1}e}", fmt::join(value, separator_), precision_);
      }
    }
  }

} // namespace Iohb
