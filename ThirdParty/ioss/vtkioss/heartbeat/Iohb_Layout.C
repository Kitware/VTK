// Copyright(C) 1999-2017 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//
//     * Neither the name of NTESS nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <heartbeat/Iohb_Layout.h>
#include <iomanip> // for operator<<, setw, etc
#include <string>  // for operator<<, string, etc
#include <vector>  // for vector, vector<>::size_type

namespace Iohb {
  Layout::Layout(bool show_labels, int precision, std::string separator, int field_width)
      : layout_(), separator_(std::move(separator)), precision_(precision), count_(0),
        fieldWidth_(field_width), showLabels(show_labels), legendStarted(false)
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

  void Layout::add(const std::string &name, double value)
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
    layout_.setf(std::ios::scientific);
    layout_.setf(std::ios::showpoint);
    layout_ << std::setprecision(precision_) << value;
  }

  void Layout::add(const std::string &name, int value)
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
    layout_ << value;
  }

  void Layout::add(const std::string &name, long value)
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
    layout_ << value;
  }

  void Layout::add(const std::string &name, const std::string &value)
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
    layout_ << value;
  }

  void Layout::add(const std::string &name, std::vector<double> &value)
  {
    if (value.size() == 1) {
      add(name, value[0]);
    }
    else {
      if (count_++ > 0 && !separator_.empty()) {
        layout_ << separator_;
      }

      if (showLabels && !name.empty()) {
        layout_ << name;
        layout_ << "=";
      }
      layout_.setf(std::ios::scientific);
      layout_.setf(std::ios::showpoint);
      for (std::vector<double>::size_type i = 0; i < value.size(); i++) {
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

  void Layout::add(const std::string &name, std::vector<int> &value)
  {
    if (value.size() == 1) {
      add(name, value[0]);
    }
    else {
      if (count_++ > 0 && !separator_.empty()) {
        layout_ << separator_;
      }

      if (showLabels && name != "") {
        layout_ << name;
        layout_ << "=";
      }
      for (std::vector<int>::size_type i = 0; i < value.size(); i++) {
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

  void Layout::add(const std::string &name, std::vector<long> &value)
  {
    if (value.size() == 1) {
      add(name, value[0]);
    }
    else {
      if (count_++ > 0 && !separator_.empty()) {
        layout_ << separator_;
      }

      if (showLabels && name != "") {
        layout_ << name;
        layout_ << "=";
      }
      for (std::vector<long>::size_type i = 0; i < value.size(); i++) {
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

  void Layout::add(const std::string &name, std::vector<std::string> &value)
  {
    if (value.size() == 1) {
      add(name, value[0]);
    }
  }
} // namespace Iohb
