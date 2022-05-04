/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIOSSFilesScanner.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkIOSSFilesScanner.h"

#include "vtkObjectFactory.h"

#include <vtksys/Directory.hxx>
#include <vtksys/FStream.hxx>
#include <vtksys/RegularExpression.hxx>
#include <vtksys/SystemTools.hxx>

#include <algorithm>
#include <cctype>

namespace
{
void rtrim(std::string& s)
{
  s.erase(
    std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(),
    s.end());
}
}

vtkStandardNewMacro(vtkIOSSFilesScanner);
//----------------------------------------------------------------------------
vtkIOSSFilesScanner::vtkIOSSFilesScanner() = default;

//----------------------------------------------------------------------------
vtkIOSSFilesScanner::~vtkIOSSFilesScanner() = default;

//----------------------------------------------------------------------------
bool vtkIOSSFilesScanner::IsMetaFile(const std::string& filename)
{
  vtksys::ifstream metafile(filename.c_str());
  if (!metafile.good())
  {
    return false;
  }

  std::string s;
  std::getline(metafile, s);
  rtrim(s); // strip trailing white-spaces
  if (s.empty() ||
    s.size() != static_cast<size_t>(std::count_if(s.begin(), s.end(), [](unsigned char c) {
      return std::isprint(c) || std::isspace(c);
    })))
  {
    return false;
  }

  // let's just check that the first value is a valid filename.
  auto metafile_path =
    vtksys::SystemTools::GetFilenamePath(vtksys::SystemTools::CollapseFullPath(filename));
  auto fpath = vtksys::SystemTools::CollapseFullPath(s, metafile_path);
  return (vtksys::SystemTools::FileExists(fpath, /*isFile=*/true));
}

//----------------------------------------------------------------------------
std::set<std::string> vtkIOSSFilesScanner::GetFilesFromMetaFile(const std::string& filename)
{
  if (!vtkIOSSFilesScanner::IsMetaFile(filename))
  {
    return { filename };
  }

  std::set<std::string> result;
  const auto metafile_path =
    vtksys::SystemTools::GetFilenamePath(vtksys::SystemTools::CollapseFullPath(filename));
  vtksys::ifstream metafile(filename.c_str());
  while (metafile.good() && !metafile.eof())
  {
    std::string fname;
    std::getline(metafile, fname);
    rtrim(fname);
    if (!fname.empty())
    {
      fname = vtksys::SystemTools::CollapseFullPath(fname, metafile_path);
      result.insert(fname);
    }
  }

  return result;
}

//----------------------------------------------------------------------------
std::set<std::string> vtkIOSSFilesScanner::GetRelatedFiles(
  const std::set<std::string>& originalSet, const std::vector<std::string>& directoryListing)
{
  if (originalSet.empty())
  {
    return originalSet;
  }

  // Matches paths `{NAME}.e-s{RS}.{NUMRANKS}.{RANK}` or {NAME}.g-s{RS}.{NUMRANKS}.{RANK}`
  // where `-s{RS}` and/or `.{NUMRANKS}.{RANK}` is optional.
  vtksys::RegularExpression extensionRegexExodus(
    R"(^(.*\.[eg][^-.]*)(-s.[0-9]+)?(\.[0-9]+(\.[0-9]+)?)?$)");
  vtksys::RegularExpression extensionRegexExodus2(
    R"(^(.*\.exo[^-.]*)(-s.[0-9]+)?(\.[0-9]+(\.[0-9]+)?)?$)");
  vtksys::RegularExpression extensionRegexCGNS(
    R"(^(.*\.cgns[^-.]*)(-s.[0-9]+)?(\.[0-9]+(\.[0-9]+)?)?$)");

  // extract process count from the filename, if any, otherwise -1.
  auto getProcessCount = [](const std::string& fname) {
    vtksys::RegularExpression procRegEx(R"(^.*\.([0-9]+)\.[0-9]+$)");
    if (procRegEx.find(fname))
    {
      return std::atoi(procRegEx.match(1).c_str());
    }
    return -1;
  };

  std::map<std::string, int> prefixes;
  std::set<std::string> result;
  for (const auto& fname : originalSet)
  {
    std::string unix_fname = fname;
    vtksys::SystemTools::ConvertToUnixSlashes(unix_fname);
    result.insert(unix_fname);

    // prefixes are used to find other files related to this one.
    auto fname_wo_path = vtksys::SystemTools::GetFilenameName(unix_fname);
    if (fname_wo_path.empty())
    {
      // this happens if the unix_fname was not a full path.
      fname_wo_path = unix_fname;
    }

    if (extensionRegexExodus2.find(fname_wo_path))
    {
      prefixes.insert(
        std::make_pair(extensionRegexExodus2.match(1), getProcessCount(fname_wo_path)));
    }
    else if (extensionRegexExodus.find(fname_wo_path))
    {
      prefixes.insert(
        std::make_pair(extensionRegexExodus.match(1), getProcessCount(fname_wo_path)));
    }
    else if (extensionRegexCGNS.find(fname_wo_path))
    {
      prefixes.insert(std::make_pair(extensionRegexCGNS.match(1), getProcessCount(fname_wo_path)));
    }
  }

  // For now, we only can the directory for the first file. Not sure if we
  // should scan all directories for all files in the original set.
  auto prefix = vtksys::SystemTools::GetFilenamePath(*result.begin());
  if (!prefix.empty())
  {
    // add dir separator to make `join` easier later on.
    prefix += "/";
  }
  std::vector<std::string> dirlist;
  if (directoryListing.empty())
  {
    vtksys::Directory directory;
    if (!directory.Load(prefix))
    {
      return result;
    }
    for (unsigned long cc = 0, max = directory.GetNumberOfFiles(); cc < max; ++cc)
    {
      dirlist.emplace_back(directory.GetFile(cc));
    }
  }
  else
  {
    dirlist = directoryListing;
  }

  for (const auto& filename : dirlist)
  {
    std::string dbaseName;
    if (extensionRegexExodus.find(filename))
    {
      dbaseName = extensionRegexExodus.match(1);
    }
    else if (extensionRegexCGNS.find(filename))
    {
      dbaseName = extensionRegexCGNS.match(1);
    }
    else
    {
      continue;
    }

    const int procCount = getProcessCount(filename);
    auto piter = prefixes.find(dbaseName);
    if (piter != prefixes.end() && piter->second == procCount)
    {
      result.insert(prefix + filename);
    }
  }

  return result;
}

//----------------------------------------------------------------------------
bool vtkIOSSFilesScanner::DoTestFilePatternMatching()
{
  auto verify = [](const std::set<std::string>& original,
                  const std::vector<std::string>& dir_listing,
                  const std::set<std::string>& expected) {
    return (vtkIOSSFilesScanner::GetRelatedFiles(original, dir_listing) == expected);
  };

  if (!verify({ "mysimoutput.e-s.000" },
        { "mysimoutput.e-s.000", "mysimoutput.e-s.001", "mysimoutput.e-s.002" },
        { "mysimoutput.e-s.000", "mysimoutput.e-s.001", "mysimoutput.e-s.002" }))
  {
    return false;
  }

  if (!verify({ "mysimoutput.exo-s.000" },
        { "mysimoutput.exo-s.000", "mysimoutput.exo-s.001", "mysimoutput.exo-s.002" },
        { "mysimoutput.exo-s.000", "mysimoutput.exo-s.001", "mysimoutput.exo-s.002" }))
  {
    return false;
  }

  if (!verify({ "mysimoutput.exo-s00" },
        { "mysimoutput.exo-s00", "mysimoutput.exo-s01", "mysimoutput.exo-s02" },
        { "mysimoutput.exo-s00", "mysimoutput.exo-s01", "mysimoutput.exo-s02" }))
  {
    return false;
  }

  if (!verify({ "/tmp/mysimoutput.e-s.000" },
        { "mysimoutput.e-s.000", "mysimoutput.e-s.001", "mysimoutput.e-s.002" },
        { "/tmp/mysimoutput.e-s.000", "/tmp/mysimoutput.e-s.001", "/tmp/mysimoutput.e-s.002" }))
  {
    return false;
  }

  if (!verify({ "C:\\Directory\\mysimoutput.e-s.000" },
        { "mysimoutput.e-s.000", "mysimoutput.e-s.001", "mysimoutput.e-s.002" },
        { "C:/Directory/mysimoutput.e-s.000", "C:/Directory/mysimoutput.e-s.001",
          "C:/Directory/mysimoutput.e-s.002" }))
  {
    return false;
  }

  if (!verify({ "/tmp space/mysimoutput.e-s.000" },
        { "mysimoutput.e-s.000", "mysimoutput.e-s.001", "mysimoutput.e-s.002" },
        { "/tmp space/mysimoutput.e-s.000", "/tmp space/mysimoutput.e-s.001",
          "/tmp space/mysimoutput.e-s.002" }))
  {
    return false;
  }

  if (!verify({ "C:\\Directory space\\mysimoutput.e-s.000" },
        { "mysimoutput.e-s.000", "mysimoutput.e-s.001", "mysimoutput.e-s.002" },
        { "C:/Directory space/mysimoutput.e-s.000", "C:/Directory space/mysimoutput.e-s.001",
          "C:/Directory space/mysimoutput.e-s.002" }))
  {
    return false;
  }

  if (!verify({ "/tmp/can.e.4.0" },
        { "can.e.4.0", "can.e.4.1", "can.e.4.2", "can.e.4.3", "can.e.2.0", "can.e.2.1" },
        { "/tmp/can.e.4.0", "/tmp/can.e.4.1", "/tmp/can.e.4.2", "/tmp/can.e.4.3" }))
  {
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------
void vtkIOSSFilesScanner::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
