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

#ifndef IOSS_Iohb_Layout_h
#define IOSS_Iohb_Layout_h

#include "vtk_ioss_mangle.h"

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
    void add(const std::string &name, double value);
    void add(const std::string &name, int value);
    void add(const std::string &name, long value);
    void add(const std::string &name, const std::string &value);

    void add(const std::string &name, std::vector<double> &value);
    void add(const std::string &name, std::vector<int> &value);
    void add(const std::string &name, std::vector<long> &value);
    void add(const std::string &name, std::vector<std::string> &value);

  private:
    std::ostringstream layout_{};
    std::string        separator_{};

    int  precision_;
    int  count_; // Number of fields on current line...
    int  fieldWidth_;
    bool showLabels;
    bool legendStarted;
  };
} // namespace Iohb

#endif // IOSS_Iohb_Layout_h
