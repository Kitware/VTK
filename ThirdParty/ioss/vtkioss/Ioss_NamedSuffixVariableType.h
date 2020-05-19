/*
 * Copyright(C) 1999-2017, 2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of NTESS nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef IOSS_Ioss_NamedSuffixVariableType_h
#define IOSS_Ioss_NamedSuffixVariableType_h

#include "vtk_ioss_mangle.h"

#include <Ioss_CodeTypes.h>
#include <string>

#include <Ioss_VariableType.h>

namespace Ioss {
  class NamedSuffixVariableType : public VariableType
  {
  public:
    //  'which' is 1-based
    std::string label(int which, const char /* suffix_sep */) const override
    {
      return suffixList[which - 1];
    }

    NamedSuffixVariableType(const std::string &my_name, int number_components, bool delete_me)
        : Ioss::VariableType(my_name, number_components, delete_me)
    {
      suffixList.resize(number_components);
      suffixList.assign(number_components, "UNSET");
    }
    NamedSuffixVariableType(const NamedSuffixVariableType &) = delete;

    //! Define the suffix list for this field.
    //  'which' is 1-based to conform to the 'label' function usage.
    // If user doesn't add suffices, then 'label' will return "UNSET"
    void add_suffix(size_t which, const std::string &suffix) { suffixList[which - 1] = suffix; }

  private:
    std::vector<std::string> suffixList{};
  };
} // namespace Ioss

#endif
