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

#include <Ioss_CompositeVariableType.h>
#include <Ioss_ConstructedVariableType.h>
#include <Ioss_NamedSuffixVariableType.h>
#include <Ioss_Utils.h>
#include <Ioss_VariableType.h>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fmt/ostream.h>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace Ioss {
  void Registry::insert(const VTM_ValuePair &value, bool delete_me)
  {
    m_registry.insert(value);
    if (delete_me) {
      m_deleteThese.push_back(value.second);
    }
  }

  Registry::~Registry()
  {
    for (auto &entry : m_deleteThese) {
      delete entry;
    }
  }

  VariableType::~VariableType() = default;

  VariableType::VariableType(const std::string &type, int comp_count, bool delete_me)
      : name_(type), componentCount(comp_count)
  {
    std::string low_type = Utils::lowercase(type);
    registry().insert(VTM_ValuePair(low_type, this), delete_me);

    // Register uppercase version also
    std::string up_type = Utils::uppercase(type);
    registry().insert(VTM_ValuePair(up_type, this), false);
  }

  void VariableType::alias(const std::string &base, const std::string &syn)
  {
    registry().insert(
        VTM_ValuePair(Utils::lowercase(syn), const_cast<VariableType *>(factory(base))), false);
    // Register uppercase version also
    std::string up_type = Utils::uppercase(syn);
    registry().insert(VTM_ValuePair(up_type, const_cast<VariableType *>(factory(base))), false);
  }

  Registry &VariableType::registry()
  {
    static Registry registry_;
    return registry_;
  }

  /** \brief Get the names of variable types known to IOSS.
   *
   *  \param[out] names The list of known variable type names.
   *  \returns The number of known variable types.
   */
  int VariableType::describe(NameList *names)
  {
    int count = 0;
    for (auto &entry : registry()) {
      names->push_back(entry.first);
      count++;
    }
    return count;
  }

  bool VariableType::add_field_type_mapping(const std::string &raw_field,
                                            const std::string &raw_type)
  {
    // See if storage type 'type' exists...
    std::string field = Utils::lowercase(raw_field);
    std::string type  = Utils::lowercase(raw_type);
    if (registry().find(type) == registry().end()) {
      return false;
    }

    // Add mapping.
    return registry().customFieldTypes.insert(std::make_pair(field, type)).second;
  }

  bool VariableType::create_named_suffix_field_type(const std::string &             type_name,
                                                    const std::vector<std::string> &suffices)
  {
    size_t count = suffices.size();
    if (count < 1) {
      return false;
    }

    std::string low_name = Utils::lowercase(type_name);
    // See if the variable already exists...
    if (registry().find(low_name) != registry().end()) {
      return false;
    }

    // Create the variable.  Note that the 'true' argument means Ioss will delete
    // the pointer.
    auto var_type = new NamedSuffixVariableType(low_name, count, true);

    for (size_t i = 0; i < count; i++) {
      var_type->add_suffix(i + 1, suffices[i]);
    }
    return true;
  }

  bool VariableType::get_field_type_mapping(const std::string &field, std::string *type)
  {
    // Returns true if a mapping exists, 'type' contains the mapped type.
    // Returns false if no custom mapping exists for this field.
    std::string low_field = Utils::lowercase(field);

    if (registry().customFieldTypes.find(low_field) == registry().customFieldTypes.end()) {
      return false;
    }

    *type = registry().customFieldTypes.find(low_field)->second;
    return true;
  }

  const VariableType *VariableType::factory(const std::string &raw_name, int copies)
  {
    VariableType *inst = nullptr;
    std::string   name = Utils::lowercase(raw_name);
    auto          iter = registry().find(name);
    if (iter == registry().end()) {
      bool can_construct = build_variable_type(name);
      if (can_construct) {
        iter = registry().find(name);
        assert(iter != registry().end());
        inst = (*iter).second;
      }
      else {
        std::ostringstream errmsg;
        fmt::print(errmsg, "ERROR: The variable type '{}' is not supported.\n", raw_name);
        IOSS_ERROR(errmsg);
      }
    }
    else {
      inst = (*iter).second;
    }

    if (copies != 1) {
      inst = CompositeVariableType::composite_variable_type(inst, copies);
    }
    assert(inst != nullptr);
    return inst;
  }

  const VariableType *VariableType::factory(const std::vector<Suffix> &suffices)
  {
    size_t size = suffices.size();
    // Maximum suffix size is currently 5.
    assert(size < 100000);
    const VariableType *ivt = nullptr;
    if (size <= 1) {
      return nullptr; // All storage types must have at least 2 components.
    }

    bool match = false;
    for (auto vtype : registry()) {
      ivt = vtype.second;
      if (ivt->suffix_count() == static_cast<int>(size)) {
        if (ivt->match(suffices)) {
          match = true;
          break;
        }
      }
    }

    if (!match) {
      match = true;
      // Check if the suffices form a sequence (1,2,3,...,N)
      // This indicates a "component" variable type that is
      // constructed "on-the-fly" for use in Sierra
      //
      size_t width = Ioss::Utils::number_width(size);
      for (size_t i = 0; i < size; i++) {
        std::string digits = fmt::format("{:0{}}", i + 1, width);
        if (!Ioss::Utils::str_equal(&suffices[i].m_data[0], digits)) {
          match = false;
          break;
        }
      }
      if (match) {
        // Create a new type.  For now, assume that the base type is
        // "Real" (Sierra type).  The name of the new type is
        // "Real[component_count]"
        // Note that this type has not yet been constructed since
        // it would have been found above.
        ivt = new ConstructedVariableType(size, true);
      }
      else {
        ivt = nullptr;
      }
    }
    return ivt;
  }

  bool VariableType::match(const std::vector<Suffix> &suffices) const
  {
    bool result = true;
    if (static_cast<int>(suffices.size()) == suffix_count()) {
      for (int i = 0; i < suffix_count(); i++) {
        if (suffices[i] != label(i + 1)) {
          result = false;
          break;
        }
      }
    }
    else {
      result = false;
    }
    return result;
  }

  std::string VariableType::label_name(const std::string &base, int which,
                                       const char suffix_sep) const
  {
    std::string my_name = base;
    std::string suffix  = label(which, suffix_sep);
    if (!suffix.empty()) {
      if (suffix_sep != 0) {
        my_name += suffix_sep;
      }
      my_name += suffix;
    }
    return my_name;
  }

  bool VariableType::build_variable_type(const std::string &raw_type)
  {
    // See if this is a multi-component instance of a base type.
    // An example would be REAL[2] which is a basic real type with
    // two components.  The suffices would be .0 and .1

    std::string type = Utils::lowercase(raw_type);

    // Step 0:
    // See if the type contains '[' and ']'
    char const *typestr = type.c_str();
    char const *lbrace  = std::strchr(typestr, '[');
    char const *rbrace  = std::strrchr(typestr, ']');

    if (lbrace == nullptr || rbrace == nullptr) {
      return false;
    }

    // Step 1:
    // First, we split off the basename (REAL/INTEGER) from the component count
    // ([2])
    // and see if the basename is a valid variable type and the count is a
    // valid integer.
    size_t len      = type.length() + 1;
    auto   typecopy = new char[len];
    Utils::copy_string(typecopy, typestr, len);

    char *base = std::strtok(typecopy, "[]");
    assert(base != nullptr);
    auto iter = VariableType::registry().find(base);
    if (iter == registry().end()) {
      delete[] typecopy;
      return false;
    }

    char *countstr = std::strtok(nullptr, "[]");
    assert(countstr != nullptr);
    int count = std::atoi(countstr);
    if (count <= 0) {
      delete[] typecopy;
      return false;
    }

    // We now know we have a valid base type and an integer
    // specifying the number of 'components' in our new type.
    // Create the new type and register it in the registry...
    new ConstructedVariableType(type, count, true);
    delete[] typecopy;
    return true;
  }

  std::string VariableType::numeric_label(int which, int ncomp, const std::string &name)
  {
    if (ncomp >= 100000) {
      std::ostringstream errmsg;
      fmt::print(errmsg,
                 "ERROR: Variable '{}' has {:n} components which is larger than the current maximum"
                 " of 100,000. Please contact developer.\n",
                 name, ncomp);
      IOSS_ERROR(errmsg);
    }

    size_t      width  = Ioss::Utils::number_width(ncomp);
    std::string digits = fmt::format("{:0{}}", which, width);
    return digits;
  }
} // namespace Ioss
