// Copyright(C) 1999-2021 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#ifndef IOSS_Iohb_Layout_h
#define IOSS_Iohb_Layout_h

#include "vtk_ioss_mangle.h"

#include <iomanip> // for operator<<, setw, etc
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace Iohb {
  class Layout
  {
  public:
    Layout(bool show_labels, int precision, std::string separator, int field_width);
    Layout(const Layout &) = delete;
    Layout &operator=(const Layout &) = delete;

    ~Layout();

    friend std::ostream &operator<<(std::ostream & /*o*/, Layout & /*lo*/);

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

  inline void Layout::output_common(const std::string &name)
  {
    if (count_++ > 0 && !separator_.empty()) {
      layout_ << separator_;
    }

    if (showLabels && name != "") {
      layout_ << name;
      layout_ << "=";
    }
    else if (fieldWidth_ != 0) {
      layout_ << std::setw(fieldWidth_);
    }
  }

  template <typename T> inline void Layout::add(const std::string &name, const T &value)
  {
    output_common(name);
    layout_ << value;
  }

  template <> inline void Layout::add(const std::string &name, const double &value)
  {
    output_common(name);
    layout_.setf(std::ios::scientific);
    layout_.setf(std::ios::showpoint);
    layout_ << std::setprecision(precision_) << value;
  }

  template <typename T>
  inline void Layout::add(const std::string &name, const std::vector<T> &value)
  {
    if (value.size() == 1) {
      add(name, value[0]);
    }
    else {
      output_common(name);
      for (size_t i = 0; i < value.size(); i++) {
        if (!showLabels && (fieldWidth_ != 0)) {
          layout_ << std::setw(fieldWidth_);
        }
        layout_ << value[i];
        if (i < value.size() - 1 && !separator_.empty()) {
          layout_ << separator_;
        }
      }
    }
  }

  template <> inline void Layout::add(const std::string &name, const std::vector<double> &value)
  {
    if (value.size() == 1) {
      add(name, value[0]);
    }
    else {
      output_common(name);
      layout_.setf(std::ios::scientific);
      layout_.setf(std::ios::showpoint);
      for (size_t i = 0; i < value.size(); i++) {
        if (!showLabels && (fieldWidth_ != 0)) {
          layout_ << std::setw(fieldWidth_);
        }
        layout_ << std::setprecision(precision_) << value[i];
        if (i < value.size() - 1 && !separator_.empty()) {
          layout_ << separator_;
        }
      }
    }
  }

} // namespace Iohb

#endif // IOSS_Iohb_Layout_h
