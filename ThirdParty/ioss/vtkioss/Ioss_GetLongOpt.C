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

/* S Manoharan. Advanced Computer Research Institute. Lyon. France */
#include <Ioss_GetLongOpt.h>
#include <cstring>
#include <fmt/ostream.h>

namespace Ioss {
  /** \brief Create an empty options database.
   *
   * \param optmark The command line symbol designating options.
   */
  GetLongOption::GetLongOption(const char optmark)
      : table(nullptr), ustring(nullptr), pname(nullptr), last(nullptr), enroll_done(0),
        optmarker(optmark)
  {
    ustring = "[valid options and arguments]";
  }

  /** \brief Frees dynamically allocated memory.
   *
   *  Frees memory for the private struct variables representing the options.
   */
  GetLongOption::~GetLongOption()
  {
    Cell *t = table;

    while (t != nullptr) {
      Cell *tmp = t;
      t         = t->next;
      delete tmp;
    }
  }

  /** \brief Extract the base file name from a full path.
   *
   *  Finds the last instance of the '/' character and
   *  extracts the part of the string that follows.
   *
   *  \param[in] pathname The full path.
   *  \return The base file name.
   */
  char *GetLongOption::basename(char *const pathname)
  {
    char *s = strrchr(pathname, '/');
    if (s == nullptr) {
      s = pathname;
    }
    else {
      ++s;
    }
    return s;
  }

  /** \brief Enroll a command line option into the database.
   *
   *  Dynamically allocates memory for the option, sets its name
   *  type, description, value, and default value, and links it
   *  to the preceding option.
   *
   * \param[in] opt The long option name.
   * \param[in] t The option type.
   * \param[in] desc A short description of the option.
   * \param[in] val The option value.
   * \param[in] optval The default value.
   * \returns 1 if successful, 0 if unsuccessful.
   */
  int GetLongOption::enroll(const char *const opt, const OptType t, const char *const desc,
                            const char *const val, const char *const optval)
  {
    if (enroll_done != 0) {
      return 0;
    }

    auto *c        = new Cell;
    c->option      = opt;
    c->type        = t;
    c->description = desc != nullptr ? desc : "no description available";
    c->value       = val;
    c->opt_value   = optval;
    c->next        = nullptr;

    if (last == nullptr) {
      table = last = c;
    }
    else {
      last->next = c;
      last       = c;
    }

    return 1;
  }

  const char *GetLongOption::program_name() const { return pname == nullptr ? "[UNSET]" : pname; }

  /** \brief Get a command line option object.
   *
   *  \param[in] opt The option name.
   *  \returns The option object.
   */
  const char *GetLongOption::retrieve(const char *const opt) const
  {
    Cell *t;
    for (t = table; t != nullptr; t = t->next) {
      if (strcmp(opt, t->option) == 0) {
        return t->value;
      }
    }
    fmt::print(stderr, "GetLongOption::retrieve - unenrolled option {}{}\n", optmarker, opt);
    return nullptr;
  }

  /** \brief parse command line arguments
   *
   *  Set the values of options in the option table based on
   *  the given command line arguments.
   *
   *  \param[in] argc Number of command line arguments passed in from main(int argc, char *argv[]).
   *  \param[in] argv Command line arguments passed in from main(int argc, char *argv[]).
   *  \returns Number of options processed, or -1 on failure.
   *
   */
  int GetLongOption::parse(int argc, char *const *argv)
  {
    int my_optind = 1;

    pname       = basename(*argv);
    enroll_done = 1;
    if (argc-- <= 1) {
      return my_optind;
    }

    while (argc >= 1) {
      char *token = *++argv;
      --argc;

      // '--' signifies end of options if followed by space
      if (token[0] != optmarker || (token[1] == optmarker && strlen(token) == 2)) {
        break; /* end of options */
      }

      ++my_optind;
      char *tmptoken = ++token;
      if (token[0] == optmarker) { // Handle a double '--'
        tmptoken = ++token;
      }

      while ((static_cast<int>(*tmptoken != 0) != 0) && *tmptoken != '=') {
        ++tmptoken;
      }
      /* (tmptoken - token) is now equal to the command line option
         length. */

      Cell *t;
      enum { NoMatch, ExactMatch, PartialMatch, MultipleMatch } matchStatus = NoMatch;

      Cell *pc = nullptr; // pointer to the partially-matched cell
      for (t = table; t != nullptr; t = t->next) {
        if (strncmp(t->option, token, (tmptoken - token)) == 0) {
          if (static_cast<int>(strlen(t->option)) == (tmptoken - token)) {
            /* an exact match found */
            int stat = setcell(t, tmptoken, *(argv + 1), pname);
            if (stat == -1) {
              return -1;
            }
            if (stat == 1) {
              ++argv;
              --argc;
              ++my_optind;
            }
            matchStatus = ExactMatch;
            break;
          }
          else {
            /* partial match found */
            if (pc == nullptr) {
              matchStatus = PartialMatch;
              pc          = t;
            }
            else {
              // Multiple partial matches...Print warning
              if (matchStatus == PartialMatch) {
                // First time, print the message header and the first
                // matched duplicate...
                fmt::print(stderr, "ERROR: {}: Multiple matches found for option '{}{}'.\n", pname,
                           optmarker, strtok(token, "= "));
                fmt::print(stderr, "\t{}{}: {}\n", optmarker, pc->option, pc->description);
              }
              fmt::print(stderr, "\t{}{}:{}\n", optmarker, t->option, t->description);
              matchStatus = MultipleMatch;
            }
          }
        } /* end if */
      }   /* end for */

      if (matchStatus == PartialMatch) {
        int stat = setcell(pc, tmptoken, *(argv + 1), pname);
        if (stat == -1) {
          return -1;
        }
        if (stat == 1) {
          ++argv;
          --argc;
          ++my_optind;
        }
      }
      else if (matchStatus == NoMatch) {
        fmt::print(stderr, "{}: unrecognized option {}{}\n", pname, optmarker, strtok(token, "= "));
        return -1; /* no match */
      }
      else if (matchStatus == MultipleMatch) {
        return -1; /* no match */
      }

    } /* end while */

    return my_optind;
  }

  /** \brief parse an argument string.
   *
   *  Set the values of options in the option table based on
   *  the given option string.
   *
   *  \param[in] str The option string.
   *  \param[in] p A string to be used in error reporting
   *  \returns 1 if successful, or -1 otherwise.
   *
   */
  int GetLongOption::parse(char *const str, char *const p)
  {
    enroll_done       = 1;
    char *      token = strtok(str, " \t");
    const char *name  = p != nullptr ? p : "GetLongOption";

    while (token != nullptr) {
      if (token[0] != optmarker || (token[1] == optmarker && strlen(token) == 2)) {
        fmt::print(stderr, "{}: nonoptions not allowed\n", name);
        return -1; /* end of options */
      }

      char *ladtoken = nullptr; /* lookahead token */
      char *tmptoken = ++token;
      while ((static_cast<int>(*tmptoken != 0) != 0) && *tmptoken != '=') {
        ++tmptoken;
      }
      /* (tmptoken - token) is now equal to the command line option
         length. */

      Cell *t;
      enum { NoMatch, ExactMatch, PartialMatch } matchStatus = NoMatch;
      Cell *pc = nullptr; // pointer to the partially-matched cell
      for (t = table; t != nullptr; t = t->next) {
        if (strncmp(t->option, token, (tmptoken - token)) == 0) {
          if (static_cast<int>(strlen(t->option)) == (tmptoken - token)) {
            /* an exact match found */
            ladtoken = strtok(nullptr, " \t");
            int stat = setcell(t, tmptoken, ladtoken, name);
            if (stat == -1) {
              return -1;
            }
            if (stat == 1) {
              ladtoken = nullptr;
            }
            matchStatus = ExactMatch;
            break;
          }
          else {
            /* partial match found */
            matchStatus = PartialMatch;
            pc          = t;
          }
        } /* end if */
      }   /* end for */

      if (matchStatus == PartialMatch) {
        ladtoken = strtok(nullptr, " \t");
        int stat = setcell(pc, tmptoken, ladtoken, name);
        if (stat == -1) {
          return -1;
        }
        if (stat == 1) {
          ladtoken = nullptr;
        }
      }
      else if (matchStatus == NoMatch) {
        fmt::print(stderr, "{}: unrecognized option {}{}\n", name, optmarker, strtok(token, "= "));
        return -1; /* no match */
      }

      token = ladtoken != nullptr ? ladtoken : strtok(nullptr, " \t");
    } /* end while */

    return 1;
  }

  /* ----------------------------------------------------------------
     GetLongOption::setcell returns
     -1 if there was an error
     0  if the nexttoken was not consumed
     1  if the nexttoken was consumed
     ------------------------------------------------------------------- */

  int GetLongOption::setcell(Cell *c, char *valtoken, char *nexttoken, const char *name)
  {
    if (c == nullptr) {
      return -1;
    }

    switch (c->type) {
    case GetLongOption::NoValue:
      if (*valtoken == '=') {
        fmt::print(stderr, "{}: unsolicited value for flag {}{}\n", name, optmarker, c->option);
        return -1; /* unsolicited value specification */
      }
      // Set to a non-zero value.  Used to be "(char*) ~0", but that
      // gives out-of-range warnings on some systems...
      c->value = (char *)1;
      return 0;
    case GetLongOption::OptionalValue:
      if (*valtoken == '=') {
        c->value = ++valtoken;
        return 0;
      }
      else {
        if (nexttoken != nullptr && nexttoken[0] != optmarker) {
          c->value = nexttoken;
          return 1;
        }
        c->value = c->opt_value;
        return 0;
      }
    case GetLongOption::MandatoryValue:
      if (*valtoken == '=') {
        c->value = ++valtoken;
        return 0;
      }
      else {
        if (nexttoken != nullptr && nexttoken[0] != optmarker) {
          c->value = nexttoken;
          return 1;
        }
        fmt::print(stderr, "{}: mandatory value for {}{} not specified\n", name, optmarker,
                   c->option);
        return -1; /* mandatory value not specified */
      }
    default: break;
    }
    return -1;
  }

  /** \brief Print the program usage string.
   *
   *  \param[in] outfile The output stream to which the usage string is printed.
   */
  void GetLongOption::usage(std::ostream &outfile) const
  {
    Cell *t;

    fmt::print(outfile, "\nusage: {} {}\n", pname, ustring);
    for (t = table; t != nullptr; t = t->next) {
      fmt::print(outfile, "\t{}{}", optmarker, t->option);
      if (t->type == GetLongOption::MandatoryValue) {
        fmt::print(outfile, " <$val>");
      }
      else if (t->type == GetLongOption::OptionalValue) {
        fmt::print(outfile, " [$val]");
      }
      fmt::print(outfile, " ({})\n", t->description);
    }
    outfile.flush();
  }
} // namespace Ioss
