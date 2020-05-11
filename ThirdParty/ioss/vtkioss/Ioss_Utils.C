// Copyright(C) 1999-2017, 2020 National Technology & Engineering Solutions
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

#include <Ioss_Assembly.h>
#include <Ioss_Blob.h>
#include <Ioss_CodeTypes.h>
#include <Ioss_MeshCopyOptions.h>
#include <Ioss_Utils.h>
#include <algorithm>
#include <cassert>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <iomanip>
#include <sstream>
#include <string>
#include <tokenize.h>
#include <vector>

#ifndef _WIN32
#include <sys/ioctl.h>
#include <sys/utsname.h>
#endif

#ifndef _POSIX_SOURCE
#define _POSIX_SOURCE
#endif
#include <cstdio>

#if defined(_MSC_VER)
#include <io.h>
#define isatty _isatty
#endif

// For memory utilities...
#if defined(_WIN32)
#include <psapi.h>
#include <windows.h>

#elif defined(__unix__) || defined(__unix) || defined(unix) ||                                     \
    (defined(__APPLE__) && defined(__MACH__))
#include <sys/resource.h>
#include <unistd.h>

#if defined(__APPLE__) && defined(__MACH__)
#include <mach/mach.h>

#elif (defined(_AIX) || defined(__TOS__AIX__)) ||                                                  \
    (defined(__sun__) || defined(__sun) || defined(sun) && (defined(__SVR4) || defined(__svr4__)))
#include <fcntl.h>
#include <procfs.h>

#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
#include <stdio.h>
#endif
#endif

#if defined(BGQ_LWK) && defined(__linux__)
#include <spi/include/kernel/location.h>
#include <spi/include/kernel/memory.h>
#endif

#include <Ioss_SubSystem.h>

#include <fstream>

std::ostream *Ioss::Utils::m_warningStream  = &std::cerr;
std::ostream *Ioss::Utils::m_debugStream    = &std::cerr;
std::ostream *Ioss::Utils::m_outputStream   = &std::cerr;
std::string   Ioss::Utils::m_preWarningText = "\nIOSS WARNING: ";

// For copy_database...
namespace {
  auto initial_time = std::chrono::high_resolution_clock::now();

  size_t max_field_size = 0;

  struct DataPool
  {
    // Data space shared by most field input/output routines...
    std::vector<char>    data;
    std::vector<int>     data_int;
    std::vector<int64_t> data_int64;
    std::vector<double>  data_double;
    std::vector<Complex> data_complex;
#ifdef SEACAS_HAVE_KOKKOS
    Kokkos::View<char *>    data_view_char;
    Kokkos::View<int *>     data_view_int;
    Kokkos::View<int64_t *> data_view_int64;
    Kokkos::View<double *>  data_view_double;
    // Kokkos::View<Kokkos_Complex *> data_view_complex cannot be a global variable,
    // Since Kokkos::initialize() has not yet been called. Also, a Kokkos:View cannot
    // have type std::complex entities.
    Kokkos::View<char **>    data_view_2D_char;
    Kokkos::View<int **>     data_view_2D_int;
    Kokkos::View<int64_t **> data_view_2D_int64;
    Kokkos::View<double **>  data_view_2D_double;
    // Kokkos::View<Kokkos_Complex **> data_view_2D_complex cannot be a global variable,
    // Since Kokkos::initialize() has not yet been called. Also, a Kokkos:View cannot
    // have type std::complex entities.
    Kokkos::View<char **, Kokkos::LayoutRight, Kokkos::HostSpace> data_view_2D_char_layout_space;
    Kokkos::View<int **, Kokkos::LayoutRight, Kokkos::HostSpace>  data_view_2D_int_layout_space;
    Kokkos::View<int64_t **, Kokkos::LayoutRight, Kokkos::HostSpace>
        data_view_2D_int64_layout_space;
    Kokkos::View<double **, Kokkos::LayoutRight, Kokkos::HostSpace>
        data_view_2D_double_layout_space;
    // Kokkos::View<Kokkos_Complex **, Kokkos::LayoutRight, Kokkos::HostSpace>
    // data_view_2D_complex_layout_space cannot be a global variable,
    // Since Kokkos::initialize() has not yet been called. Also, a Kokkos:View cannot
    // have type std::complex entities.
#endif
  };

  void show_step(int istep, double time, const Ioss::MeshCopyOptions &options, int rank);

  void transfer_nodeblock(Ioss::Region &region, Ioss::Region &output_region, DataPool &pool,
                          const Ioss::MeshCopyOptions &options, int rank);
  void transfer_structuredblocks(Ioss::Region &region, Ioss::Region &output_region,
                                 const Ioss::MeshCopyOptions &options, int rank);
  void transfer_elementblocks(Ioss::Region &region, Ioss::Region &output_region,
                              const Ioss::MeshCopyOptions &options, int rank);
  void transfer_edgeblocks(Ioss::Region &region, Ioss::Region &output_region,
                           const Ioss::MeshCopyOptions &options, int rank);
  void transfer_faceblocks(Ioss::Region &region, Ioss::Region &output_region,
                           const Ioss::MeshCopyOptions &options, int rank);
  void transfer_nodesets(Ioss::Region &region, Ioss::Region &output_region,
                         const Ioss::MeshCopyOptions &options, int rank);
  void transfer_edgesets(Ioss::Region &region, Ioss::Region &output_region,
                         const Ioss::MeshCopyOptions &options, int rank);
  void transfer_facesets(Ioss::Region &region, Ioss::Region &output_region,
                         const Ioss::MeshCopyOptions &options, int rank);
  void transfer_elemsets(Ioss::Region &region, Ioss::Region &output_region,
                         const Ioss::MeshCopyOptions &options, int rank);
  void transfer_sidesets(Ioss::Region &region, Ioss::Region &output_region,
                         const Ioss::MeshCopyOptions &options, int rank);
  void transfer_commsets(Ioss::Region &region, Ioss::Region &output_region,
                         const Ioss::MeshCopyOptions &options, int rank);
  void transfer_coordinate_frames(Ioss::Region &region, Ioss::Region &output_region);
  void transfer_assemblies(Ioss::Region &region, Ioss::Region &output_region,
                           const Ioss::MeshCopyOptions &options, int rank);
  void transfer_blobs(Ioss::Region &region, Ioss::Region &output_region,
                      const Ioss::MeshCopyOptions &options, int rank);

  template <typename T>
  void transfer_fields(const std::vector<T *> &entities, Ioss::Region &output_region,
                       Ioss::Field::RoleType role, const Ioss::MeshCopyOptions &options, int rank);

  void transfer_fields(const Ioss::GroupingEntity *ige, Ioss::GroupingEntity *oge,
                       Ioss::Field::RoleType role, const std::string &prefix = "");

  void add_proc_id(Ioss::Region &region, int rank);

  template <typename T>
  void transfer_field_data(const std::vector<T *> &entities, Ioss::Region &output_region,
                           DataPool &pool, Ioss::Field::RoleType role,
                           const Ioss::MeshCopyOptions &options);

  void transfer_field_data(Ioss::GroupingEntity *ige, Ioss::GroupingEntity *oge, DataPool &pool,
                           Ioss::Field::RoleType role, const Ioss::MeshCopyOptions &options,
                           const std::string &prefix = "");

  void transfer_properties(const Ioss::GroupingEntity *ige, Ioss::GroupingEntity *oge);

  void transfer_qa_info(Ioss::Region &in, Ioss::Region &out);

  void transfer_field_data_internal(Ioss::GroupingEntity *ige, Ioss::GroupingEntity *oge,
                                    DataPool &pool, const std::string &field_name,
                                    const Ioss::MeshCopyOptions &options);

  template <typename INT>
  void set_owned_node_count(Ioss::Region &region, int my_processor, INT dummy);

  ////////////////////////////////////////////////////////////////////////
  bool is_separator(const char separator, const char value) { return separator == value; }

  size_t match(const char *name1, const char *name2)
  {
    size_t l1  = std::strlen(name1);
    size_t l2  = std::strlen(name2);
    size_t len = l1 < l2 ? l1 : l2;
    for (size_t i = 0; i < len; i++) {
      if (name1[i] != name2[i]) {
        while (i > 0 && (isdigit(name1[i - 1]) != 0) && (isdigit(name2[i - 1]) != 0)) {
          i--;
          // Back up to first non-digit so to handle "evar0000, evar0001, ..., evar 1123"
        }
        return i;
      }
    }
    return len;
  }

  // Split 'str' into 'tokens' based on the 'separator' character.
  // If 'str' starts with 1 or more 'separator', they are part of the
  // first token and not used for splitting.  If there are multiple
  // 'separator' characters in a row, then the first is used to split
  // and the subsequent 'separator' characters are put as leading
  // characters of the next token.
  // `__this___is_a_string__for_tokens` will split to 6 tokens:
  // `__this` `__is` `a` `string` `_for` `tokens`
  void field_tokenize(const std::string &str, const char separator,
                      std::vector<std::string> &tokens)
  {
    std::string curr_token;
    // Skip leading separators...
    size_t i = 0;
    while (i < str.length() && is_separator(separator, str[i])) {
      curr_token += str[i++];
    }
    for (; i < str.length(); ++i) {
      char curr_char = str[i];

      // determine if current character is a separator
      bool is_sep = is_separator(separator, curr_char);
      if (is_sep && curr_token != "") {
        // we just completed a token
        tokens.push_back(curr_token);
        curr_token.clear();
        while (i++ < str.length() && is_separator(separator, str[i])) {
          curr_token += str[i];
        }
        i--;
      }
      else if (!is_sep) {
        curr_token += curr_char;
      }
    }
    if (curr_token != "") {
      tokens.push_back(curr_token);
    }
  }

} // namespace

void Ioss::Utils::time_and_date(char *time_string, char *date_string, size_t length)
{
  time_t      calendar_time = time(nullptr);
  auto *      lt            = std::localtime(&calendar_time);
  std::string time          = fmt::format("{:%H:%M:%S}", *lt);
  std::string date;
  if (length >= 10) {
    date = fmt::format("{:%Y/%m/%d}", *lt);
  }
  else {
    date = fmt::format("{:%y/%m/%d}", *lt);
  }
  copy_string(time_string, time, 9);
  copy_string(date_string, date, length + 1);
}

void Ioss::Utils::check_non_null(void *ptr, const char *type, const std::string &name,
                                 const std::string &func)
{
  if (ptr == nullptr) {
    std::ostringstream errmsg;
    fmt::print(errmsg,
               "INTERNAL ERROR: Could not find {} '{}'. Something is wrong in {}. Please report.\n",
               type, name, func);
    IOSS_ERROR(errmsg);
  }
}

std::string Ioss::Utils::decode_filename(const std::string &filename, int processor,
                                         int num_processors)
{
  // Current format for per-processor file names is:
  // PREFIX/basename.num_proc.cur_proc
  // the 'cur_proc' field is padded to be the same width as
  // the 'num_proc' field
  // Examples: basename.8.1, basename.64.03, basename.128.001

  // Create a std::string containing the total number of processors
  size_t proc_width = number_width(num_processors);

  std::string decoded_filename =
      fmt::format("{}.{}.{:0{}}", filename, num_processors, processor, proc_width);
  return decoded_filename;
}

size_t Ioss::Utils::get_number(const std::string &suffix)
{
  int  N       = 0;
  bool all_dig = suffix.find_first_not_of("0123456789") == std::string::npos;
  if (all_dig) {
    N = std::stoi(suffix);
  }
  return N;
}

int64_t Ioss::Utils::extract_id(const std::string &name_id)
{
  int64_t id = 0;

  std::vector<std::string> tokens = Ioss::tokenize(name_id, "_");
  if (tokens.size() > 1) {
    // Check whether last token is an integer...
    std::string str_id = tokens.back();
    id                 = get_number(str_id);
  }
  return id;
}

std::string Ioss::Utils::format_id_list(const std::vector<size_t> &ids, const std::string &rng_sep,
                                        const std::string &seq_sep)
{
  // Based on function from cubit (but I wrote original cubit version long time ago... ;-)
  if (ids.empty()) {
    return "";
  }

  // PRECONDITION: `ids` is monotonically increasing -- will throw IOSS_ERROR if violated.
  if (!std::is_sorted(ids.begin(), ids.end(), [](size_t a, size_t b) { return a <= b; })) {
    std::ostringstream errmsg;
    fmt::print(errmsg,
               "INTERNAL ERROR: ({}) The `ids` vector is not in monotonically increasing order as "
               "required.\n",
               __func__);
    IOSS_ERROR(errmsg);
  }

  size_t             num = 0;
  std::ostringstream ret_str;
  while (num < ids.size()) {
    fmt::print(ret_str, "{}{}", num == 0 ? "" : seq_sep, ids[num]);
    size_t begin    = ids[num]; // first id in range of 1 or more ids
    size_t previous = ids[num]; // last id in range of 1 or more ids
    // Gather a range or single value... (begin .. previous)
    while (previous == ids[num] && ++num < ids.size() && ids[num] == previous + 1) {
      previous++;
    }

    if (begin != previous) {
      fmt::print(ret_str, "{}{}", previous == begin + 1 ? seq_sep : rng_sep, previous);
    }
  }
  return ret_str.str();
}

std::string Ioss::Utils::encode_entity_name(const std::string &entity_type, int64_t id)
{
  // ExodusII stores block, nodeset, and sideset ids as integers
  // Sierra   stores these as std::strings. The string is created by
  // concatenating the type, the character '_' and the id.

  return fmt::format("{}_{}", entity_type, id);
}

char **Ioss::Utils::get_name_array(size_t count, int size)
{
  auto names = new char *[count];
  for (size_t i = 0; i < count; i++) {
    names[i] = new char[size + 1];
    std::memset(names[i], '\0', size + 1);
  }
  return names;
}

void Ioss::Utils::delete_name_array(char **names, int count)
{
  for (int i = 0; i < count; i++) {
    delete[] names[i];
  }
  delete[] names;
}

std::string Ioss::Utils::fixup_type(const std::string &base, int nodes_per_element, int spatial)
{
  std::string type = base;
  Ioss::Utils::fixup_name(type); // Convert to lowercase; replace spaces with '_'

  // Fixup an exodusII kluge/ambiguity.
  // The element block type does not fully define the element. For
  // example, a block of type 'triangle' may have either 3 or 6
  // nodes.  To fix this, check the block type name and see if it
  // ends with a number.  If it does, assume it is OK; if not, append
  // the 'nodes_per_element'.
  if (isdigit(*(type.rbegin())) == 0) {
    if (nodes_per_element > 1) {
      type += std::to_string(nodes_per_element);
    }
  }

  // Fixup an exodusII kludge.  For triangular elements, the same
  // name is used for 2D elements and 3D shell elements.  Convert
  // to unambiguous names for the IO Subsystem.  The 2D name
  // stays the same, the 3D name becomes 'trishell#'
  if (spatial == 3) {
    if (type == "triangle3") {
      type = "trishell3";
    }
    else if (type == "triangle4") {
      type = "trishell4";
    }
    else if (type == "triangle6") {
      type = "trishell6";
    }
    else if (type == "tri3") {
      type = "trishell3";
    }
    else if (type == "tri4") {
      type = "trishell4";
    }
    else if (type == "tri6") {
      type = "trishell6";
    }
  }

  if (spatial == 2) {
    if (type == "shell2") {
      type = "shellline2d2";
    }
    else if (type == "rod2" || type == "bar2" || type == "truss2") {
      type = "rod2d2";
    }
    else if (type == "shell3") {
      type = "shellline2d3";
    }
    else if (type == "bar3" || type == "rod3" || type == "truss3") {
      type = "rod2d3";
    }
    else if (type == "bar4" || type == "rod4" || type == "truss4") {
      type = "rod2d4";
    }
  }

  if (Ioss::Utils::substr_equal("super", type)) {
    // A super element can have a varying number of nodes.  Create
    // an IO element type for this super element just so the IO
    // system can read a mesh containing super elements.  This
    // allows the "omit volume" command to be used in the Sierra
    // applications to skip creating a corresponding element block
    // in the application.
    type = "super" + std::to_string(nodes_per_element);
  }
  return type;
}

std::string Ioss::Utils::local_filename(const std::string &relative_filename,
                                        const std::string &type,
                                        const std::string &working_directory)
{
  if (relative_filename[0] == '/' || type == "generated" || working_directory.empty()) {
    return relative_filename;
  }
  std::string filename = working_directory;
  filename += relative_filename;
  return filename;
}

int Ioss::Utils::field_warning(const Ioss::GroupingEntity *ge, const Ioss::Field &field,
                               const std::string &inout)
{
  fmt::print(Ioss::WARNING(), "{} '{}'. Unknown {} field '{}'\n", ge->type_string(), ge->name(),
             inout, field.get_name());
  return -4;
}

namespace {
  const Ioss::VariableType *match_composite_field(char **names, Ioss::IntVector &which_names,
                                                  const char suffix_separator)
  {
    // ASSUME: Fields are in order...
    // The field we are trying to match will be a composite field of
    // type base_x_1, base_y_1, base_z_1, ...., base_y_3, base_z_3.
    // The composite field type currently always has a numeric Real[N]
    // type field as the last suffix and the other field as the first
    // suffix.
    // If we take the last suffix of the last name, it should give us
    // the 'N' in the Real[N] field.  Dividing 'which_names.size()' by
    // 'N' will give the number of components in the inner field.

    char suffix[2] = {suffix_separator, '\0'};

    std::vector<std::string> tokens =
        Ioss::tokenize(names[which_names[which_names.size() - 1]], suffix);

    if (tokens.size() <= 2) {
      return nullptr;
    }

    assert(tokens.size() > 2);

    // Check that suffix is a number -- all digits
    size_t N = Ioss::Utils::get_number(tokens[tokens.size() - 1]);

    if (N == 0) {
      return nullptr;
    }

    if (which_names.size() % N != 0) {
      return nullptr;
    }

    size_t inner_token = tokens.size() - 2;
    size_t inner_comp  = which_names.size() / N;

    // Gather the first 'inner_ccomp' inner field suffices...
    std::vector<Ioss::Suffix> suffices;
    for (size_t i = 0; i < inner_comp; i++) {
      std::vector<std::string> ltokens = Ioss::tokenize(names[which_names[i]], suffix);
      // The second-last token is the suffix for this component...
      Ioss::Suffix tmp(ltokens[inner_token]);
      suffices.push_back(tmp);
    }

    // check that the suffices on the next copies of the inner field
    // match the first copy...
    size_t j = inner_comp;
    for (size_t copy = 1; copy < N; copy++) {
      for (size_t i = 0; i < inner_comp; i++) {
        std::vector<std::string> ltokens = Ioss::tokenize(names[which_names[j++]], suffix);
        // The second-last token is the suffix for this component...
        if (suffices[i] != ltokens[inner_token]) {
          return nullptr;
        }
      }
    }

    // All 'N' copies of the inner field match, now see the
    // suffices actually defines a field...
    const Ioss::VariableType *type = Ioss::VariableType::factory(suffices);
    if (type != nullptr) {
      type = Ioss::VariableType::factory(type->name(), N);
    }
    return type;
  }

  const Ioss::VariableType *match_single_field(char **names, Ioss::IntVector &which_names,
                                               const char suffix_separator)
  {
    // Strip off the suffix from each name indexed in 'which_names'
    // and see if it defines a valid type...
    std::vector<Ioss::Suffix> suffices;

    char suffix[2] = {suffix_separator, '\0'};

    for (int which_name : which_names) {
      std::vector<std::string> tokens     = Ioss::tokenize(names[which_name], suffix);
      size_t                   num_tokens = tokens.size();

      // The last token is the suffix for this component...
      Ioss::Suffix tmp(tokens[num_tokens - 1]);
      suffices.push_back(tmp);
    }
    const Ioss::VariableType *type = Ioss::VariableType::factory(suffices);
    return type;
  }

  Ioss::Field get_next_field(char **names, int num_names, size_t count,
                             Ioss::Field::RoleType fld_role, const char suffix_separator,
                             const int *truth_table)
  {
    // NOTE: 'names' are all lowercase at this point.

    // Assumption 1: To convert to a non-SCALAR type, the variable name
    // must have an field_suffix_sep in the name separating the suffixes from
    // the main name.

    // Find first unused name (used names have '\0' as first character...
    int  index       = 0;
    bool found_valid = false;
    for (index = 0; index < num_names; index++) {
      assert(truth_table == nullptr || truth_table[index] == 1 || truth_table[index] == 0);
      if ((truth_table == nullptr || truth_table[index] == 1) && names[index][0] != '\0') {
        found_valid = true;
        break;
      }
    }

    if (!found_valid) {
      // Return an invalid field...
      return Ioss::Field("", Ioss::Field::INVALID, IOSS_SCALAR(), fld_role, 1);
    }

    // At this point, name[index] should be a valid potential field
    // name and all names[i] with i < index are either already used or
    // not valid for this grouping entity (truth_table entry == 0).
    assert(index < num_names && names[index][0] != '\0' &&
           (truth_table == nullptr || truth_table[index] == 1));
    char *name = names[index];

    // Split the name up into tokens separated by the
    // 'suffix_separator'.  Note that the basename itself could
    // contain a suffix_separator (back_stress_xx or
    // back_stress_xx_01). Need to ignore embedded separators
    // (back_stress) and also recognize composite variable types
    // (back_stress_xx_01). At the current time, a composite variable
    // type can only contain two non-composite variable types, so we
    // only need to look to be concerned with the last 1 or 2 tokens...
    std::vector<std::string> tokens;
    field_tokenize(name, suffix_separator, tokens);
    size_t num_tokens = tokens.size();

    // Check that tokenizer did not return empty tokens...
    bool invalid = tokens[0].empty() || tokens[num_tokens - 1].empty();
    if (num_tokens == 1 || invalid) {
      // It is not a (Sierra-generated) name for a non-SCALAR variable
      // Return a SCALAR field
      Ioss::Field field(name, Ioss::Field::REAL, IOSS_SCALAR(), fld_role, count);
      field.set_index(index);
      names[index][0] = '\0';
      return field;
    }

    // KNOW: num_tokens > 1 at this point.  Possible that we still
    // just have a scalar with one or more embedded separator characters...
    int suffix_size = 1;
    if (num_tokens > 2) {
      suffix_size = 2;
    }

    // If num_tokens > 2, then we can potentially have a composite
    // variable type which would have a double suffix (_xx_01).

    // Gather all names which match in the first
    // (num_tokens-suffix_size) tokens and see if their suffices form
    // a valid variable type...
    while (suffix_size > 0) {
      Ioss::IntVector which_names; // Contains index of names that
      // potentially match as components
      // of a higher-order type.

      std::string base_name = tokens[0];
      for (size_t i = 1; i < num_tokens - suffix_size; i++) {
        base_name += suffix_separator;
        base_name += tokens[i];
      }
      base_name += suffix_separator;
      size_t bn_len = base_name.length(); // Length of basename portion only
      size_t length = std::strlen(name);  // Length of total name (with suffix)

      // Add the current name...
      which_names.push_back(index);

      // Gather all other names that are valid for this entity, and
      // have the same overall length and match in the first 'bn_len'
      // characters.
      //
      // Check that they have the same number of tokens,
      // It is possible that the first name(s) that match with two
      // suffices have a basename that match other names with only a
      // single suffix lc_cam_x, lc_cam_y, lc_sfarea.
      for (int i = index + 1; i < num_names; i++) {
        char *                   tst_name = names[i];
        std::vector<std::string> subtokens;
        field_tokenize(tst_name, suffix_separator, subtokens);
        if ((truth_table == nullptr || truth_table[i] == 1) && // Defined on this entity
            std::strlen(tst_name) == length &&                 // names must be same length
            std::strncmp(name, tst_name, bn_len) == 0 &&       // base portion must match
            subtokens.size() == num_tokens) {
          which_names.push_back(i);
        }
      }

      const Ioss::VariableType *type = nullptr;
      if (suffix_size == 2) {
        if (which_names.size() > 1) {
          type = match_composite_field(names, which_names, suffix_separator);
        }
      }
      else {
        assert(suffix_size == 1);
        type = match_single_field(names, which_names, suffix_separator);
      }

      if (type != nullptr) {
        // A valid variable type was recognized.
        // Mark the names which were used so they aren't used for another field on this entity.
        // Create a field of that variable type.
        assert(type->component_count() == static_cast<int>(which_names.size()));
        Ioss::Field field(base_name.substr(0, bn_len - 1), Ioss::Field::REAL, type, fld_role,
                          count);
        field.set_index(index);
        for (const auto &which_name : which_names) {
          names[which_name][0] = '\0';
        }
        return field;
      }
      if (suffix_size == 1) {
        Ioss::Field field(name, Ioss::Field::REAL, IOSS_SCALAR(), fld_role, count);
        field.set_index(index);
        names[index][0] = '\0';
        return field;
      }

      suffix_size--;
    }
    return Ioss::Field("", Ioss::Field::INVALID, IOSS_SCALAR(), fld_role, 1);
  }

  // common
  bool define_field(size_t nmatch, size_t match_length, char **names,
                    std::vector<Ioss::Suffix> &suffices, size_t entity_count,
                    Ioss::Field::RoleType fld_role, std::vector<Ioss::Field> &fields)
  {
    // Try to define a field of size 'nmatch' with the suffices in 'suffices'.
    // If this doesn't define a known field, then assume it is a scalar instead
    // and return false.
    if (nmatch > 1) {
      const Ioss::VariableType *type = Ioss::VariableType::factory(suffices);
      if (type == nullptr) {
        nmatch = 1;
      }
      else {
        char *name         = names[0];
        name[match_length] = '\0';
        Ioss::Field field(name, Ioss::Field::REAL, type, fld_role, entity_count);
        if (field.is_valid()) {
          fields.push_back(field);
        }
        for (size_t j = 0; j < nmatch; j++) {
          names[j][0] = '\0';
        }
        return true;
      }
    }

    // NOTE: nmatch could be reset inside previous if block.
    // This is not an 'else' block, it is a new if block.
    if (nmatch == 1) {
      Ioss::Field field(names[0], Ioss::Field::REAL, IOSS_SCALAR(), fld_role, entity_count);
      if (field.is_valid()) {
        fields.push_back(field);
      }
      names[0][0] = '\0';
      return false;
    }
    return false; // Can't get here...  Quiet the compiler
  }
} // namespace
// Read scalar fields off an input database and determine whether
// they are components of a higher order type (vector, tensor, ...).
// This routine is used if there is no field component separator.  E.g.,
// fieldx, fieldy, fieldz instead of field_x field_y field_z

void Ioss::Utils::get_fields(int64_t entity_count, // The number of objects in this entity.
                             char ** names,        // Raw list of field names from exodus
                             size_t  num_names,    // Number of names in list
                             Ioss::Field::RoleType fld_role, // Role of field
                             bool enable_field_recognition, const char suffix_separator,
                             int *local_truth, // Truth table for this entity;
                             // null if not applicable.
                             std::vector<Ioss::Field> &fields) // The fields that were found.
{
  if (!enable_field_recognition) {
    // Create a separate field for each name.
    for (size_t i = 0; i < num_names; i++) {
      if (local_truth == nullptr || local_truth[i] == 1) {
        Ioss::Field field(names[i], Ioss::Field::REAL, IOSS_SCALAR(), fld_role, entity_count);
        fields.push_back(field);
        names[i][0] = '\0';
      }
    }
  }
  else if (suffix_separator != 0) {
    while (true) {
      // NOTE: 'get_next_field' determines storage type (vector, tensor,...)
      Ioss::Field field =
          get_next_field(names, num_names, entity_count, fld_role, suffix_separator, local_truth);
      if (field.is_valid()) {
        fields.push_back(field);
      }
      else {
        break;
      }
    }
  }
  else {
    size_t                    nmatch = 1;
    size_t                    ibeg   = 0;
    size_t                    pmat   = 0;
    std::vector<Ioss::Suffix> suffices;
  top:

    while (ibeg + nmatch < num_names) {
      if (local_truth != nullptr) {
        while (ibeg < num_names && local_truth[ibeg] == 0) {
          ibeg++;
        }
      }
      for (size_t i = ibeg + 1; i < num_names; i++) {
        size_t mat = match(names[ibeg], names[i]);
        if (local_truth != nullptr && local_truth[i] == 0) {
          mat = 0;
        }

        // For all fields, the total length of the name is the same
        // for all components of that field.  The 'basename' of the
        // field will also be the same for all cases.
        //
        // It is possible that the length of the match won't be the
        // same for all components of a field since the match may
        // include a portion of the suffix; (sigxx, sigxy, sigyy
        // should match only 3 characters of the basename (sig), but
        // sigxx and sigxy will match 4 characters) so consider a
        // valid match if the match length is >= previous match length.
        if ((std::strlen(names[ibeg]) == std::strlen(names[i])) && mat > 0 &&
            (pmat == 0 || mat >= pmat)) {
          nmatch++;
          if (nmatch == 2) {
            // Get suffix for first field in the match
            pmat = mat;
            Ioss::Suffix tmp(&names[ibeg][pmat]);
            suffices.push_back(tmp);
          }
          // Get suffix for next fields in the match
          Ioss::Suffix tmp(&names[i][pmat]);
          suffices.push_back(tmp);
        }
        else {

          bool multi_component =
              define_field(nmatch, pmat, &names[ibeg], suffices, entity_count, fld_role, fields);
          if (!multi_component) {
            // Although we matched multiple suffices, it wasn't a
            // higher-order field, so we only used 1 name instead of
            // the 'nmatch' we thought we might use.
            i = ibeg + 1;
          }

          // Cleanout the suffices vector.
          clear(suffices);

          // Reset for the next time through the while loop...
          nmatch = 1;
          pmat   = 0;
          ibeg   = i;
          break;
        }
      }
    }
    // We've gone through the entire list of names; see if what we
    // have forms a multi-component field; if not, then define a
    // scalar field and jump up to the loop again to handle the others
    // that had been gathered.
    if (ibeg < num_names) {
      if (local_truth == nullptr || local_truth[ibeg] == 1) {
        bool multi_component =
            define_field(nmatch, pmat, &names[ibeg], suffices, entity_count, fld_role, fields);
        clear(suffices);
        if (nmatch > 1 && !multi_component) {
          ibeg++;
          goto top;
        }
      }
      else {
        ibeg++;
        goto top;
      }
    }
  }
}

std::string Ioss::Utils::platform_information()
{
#ifndef _WIN32
  struct utsname sys_info
  {
  };
  uname(&sys_info);
  std::string info =
      fmt::format("Node: {0}, OS: {1} {2}, {3}, Machine: {4}", sys_info.nodename, sys_info.sysname,
                  sys_info.release, sys_info.version, sys_info.machine);
#else
  std::string                 info = "Node: Unknown, OS: Unknown, Machine: Unknown";
#endif
  return info;
}

size_t Ioss::Utils::get_memory_info()
{
  // Code from http://nadeausoftware.com/sites/NadeauSoftware.com/files/getRSS.c
  size_t memory_usage = 0;
#if defined(_WIN32)
  /* Windows -------------------------------------------------- */
  PROCESS_MEMORY_COUNTERS info;
  GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info));
  memory_usage = (size_t)info.WorkingSetSize;

#elif defined(__APPLE__) && defined(__MACH__)
  kern_return_t               error;
  mach_msg_type_number_t      outCount;
  mach_task_basic_info_data_t taskinfo{};

  taskinfo.virtual_size = 0;
  outCount              = MACH_TASK_BASIC_INFO_COUNT;
  error                 = task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                    reinterpret_cast<task_info_t>(&taskinfo), &outCount);
  if (error == KERN_SUCCESS) {
    memory_usage = taskinfo.resident_size;
  }
#elif __linux__
#if defined(BGQ_LWK)
  uint64_t heap;
  Kernel_GetMemorySize(KERNEL_MEMSIZE_HEAP, &heap);
  memory_usage = heap;
#else
  // On Linux, the /proc pseudo-file system contains a directory for
  // each running or zombie process. The /proc/[pid]/stat,
  // /proc/[pid]/statm, and /proc/[pid]/status pseudo-files for the
  // process with id [pid] all contain a process's current resident
  // set size, among other things. But the /proc/[pid]/statm
  // pseudo-file is the easiest to read since it contains a single
  // line of text with white-space delimited values:
  //
  // * total program size
  // * resident set size
  // * shared pages
  // * text (code) size
  // * library size
  // * data size (heap + stack)
  // * dirty pages
  //
  // The second value provides the process's current resident set size
  // in pages. To get the field for the current process, open
  // /proc/self/statm and parse the second integer value. Multiply the
  // field by the page size from sysconf( ).

  long  rss = 0L;
  FILE *fp  = NULL;
  if ((fp = fopen("/proc/self/statm", "r")) == NULL)
    return (size_t)0L; // Can't open? */
  if (fscanf(fp, "%*s%ld", &rss) != 1) {
    fclose(fp);
    return (size_t)0L; // Can't read? */
  }
  fclose(fp);
  memory_usage = (size_t)rss * (size_t)sysconf(_SC_PAGESIZE);
#endif
#endif
  return memory_usage;
}

size_t Ioss::Utils::get_hwm_memory_info()
{
  // Code from http://nadeausoftware.com/sites/NadeauSoftware.com/files/getRSS.c
  size_t memory_usage = 0;
#if defined(_WIN32)
  /* Windows -------------------------------------------------- */
  PROCESS_MEMORY_COUNTERS info;
  GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info));
  memory_usage = (size_t)info.PeakWorkingSetSize;

#elif (defined(_AIX) || defined(__TOS__AIX__)) ||                                                  \
    (defined(__sun__) || defined(__sun) || defined(sun) && (defined(__SVR4) || defined(__svr4__)))
  /* AIX and Solaris ------------------------------------------ */
  struct psinfo psinfo;
  int           fd = -1;
  if ((fd = open("/proc/self/psinfo", O_RDONLY)) == -1)
    return (size_t)0L; /* Can't open? */
  if (read(fd, &psinfo, sizeof(psinfo)) != sizeof(psinfo)) {
    close(fd);
    return (size_t)0L; /* Can't read? */
  }
  close(fd);
  memory_usage = (size_t)(psinfo.pr_rssize * 1024L);

#elif (defined(__APPLE__) && defined(__MACH__)) || (defined(__linux__) && !defined(BGQ_LWK))
  /* BSD, Linux, and OSX -------------------------------------- */
  struct rusage rusage;
  getrusage(RUSAGE_SELF, &rusage);
#if defined(__APPLE__) && defined(__MACH__)
  memory_usage = (size_t)rusage.ru_maxrss;
#else
  memory_usage = (size_t)(rusage.ru_maxrss * 1024L);
#endif
#endif
  return memory_usage;
}

bool Ioss::Utils::block_is_omitted(Ioss::GroupingEntity *block)
{
  bool omitted = false;
  if (block->property_exists("omitted")) {
    omitted = (block->get_property("omitted").get_int() == 1);
  }
  return omitted;
}

void Ioss::Utils::calculate_sideblock_membership(IntVector &            face_is_member,
                                                 const Ioss::SideBlock *ef_blk,
                                                 size_t int_byte_size, const void *element,
                                                 const void *sides, int64_t number_sides,
                                                 const Ioss::Region *region)
{
  assert(ef_blk != nullptr);

  face_is_member.reserve(number_sides);

  const ElementTopology *unknown = Ioss::ElementTopology::factory("unknown");

  // Topology of faces in this face block...
  const ElementTopology *ftopo = ef_blk->topology();

  // Topology of parent element for faces in this face block
  const ElementTopology *parent_topo = ef_blk->parent_element_topology();

  // If split by element block then parent_block will be non-nullptr
  const ElementBlock *parent_block = ef_blk->parent_element_block();

  // The element block containing the face we are working on...
  Ioss::ElementBlock *block = nullptr;

  // Topology of face/edge in current element block
  const ElementTopology *common_ftopo = nullptr;

  // Topology of elements in the element block containing this element
  const ElementTopology *block_topo = nullptr;

  // Topology of the face we are currently working with...
  const ElementTopology *topo = nullptr;

  // The element side that the current face is on the element...
  int64_t current_side = -1;

  if (number_sides > 0 && (element == nullptr || sides == nullptr)) {
    std::ostringstream errmsg;
    fmt::print(errmsg, "INTERNAL ERROR: null element or sides pointer passed to {}.", __func__);
    IOSS_ERROR(errmsg);
  }

  for (int64_t iel = 0; iel < number_sides; iel++) {
    int64_t elem_id = 0;
    int64_t side_id = 0;
    if (int_byte_size == 4) {
      elem_id = ((int *)element)[iel];
      side_id = ((int *)sides)[iel];
    }
    else {
      elem_id = ((int64_t *)element)[iel];
      side_id = ((int64_t *)sides)[iel];
    }

    // Get the element block containing this face...
    if (block == nullptr || !block->contains(elem_id)) {
      block      = region->get_element_block(elem_id);
      block_topo = block->topology();
      // nullptr if hetero face/edge on element
      common_ftopo = block->topology()->boundary_type(0);
      if (common_ftopo != nullptr) {
        topo = common_ftopo;
      }
      current_side = -1;
    }

    // If the element topology of the element block containing this
    // face has heterogeneous topology (eg. wedge), then determine the
    // topology corresponding to the current side..
    if (common_ftopo == nullptr && side_id != current_side) {
      current_side = side_id;
      topo         = block->topology()->boundary_type(side_id);
    }

    bool face_topo_match  = ftopo == unknown || topo == ftopo;
    bool block_topo_match = parent_topo == unknown || block_topo == parent_topo;
    // See if the face topology and the parent element topology for
    // the current face match the topology associated with this face block.
    if (face_topo_match && block_topo_match && (parent_block == nullptr || parent_block == block) &&
        !block_is_omitted(block)) {
      // This face/edge  belongs in the face/edge block
      face_is_member.push_back(1);
    }
    else {
      face_is_member.push_back(0);
    }
  }
}

int64_t Ioss::Utils::get_side_offset(const Ioss::SideBlock *sb)
{

  const Ioss::ElementTopology *side_topo   = sb->topology();
  const Ioss::ElementTopology *parent_topo = sb->parent_element_topology();
  int64_t                      side_offset = 0;
  if ((side_topo != nullptr) && (parent_topo != nullptr)) {
    int side_topo_dim = side_topo->parametric_dimension();
    int elem_topo_dim = parent_topo->parametric_dimension();
    int elem_spat_dim = parent_topo->spatial_dimension();

    if (side_topo_dim + 1 < elem_spat_dim && side_topo_dim < elem_topo_dim) {
      side_offset = parent_topo->number_faces();
    }
  }
  return side_offset;
}

unsigned int Ioss::Utils::hash(const std::string &name)
{
  // Hash function from Aho, Sethi, Ullman "Compilers: Principles,
  // Techniques, and Tools.  Page 436

  const char * symbol = name.c_str();
  unsigned int hashval;
  unsigned int g;
  for (hashval = 0; *symbol != '\0'; symbol++) {
    hashval = (hashval << 4) + *symbol;
    g       = hashval & 0xf0000000;
    if (g != 0) {
      hashval = hashval ^ (g >> 24);
      hashval = hashval ^ g;
    }
  }
  return hashval;
}

double Ioss::Utils::timer()
{
  auto now = std::chrono::high_resolution_clock::now();
  return std::chrono::duration<double>(now - initial_time).count();
}

void Ioss::Utils::input_file(const std::string &file_name, std::vector<std::string> *lines,
                             size_t max_line_length)
{
  // Create an ifstream for the input file. This does almost the same
  // function as sierra::Env::input() except this is for a single
  // processor and the sierra::Env::input() is for parallel...

  if (!file_name.empty()) {
    // Open the file and read into the vector...
    std::string   input_line;
    std::ifstream infile(file_name);
    lines->push_back(file_name.substr(0, max_line_length));
    while (!std::getline(infile, input_line).fail()) {
      if (max_line_length == 0 || input_line.length() <= max_line_length) {
        lines->push_back(input_line);
      }
      else {
        // Split the line into pieces of length "max_line_length-1"
        // and append a "\" to all but the last. Don't worry about
        // splitting at whitespace...
        size_t ibeg = 0;
        do {
          std::string sub = input_line.substr(ibeg, max_line_length - 1);
          if (ibeg + max_line_length - 1 < input_line.length()) {
            sub += "\\";
          }
          lines->push_back(sub);
          ibeg += max_line_length - 1;
        } while (ibeg < input_line.length());
      }
    }
  }
}

bool Ioss::Utils::str_equal(const std::string &s1, const std::string &s2)
{
  return (s1.size() == s2.size()) &&
         std::equal(s1.begin(), s1.end(), s2.begin(),
                    [](char a, char b) { return std::tolower(a) == std::tolower(b); });
}

bool Ioss::Utils::substr_equal(const std::string &prefix, const std::string &str)
{
  return (str.size() >= prefix.size()) && str_equal(prefix, str.substr(0, prefix.size()));
}

std::string Ioss::Utils::uppercase(std::string name)
{
  std::transform(name.begin(), name.end(), name.begin(), [](char c) { return std::toupper(c); });
  return name;
}

std::string Ioss::Utils::lowercase(std::string name)
{
  std::transform(name.begin(), name.end(), name.begin(), [](char c) { return std::tolower(c); });
  return name;
}

bool Ioss::Utils::check_set_bool_property(const Ioss::PropertyManager &properties,
                                          const std::string &prop_name, bool &prop_value)
{
  bool found_property = false;
  if (properties.exists(prop_name)) {
    found_property = true;
    if (properties.get(prop_name).get_type() == Ioss::Property::INTEGER) {
      prop_value = properties.get(prop_name).get_int() != 0;
    }
    else {
      std::string yesno = Ioss::Utils::uppercase(properties.get(prop_name).get_string());
      if (yesno == "TRUE" || yesno == "YES" || yesno == "ON") {
        prop_value = true;
      }
      else if (yesno == "FALSE" || yesno == "NO" || yesno == "OFF") {
        prop_value = false;
      }
      else {
        std::ostringstream errmsg;
        fmt::print(errmsg,
                   "ERROR: Unrecognized value found for {}. "
                   "Found '{}' which is not one of TRUE|FALSE|YES|NO|ON|OFF",
                   prop_name, yesno);
        IOSS_ERROR(errmsg);
      }
    }
  }
  return found_property;
}

void Ioss::Utils::fixup_name(char *name)
{
  assert(name != nullptr);

  size_t len = std::strlen(name);
  for (size_t i = 0; i < len; i++) {
    name[i] = static_cast<char>(tolower(name[i])); // guaranteed(?) to be ascii...
    if (name[i] == ' ') {
      name[i] = '_';
    }
  }
}

void Ioss::Utils::fixup_name(std::string &name)
{
  name = Ioss::Utils::lowercase(name);

  size_t len = name.length();
  for (size_t i = 0; i < len; i++) {
    if (name[i] == ' ') {
      name[i] = '_';
    }
  }
}

namespace {

  /** \brief Hash function from Aho, Sethi, Ullman "Compilers: Principles,
   *         Techniques, and Tools.  Page 436
   */
  std::string two_letter_hash(const char *symbol)
  {
    const int    HASHSIZE = 673; // Largest prime less than 676 (26*26)
    unsigned int hashval;
    for (hashval = 0; *symbol != '\0'; symbol++) {
      hashval        = (hashval << 4) + *symbol;
      unsigned int g = hashval & 0xf0000000;
      if (g != 0) {
        hashval = hashval ^ (g >> 24);
        hashval = hashval ^ g;
      }
    }

    // Convert to base-26 'number'
    hashval %= HASHSIZE;
    char word[3] = {char(hashval / 26 + 'a'), char(hashval % 26 + 'a'), '\0'};
    return (std::string(word));
  }
} // namespace

std::string Ioss::Utils::variable_name_kluge(const std::string &name, size_t component_count,
                                             size_t copies, size_t max_var_len)
{

  // Width = 'max_var_len'.
  // Reserve space for suffix '_00...'
  // Reserve 3 for hash   '.xx'
  int hash_len = 3;
  int comp_len = 3; // _00
  int copy_len = 0;

  if (copies > 1) {
    assert(component_count % copies == 0);
    component_count /= copies;
  }

  if (component_count <= 1) {
    comp_len = 0;
  }
  else {
    comp_len = number_width(component_count) + 1; // _00000
  }

  if (copies <= 1) {
    copy_len = 0;
  }
  else {
    copy_len = number_width(copies) + 1; // _00000
  }

  size_t maxlen = max_var_len - comp_len - copy_len;

  std::string new_str = name;
  if (name.length() <= maxlen) {
    // If name fits without kluging, then just use name as it is
    // without adding on the hash...
    return lowercase(new_str);
  }
  // Know that the name is too long, try to shorten. Need room for
  // hash now.
  maxlen -= hash_len;
  int len = name.length();

  // Take last 'maxlen' characters.  Motivation for this is that the
  // beginning of the composed (or generated) variable name is the
  // names of the mechanics and mechanics instances in which this
  // variable is nested, so they will be similar for all variables at
  // the same scope and the differences will occur at the variable
  // name level...
  //
  // However, there will likely be variables at the
  // same level but in different scope that have the same name which
  // would cause a clash, so we *hope* that the hash will make those
  // scope names unique...
  std::string s = std::string(name).substr(len - maxlen, len);
  assert(s.length() <= maxlen);
  new_str = s;

  // NOTE: The hash is not added if the name is not shortened.
  std::string hash_string = two_letter_hash(name.c_str());
  new_str += std::string(".");
  new_str += hash_string;
  return lowercase(new_str);
}

void Ioss::Utils::generate_history_mesh(Ioss::Region *region)
{
  Ioss::DatabaseIO *db = region->get_database();
  if (db->parallel_rank() == 0) {
    region->begin_mode(Ioss::STATE_DEFINE_MODEL);

    // Node Block
    Ioss::NodeBlock *nb = new Ioss::NodeBlock(db, "nodeblock_1", 1, 3);
    region->add(nb);

    // Element Block
    Ioss::ElementBlock *eb = new Ioss::ElementBlock(db, "e1", "sphere", 1);
    eb->property_add(Ioss::Property("id", 1));
    eb->property_add(Ioss::Property("guid", 1));
    region->add(eb);
    region->end_mode(Ioss::STATE_DEFINE_MODEL);

    region->begin_mode(Ioss::STATE_MODEL);
    static double coord[3] = {1.1, 2.2, 3.3};
    static int    ids[1]   = {1};
    nb->put_field_data("ids", ids, sizeof(int));
    nb->put_field_data("mesh_model_coordinates", coord, 3 * sizeof(double));

    static int connect[1] = {1};
    eb->put_field_data("ids", ids, sizeof(int));
    eb->put_field_data("connectivity", connect, 1 * sizeof(int));

    region->end_mode(Ioss::STATE_MODEL);
  }
}

// Safer than Ioss::Utils::copy_string -- guarantees null termination
void Ioss::Utils::copy_string(char *dest, char const *source, size_t elements)
{
  char *d;
  for (d = dest; d + 1 < dest + elements && *source; d++, source++) {
    *d = *source;
  }
  *d = '\0';
}

namespace {
  const int tab64[64] = {63, 0,  58, 1,  59, 47, 53, 2,  60, 39, 48, 27, 54, 33, 42, 3,
                         61, 51, 37, 40, 49, 18, 28, 20, 55, 30, 34, 11, 43, 14, 22, 4,
                         62, 57, 46, 52, 38, 26, 32, 41, 50, 36, 17, 19, 29, 10, 13, 21,
                         56, 45, 25, 31, 35, 16, 9,  12, 44, 24, 15, 8,  23, 7,  6,  5};
} // namespace

int Ioss::Utils::log_power_2(uint64_t value)
{
  assert(value > 0);
  value = (value << 1) - 1;
  value |= value >> 1;
  value |= value >> 2;
  value |= value >> 4;
  value |= value >> 8;
  value |= value >> 16;
  value |= value >> 32;
  return tab64[(((value - (value >> 1)) * 0x07EDD5E59A4E28C2)) >> 58];
}

int Ioss::Utils::term_width()
{
  int cols = 0;
  if (isatty(fileno(stdout))) {
#if defined(TIOCGWINSZ)
    struct winsize ts;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ts);
    cols = ts.ws_col;
#elif TIOCGSIZE
    struct ttysize ts;
    ioctl(STDOUT_FILENO, TIOCGSIZE, &ts);
    cols = ts.ts_cols;
#endif /* TIOCGSIZE */
  }
  return cols != 0 ? cols : 100;
}

void Ioss::Utils::copy_database(Ioss::Region &region, Ioss::Region &output_region,
                                Ioss::MeshCopyOptions &options)
{
  DataPool data_pool;

  Ioss::DatabaseIO *dbi = region.get_database();

  int rank = dbi->util().parallel_rank();

  bool appending = output_region.get_database()->open_create_behavior() == Ioss::DB_APPEND;

  if (!appending) {

    if (options.debug && rank == 0) {
      fmt::print(Ioss::DEBUG(), "DEFINING MODEL ... \n");
    }
    dbi->progress("DEFINING MODEL");
    if (!output_region.begin_mode(Ioss::STATE_DEFINE_MODEL)) {
      if (options.verbose) {
        std::ostringstream errmsg;
        fmt::print(errmsg, "ERROR: Could not put output region into define model state\n");
        IOSS_ERROR(errmsg);
      }
    }

    // Get all properties of input database...
    transfer_properties(&region, &output_region);
    transfer_qa_info(region, output_region);

    transfer_nodeblock(region, output_region, data_pool, options, rank);

#ifdef SEACAS_HAVE_MPI
    // This also assumes that the node order and count is the same for input
    // and output regions... (This is checked during nodeset output)
    if (output_region.get_database()->needs_shared_node_information()) {
      if (options.ints_64_bit)
        set_owned_node_count(region, rank, (int64_t)0);
      else
        set_owned_node_count(region, rank, (int)0);
    }
#endif

    transfer_edgeblocks(region, output_region, options, rank);
    transfer_faceblocks(region, output_region, options, rank);
    transfer_elementblocks(region, output_region, options, rank);
    transfer_structuredblocks(region, output_region, options, rank);

    transfer_nodesets(region, output_region, options, rank);
    transfer_edgesets(region, output_region, options, rank);
    transfer_facesets(region, output_region, options, rank);
    transfer_elemsets(region, output_region, options, rank);

    transfer_sidesets(region, output_region, options, rank);
    transfer_commsets(region, output_region, options, rank);

    transfer_coordinate_frames(region, output_region);
    transfer_assemblies(region, output_region, options, rank);
    transfer_blobs(region, output_region, options, rank);

    if (options.debug && rank == 0) {
      fmt::print(Ioss::DEBUG(), "END STATE_DEFINE_MODEL...\n");
    }
    dbi->progress("END STATE_DEFINE_MODEL");

    output_region.end_mode(Ioss::STATE_DEFINE_MODEL);
    dbi->progress("output_region.end_mode(Ioss::STATE_DEFINE_MODEL) finished");

    if (options.verbose && rank == 0) {
      fmt::print(Ioss::DEBUG(), "Maximum Field size = {:n} bytes.\n", max_field_size);
    }
    data_pool.data.resize(max_field_size);
    if (options.verbose && rank == 0) {
      fmt::print(Ioss::DEBUG(), "Resize finished...\n");
    }

    if (options.debug && rank == 0) {
      fmt::print(Ioss::DEBUG(), "TRANSFERRING MESH FIELD DATA ...\n");
    }
    dbi->progress("TRANSFERRING MESH FIELD DATA ... ");

    // Model defined, now fill in the model data...
    output_region.begin_mode(Ioss::STATE_MODEL);

    // Transfer MESH field_data from input to output...
    // Transfer MESH field_data from input to output...
    bool node_major = output_region.node_major();

    if (!node_major) {
      transfer_field_data(region.get_element_blocks(), output_region, data_pool, Ioss::Field::MESH,
                          options);
      transfer_field_data(region.get_element_blocks(), output_region, data_pool,
                          Ioss::Field::ATTRIBUTE, options);
    }

    if (region.mesh_type() != Ioss::MeshType::STRUCTURED) {
      transfer_field_data(region.get_node_blocks(), output_region, data_pool, Ioss::Field::MESH,
                          options);
      transfer_field_data(region.get_node_blocks(), output_region, data_pool,
                          Ioss::Field::ATTRIBUTE, options);
    }

    if (node_major) {
      transfer_field_data(region.get_element_blocks(), output_region, data_pool, Ioss::Field::MESH,
                          options);
      transfer_field_data(region.get_element_blocks(), output_region, data_pool,
                          Ioss::Field::ATTRIBUTE, options);
    }

    // Structured Blocks -- Contain a NodeBlock that also needs its field data transferred...
    const auto &sbs = region.get_structured_blocks();
    for (const auto &isb : sbs) {
      const std::string &name = isb->name();
      if (options.debug && rank == 0) {
        fmt::print(Ioss::DEBUG(), "{}, ", name);
      }
      // Find matching output structured block
      Ioss::StructuredBlock *osb = output_region.get_structured_block(name);
      if (osb != nullptr) {
        transfer_field_data(isb, osb, data_pool, Ioss::Field::MESH, options);
        transfer_field_data(isb, osb, data_pool, Ioss::Field::ATTRIBUTE, options);

        auto &inb = isb->get_node_block();
        auto &onb = osb->get_node_block();
        if (options.debug && rank == 0) {
          fmt::print(Ioss::DEBUG(), "NB: {}, ", inb.name());
        }

        transfer_field_data(&inb, &onb, data_pool, Ioss::Field::MESH, options);
        transfer_field_data(&inb, &onb, data_pool, Ioss::Field::ATTRIBUTE, options);
      }
    }

    transfer_field_data(region.get_assemblies(), output_region, data_pool, Ioss::Field::MESH,
                        options);
    transfer_field_data(region.get_assemblies(), output_region, data_pool, Ioss::Field::ATTRIBUTE,
                        options);

    transfer_field_data(region.get_blobs(), output_region, data_pool, Ioss::Field::MESH, options);
    transfer_field_data(region.get_blobs(), output_region, data_pool, Ioss::Field::ATTRIBUTE,
                        options);

    transfer_field_data(region.get_edge_blocks(), output_region, data_pool, Ioss::Field::MESH,
                        options);
    transfer_field_data(region.get_edge_blocks(), output_region, data_pool, Ioss::Field::ATTRIBUTE,
                        options);

    transfer_field_data(region.get_face_blocks(), output_region, data_pool, Ioss::Field::MESH,
                        options);
    transfer_field_data(region.get_face_blocks(), output_region, data_pool, Ioss::Field::ATTRIBUTE,
                        options);

    transfer_field_data(region.get_nodesets(), output_region, data_pool, Ioss::Field::MESH,
                        options);
    transfer_field_data(region.get_nodesets(), output_region, data_pool, Ioss::Field::ATTRIBUTE,
                        options);

    transfer_field_data(region.get_edgesets(), output_region, data_pool, Ioss::Field::MESH,
                        options);
    transfer_field_data(region.get_edgesets(), output_region, data_pool, Ioss::Field::ATTRIBUTE,
                        options);

    transfer_field_data(region.get_facesets(), output_region, data_pool, Ioss::Field::MESH,
                        options);
    transfer_field_data(region.get_facesets(), output_region, data_pool, Ioss::Field::ATTRIBUTE,
                        options);

    transfer_field_data(region.get_elementsets(), output_region, data_pool, Ioss::Field::MESH,
                        options);
    transfer_field_data(region.get_elementsets(), output_region, data_pool, Ioss::Field::ATTRIBUTE,
                        options);

    transfer_field_data(region.get_commsets(), output_region, data_pool, Ioss::Field::MESH,
                        options);
    transfer_field_data(region.get_commsets(), output_region, data_pool, Ioss::Field::ATTRIBUTE,
                        options);
    transfer_field_data(region.get_commsets(), output_region, data_pool, Ioss::Field::COMMUNICATION,
                        options);

    // Side Sets
    if (region.mesh_type() == Ioss::MeshType::UNSTRUCTURED) {
      const auto &fss = region.get_sidesets();
      for (const auto &ifs : fss) {
        const std::string &name = ifs->name();
        if (options.debug && rank == 0) {
          fmt::print(Ioss::DEBUG(), "{}, ", name);
        }
        // Find matching output sideset
        Ioss::SideSet *ofs = output_region.get_sideset(name);

        if (ofs != nullptr) {
          transfer_field_data(ifs, ofs, data_pool, Ioss::Field::MESH, options);
          transfer_field_data(ifs, ofs, data_pool, Ioss::Field::ATTRIBUTE, options);

          const auto &fbs = ifs->get_side_blocks();
          for (const auto &ifb : fbs) {

            // Find matching output sideblock
            const std::string &fbname = ifb->name();
            if (options.debug && rank == 0) {
              fmt::print(Ioss::DEBUG(), "{}, ", fbname);
            }
            Ioss::SideBlock *ofb = ofs->get_side_block(fbname);

            if (ofb != nullptr) {
              transfer_field_data(ifb, ofb, data_pool, Ioss::Field::MESH, options);
              transfer_field_data(ifb, ofb, data_pool, Ioss::Field::ATTRIBUTE, options);
            }
          }
        }
      }
      if (options.debug && rank == 0) {
        fmt::print(Ioss::DEBUG(), "\n");
      }
    }
    if (options.debug && rank == 0) {
      fmt::print(Ioss::DEBUG(), "END STATE_MODEL... \n");
    }
    dbi->progress("END STATE_MODEL... ");
    output_region.end_mode(Ioss::STATE_MODEL);

    if (options.add_proc_id) {
      Ioss::Utils::clear(data_pool.data);
      add_proc_id(output_region, rank);
      return;
    }

    if (options.delete_timesteps) {
      Ioss::Utils::clear(data_pool.data);
      return;
    }
  } // !appending

  if (options.debug && rank == 0) {
    fmt::print(Ioss::DEBUG(), "DEFINING TRANSIENT FIELDS ... \n");
  }
  dbi->progress("DEFINING TRANSIENT FIELDS ... ");

  if (region.property_exists("state_count") && region.get_property("state_count").get_int() > 0) {
    if (options.verbose && rank == 0) {
      fmt::print(Ioss::DEBUG(), "\nNumber of time steps on database = {}\n\n",
                 region.get_property("state_count").get_int());
    }

    output_region.begin_mode(Ioss::STATE_DEFINE_TRANSIENT);

    // NOTE: For most types, the fields are transferred from input to output
    //       via the copy constructor.  The "special" ones are handled here.
    // The below lines handle both methods of handling global variables...
    transfer_fields(&region, &output_region, Ioss::Field::REDUCTION);
    transfer_fields(&region, &output_region, Ioss::Field::TRANSIENT);

    // Structured Blocks -- Contain a NodeBlock that also needs its fields transferred...
    const auto &sbs = region.get_structured_blocks();
    for (const auto &isb : sbs) {

      // Find matching output structured block
      const std::string &    name = isb->name();
      Ioss::StructuredBlock *osb  = output_region.get_structured_block(name);
      if (osb != nullptr) {
        transfer_fields(isb, osb, Ioss::Field::TRANSIENT);
        transfer_fields(isb, osb, Ioss::Field::REDUCTION);

        auto &inb = isb->get_node_block();
        auto &onb = osb->get_node_block();
        transfer_fields(&inb, &onb, Ioss::Field::TRANSIENT);
        transfer_fields(&inb, &onb, Ioss::Field::REDUCTION);
      }
    }

    if (options.debug && rank == 0) {
      fmt::print(Ioss::DEBUG(), "END STATE_DEFINE_TRANSIENT... \n");
    }
    dbi->progress("END STATE_DEFINE_TRANSIENT... ");
    output_region.end_mode(Ioss::STATE_DEFINE_TRANSIENT);
  }

  if (options.debug && rank == 0) {
    fmt::print(Ioss::DEBUG(), "TRANSFERRING TRANSIENT FIELDS ... \n");
  }
  dbi->progress("TRANSFERRING TRANSIENT FIELDS... ");

  output_region.begin_mode(Ioss::STATE_TRANSIENT);
  // Get the timesteps from the input database.  Step through them
  // and transfer fields to output database...

  int step_count = region.get_property("state_count").get_int();

  for (int istep = 1; istep <= step_count; istep++) {
    double time = region.get_state_time(istep);
    if (time < options.minimum_time) {
      continue;
    }
    if (time > options.maximum_time) {
      break;
    }

    int ostep = output_region.add_state(time);
    show_step(istep, time, options, rank);

    output_region.begin_state(ostep);
    region.begin_state(istep);

    for (int i = 0; i < 2; i++) {
      auto field_type = Ioss::Field::TRANSIENT;
      if (i > 0) {
        field_type = Ioss::Field::REDUCTION;
      }

      transfer_field_data(&region, &output_region, data_pool, field_type, options);

      transfer_field_data(region.get_assemblies(), output_region, data_pool, field_type, options);
      transfer_field_data(region.get_blobs(), output_region, data_pool, field_type, options);

      if (region.mesh_type() != Ioss::MeshType::STRUCTURED) {
        transfer_field_data(region.get_node_blocks(), output_region, data_pool, field_type,
                            options);
      }
      transfer_field_data(region.get_edge_blocks(), output_region, data_pool, field_type, options);
      transfer_field_data(region.get_face_blocks(), output_region, data_pool, field_type, options);
      transfer_field_data(region.get_element_blocks(), output_region, data_pool, field_type,
                          options);

      {
        // Structured Blocks -- handle embedded NodeBlock also.
        const auto &sbs = region.get_structured_blocks();
        for (const auto &isb : sbs) {
          const std::string &name = isb->name();
          if (options.debug && rank == 0) {
            fmt::print(Ioss::DEBUG(), "{}, ", name);
          }
          // Find matching output structured block
          Ioss::StructuredBlock *osb = output_region.get_structured_block(name);
          if (osb != nullptr) {
            transfer_field_data(isb, osb, data_pool, field_type, options);

            auto &inb = isb->get_node_block();
            auto &onb = osb->get_node_block();
            transfer_field_data(&inb, &onb, data_pool, field_type, options);
          }
        }
      }

      transfer_field_data(region.get_nodesets(), output_region, data_pool, field_type, options);
      transfer_field_data(region.get_edgesets(), output_region, data_pool, field_type, options);
      transfer_field_data(region.get_facesets(), output_region, data_pool, field_type, options);
      transfer_field_data(region.get_elementsets(), output_region, data_pool, field_type, options);

      // Side Sets
      {
        const auto &fss = region.get_sidesets();
        for (const auto &ifs : fss) {
          const std::string &name = ifs->name();
          if (options.debug && rank == 0) {
            fmt::print(Ioss::DEBUG(), "{}, ", name);
          }

          // Find matching output sideset
          Ioss::SideSet *ofs = output_region.get_sideset(name);
          if (ofs != nullptr) {
            transfer_field_data(ifs, ofs, data_pool, field_type, options);

            const auto &fbs = ifs->get_side_blocks();
            for (const auto &ifb : fbs) {

              // Find matching output sideblock
              const std::string &fbname = ifb->name();
              if (options.debug && rank == 0) {
                fmt::print(Ioss::DEBUG(), "{}, ", fbname);
              }

              Ioss::SideBlock *ofb = ofs->get_side_block(fbname);
              if (ofb != nullptr) {
                transfer_field_data(ifb, ofb, data_pool, field_type, options);
              }
            }
          }
        }
      }
    }
    region.end_state(istep);
    output_region.end_state(ostep);
    if (options.delay > 0.0) {
#ifndef _MSC_VER
      struct timespec delay;
      delay.tv_sec  = (int)options.delay;
      delay.tv_nsec = (options.delay - delay.tv_sec) * 1000000000L;
      nanosleep(&delay, nullptr);
#else
      Sleep((int)(options.delay * 1000));
#endif
    }
  }
  if (options.debug && rank == 0) {
    fmt::print(Ioss::DEBUG(), "END STATE_TRANSIENT... \n");
  }
  dbi->progress("END STATE_TRANSIENT (begin) ... ");

  output_region.end_mode(Ioss::STATE_TRANSIENT);
  dbi->progress("END STATE_TRANSIENT (end) ... ");
  Ioss::Utils::clear(data_pool.data);

  output_region.output_summary(std::cout);
}

namespace {
  template <typename T> void transfer_mesh_info(const T *input, T *output)
  {
    transfer_properties(input, output);
    transfer_fields(input, output, Ioss::Field::MESH);
    transfer_fields(input, output, Ioss::Field::ATTRIBUTE);
    transfer_fields(input, output, Ioss::Field::MESH_REDUCTION);
  }

  void transfer_nodeblock(Ioss::Region &region, Ioss::Region &output_region, DataPool &pool,
                          const Ioss::MeshCopyOptions &options, int rank)
  {
    const auto &nbs = region.get_node_blocks();
    for (const auto &inb : nbs) {
      const std::string &name = inb->name();
      if (options.debug && rank == 0) {
        fmt::print(Ioss::DEBUG(), "{}, ", name);
      }
      size_t num_nodes = inb->entity_count();
      size_t degree    = inb->get_property("component_degree").get_int();
      if (options.verbose && rank == 0) {
        fmt::print(Ioss::DEBUG(), " Number of Coordinates per Node = {:14n}\n", degree);
        fmt::print(Ioss::DEBUG(), " Number of Nodes                = {:14n}\n", num_nodes);
      }
      auto nb = new Ioss::NodeBlock(*inb);
      output_region.add(nb);

      if (output_region.get_database()->needs_shared_node_information()) {
        // If the "owning_processor" field exists on the input
        // nodeblock, transfer it and the "ids" field to the output
        // nodeblock at this time since it is used to determine
        // per-processor sizes of nodeblocks and nodesets.
        if (inb->field_exists("owning_processor")) {
          size_t isize = inb->get_field("ids").get_size();
          pool.data.resize(isize);
          inb->get_field_data("ids", pool.data.data(), isize);
          nb->put_field_data("ids", pool.data.data(), isize);

          isize = inb->get_field("owning_processor").get_size();
          pool.data.resize(isize);
          inb->get_field_data("owning_processor", pool.data.data(), isize);
          nb->put_field_data("owning_processor", pool.data.data(), isize);
        }
      }
    }
    if (options.debug && rank == 0) {
      fmt::print(Ioss::DEBUG(), "\n");
    }
  }

  template <typename T>
  void transfer_fields(const std::vector<T *> &entities, Ioss::Region &output_region,
                       Ioss::Field::RoleType role, const Ioss::MeshCopyOptions &options, int rank)
  {
    for (const auto &entity : entities) {
      const std::string &name = entity->name();
      if (options.debug && rank == 0) {
        fmt::print(Ioss::DEBUG(), "{}, ", name);
      }

      // Find the corresponding output node_block...
      Ioss::GroupingEntity *oeb = output_region.get_entity(name, entity->type());
      if (oeb != nullptr) {
        transfer_fields(entity, oeb, role);
      }
    }
    if (options.debug && rank == 0) {
      fmt::print(Ioss::DEBUG(), "\n");
    }
  }

  template <typename T>
  void transfer_field_data(const std::vector<T *> &entities, Ioss::Region &output_region,
                           DataPool &pool, Ioss::Field::RoleType role,
                           const Ioss::MeshCopyOptions &options)
  {
    for (const auto &entity : entities) {
      const std::string &name = entity->name();

      // Find the corresponding output block...
      Ioss::GroupingEntity *output = output_region.get_entity(name, entity->type());
      if (output != nullptr) {
        transfer_field_data(entity, output, pool, role, options);
      }
    }
  }

  template <typename T>
  void transfer_blocks(const std::vector<T *> &blocks, Ioss::Region &output_region,
                       const Ioss::MeshCopyOptions &options, int rank)
  {
    if (!blocks.empty()) {
      size_t total_entities = 0;
      for (const auto &iblock : blocks) {
        const std::string &name = iblock->name();
        if (options.debug && rank == 0) {
          fmt::print(Ioss::DEBUG(), "{}, ", name);
        }
        size_t count = iblock->entity_count();
        total_entities += count;

        auto block = new T(*iblock);
        output_region.add(block);
      }
      if (options.verbose && rank == 0) {
        fmt::print(Ioss::DEBUG(), " Number of {:20s} = {:14n}\n",
                   (*blocks.begin())->type_string() + "s", blocks.size());
        fmt::print(Ioss::DEBUG(), " Number of {:20s} = {:14n}\n",
                   (*blocks.begin())->contains_string() + "s", total_entities);
      }
      if (options.debug && rank == 0) {
        fmt::print(Ioss::DEBUG(), "\n");
      }
    }
  }

  void transfer_structuredblocks(Ioss::Region &region, Ioss::Region &output_region,
                                 const Ioss::MeshCopyOptions &options, int rank)
  {
    auto blocks = region.get_structured_blocks();
    if (!blocks.empty()) {
      size_t total_entities = 0;
      if (options.reverse) {
        // Defines the CGNS zones in the reverse order they
        // were read from the input mesh.  This is used in
        // testing to verify that we handle zone reordering
        // correctly.
        for (int i = blocks.size() - 1; i >= 0; i--) {
          const auto &       iblock = blocks[i];
          const std::string &name   = iblock->name();
          if (options.debug && rank == 0) {
            fmt::print(Ioss::DEBUG(), "{}, ", name);
          }
          size_t count = iblock->entity_count();
          total_entities += count;

          auto block = iblock->clone(output_region.get_database());
          output_region.add(block);
          transfer_mesh_info(iblock, block);

          // Now do the transfer on the NodeBlock contained in the StructuredBlock
          auto &inb = iblock->get_node_block();
          auto &onb = block->get_node_block();
          if (options.debug && rank == 0) {
            fmt::print(Ioss::DEBUG(), "(NB: {}), ", inb.name());
          }
          transfer_mesh_info(&inb, &onb);
        }
      }
      else {
        for (const auto &iblock : blocks) {
          const std::string &name = iblock->name();
          if (options.debug && rank == 0) {
            fmt::print(Ioss::DEBUG(), "{}, ", name);
          }
          size_t count = iblock->entity_count();
          total_entities += count;

          auto block = iblock->clone(output_region.get_database());
          output_region.add(block);
          transfer_mesh_info(iblock, block);

          // Now do the transfer on the NodeBlock contained in the StructuredBlock
          auto &inb = iblock->get_node_block();
          auto &onb = block->get_node_block();
          if (options.debug && rank == 0) {
            fmt::print(Ioss::DEBUG(), "(NB: {}), ", inb.name());
          }
          transfer_mesh_info(&inb, &onb);
        }
      }

      if (options.verbose && rank == 0) {
        fmt::print(Ioss::DEBUG(), " Number of {:20s} = {:14n}\n",
                   (*blocks.begin())->type_string() + "s", blocks.size());
        fmt::print(Ioss::DEBUG(), " Number of {:20s} = {:14n}\n",
                   (*blocks.begin())->contains_string() + "s", total_entities);
      }
      if (options.debug && rank == 0) {
        fmt::print(Ioss::DEBUG(), "\n");
      }
    }
  }

  void transfer_elementblocks(Ioss::Region &region, Ioss::Region &output_region,
                              const Ioss::MeshCopyOptions &options, int rank)
  {
    const auto &ebs = region.get_element_blocks();
    transfer_blocks(ebs, output_region, options, rank);
  }

  void transfer_edgeblocks(Ioss::Region &region, Ioss::Region &output_region,
                           const Ioss::MeshCopyOptions &options, int rank)
  {
    const auto &ebs = region.get_edge_blocks();
    transfer_blocks(ebs, output_region, options, rank);
  }

  void transfer_faceblocks(Ioss::Region &region, Ioss::Region &output_region,
                           const Ioss::MeshCopyOptions &options, int rank)
  {
    const auto &ebs = region.get_face_blocks();
    transfer_blocks(ebs, output_region, options, rank);
  }

  void transfer_sidesets(Ioss::Region &region, Ioss::Region &output_region,
                         const Ioss::MeshCopyOptions &options, int rank)
  {
    const auto &fss         = region.get_sidesets();
    size_t      total_sides = 0;
    for (const auto &ss : fss) {
      const std::string &name = ss->name();
      if (options.debug && rank == 0) {
        fmt::print(Ioss::DEBUG(), "{}, ", name);
      }
      auto surf = new Ioss::SideSet(*ss);
      output_region.add(surf);

      // Fix up the optional 'owner_block' in copied SideBlocks...
      const auto &fbs = ss->get_side_blocks();
      for (const auto &ifb : fbs) {
        if (ifb->parent_block() != nullptr) {
          auto               fb_name = ifb->parent_block()->name();
          Ioss::EntityBlock *parent =
              dynamic_cast<Ioss::EntityBlock *>(output_region.get_entity(fb_name));

          Ioss::SideBlock *ofb = surf->get_side_block(ifb->name());
          ofb->set_parent_block(parent);
        }
      }
    }

    if (options.verbose && rank == 0 && !fss.empty()) {
      fmt::print(Ioss::DEBUG(), " Number of {:20s} = {:14n}\n", (*fss.begin())->type_string() + "s",
                 fss.size());
      fmt::print(Ioss::DEBUG(), " Number of {:20s} = {:14n}\n",
                 (*fss.begin())->contains_string() + "s", total_sides);
    }
    if (options.debug && rank == 0) {
      fmt::print(Ioss::DEBUG(), "\n");
    }
  }

  template <typename T>
  void transfer_sets(const std::vector<T *> &sets, Ioss::Region &output_region,
                     const Ioss::MeshCopyOptions &options, int rank)
  {
    if (!sets.empty()) {
      size_t total_entities = 0;
      for (const auto &set : sets) {
        const std::string &name = set->name();
        if (options.debug && rank == 0) {
          fmt::print(Ioss::DEBUG(), "{}, ", name);
        }
        size_t count = set->entity_count();
        total_entities += count;
        auto o_set = new T(*set);
        output_region.add(o_set);
      }

      if (options.verbose && rank == 0) {
        fmt::print(Ioss::DEBUG(), " Number of {:20s} = {:14n}",
                   (*sets.begin())->type_string() + "s", sets.size());
        fmt::print(Ioss::DEBUG(), "\tLength of entity list = {:14n}\n", total_entities);
      }
      if (options.debug && rank == 0) {
        fmt::print(Ioss::DEBUG(), "\n");
      }
    }
  }

  void transfer_assemblies(Ioss::Region &region, Ioss::Region &output_region,
                           const Ioss::MeshCopyOptions &options, int rank)
  {
    const auto &assem = region.get_assemblies();
    if (!assem.empty()) {
      for (const auto &assm : assem) {
        const std::string &name = assm->name();
        if (options.debug && rank == 0) {
          fmt::print(stderr, "{}, ", name);
        }
        auto o_assem = new Ioss::Assembly(*assm);
        output_region.add(o_assem);
      }

      if (options.verbose && rank == 0) {
        fmt::print(stderr, " Number of {:20s} = {:14n}\n", "Assemblies", assem.size());
      }
      if (options.debug && rank == 0) {
        fmt::print(stderr, "\n");
      }
    }
  }

  void transfer_blobs(Ioss::Region &region, Ioss::Region &output_region,
                      const Ioss::MeshCopyOptions &options, int rank)
  {
    const auto &blobs = region.get_blobs();
    if (!blobs.empty()) {
      size_t total_entities = 0;
      for (const auto &blob : blobs) {
        const std::string &name = blob->name();
        if (options.debug && rank == 0) {
          fmt::print(stderr, "{}, ", name);
        }
        size_t count = blob->entity_count();
        total_entities += count;
        auto o_blob = new Ioss::Blob(*blob);
        output_region.add(o_blob);
      }

      if (options.verbose && rank == 0) {
        fmt::print(stderr, " Number of {:20s} = {:14n}", (*blobs.begin())->type_string() + "s",
                   blobs.size());
        fmt::print(stderr, "\tLength of entity list = {:14n}\n", total_entities);
      }
      if (options.debug && rank == 0) {
        fmt::print(Ioss::DEBUG(), "\n");
      }
    }
  }

  void transfer_nodesets(Ioss::Region &region, Ioss::Region &output_region,
                         const Ioss::MeshCopyOptions &options, int rank)
  {
    const auto &nss = region.get_nodesets();
    transfer_sets(nss, output_region, options, rank);
  }

  void transfer_edgesets(Ioss::Region &region, Ioss::Region &output_region,
                         const Ioss::MeshCopyOptions &options, int rank)
  {
    const auto &nss = region.get_edgesets();
    transfer_sets(nss, output_region, options, rank);
  }

  void transfer_facesets(Ioss::Region &region, Ioss::Region &output_region,
                         const Ioss::MeshCopyOptions &options, int rank)
  {
    const auto &nss = region.get_facesets();
    transfer_sets(nss, output_region, options, rank);
  }

  void transfer_elemsets(Ioss::Region &region, Ioss::Region &output_region,
                         const Ioss::MeshCopyOptions &options, int rank)
  {
    const auto &nss = region.get_elementsets();
    transfer_sets(nss, output_region, options, rank);
  }

  void transfer_commsets(Ioss::Region &region, Ioss::Region &output_region,
                         const Ioss::MeshCopyOptions &options, int rank)
  {
    const auto &css = region.get_commsets();
    for (const auto &ics : css) {
      const std::string &name = ics->name();
      if (options.debug && rank == 0) {
        fmt::print(Ioss::DEBUG(), "{}, ", name);
      }
      auto cs = new Ioss::CommSet(*ics);
      output_region.add(cs);
    }
    if (options.debug && rank == 0) {
      fmt::print(Ioss::DEBUG(), "\n");
    }
  }

  void transfer_coordinate_frames(Ioss::Region &region, Ioss::Region &output_region)
  {
    const Ioss::CoordinateFrameContainer &cf = region.get_coordinate_frames();
    for (const auto &frame : cf) {
      output_region.add(frame);
    }
  }

  void transfer_fields(const Ioss::GroupingEntity *ige, Ioss::GroupingEntity *oge,
                       Ioss::Field::RoleType role, const std::string &prefix)
  {
    // Check for transient fields...
    Ioss::NameList fields;
    ige->field_describe(role, &fields);

    // Iterate through results fields and transfer to output
    // database...  If a prefix is specified, only transfer fields
    // whose names begin with the prefix
    for (const auto &field_name : fields) {
      Ioss::Field field = ige->get_field(field_name);
      if (field.get_size() > max_field_size) {
        max_field_size = field.get_size();
      }
      if (field_name != "ids" && !oge->field_exists(field_name) &&
          Ioss::Utils::substr_equal(prefix, field_name)) {
        // If the field does not already exist, add it to the output node block
        oge->field_add(field);
      }
    }
  }

  void transfer_field_data(Ioss::GroupingEntity *ige, Ioss::GroupingEntity *oge, DataPool &pool,
                           Ioss::Field::RoleType role, const Ioss::MeshCopyOptions &options,
                           const std::string &prefix)
  {
    // Iterate through the TRANSIENT-role fields of the input
    // database and transfer to output database.
    Ioss::NameList state_fields;
    ige->field_describe(role, &state_fields);

    // Complication here is that if the 'role' is 'Ioss::Field::MESH',
    // then the 'ids' field must be transferred first...
    if (role == Ioss::Field::MESH && ige->field_exists("ids")) {
      assert(oge->field_exists("ids"));
      transfer_field_data_internal(ige, oge, pool, "ids", options);
    }

    for (const auto &field_name : state_fields) {
      // All of the 'Ioss::EntityBlock' derived classes have a
      // 'connectivity' field, but it is only interesting on the
      // Ioss::ElementBlock class. On the other classes, it just
      // generates overhead...
      if (field_name == "connectivity" && ige->type() != Ioss::ELEMENTBLOCK) {
        continue;
      }
      if (field_name == "ids") {
        continue;
      }
      if (Ioss::Utils::substr_equal(prefix, field_name)) {
        assert(oge->field_exists(field_name));
        transfer_field_data_internal(ige, oge, pool, field_name, options);
      }
    }
  }

  void transfer_field_data_internal(Ioss::GroupingEntity *ige, Ioss::GroupingEntity *oge,
                                    DataPool &pool, const std::string &field_name,
                                    const Ioss::MeshCopyOptions &options)
  {

    size_t isize = ige->get_field(field_name).get_size();
    assert(isize == oge->get_field(field_name).get_size());

    int basic_type = ige->get_field(field_name).get_type();

    if (field_name == "mesh_model_coordinates_x") {
      return;
    }
    if (field_name == "mesh_model_coordinates_y") {
      return;
    }
    if (field_name == "mesh_model_coordinates_z") {
      return;
    }
    if (field_name == "connectivity_raw") {
      return;
    }
    if (field_name == "element_side_raw") {
      return;
    }
    if (field_name == "ids_raw") {
      return;
    }
    if (field_name == "implicit_ids") {
      return;
    }
    if (field_name == "node_connectivity_status") {
      return;
    }
    if (field_name == "owning_processor") {
      return;
    }
    if (field_name == "entity_processor_raw") {
      return;
    }
    if (field_name == "ids" && ige->type() == Ioss::SIDEBLOCK) {
      return;
    }
    if (field_name == "ids" && ige->type() == Ioss::STRUCTUREDBLOCK) {
      return;
    }
    if (field_name == "cell_ids" && ige->type() == Ioss::STRUCTUREDBLOCK) {
      return;
    }
    if (field_name == "cell_node_ids" && ige->type() == Ioss::STRUCTUREDBLOCK) {
      return;
    }

    if (options.data_storage_type == 1 || options.data_storage_type == 2) {
      if (pool.data.size() < isize) {
        pool.data.resize(isize);
      }
    }
    else {
    }

    assert(pool.data.size() >= isize);

    switch (options.data_storage_type) {
    case 1: ige->get_field_data(field_name, pool.data.data(), isize); break;
    case 2:
      if ((basic_type == Ioss::Field::CHARACTER) || (basic_type == Ioss::Field::STRING)) {
        ige->get_field_data(field_name, pool.data);
      }
      else if (basic_type == Ioss::Field::INT32) {
        ige->get_field_data(field_name, pool.data_int);
      }
      else if (basic_type == Ioss::Field::INT64) {
        ige->get_field_data(field_name, pool.data_int64);
      }
      else if (basic_type == Ioss::Field::REAL) {
        ige->get_field_data(field_name, pool.data_double);
      }
      else if (basic_type == Ioss::Field::COMPLEX) {
        ige->get_field_data(field_name, pool.data_complex);
      }
      else {
      }
      break;
#ifdef SEACAS_HAVE_KOKKOS
    case 3:
      if ((basic_type == Ioss::Field::CHARACTER) || (basic_type == Ioss::Field::STRING)) {
        ige->get_field_data<char>(field_name, pool.data_view_char);
      }
      else if (basic_type == Ioss::Field::INT32) {
        ige->get_field_data<int>(field_name, pool.data_view_int);
      }
      else if (basic_type == Ioss::Field::INT64) {
        ige->get_field_data<int64_t>(field_name, pool.data_view_int64);
      }
      else if (basic_type == Ioss::Field::REAL) {
        ige->get_field_data<double>(field_name, pool.data_view_double);
      }
      else if (basic_type == Ioss::Field::COMPLEX) {
        // Since data_view_complex cannot be a global variable.
        ige->get_field_data(field_name, pool.data.data(), isize);
      }
      else {
      }
      break;
    case 4:
      if ((basic_type == Ioss::Field::CHARACTER) || (basic_type == Ioss::Field::STRING)) {
        ige->get_field_data<char>(field_name, pool.data_view_2D_char);
      }
      else if (basic_type == Ioss::Field::INT32) {
        ige->get_field_data<int>(field_name, pool.data_view_2D_int);
      }
      else if (basic_type == Ioss::Field::INT64) {
        ige->get_field_data<int64_t>(field_name, pool.data_view_2D_int64);
      }
      else if (basic_type == Ioss::Field::REAL) {
        ige->get_field_data<double>(field_name, pool.data_view_2D_double);
      }
      else if (basic_type == Ioss::Field::COMPLEX) {
        // Since data_view_complex cannot be a global variable.
        ige->get_field_data(field_name, pool.data.data(), isize);
      }
      else {
      }
      break;
    case 5:
      if ((basic_type == Ioss::Field::CHARACTER) || (basic_type == Ioss::Field::STRING)) {
        ige->get_field_data<char, Kokkos::LayoutRight, Kokkos::HostSpace>(
            field_name, pool.data_view_2D_char_layout_space);
      }
      else if (basic_type == Ioss::Field::INT32) {
        ige->get_field_data<int, Kokkos::LayoutRight, Kokkos::HostSpace>(
            field_name, pool.data_view_2D_int_layout_space);
      }
      else if (basic_type == Ioss::Field::INT64) {
        ige->get_field_data<int64_t, Kokkos::LayoutRight, Kokkos::HostSpace>(
            field_name, pool.data_view_2D_int64_layout_space);
      }
      else if (basic_type == Ioss::Field::REAL) {
        ige->get_field_data<double, Kokkos::LayoutRight, Kokkos::HostSpace>(
            field_name, pool.data_view_2D_double_layout_space);
      }
      else if (basic_type == Ioss::Field::COMPLEX) {
        // Since data_view_complex cannot be a global variable.
        ige->get_field_data(field_name, pool.data.data(), isize);
      }
      else {
      }
      break;
#endif
    default:
      if (field_name == "mesh_model_coordinates") {
        fmt::print(Ioss::DEBUG(), "data_storage option not recognized.");
      }
      return;
    }

    switch (options.data_storage_type) {
    case 1: oge->put_field_data(field_name, pool.data.data(), isize); break;
    case 2:
      if ((basic_type == Ioss::Field::CHARACTER) || (basic_type == Ioss::Field::STRING)) {
        oge->put_field_data(field_name, pool.data);
      }
      else if (basic_type == Ioss::Field::INT32) {
        oge->put_field_data(field_name, pool.data_int);
      }
      else if (basic_type == Ioss::Field::INT64) {
        oge->put_field_data(field_name, pool.data_int64);
      }
      else if (basic_type == Ioss::Field::REAL) {
        oge->put_field_data(field_name, pool.data_double);
      }
      else if (basic_type == Ioss::Field::COMPLEX) {
        oge->put_field_data(field_name, pool.data_complex);
      }
      else {
      }
      break;
#ifdef SEACAS_HAVE_KOKKOS
    case 3:
      if ((basic_type == Ioss::Field::CHARACTER) || (basic_type == Ioss::Field::STRING)) {
        oge->put_field_data<char>(field_name, pool.data_view_char);
      }
      else if (basic_type == Ioss::Field::INT32) {
        oge->put_field_data<int>(field_name, pool.data_view_int);
      }
      else if (basic_type == Ioss::Field::INT64) {
        oge->put_field_data<int64_t>(field_name, pool.data_view_int64);
      }
      else if (basic_type == Ioss::Field::REAL) {
        oge->put_field_data<double>(field_name, pool.data_view_double);
      }
      else if (basic_type == Ioss::Field::COMPLEX) {
        // Since data_view_complex cannot be a global variable.
        oge->put_field_data(field_name, pool.data.data(), isize);
      }
      else {
      }
      break;
    case 4:
      if ((basic_type == Ioss::Field::CHARACTER) || (basic_type == Ioss::Field::STRING)) {
        oge->put_field_data<char>(field_name, pool.data_view_2D_char);
      }
      else if (basic_type == Ioss::Field::INT32) {
        oge->put_field_data<int>(field_name, pool.data_view_2D_int);
      }
      else if (basic_type == Ioss::Field::INT64) {
        oge->put_field_data<int64_t>(field_name, pool.data_view_2D_int64);
      }
      else if (basic_type == Ioss::Field::REAL) {
        oge->put_field_data<double>(field_name, pool.data_view_2D_double);
      }
      else if (basic_type == Ioss::Field::COMPLEX) {
        // Since data_view_complex cannot be a global variable.
        oge->put_field_data(field_name, pool.data.data(), isize);
      }
      else {
      }
      break;
    case 5:
      if ((basic_type == Ioss::Field::CHARACTER) || (basic_type == Ioss::Field::STRING)) {
        oge->put_field_data<char, Kokkos::LayoutRight, Kokkos::HostSpace>(
            field_name, pool.data_view_2D_char_layout_space);
      }
      else if (basic_type == Ioss::Field::INT32) {
        oge->put_field_data<int, Kokkos::LayoutRight, Kokkos::HostSpace>(
            field_name, pool.data_view_2D_int_layout_space);
      }
      else if (basic_type == Ioss::Field::INT64) {
        oge->put_field_data<int64_t, Kokkos::LayoutRight, Kokkos::HostSpace>(
            field_name, pool.data_view_2D_int64_layout_space);
      }
      else if (basic_type == Ioss::Field::REAL) {
        oge->put_field_data<double, Kokkos::LayoutRight, Kokkos::HostSpace>(
            field_name, pool.data_view_2D_double_layout_space);
      }
      else if (basic_type == Ioss::Field::COMPLEX) {
        // Since data_view_complex cannot be a global variable.
        oge->put_field_data(field_name, pool.data.data(), isize);
      }
      else {
      }
      break;
#endif
    default: return;
    }
  }

  void transfer_qa_info(Ioss::Region &in, Ioss::Region &out)
  {
    out.add_information_records(in.get_information_records());

    const std::vector<std::string> &qa = in.get_qa_records();
    for (size_t i = 0; i < qa.size(); i += 4) {
      out.add_qa_record(qa[i + 0], qa[i + 1], qa[i + 2], qa[i + 3]);
    }
  }

  void transfer_properties(const Ioss::GroupingEntity *ige, Ioss::GroupingEntity *oge)
  {
    Ioss::NameList properties;
    ige->property_describe(&properties);

    // Iterate through properties and transfer to output database...
    for (const auto &property : properties) {
      if (!oge->property_exists(property)) {
        oge->property_add(ige->get_property(property));
      }
    }
  }

  void show_step(int istep, double time, const Ioss::MeshCopyOptions &options, int rank)
  {
    if (options.verbose && rank == 0) {
      fmt::print(Ioss::DEBUG(), "\r\tTime step {:5d} at time {:10.5e}", istep, time);
    }
  }

  template <typename INT>
  void set_owned_node_count(Ioss::Region &region, int my_processor, INT /*dummy*/)
  {
    Ioss::NodeBlock *nb = region.get_node_block("nodeblock_1");
    if (nb->field_exists("owning_processor")) {
      std::vector<int> my_data;
      nb->get_field_data("owning_processor", my_data);

      INT owned = std::count(my_data.begin(), my_data.end(), my_processor);
      nb->property_add(Ioss::Property("locally_owned_count", owned));

      // Set locally_owned_count property on all nodesets...
      const Ioss::NodeSetContainer &nss = region.get_nodesets();
      for (auto ns : nss) {

        std::vector<INT> ids;
        ns->get_field_data("ids_raw", ids);
        owned = 0;
        for (size_t n = 0; n < ids.size(); n++) {
          INT id = ids[n];
          if (my_data[id - 1] == my_processor) {
            ++owned;
          }
        }
        ns->property_add(Ioss::Property("locally_owned_count", owned));
      }
    }
  }

  void add_proc_id(Ioss::Region &region, int rank)
  {
    region.begin_mode(Ioss::STATE_DEFINE_TRANSIENT);
    auto &sblocks = region.get_structured_blocks();
    for (auto &sb : sblocks) {
      sb->field_add(
          Ioss::Field("processor_id", Ioss::Field::REAL, "scalar", Ioss::Field::TRANSIENT));
    }

    auto &eblocks = region.get_element_blocks();
    for (auto &eb : eblocks) {
      eb->field_add(
          Ioss::Field("processor_id", Ioss::Field::REAL, "scalar", Ioss::Field::TRANSIENT));
    }
    region.end_mode(Ioss::STATE_DEFINE_TRANSIENT);

    region.begin_mode(Ioss::STATE_TRANSIENT);

    auto step = region.add_state(0.0);
    region.begin_state(step);

    for (auto &sb : sblocks) {
      std::vector<double> proc_id(sb->entity_count(), rank);
      sb->put_field_data("processor_id", proc_id);
    }

    for (auto &eb : eblocks) {
      std::vector<double> proc_id(eb->entity_count(), rank);
      eb->put_field_data("processor_id", proc_id);
    }

    region.end_state(step);
    region.end_mode(Ioss::STATE_TRANSIENT);
  }
} // namespace
