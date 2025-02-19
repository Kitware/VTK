// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) 2021, Los Alamos National Laboratory
// SPDX-FileCopyrightText: Copyright (c) 2021. Triad National Security, LLC
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-LANL-Triad-USGov
#include "X3D_tokens.hxx"

#include <iomanip>
#include <ios>
#include <sstream>

using namespace std;

namespace X3D
{
VTK_ABI_NAMESPACE_BEGIN

// Get w characters from is and return trimmed string.
string fixed_get(istream& is, unsigned int width)
{
  const char ws[] = " \t\n\r\f\v"; // whitespace characters

  if (width)
  { // read next width characters
    string result;
    char c;
    for (unsigned int i = 0; i < width; i++)
      if (is.get(c) && c != '\n')
        result.push_back(c);
      else
        throw ScanError("Unexpected EOL following \"" + result + "\" at character: ", is.tellg());
    result.erase(0, result.find_first_not_of(ws)); // trim leading ws
    result.erase(result.find_last_not_of(ws) + 1); // trim trailing ws
    return result;
  }
  else
  { // read next ws terminated string
    string result;
    is >> result;
    return result;
  }
}

// Return string containing s formatted as Aw
string Aformat::operator()(string s)
{
  ostringstream f;

  value = s;
  f << left << std::setw(width) << s;
  if (width && f.str().size() > width) // width overflow;
    return f.str().substr(0, width);   // truncate string
  else
    return f.str();
}

// Return string containing i formatted as Iw.
string Iformat::operator()(int i)
{
  ostringstream f;

  value = i;
  f << std::setw(width) << i;
  if (width && f.str().size() > width) // width overflow;
    return string(width, '*');         // return width '*'s
  else
    return f.str();
}

// Return string containing d formatted as 1PEw.d.
string PEformat::operator()(double x)
{
  ostringstream f;

  value = x;
  f << scientific << showpoint << uppercase << std::setprecision(precision) << std::setw(width)
    << x;
  if (width && f.str().size() > width) // width overflow;
    return string(width, '*');         // return width '*'s
  else
    return f.str();
}
VTK_ABI_NAMESPACE_END
}
