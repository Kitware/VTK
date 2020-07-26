/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIossFilesScanner.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkIossFilesScanner.h"

#include "vtkObjectFactory.h"

#include <vtksys/Directory.hxx>
#include <vtksys/FStream.hxx>
#include <vtksys/RegularExpression.hxx>
#include <vtksys/SystemTools.hxx>

#include <algorithm>
#include <cctype>

vtkStandardNewMacro(vtkIossFilesScanner);
//----------------------------------------------------------------------------
vtkIossFilesScanner::vtkIossFilesScanner() {}

//----------------------------------------------------------------------------
vtkIossFilesScanner::~vtkIossFilesScanner() {}

//----------------------------------------------------------------------------
bool vtkIossFilesScanner::IsMetaFile(const std::string& filename)
{
  vtksys::ifstream metafile(filename.c_str());
  if (!metafile.good())
  {
    return false;
  }

  std::string s;
  std::getline(metafile, s);
  if (s.size() == 0 ||
    s.size() != static_cast<size_t>(std::count_if(s.begin(), s.end(), [](unsigned char c) {
      return std::isprint(c);
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
std::set<std::string> vtkIossFilesScanner::GetFilesFromMetaFile(const std::string& filename)
{
  if (!vtkIossFilesScanner::IsMetaFile(filename))
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
    if (!fname.empty())
    {
      fname = vtksys::SystemTools::CollapseFullPath(fname, metafile_path);
      result.insert(fname);
    }
  }

  return result;
}

//----------------------------------------------------------------------------
std::set<std::string> vtkIossFilesScanner::GetRelatedFiles(
  const std::set<std::string>& originalSet, const std::vector<std::string>& directoryListing)
{
  if (originalSet.size() == 0)
  {
    return originalSet;
  }

  // clang-format off
  // Matches paths `{NAME}.e-s{RS}.{NUMRANKS}.{RANK}` or {NAME}.g-s{RS}.{NUMRANKS}.{RANK}`
  // where `-s{RS}` and/or `.{NUMRANKS}.{RANK}` is optional.
  vtksys::RegularExpression extensionRegex(R"(^(.*\.[eg][^-.]*)(-s.[0-9]+)?(\.[0-9]+(\.[0-9]+)?)?$)");
  // clang-format on

  std::set<std::string> prefixes;
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
    if (extensionRegex.find(fname_wo_path))
    {
      prefixes.insert(extensionRegex.match(1));
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
      dirlist.push_back(directory.GetFile(cc));
    }
  }
  else
  {
    dirlist = directoryListing;
  }

  for (const auto& filename : dirlist)
  {
    if (extensionRegex.find(filename) && prefixes.find(extensionRegex.match(1)) != prefixes.end())
    {
      result.insert(prefix + filename);
    }
  }

  return result;
}
//----------------------------------------------------------------------------
void vtkIossFilesScanner::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
