// Copyright(C) 1999-2020, 2023 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_SmartAssert.h"
#include <cstdlib>
#include <fstream>
#include <stdexcept>

namespace {
  // in case we're logging using the default logger...
  struct stream_holder
  {
    stream_holder() = default;
    ~stream_holder()
    {
      if (owns_) {
        delete out_;
      }
      out_ = nullptr;
    }
    std::ostream *out_{nullptr};
    bool          owns_{false};
  };
  // information about the stream we write to, in case
  // we're using the default logger
  stream_holder default_logger_info;
} // anonymous namespace

namespace Ioss {
  namespace SmartAssert {

    // returns a message corresponding to the type of level
    std::string get_typeof_level(int nLevel)
    {
      switch (nLevel) {
      case lvl_warn: return "Warning";
      case lvl_debug: return "Assertion failed";
      case lvl_error: return "Assertion failed (Error)";
      case lvl_fatal: return "Assertion failed (FATAL)";
      default: {
        std::ostringstream out;
        out << "Assertion failed (level=" << nLevel << ")";
        return out.str();
      }
      };
    }

    // helpers, for dumping the assertion context
    void dump_context_summary(const assert_context &context, std::ostream &out)
    {
      out << "\n"
          << get_typeof_level(context.get_level()) << " in " << context.get_context_file() << ":"
          << context.get_context_line() << '\n';
      if (!context.get_level_msg().empty()) {
        // we have a user-friendly message
        out << context.get_level_msg();
      }
      else {
        out << "\nExpression: " << context.get_expr();
      }
      out << '\n';
    }

    void dump_context_detail(const assert_context &context, std::ostream &out)
    {
      out << "\n"
          << get_typeof_level(context.get_level()) << " in " << context.get_context_file() << ":"
          << context.get_context_line() << '\n';
      if (!context.get_level_msg().empty()) {
        out << "User-friendly msg: '" << context.get_level_msg() << "'\n";
      }
      out << "\nExpression: '" << context.get_expr() << "'\n";

      using ac_vals_array        = assert_context::vals_array;
      const ac_vals_array &aVals = context.get_vals_array();
      if (!aVals.empty()) {
        bool bFirstTime = true;
        auto first      = aVals.cbegin();
        auto last       = aVals.cend();
        while (first != last) {
          if (bFirstTime) {
            out << "Values: ";
            bFirstTime = false;
          }
          else {
            out << "        ";
          }
          out << first->second << "='" << first->first << "'\n";
          ++first;
        }
      }
      out << '\n';
    }

    ///////////////////////////////////////////////////////
    // logger

    void default_logger(const assert_context &context)
    {
      if (default_logger_info.out_ == nullptr) {
        return;
      }
      dump_context_detail(context, *(default_logger_info.out_));
    }

    ///////////////////////////////////////////////////////
    // handlers

    // warn : just dump summary to console
    void default_warn_handler(const assert_context &context)
    {
      dump_context_summary(context, std::cout);
    }

    // debug: ask user what to do
    void default_debug_handler(const assert_context &context)
    {
#if 1
      dump_context_detail(context, std::cerr);
      abort();
#else
      static bool ignore_all = false;
      if (ignore_all)
        // ignore All asserts
        return;
      typedef std::pair<std::string, int> file_and_line;
      static std::set<file_and_line>      ignorer;
      if (ignorer.find(file_and_line(context.get_context_file(), context.get_context_line())) !=
          ignorer.end())
        // this is Ignored Forever
        return;

      dump_context_summary(context, std::cerr);
      std::cerr << "\nPress (I)gnore/ Ignore (F)orever/ Ignore (A)ll/ (D)ebug/ A(b)ort: ";
      std::cerr.flush();
      char ch = 0;

      bool bContinue = true;
      while (bContinue && std::cin.get(ch)) {
        bContinue = false;
        switch (ch) {
        case 'i':
        case 'I':
          // ignore
          break;

        case 'f':
        case 'F':
          // ignore forever
          ignorer.insert(file_and_line(context.get_context_file(), context.get_context_line()));
          break;

        case 'a':
        case 'A':
          // ignore all
          ignore_all = true;
          break;

        case 'b':
        case 'B': abort(); break;

        default: bContinue = true; break;
        }
      }
#endif
    }

    // error : throw a runtime exception
    void default_error_handler(const assert_context &context)
    {
      std::ostringstream out;
      dump_context_summary(context, out);
      throw std::runtime_error(out.str());
    }

    // fatal : dump error and abort
    void default_fatal_handler(const assert_context &context)
    {
      dump_context_detail(context, std::cerr);
      abort();
    }

  } // namespace SmartAssert

  namespace Private {

    void init_assert()
    {
      Assert::set_log(&::Ioss::SmartAssert::default_logger);
      Assert::set_handler(lvl_warn, &::Ioss::SmartAssert::default_warn_handler);
      Assert::set_handler(lvl_debug, &::Ioss::SmartAssert::default_debug_handler);
      Assert::set_handler(lvl_error, &::Ioss::SmartAssert::default_error_handler);
      Assert::set_handler(lvl_fatal, &::Ioss::SmartAssert::default_fatal_handler);
    }

    // sets the default logger to write to this stream
    void set_default_log_stream(std::ostream &out)
    {
      default_logger_info.out_  = &out;
      default_logger_info.owns_ = false;
    }

    // sets the default logger to write to this file
    void set_default_log_name(const char *str)
    {
      default_logger_info.owns_ = false;
      default_logger_info.out_  = new std::ofstream(str);
      default_logger_info.owns_ = true;
    }

  } // namespace Private
} // namespace Ioss
