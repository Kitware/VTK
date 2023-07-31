// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkURILoader.h"

#include "vtkFileResourceStream.h"
#include "vtkMemoryResourceStream.h"
#include "vtkObjectFactory.h"
#include "vtkValueFromString.h"

#include <vtksys/Base64.h>
#include <vtksys/SystemTools.hxx>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
struct vtkURILoader::vtkInternals
{
  vtkSmartPointer<vtkURI> BaseURI{};
};

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkURILoader);

//------------------------------------------------------------------------------
vtkURILoader::vtkURILoader()
  : Impl{ new vtkInternals{} }
{
}

//------------------------------------------------------------------------------
vtkURILoader::~vtkURILoader() = default;

//------------------------------------------------------------------------------
bool vtkURILoader::SetBaseURI(const std::string& uri)
{
  if (uri.empty()) // remove base uri
  {
    return this->SetBaseURI(nullptr);
  }

  auto parsed = vtkURI::Parse(uri);
  if (!parsed)
  {
    return false;
  }

  return this->SetBaseURI(parsed);
}

//------------------------------------------------------------------------------
bool vtkURILoader::SetBaseURI(vtkURI* uri)
{
  if (uri && !uri->IsAbsolute())
  {
    vtkErrorMacro("Base URI must be absolute or null");
    return false;
  }

  this->Impl->BaseURI = uri;
  return true;
}

//------------------------------------------------------------------------------
bool vtkURILoader::SetBaseFileName(const std::string& filepath)
{
  if (!vtksys::SystemTools::FileExists(filepath))
  {
    vtkErrorMacro("Can not find \"" << filepath << "\"");
    return false;
  }

  auto fullPath = vtksys::SystemTools::CollapseFullPath(filepath);
  if (fullPath.front() != '/') // add / for windows path
  {
    fullPath.insert(fullPath.begin(), '/');
  }

  this->Impl->BaseURI = vtkURI::Make("file", "", vtkURI::PercentEncode(fullPath));

  return this->Impl->BaseURI;
}

//------------------------------------------------------------------------------
bool vtkURILoader::SetBaseDirectory(const std::string& dirpath)
{
  if (!vtksys::SystemTools::FileExists(dirpath))
  {
    vtkErrorMacro("Can not find \"" << dirpath << "\"");
    return false;
  }

  if (!vtksys::SystemTools::FileIsDirectory(dirpath))
  {
    vtkErrorMacro("\"" << dirpath << "\" is not a directory. Use SetBaseFileName.");
    return false;
  }

  // add /. because relative URI truncate the last path component
  auto fullPath = vtksys::SystemTools::CollapseFullPath(dirpath) + "/.";
  if (fullPath.front() != '/') // add / for windows path
  {
    fullPath.insert(fullPath.begin(), '/');
  }

  this->Impl->BaseURI = vtkURI::Make("file", "", vtkURI::PercentEncode(fullPath));

  return this->Impl->BaseURI;
}

//------------------------------------------------------------------------------
vtkURI* vtkURILoader::GetBaseURI() const
{
  return this->Impl->BaseURI;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkURI> vtkURILoader::Resolve(const vtkURI* uri)
{
  return vtkURI::Resolve(this->Impl->BaseURI, uri);
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkResourceStream> vtkURILoader::Load(const char* uri, std::size_t size)
{
  auto parsed = vtkURI::Parse(uri, size);
  if (!parsed)
  {
    return nullptr;
  }

  return this->Load(parsed);
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkResourceStream> vtkURILoader::Load(const vtkURI* uri)
{
  if (!uri->IsReference())
  {
    vtkErrorMacro("Given URI must be a valid URI reference.");
    return nullptr;
  }

  auto resolved = vtkURI::Resolve(this->Impl->BaseURI, uri);
  if (!resolved)
  {
    return nullptr;
  }

  return this->DoLoad(*resolved);
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkResourceStream> vtkURILoader::LoadResolved(const vtkURI* uri)
{
  if (!uri)
  {
    vtkErrorMacro("uri must not be null");
    return nullptr;
  }

  if (!uri->IsFull())
  {
    vtkErrorMacro("uri must be complete to be loaded");
    return nullptr;
  }

  return this->DoLoad(*uri);
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkResourceStream> vtkURILoader::DoLoad(const vtkURI& uri)
{
  // Case insensitive
  const auto scheme = vtksys::SystemTools::LowerCase(uri.GetScheme().GetValue());
  if (scheme == "file")
  {
    return this->LoadFile(uri);
  }
  else if (scheme == "data")
  {
    return this->LoadData(uri);
  }

  vtkErrorMacro("Unknown URI scheme for \"" << uri.ToString() << "\"");
  return nullptr;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkResourceStream> vtkURILoader::LoadFile(const vtkURI& uri)
{
  // Decode
  auto decodedPath = vtkURI::PercentDecode(uri.GetPath().GetValue());

  auto cpath = decodedPath.c_str();
#if defined(_WIN32)
  if (*cpath == '/') // discard first / on Windows
  {
    ++cpath;
  }
#endif

  auto stream = vtkSmartPointer<vtkFileResourceStream>::New();
  if (!stream->Open(cpath))
  {
    vtkErrorMacro("Failed to open file \"" << cpath << "\"");
    return nullptr;
  }

  return stream;
}

namespace
{

// https://datatracker.ietf.org/doc/html/rfc2397#section-3
// Data URI "header" info
struct DataURIInfo
{
  std::string Type{};  // <mediatype>
  bool base64{};       // true if ;base64 has been specified at the end of parameters
  const char* Begin{}; // pointer to the first character of the data stream (raw or base64)
  const char* End{};   // pointer one past the last character of the data stream
};

// Helper to extract data URI header
DataURIInfo ExtractDataURI(const char* begin, const char* end)
{
  DataURIInfo output{};

  if (begin == end)
  {
    vtkErrorWithObjectMacro(nullptr, "Empty data URI");
    return output;
  }

  const auto typeEnd = std::find_if(begin, end, [](char c) { return c == ';' || c == ','; });

  if (typeEnd == end)
  {
    vtkErrorWithObjectMacro(nullptr, "No ',' in data URI");
    return output;
  }

  if (begin != typeEnd) // type specified
  {
    auto type = std::string{ begin, typeEnd };
    begin = typeEnd;

    output.Type = std::move(type);
  }
  else
  {
    output.Type = "text/plain;charset=US-ASCII";
  }

  while (*begin == ';')
  {
    ++begin; // discard ;
    const auto paramEnd = std::find_if(begin, end, [](char c) { return c == ';' || c == ','; });

    if (paramEnd == end)
    {
      vtkErrorWithObjectMacro(nullptr, "Truncated data URI header");
      return output;
    }

    if (*paramEnd == ',')
    {
      auto param = std::string{ begin, paramEnd };
      if (param == "base64")
      {
        output.base64 = true;
      }

      // Parameters aren't stored since unused, but if needed its where it should be done
      begin = paramEnd;
      break;
    }

    // Parameters aren't stored since unused, but if needed its where it should be done
    begin = paramEnd;
  }

  if (*begin != ',')
  {
    vtkErrorWithObjectMacro(nullptr, "Incomplete data URI, missing ','");
    return output;
  }

  output.Begin = begin + 1;
  output.End = end;

  return output;
}

}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkResourceStream> vtkURILoader::LoadData(const vtkURI& uri)
{
  const auto& path = uri.GetPath().GetValue();
  const auto info = ExtractDataURI(path.data(), path.data() + path.size());

  if (info.base64)
  {
    const auto size = static_cast<std::size_t>(std::distance(info.Begin, info.End));
    if (size % 4 != 0)
    {
      vtkErrorMacro("Truncated base64 data. " << size << " is not a multiple of 4.");
      return nullptr;
    }

    std::vector<unsigned char> data;
    data.resize(size / 4 * 3); // 4 base64 value -> 3 bytes
    const auto decoded = vtksysBase64_Decode(
      reinterpret_cast<const unsigned char*>(info.Begin), data.size(), data.data(), size);
    data.resize(decoded);

    auto stream = vtkSmartPointer<vtkMemoryResourceStream>::New();
    stream->SetBuffer(std::move(data));

    return stream;
  }

  // raw data: convert %xx in string if any
  auto data = vtkURI::PercentDecode(info.Begin, std::distance(info.Begin, info.End));

  auto stream = vtkSmartPointer<vtkMemoryResourceStream>::New();
  stream->SetBuffer(std::move(data));

  return stream;
}

//------------------------------------------------------------------------------
void vtkURILoader::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "Base URI: " << (this->HasBaseURI() ? this->GetBaseURI()->ToString() : "None")
     << std::endl;
  Superclass::PrintSelf(os, indent.GetNextIndent());
}
