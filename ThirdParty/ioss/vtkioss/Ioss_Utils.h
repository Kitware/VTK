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

#ifndef IOSS_Ioss_Utils_h
#define IOSS_Ioss_Utils_h

#include "vtk_ioss_mangle.h"

#include <Ioss_CodeTypes.h>
#include <Ioss_Field.h>
#include <algorithm> // for sort, lower_bound, copy, etc
#include <cassert>
#include <cmath>
#include <cstddef>   // for size_t
#include <cstdint>   // for int64_t
#include <cstdlib>   // for nullptrr
#include <iostream>  // for ostringstream, etcstream, etc
#include <stdexcept> // for runtime_error
#include <string>    // for string
#include <vector>    // for vector
namespace Ioss {
  class Field;
  class GroupingEntity;
  class Region;
  class SideBlock;
  class PropertyManager;
  struct MeshCopyOptions;
} // namespace Ioss

#define IOSS_ERROR(errmsg) throw std::runtime_error((errmsg).str())

namespace {
  // SEE: http://lemire.me/blog/2017/04/10/removing-duplicates-from-lists-quickly
  template <typename T> size_t unique(std::vector<T> &out, bool skip_first)
  {
    if (out.empty())
      return 0;
    size_t i    = 1;
    size_t pos  = 1;
    T      oldv = out[0];
    if (skip_first) {
      i    = 2;
      pos  = 2;
      oldv = out[1];
    }
    for (; i < out.size(); ++i) {
      T newv   = out[i];
      out[pos] = newv;
      pos += (newv != oldv);
      oldv = newv;
    }
    return pos;
  }
} // namespace

namespace Ioss {
  /* \brief Utility methods.
   */
  class Utils
  {
  public:
    Utils()  = default;
    ~Utils() = default;

    /**
     * \defgroup IossStreams Streams used for IOSS output
     *@{
     */
    static std::ostream
        *m_outputStream; ///< general informational output (very rare). Default std::cerr
    static std::ostream *m_debugStream;   ///< debug output when requested. Default std::cerr
    static std::ostream *m_warningStream; ///< IOSS warning output. Default std::cerr
    static std::string m_preWarningText;  ///< is a string that prepends all warning message output.
                                          ///< Default is "\nIOSS WARNING: "

    /** \brief set the stream for all streams (output, debug, and warning) to the specified
     * `out_stream`
     */
    static void set_all_streams(std::ostream &out_stream)
    {
      m_outputStream  = &out_stream;
      m_debugStream   = &out_stream;
      m_warningStream = &out_stream;
    }

    /** \brief set the output stream to the specified `output_stream`
     */
    static void set_output_stream(std::ostream &output_stream) { m_outputStream = &output_stream; }

    /** \brief set the debug stream to the specified `debug_stream`
     */
    static void set_debug_stream(std::ostream &debug_stream) { m_debugStream = &debug_stream; }

    /** \brief set the warning stream to the specified `warning_stream`
     */
    static void set_warning_stream(std::ostream &warning_stream)
    {
      m_warningStream = &warning_stream;
    }

    /** \brief set the pre-warning text
     * Sets the text output prior to a warning to the specified text.
     * Pass an empty string to disable this.  Default is `"\nIOSS WARNING: "`
     */
    static void set_pre_warning_text(const std::string &text) { m_preWarningText = text; }
    /** @}*/

    static void check_dynamic_cast(const void *ptr)
    {
      if (ptr == nullptr) {
        std::ostringstream errmsg;
        errmsg << "INTERNAL ERROR: Invalid dynamic cast returned nullptr\n";
        IOSS_ERROR(errmsg);
      }
    }

    template <typename T> static void uniquify(std::vector<T> &vec, bool skip_first = false)
    {
      auto it = vec.begin();
      if (skip_first) {
        it++;
      }
      std::sort(it, vec.end());
      vec.resize(unique(vec, skip_first));
      vec.shrink_to_fit();
    }

    template <typename T> static void generate_index(std::vector<T> &index)
    {
      T sum = 0;
      for (size_t i = 0; i < index.size() - 1; i++) {
        T cnt    = index[i];
        index[i] = sum;
        sum += cnt;
      }
      index.back() = sum;
    }

    template <typename T> static T find_index_location(T node, const std::vector<T> &index)
    {
      // 0-based node numbering
      // index[p] = first node (0-based) on processor p

#if 1
      // Assume data coherence.  I.e., a new search will be close to the
      // previous search.
      static size_t prev = 1;

      size_t nproc = index.size();
      if (prev < nproc && index[prev - 1] <= node && index[prev] > node) {
        return prev - 1;
      }

      for (size_t p = 1; p < nproc; p++) {
        if (index[p] > node) {
          prev = p;
          return p - 1;
        }
      }
      std::ostringstream errmsg;
      errmsg << "FATAL ERROR: find_index_location. Searching for " << node << " in:\n";
      for (auto idx : index) {
        errmsg << idx << ", ";
      }
      errmsg << "\n";
      IOSS_ERROR(errmsg);
#else
      return std::distance(index.begin(), std::upper_bound(index.begin(), index.end(), node)) - 1;
#endif
    }

    static void copy_string(char *dest, char const *source, size_t elements);

    static void copy_string(char *dest, const std::string &source, size_t elements)
    {
      copy_string(dest, source.c_str(), elements);
    }

    template <size_t size> static void copy_string(char (&output)[size], const std::string &source)
    {
      copy_string(output, source.c_str(), size);
    }

    template <size_t size> static void copy_string(char (&output)[size], const char *source)
    {
      // Copy the string — don’t copy too many bytes.
      copy_string(output, source, size);
    }

    template <typename T> static void clear(std::vector<T> &vec)
    {
      vec.clear();
      vec.shrink_to_fit();
      assert(vec.capacity() == 0);
    }

    /**
     * Returns the number of digits required to print the number.
     * If `use_commas` is specified, then the width will be adjusted
     * to account for the comma used every 3 digits.
     * (1,234,567,890 would return 13)
     * Typically used with the `fmt::print()` functions as:
     * ```
     * fmt::print("{:{}n}", number, number_width(number,true))
     * fmt::print("{:{}d}", number, number_width(number,false))
     * ```
     */
    inline static int number_width(const size_t number, bool use_commas = false)
    {
      if (number == 0) {
        return 1;
      }
      int width = int(std::floor(std::log10(number))) + 1;
      if (use_commas) {
        width += ((width - 1) / 3);
      }
      return width;
    }

    inline static int power_2(int count)
    {
      // Return the power of two which is equal to or greater than `count`
      // count = 15 -> returns 16
      // count = 16 -> returns 16
      // count = 17 -> returns 32

      // Use brute force...
      int pow2 = 1;
      while (pow2 < count) {
        pow2 *= 2;
      }
      return pow2;
    }

    template <typename T> static bool check_block_order(const std::vector<T *> &blocks)
    {
#ifndef NDEBUG
      // Verify that element blocks are defined in sorted offset order...
      typename std::vector<T *>::const_iterator I;

      int64_t eb_offset = -1;
      for (I = blocks.begin(); I != blocks.end(); ++I) {
        int64_t this_off = (*I)->get_offset();
        if (this_off < eb_offset) {
          {
            {
              return false;
            }
          }
        }
        eb_offset = this_off;
      }
#endif
      return true;
    }

    static int term_width();

    static int log_power_2(uint64_t value);

    static char **get_name_array(size_t count, int size);
    static void   delete_name_array(char **names, int count);

    /** \brief Get formatted time and date strings.
     *
     *  Fill time_string and date_string with current time and date
     *  formatted as "HH:MM:SS" for time and "yy/mm/dd" or "yyyy/mm/dd"
     *  for date.
     *
     *  \param[out] time_string The formatted time string.
     *  \param[out] date_string The formatted date string.
     *  \param[in] length Use 8 for short-year date format, or 10 for long-year date format.
     */
    static void time_and_date(char *time_string, char *date_string, size_t length);

    static std::string decode_filename(const std::string &filename, int processor,
                                       int num_processors);
    static size_t      get_number(const std::string &suffix);
    static int64_t     extract_id(const std::string &name_id);
    static std::string encode_entity_name(const std::string &entity_type, int64_t id);

    /** \brief create a string that describes the list of input `ids` collapsing ranges if possible.
     *
     * Traverse the sorted input vector `ids` and return a string that has all sequential ranges
     * collapsed and separated by `rng_sep` and all individual ids or ranges separated by `seq_sep`.
     * Will throw an exception if `ids` is not sorted.   An empty list returns an empty string.
     * The sequence of ids `1, 2, 3, 5, 6, 7` with `rng_sep=".."` will return the default
     * string `1..3, 5..8`
     */
    static std::string format_id_list(const std::vector<size_t> &ids,
                                      const std::string &        rng_sep = " to ",
                                      const std::string &        seq_sep = ", ");

    /** \brief Convert a string to lower case, and convert spaces to `_`.
     *
     *  The conversion is performed in place.
     *
     *  \param[in,out] name On input, the string to convert. On output, the converted string.
     *
     */
    static void fixup_name(char *name);

    /** \brief Convert a string to lower case, and convert spaces to `_`.
     *
     *  The conversion is performed in place.
     *
     *  \param[in,out] name On input, the string to convert. On output, the converted string.
     *
     */
    static void fixup_name(std::string &name);

    /** \brief Check whether property `prop_name` exists and if so, set `prop_value`
     *
     * based on the property value.  Either "TRUE", "YES", "ON", or nonzero for true;
     * or "FALSE", "NO", "OFF", or 0 for false.
     * \param[in] properties the Ioss::PropertyManager containing the properties to be checked.
     * \param[in] prop_name the name of the property to check whether it exists and if so, set its
     * value.
     * \param[out] prop_value if `prop_name` exists and has a valid value, set prop_value
     * accordingly. Does not modify if `prop_name` does not exist. \returns true/false depending on
     * whether property found and value set.
     */

    static bool check_set_bool_property(const Ioss::PropertyManager &properties,
                                        const std::string &prop_name, bool &prop_value);

    /** \brief Determine whether an entity has the property `omitted`.
     *
     *  \param[in] block The entity.
     *  \returns True if the entity has the property `omitted`.
     */
    static bool block_is_omitted(Ioss::GroupingEntity *block);

    /** \brief Process the base element type `base` which has
     *         `nodes_per_element` nodes and a spatial dimension of `spatial`
     *         into a form that the IO system can (hopefully) recognize.
     *
     *  Lowercases the name; converts spaces to `_`, adds
     *  nodes_per_element at end of name (if not already there), and
     *  does some other transformations to remove some exodusII ambiguity.
     *
     *  \param[in] base The element base name.
     *  \param[in] nodes_per_element The number of nodes per element.
     *  \param[in] spatial The spatial dimension of the element.
     *  \returns The Ioss-formatted element name.
     */
    static std::string fixup_type(const std::string &base, int nodes_per_element, int spatial);

    /** \brief Convert a string to upper case.
     *
     *  \param[in] name The string to convert.
     *  \returns The converted string.
     */
    static std::string uppercase(std::string name);

    /** \brief Convert a string to lower case.
     *
     *  \param[in] name The string to convert.
     *  \returns The converted string.
     */
    static std::string lowercase(std::string name);

    static void check_non_null(void *ptr, const char *type, const std::string &name,
                               const std::string &func);

    /** \brief Case-insensitive string comparison.
     *
     *  \param[in] s1 First string
     *  \param[in] s2 Second string
     *  \returns `true` if strings are equal
     */
    static bool str_equal(const std::string &s1, const std::string &s2);

    /** \brief Case-insensitive substring comparison.
     *
     *  \param[in] prefix
     *  \param[in] str
     *  \returns `true` if `str` begins with `prefix` or `prefix` is empty
     */
    static bool substr_equal(const std::string &prefix, const std::string &str);

    /** \brief Get a string containing `uname` output.
     *
     *  This output contains information about the current computing platform.
     *  This is used as information data in the created results file to help
     *  in tracking when/where/... the file was created.
     *
     *  \returns The platform information string.
     */
    static std::string platform_information();

    /** \brief Return amount of memory being used on this processor */
    static size_t get_memory_info();
    static size_t get_hwm_memory_info();

    /** \brief Get a filename relative to the specified working directory (if any)
     *         of the current execution.
     *
     *  Working_directory must end with `/` or be empty.
     *
     *  \param[in] relative_filename The file path to be appended to the working directory path.
     *  \param[in] type The file type. "generated" file types are treated differently.
     *  \param[in] working_directory the path to which the relative_filename path is appended.
     *  \returns The full path (working_directory + relative_filename)
     */
    static std::string local_filename(const std::string &relative_filename, const std::string &type,
                                      const std::string &working_directory);

    static void get_fields(int64_t entity_count, char **names, size_t num_names,
                           Ioss::Field::RoleType fld_role, bool enable_field_recognition,
                           char suffix_separator, int *local_truth,
                           std::vector<Ioss::Field> &fields);

    static int field_warning(const Ioss::GroupingEntity *ge, const Ioss::Field &field,
                             const std::string &inout);

    static void calculate_sideblock_membership(IntVector &face_is_member, const SideBlock *ef_blk,
                                               size_t int_byte_size, const void *element,
                                               const void *sides, int64_t number_sides,
                                               const Region *region);

    /** \brief Get the appropriate index offset for the sides of elements in a SideBlock.
     *
     *  And yet another idiosyncrasy of sidesets...
     *  The side of an element (especially shells) can be
     *  either a face or an edge in the same sideset.  The
     *  ordinal of an edge is (local_edge_number+numfaces) on the
     *  database, but needs to be (local_edge_number) for Sierra...
     *
     *  If the sideblock has a "parent_element_topology" and a
     *  "topology", then we can determine whether to offset the
     *  side ordinals...
     *
     *  \param[in] sb Compute the offset for element sides in this SideBlock
     *  \returns The offset.
     */
    static int64_t get_side_offset(const Ioss::SideBlock *sb);

    static unsigned int hash(const std::string &name);

    static double timer();

    /** \brief Convert an input file to a vector of strings containing one string for each line of
     * the file.
     *
     *  Should only be called by a single processor or each processor will be accessing the file
     *  at the same time...
     *
     *  \param[in] file_name The name of the file.
     *  \param[out] lines The vector of strings containing the lines of the file
     *  \param[in] max_line_length The maximum number of characters in any line of the file.
     */
    static void input_file(const std::string &file_name, std::vector<std::string> *lines,
                           size_t max_line_length = 0);

    template <class T> static std::string to_string(const T &t) { return std::to_string(t); }

    //! \brief Tries to shorten long variable names to an acceptable
    //! length, and converts to lowercase and spaces to `_`
    //!
    //! Many databases have a maximum length for variable names which can
    //! cause a problem with variable name length.
    //!

    //! This routine tries to shorten long variable names to an
    //! acceptable length (`max_var_len` characters max).  If the name
    //! is already less than this length, it is returned unchanged...
    //!
    //! Since there is a (good) chance that two shortened names will match,
    //! a 2-letter `hash` code is appended to the end of the variable name.
    //!
    //! So, we shorten the name to a maximum of `max_var_len`-3
    //! characters and append a 2 character hash+separator.
    //!
    //! It also converts name to lowercase and converts spaces to `_`
    static std::string variable_name_kluge(const std::string &name, size_t component_count,
                                           size_t copies, size_t max_var_len);

    /** \brief Create a nominal mesh for use in history databases.
     *
     *  The model for a history file is a single sphere element (1 node, 1 element).
     *  This is needed for some applications that read this file that require a
     *  "mesh" even though a history file is just a collection of global variables
     *  with no real mesh. This routine will add the mesh portion to a history file.
     *
     *  \param[in,out] region The region on which the nominal mesh is to be defined.
     */
    static void generate_history_mesh(Ioss::Region *region);

    //! Copy the mesh in `region` to `output_region`.  Behavior can be controlled
    //! via options in `options`
    static void copy_database(Ioss::Region &region, Ioss::Region &output_region,
                              Ioss::MeshCopyOptions &options);
  };

  inline std::ostream &OUTPUT() { return *Utils::m_outputStream; }

  inline std::ostream &DEBUG() { return *Utils::m_debugStream; }

  inline std::ostream &WARNING()
  {
    *Utils::m_warningStream << Utils::m_preWarningText;
    return *Utils::m_warningStream;
  }

} // namespace Ioss
#endif
