/*
 * Copyright(C) 1999-2025 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/* S Manoharan. Advanced Computer Research Institute. Lyon. France */

#pragma once

#include <cstdlib>
#include <iostream>
#include <string>

#include "ioss_export.h"
#include "vtk_ioss_mangle.h"

namespace Ioss {
  /** \brief A database of program command line and environment variable options and methods for
   * manipulating them.
   *
   *  A collection of long command line option names for a program that uses the Ioss library.
   */
  class IOSS_EXPORT GetLongOption
  {
  public:
    enum class OptType { NoValue, OptionalValue, MandatoryValue };

  private:
    struct Cell
    {
      const char *option{nullptr};      // option name
      const char *description{nullptr}; // a description of option
      const char *value{nullptr};       // value of option (string)
      const char *opt_value{
          nullptr};          // If optional value and value not entered, assign opt_value to value
      Cell   *next{nullptr}; // pointer to the next cell
      OptType type{OptType::NoValue}; // option type
      bool    extra_line{false};      // True if `usage()` should output extra line at end of entry

      Cell() = default;
    };

    Cell       *table{nullptr}; // option table
    const char *ustring{"[valid options and arguments]"};
    char       *pname{nullptr};        // program basename
    Cell       *last{nullptr};         // last entry in option table
    char        optmarker;             // option marker
    bool        options_parsed{false}; // parsed options, cannot enroll anymore options

    int setcell(Cell *c, char *valtoken, char *nexttoken, const char *name);

  public:
    explicit GetLongOption(char optmark = '-');
    ~GetLongOption();

    static char *basename(char *pathname);

    int parse(int argc, char *const *argv);
    int parse(char *str, char *p);

    bool        enroll(const char *opt, OptType t, const char *desc, const char *val,
                       const char *optval = nullptr, bool extra_line = false);
    const char *retrieve(const char *opt) const;
    const char *program_name() const;

    void usage(std::ostream &outfile = std::cout) const;

    /** \brief Set the program usage string.
     *
     *  The program usage string should define the command line
     *  syntax for program options and arguments and contain
     *  other helpful usage text.
     *  \param[in] str The usage string.
     */
    void usage(const char *str) { ustring = str; }

    template <class INT, typename std::enable_if_t<std::is_integral_v<INT>, INT> * = nullptr>
    INT get_option_value(const char *option_txt, INT default_value)
    {
      INT         value = default_value;
      const char *temp  = retrieve(option_txt);
      if (temp != nullptr) {
        value = std::strtol(temp, nullptr, 10);
      }
      return value;
    }

    template <class DBL, typename std::enable_if_t<std::is_floating_point_v<DBL>, DBL> * = nullptr>
    DBL get_option_value(const char *option_txt, DBL default_value)
    {
      DBL         value = default_value;
      const char *temp  = retrieve(option_txt);
      if (temp != nullptr) {
        value = std::strtod(temp, nullptr);
      }
      return value;
    }

    std::string get_option_value(const char *option_txt, const std::string &default_value) const
    {
      auto        value = default_value;
      const char *temp  = retrieve(option_txt);
      if (temp != nullptr) {
        value = temp;
      }
      return value;
    }
  };
} // namespace Ioss
