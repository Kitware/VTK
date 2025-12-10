// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkStringFormatter.h"

#include <array>
#include <regex>

namespace vtk
{
VTK_ABI_NAMESPACE_BEGIN

constexpr std::string_view printf_escape_regex = "(%%)";

// clang-format off
// https://en.cppreference.com/w/cpp/io/c/fprintf
constexpr std::string_view printf_specifier_regex =
  // Group 1 start: entire specifier
  "("
    // % specifier
    "%"
    // Group 2 start: all classes
    "("
      // Group 3: character
      "("
        // Group (1): flags
        "(\\-)?"
        // Groups (2 & 3): no zero-padding
        "(())?"
        // Group (4): width
        "(\\*|[1-9]\\d*)?"
        // Group (5): no precision
        "()?"
        // Group (6): length modifier
        "(l)?"
        // Group (7): character specifier
        "(c)"
      // Group 3 end: character
      ")"
      // OR
      "|"
      // Group 11 start: string
      "("
        // Group (1): flags
        "(\\-)?"
        // Groups (2 & 3): no zero-padding
        "(())?"
        // Group (4): width
        "(\\*|[1-9]\\d*)?"
        // Group (5): precision
        "(\\.|\\.\\*|\\.\\d+)?"
        // Group (6): length modifier
        "(l)?"
        // Group (7): string specifier
        "(s)"
      // Group 11 end: string
      ")"
      // OR
      "|"
      // Group 19 start: signed decimal integer
      "("
        // Group (1): flags
        "([\\-\\+ ]{0,3})?"
        // Groups (2 & 3): zero-padding
        "(0(?=([1-9]|\\*)))?"
        // Group (4): width
        "(\\*|[1-9]\\d*)?"
        // Group (5): precision
        "(\\.|\\.\\*|\\.\\d+)?"
        // Group (6): length modifier
        "(hh|h|ll|l|j|z|t)?"
        // Group (7): signed decimal integer specifier
        "([di])"
      // Group 19 end: signed decimal integer
      ")"
      // OR
      "|"
      // Group 27 start: unsigned octal/hex integer
      "("
        // Group (1): flags
        "([#\\-]{0,2})?"
        // Groups (2 & 3): zero-padding
        "(0(?=([1-9]|\\*)))?"
        // Group (4): width
        "(\\*|[1-9]\\d*)?"
        // Group (5): precision
        "(\\.|\\.\\*|\\.\\d+)?"
        // Group (6): length modifier
        "(hh|h|ll|l|j|z|t)?"
        // Group (7): unsigned octal/hex integer specifier
        "([oxX])"
      // Group 27 end: unsigned octal/hex integer
      ")"
      // OR
      "|"
      // Group 35 start: unsigned decimal integer
      "("
        // Group (1): flags
        "(\\-)?"
        // Groups (2 & 3): zero-padding
        "(0(?=([1-9]|\\*)))?"
        // Group (4): width
        "(\\*|[1-9]\\d*)?"
        // Group (5): precision
        "(\\.|\\.\\*|\\.\\d+)?"
        // Group (6): length modifier
        "(hh|h|ll|l|j|z|t)?"
        // Group (7): unsigned decimal integer specifier
        "([u])"
      // Group 35 end: unsigned decimal integer
      ")"
      // OR
      "|"
      // Group 43 start: floating point decimal
      "("
        // Group (1): flags
        "([#\\-+ ]{0,4})?"
        // Groups (2 & 3): zero-padding
        "(0(?=([1-9]|\\*)))?"
        // Group (4): width
        "(\\*|[1-9]\\d*)?"
        // Group (5): precision
        "(\\.|\\.\\*|\\.\\d+)?"
        // Group (6): length modifier
        "([lL])?"
        // Group (7): floating point decimal specifier
        "([fF])"
      // Group 43 end: floating point decimal
      ")"
      // OR
      "|"
      // Group 51 start: floating point decimal exponent
      "("
        // Group (1): flags
        "([#\\-+ ]{0,4})?"
        // Groups (2 & 3): zero-padding
        "(0(?=([1-9]|\\*)))?"
        // Group (4): width
        "(\\*|[1-9]\\d*)?"
        // Group (5): precision
        "(\\.|\\.\\*|\\.\\d+)?"
        // Group (6): length modifier
        "([lL])?"
        // Group (7): floating point decimal exponent specifier
        "([eE])"
      // Group 51 end: floating point decimal exponent
      ")"
      // OR
      "|"
      // Group 59 start: floating point hexadecimal exponent
      "("
        // Group (1): flags
        "([#\\-+ ]{0,4})?"
        // Groups (2 & 3): zero-padding
        "(0(?=([1-9]|\\*)))?"
        // Group (4): width
        "(\\*|[1-9]\\d*)?"
        // Group (5): precision
        "(\\.|\\.\\*|\\.\\d+)?"
        // Group (6): length modifier
        "([lL])?"
        // Group (7): floating point hexadecimal exponent specifier
        "([aA])"
      // Group 59 end: floating point hexadecimal exponent
      ")"
      // OR
      "|"
      // Group 67 start: floating point general
      "("
        // Group (1): flags
        "([#\\-+ ]{0,4})?"
        // Groups (2 & 3): zero-padding
        "(0(?=([1-9]|\\*)))?"
        // Group (4): width
        "(\\*|[1-9]\\d*)?"
        // Group (5): precision
        "(\\.|\\.\\*|\\.\\d+)?"
        // Group (6): length modifier
        "([lL])?"
        // Group (7): floating point general specifier
        "([gG])"
      // Group 67 end: floating point general
      ")"
      // OR
      "|"
      // Group 75 start: number of characters
      "("
        // Group (1): no flags
        "()?"
        // Groups (2 & 3): no zero-padding
        "(())?"
        // Group (4): no width
        "()?"
        // Group (5): no precision
        "()?"
        // Group (6): no length modifier
        "()?"
        // Group (7): number of characters specifier
        "(n)"
      // Group 75 end: number of characters
      ")"
      // OR
      "|"
      // Group 83 start: pointer
      "("
        // Group (1): no flags
        "()?"
        // Groups (2 & 3): no zero-padding
        "(())?"
        // Group (4): width
        "(\\*|[1-9]\\d*)?"
        // Group (5): no precision
        "()?"
        // Group (6): no length modifier
        "()?"
        // Group (7): pointer specifier
        "(p)"
      // Group 83 end: pointer
      ")"
    // Group 2 end: all classes
    ")"
  // Group 1 end: entire specifier
  ")";
// clang-format on

constexpr std::string_view std_format_begin_escape_regex = "(\\{)";

constexpr std::string_view std_format_end_escape_regex = "(\\})";

constexpr std::size_t printf_groups_per_class = 8;

enum ClassType
{
  CHARACTER = 3,
  STRING = 11,
  SIGNED_DECIMAL_INTEGER = 19,
  OCTAL_HEX_INTEGER = 27,
  UNSIGNED_DECIMAL_INTEGER = 35,
  FLOATING_POINT_DECIMAL = 43,
  FLOATING_POINT_DECIMAL_EXPONENT = 51,
  FLOATING_POINT_HEX_EXPONENT = 59,
  FLOATING_POINT_GENERAL_EXPONENT = 67,
  NUMBER_OF_CHARACTERS = 75,
  POINTER = 83
};

enum GroupType
{
  FLAGS = 1,
  ZERO_PADDING_WITH_FORWARD_LOOK_UP = 2, // and 3
  WIDTH = 4,
  PRECISION = 5,
  LENGTH_MODIFIER = 6,
  SPECIFIER = 7
};

static bool has_duplicates_flags(const std::string& used_flags)
{
  unsigned char seen = 0;
  for (char c : used_flags)
  {
    if (c == '0')
    {
      continue; // Handled elsewhere
    }
    unsigned char bit = 0;
    switch (c)
    {
      case '#':
        bit = 1;
        break;
      case '-':
        bit = 2;
        break;
      case '+':
        bit = 4;
        break;
      case ' ':
        bit = 8;
        break;
      default:
        continue; // Ignore invalid flags here
    }
    if (seen & bit)
    {
      return true;
    }
    seen |= bit;
  }
  return false;
}

bool is_printf_format(const std::string& format)
{
  // Compile the regex objects
  static std::regex printf_escape_regex_obj{ std::string(printf_escape_regex) };
  static std::regex printf_specifier_regex_obj{ std::string(printf_specifier_regex) };

  size_t pos = 0;
  size_t escapes_found = 0;
  size_t specifiers_found = 0;
  while (pos < format.size())
  {
    if (format[pos] != '%')
    {
      // Consume plain text until '%' or end
      while (pos < format.size() && format[pos] != '%')
      {
        pos++;
      }
    }
    else
    {
      std::smatch match;
      // Handle '%' by checking for escape sequence
      if (std::regex_search(format.begin() + pos, format.end(), match, printf_escape_regex_obj,
            std::regex_constants::match_continuous))
      {
        ++escapes_found;
        pos += match.length(); // e.g., "%%" advances by 2
      }
      // Handle any variable specifier
      else if (std::regex_search(format.begin() + pos, format.end(), match,
                 printf_specifier_regex_obj, std::regex_constants::match_continuous))
      {
        ++specifiers_found;
        {
          // check for duplicate flags
          std::array<ClassType, 6> classes = { SIGNED_DECIMAL_INTEGER, OCTAL_HEX_INTEGER,
            FLOATING_POINT_DECIMAL, FLOATING_POINT_DECIMAL_EXPONENT, FLOATING_POINT_HEX_EXPONENT,
            FLOATING_POINT_GENERAL_EXPONENT };
          for (const auto& classType : classes)
          {
            if (!match[classType].str().empty())
            { // group 1: flags
              if (has_duplicates_flags(match[classType + FLAGS].str()))
              {
                return false; // Duplicate flags
              }
              break;
            }
          }
        }
        {
          // check if both plus and space/minus flags are present
          std::array<ClassType, 5> classes = { SIGNED_DECIMAL_INTEGER, FLOATING_POINT_DECIMAL,
            FLOATING_POINT_DECIMAL_EXPONENT, FLOATING_POINT_HEX_EXPONENT,
            FLOATING_POINT_GENERAL_EXPONENT };
          for (const auto& classType : classes)
          {
            if (!match[classType].str().empty())
            {
              const auto& flags = match[classType + FLAGS].str();
              if (!flags.empty())
              {
                // check if both plus and space flags are present
                if (flags.find('+') != std::string::npos && flags.find(' ') != std::string::npos)
                {
                  return false; // Conflicting plus and space flags
                }
                // check if both plus and minus flags are present
                if (flags.find('+') != std::string::npos && flags.find('-') != std::string::npos)
                {
                  return false; // Conflicting plus and minus flags
                }
                break;
              }
            }
          }
        }

        pos += match.length(); // e.g., "%d" or "%10.5f" advances by 2 or 5
      }
      else
      {
        return false; // '%' not followed by valid escape or specifier
      }
    }
  }
  return specifiers_found > 0 || escapes_found > 0;
}

static bool printf_specifier_type_to_std_format(const char& input, char& output)
{
  switch (input)
  {
    // case '%': // Literal % (handled elsewhere)
    //   output = '%';
    //   break;
    case 'c':
      output = 'c'; // Character
      break;
    case 's':
      output = 's'; // String
      break;
    case 'd':
    case 'i':
      output = 'd'; // Decimal integer
      break;
    case 'o':
      output = 'o'; // Octal
      break;
    case 'x':
      output = 'x'; // Lowercase hex
      break;
    case 'X':
      output = 'X'; // Uppercase hex
      break;
    case 'u':
      output = 'd'; // Decimal unsigned - std::format uses 'd' for both
      break;
    case 'f':
      output = 'f'; // Fixed point
      break;
    case 'F':
      output = 'F'; // Fixed point with uppercase INF/NAN
      break;
    case 'e':
      output = 'e'; // Lowercase scientific
      break;
    case 'E':
      output = 'E'; // Uppercase scientific
      break;
    case 'a':
      output = 'a'; // Lowercase hex floating point
      break;
    case 'A':
      output = 'A'; // Uppercase hex floating point
      break;
    case 'g':
      output = 'g'; // General format, lowercase
      break;
    case 'G':
      output = 'G'; // General format, uppercase
      break;
    case 'n': // Not directly supported in std::format
      // Returns the number of characters written so far by this call to the function.
      return false;
    case 'p':
      output = 'p'; // Pointer
      break;
    default:
      return false;
  }
  return true;
}

// Struct to hold parsed format specifier components
struct PrintfSpecifier
{
  ClassType Type;                // Type of specifier
  bool HasSpaceFill = false;     // Space fill flag
  bool HasLeftJustify = false;   // Left justify flag
  bool HasShowSign = false;      // Show sign flag
  bool HasAlternateForm = false; // Alternate form flag
  bool HasZeroPadding = false;   // Zero padding flag
  std::string Width;             // Field Width (including '*')
  std::string Precision;         // Precision for floating-point or string
  std::string LengthModifier;    // Length modifier (l, ll, h, etc.)
  char SpecifierType;            // Format Specifier (d, f, s, etc.)
  bool HasWidth() const { return !this->Width.empty(); }
  bool HasPrecision() const { return !this->Precision.empty(); }
  bool HasLengthModifier() const { return !this->LengthModifier.empty(); }
};

static std::string handle_character_specifier(const PrintfSpecifier& spec, int& argIndex)
{
  std::string formatSpec;
  if (spec.HasLeftJustify)
  {
    formatSpec += "<";
  }
  if (spec.HasWidth())
  {
    if (!spec.HasLeftJustify)
    {
      formatSpec += ">";
    }
    formatSpec += (spec.Width == "*") ? "{" + vtk::to_string(argIndex++) + "}" : spec.Width;
  }
  return "{" + vtk::to_string(argIndex++) + ":" + formatSpec + "c}";
}

static std::string handle_string_specifier(const PrintfSpecifier& spec, int& argIndex)
{
  std::string formatSpec;
  if (spec.HasLeftJustify)
  {
    formatSpec += "<";
  }
  if (spec.HasWidth())
  {
    if (!spec.HasLeftJustify)
    {
      formatSpec += ">";
    }
    formatSpec += (spec.Width == "*") ? "{" + vtk::to_string(argIndex++) + "}" : spec.Width;
  }
  if (spec.HasPrecision())
  {
    if (spec.Precision == ".*")
    {
      formatSpec += ".{" + vtk::to_string(argIndex++) + "}";
    }
    else if (spec.Precision == ".")
    {
      formatSpec += ".0";
    }
    else
    {
      formatSpec += spec.Precision;
    }
  }
  return "{" + vtk::to_string(argIndex++) + ":" + formatSpec + "s}";
}

static std::string handle_integer_specifier(const PrintfSpecifier& spec, int& argIndex)
{
  std::string formatSpec;
  auto addFlags = [&]()
  {
    if (spec.HasSpaceFill)
    {
      formatSpec += " ";
    }
    if (spec.HasLeftJustify)
    {
      formatSpec += "<";
    }
    if (spec.HasShowSign)
    {
      formatSpec += "+";
    }
    if (spec.HasAlternateForm)
    {
      formatSpec += "#";
    }
    if (spec.HasZeroPadding)
    {
      formatSpec += "0";
    }
  };
  // Handle when width is empty precision is empty
  if (!spec.HasWidth() && !spec.HasPrecision())
  {
    addFlags();
  }
  // Handle when width exist and precision is empty
  else if (spec.HasWidth())
  {
    addFlags();
    if (spec.Width == "*")
    {
      formatSpec += "{" + vtk::to_string(argIndex++) + "}";
    }
    else
    {
      formatSpec += spec.Width;
    }
    if (spec.HasPrecision())
    {
      vtkLogF(WARNING,
        "Precision ignored for integer with width (e.g., %%10.5d). "
        "Using width only ({:10d}). For printf-like behavior, "
        "pre-format the integer with zeros and use a string specifier.");
      if (spec.Precision == ".*")
      {
        argIndex++;
      }
    }
  }
  // Handle when width is empty and precision exist
  else // !spec.HasWidth() && spec.HasPrecision()
  {
    // Map precision to zero-padded width
    if (spec.Precision != ".")
    {
      formatSpec += "0"; // Precision implies zero-padding
      if (spec.HasShowSign)
      {
        formatSpec += "+";
      }
      if (spec.HasAlternateForm)
      {
        formatSpec += "#";
      }
      if (spec.HasSpaceFill)
      {
        formatSpec += " ";
      }
      formatSpec += (spec.Precision == ".*") ? "{" + vtk::to_string(argIndex++) + "}"
                                             : spec.Precision.substr(1);
    }
    else
    {
      addFlags(); // For "." alone, use flags normally
    }
  }
  // Handle type
  char converted_type;
  if (printf_specifier_type_to_std_format(spec.SpecifierType, converted_type))
  {
    formatSpec += converted_type;
  }
  else
  {
    // Unsupported specifier
    vtkLogF(WARNING, "Unsupported format specifier: %c", spec.SpecifierType);
    formatSpec += "d"; // Default to decimal
  }

  return "{" + vtk::to_string(argIndex++) + ":" + formatSpec + "}";
}

static std::string handle_default_specifier(PrintfSpecifier& spec, int& argIndex)
{
  std::string formatSpec;
  // Handle space
  if (spec.HasSpaceFill)
  {
    formatSpec += " ";
  }
  // Handle left justify
  if (spec.HasLeftJustify)
  {
    formatSpec += "<";
  }
  // Handle show sign
  if (spec.HasShowSign)
  {
    formatSpec += "+";
  }
  // Handle alternate form
  if (spec.HasAlternateForm)
  {
    formatSpec += "#";
  }
  // Handle zero-padding
  if (spec.HasZeroPadding)
  {
    formatSpec += "0";
  }

  // Handle Width
  if (spec.HasWidth())
  {
    if (spec.Width == "*")
    {
      formatSpec += "{" + vtk::to_string(argIndex++) + "}";
    }
    else
    {
      formatSpec += spec.Width;
    }
  }
  // Handle Precision
  if (spec.HasPrecision())
  {
    if (spec.Precision == ".*")
    {
      // Dynamic precision
      formatSpec += ".{" + vtk::to_string(argIndex++) + "}";
    }
    else if (spec.Precision == ".")
    {
      // Default precision
      formatSpec += ".6"; // Default for floating point
    }
    else
    {
      // Remove the leading dot
      formatSpec += "." + spec.Precision.substr(1);
    }
  }
  // Handle type
  char converted_type;
  if (printf_specifier_type_to_std_format(spec.SpecifierType, converted_type))
  {
    formatSpec += converted_type;
  }
  else
  {
    // Unsupported specifier
    vtkLogF(WARNING, "Unsupported format specifier: %c", spec.SpecifierType);
    formatSpec += " type ";
  }
  return "{" + vtk::to_string(argIndex++) + ":" + formatSpec + "}";
}

// Function to convert a parsed printf format Specifier to std::format syntax
static std::string printf_specifier_to_std_format(PrintfSpecifier& spec, int& argIndex)
{
  switch (spec.Type)
  {
    case ClassType::CHARACTER:
    {
      return handle_character_specifier(spec, argIndex);
    }
    case ClassType::STRING:
    {
      return handle_string_specifier(spec, argIndex);
    }
    case ClassType::SIGNED_DECIMAL_INTEGER:
    case ClassType::OCTAL_HEX_INTEGER:
    case ClassType::UNSIGNED_DECIMAL_INTEGER:
    {
      return handle_integer_specifier(spec, argIndex);
    }
    case ClassType::FLOATING_POINT_DECIMAL:
    case ClassType::FLOATING_POINT_DECIMAL_EXPONENT:
    case ClassType::FLOATING_POINT_HEX_EXPONENT:
    case ClassType::FLOATING_POINT_GENERAL_EXPONENT:
    case ClassType::NUMBER_OF_CHARACTERS:
    case ClassType::POINTER:
    default:
    {
      return handle_default_specifier(spec, argIndex);
    }
  }
}

// Complete function to convert printf-like format to std::format
std::string printf_to_std_format(const std::string& format)
{
  // Compile the regex objects
  static std::regex printf_escape_regex_obj{ std::string(printf_escape_regex) };
  static std::regex printf_specifier_regex_obj{ std::string(printf_specifier_regex) };
  static std::regex std_format_begin_escape_regex_obj{ std::string(std_format_begin_escape_regex) };
  static std::regex std_format_end_escape_regex_obj{ std::string(std_format_end_escape_regex) };

  std::string std_format;
  size_t pos = 0;
  int argIndex = 0; // Track argument index for std::format

  while (pos < format.size())
  {
    if (format[pos] != '%')
    {
      // Copy plain text until '%' or end
      size_t start = pos;
      while (pos < format.size() && format[pos] != '%')
      {
        pos++;
      }
      std_format += format.substr(start, pos - start);
    }
    else
    {
      std::smatch match;
      // Handle '%' by checking for escape sequence
      if (std::regex_search(format.begin() + pos, format.end(), match, printf_escape_regex_obj,
            std::regex_constants::match_continuous))
      {
        // Handle escaped percentages (%%): add a single % to output
        std_format += "%";
        pos += match.length();
      }
      // Handle any variable specifier
      else if (std::regex_search(format.begin() + pos, format.end(), match,
                 printf_specifier_regex_obj, std::regex_constants::match_continuous))
      {
        // Parse the format Specifier components
        PrintfSpecifier spec;
        for (size_t classType = 3; classType < match.size(); classType += printf_groups_per_class)
        {
          if (!match[classType].str().empty())
          {
            spec.Type = static_cast<ClassType>(classType);
            // Parse flags
            std::string flags = match[classType + FLAGS].str();
            for (char flag : flags)
            {
              switch (flag)
              {
                case ' ':
                  spec.HasSpaceFill = true;
                  break;
                case '-':
                  spec.HasLeftJustify = true;
                  break;
                case '+':
                  spec.HasShowSign = true;
                  break;
                case '#':
                  spec.HasAlternateForm = true;
                  break;
                default:
                  break;
              }
            }
            // Parse zero-padding
            if (!match[classType + GroupType::ZERO_PADDING_WITH_FORWARD_LOOK_UP].str().empty())
            {
              spec.HasZeroPadding = true;
            }
            // Parse Width
            spec.Width = match[classType + GroupType::WIDTH].str();
            // Parse Precision
            spec.Precision = match[classType + GroupType::PRECISION].str();
            // Parse length modifier
            spec.LengthModifier = match[classType + GroupType::LENGTH_MODIFIER].str();
            // Parse standard conversion Specifier
            spec.SpecifierType = match[classType + GroupType::SPECIFIER].str()[0];

            // Convert to std::format syntax
            std_format += printf_specifier_to_std_format(spec, argIndex);

            pos += match.length();
            break;
          }
        }
      }
      // Handle std::format begin escape sequence
      else if (std::regex_search(format.begin() + pos, format.end(),
                 std_format_begin_escape_regex_obj, std::regex_constants::match_continuous))
      {
        std_format += "{{";
        pos += match.length();
      }
      // Handle std::format end escape sequence
      else if (std::regex_search(format.begin() + pos, format.end(),
                 std_format_end_escape_regex_obj, std::regex_constants::match_continuous))
      {
        std_format += "}}";
        pos += match.length();
      }
      else
      {
        vtkLogF(ERROR, "Invalid format specifier at position %zu in '%s'. Moving on", pos,
          format.c_str());
        // Invalid format Specifier, copy as-is and move past %
        // std_format += format[pos];
        pos++;
      }
    }
  }
  return std_format;
}

std::string to_std_format(const std::string& format)
{
  if (is_printf_format(format))
  {
    return printf_to_std_format(format);
  }
  return format; // Already in std::format style
}

VTK_ABI_NAMESPACE_END
};
