// Copyright(C) 1999-2021 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include <heartbeat/Iohb_Layout.h>
#include <string> // for operator<<, string, etc
#include <vector> // for vector, vector<>::size_type

namespace Iohb {
  Layout::Layout(bool show_labels, int precision, std::string separator, int field_width)
      : layout_(), separator_(std::move(separator)), precision_(precision),
        fieldWidth_(field_width), showLabels(show_labels)
  {
  }

  Layout::~Layout() = default;

  std::ostream &operator<<(std::ostream &o, Layout &lo)
  {
    o << lo.layout_.str();
    return o;
  }

  void Layout::add_literal(const std::string &label) { layout_ << label; }

  void Layout::add_legend(const std::string &label)
  {
    if (legendStarted && !separator_.empty()) {
      layout_ << separator_;
    }
    else {
      legendStarted = true;
    }

    if (fieldWidth_ != 0) {
      layout_ << std::setw(fieldWidth_) << label;
    }
    else {
      layout_ << label;
    }
  }
} // namespace Iohb
