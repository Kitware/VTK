/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMetaImageReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMetaImageReader.h"

#include "vtkObjectFactory.h"
#include "vtkImageData.h"
#include "vtkXMLImageDataWriter.h"

#include <vtkstd/string>

#include <sys/stat.h>

//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkMetaImageReader, "1.7");
vtkStandardNewMacro(vtkMetaImageReader);

//----------------------------------------------------------------------------
vtkMetaImageReader::vtkMetaImageReader()
{
  this->MHDFileName = 0;
}

//----------------------------------------------------------------------------
vtkMetaImageReader::~vtkMetaImageReader()
{
  this->SetFileName(0);
}

//----------------------------------------------------------------------------
void vtkMetaImageReader::SetFileName(const char* fname)
{
  this->SetMHDFileName(fname);
}

//----------------------------------------------------------------------------
class vtkMetaImageReaderInternal
{
public:
  static void ReplaceString(vtkstd::string& source, const char* replace, 
    const char* with);
  static void ConvertToUnixSlashes(vtkstd::string& path);
  static vtkstd::string GetFilenamePath(const vtkstd::string& filename);
  static int StringEquals(const char* s1, const char* s2, size_t maxlen);
  static int GetLineFromStream(istream& is, vtkstd::string& line,
    bool *has_newline /* = 0 */, size_t maxlen /* = 0 */);
};

//----------------------------------------------------------------------------
// replace replace with with as many times as it shows up in source.
// write the result into source.
void vtkMetaImageReaderInternal::ReplaceString(vtkstd::string& source,
  const char* replace,
  const char* with)
{
  const char *src = source.c_str();
  char *searchPos = const_cast<char *>(strstr(src,replace));

  // get out quick if string is not found
  if (!searchPos)
    {
    return;
    }

  // perform replacements until done
  size_t replaceSize = strlen(replace);
  char *orig = strdup(src);
  char *currentPos = orig;
  searchPos = searchPos - src + orig;

  // initialize the result
  source.erase(source.begin(),source.end());
  do
    {
    *searchPos = '\0';
    source += currentPos;
    currentPos = searchPos + replaceSize;
    // replace
    source += with;
    searchPos = strstr(currentPos,replace);
    }
  while (searchPos);

  // copy any trailing text
  source += currentPos;
  free(orig);
}

//----------------------------------------------------------------------------
// convert windows slashes to unix slashes 
void vtkMetaImageReaderInternal::ConvertToUnixSlashes(vtkstd::string& path)
{
  vtkstd::string::size_type pos = 0;
  while((pos = path.find('\\', pos)) != vtkstd::string::npos)
    {
    path[pos] = '/';
    pos++;
    }
  // Remove all // from the path just like most unix shells
  int start_find = 0;

#ifdef _WIN32
  // However, on windows if the first characters are both slashes,
  // then keep them that way, so that network paths can be handled.
  start_find = 1;
#endif

  while((pos = path.find("//", start_find)) != vtkstd::string::npos)
    {
    vtkMetaImageReaderInternal::ReplaceString(path, "//", "/");
    }

  // remove any trailing slash
  if(path.size() && path[path.size()-1] == '/')
    {
    path = path.substr(0, path.size()-1);
    }

  // if there is a tilda ~ then replace it with HOME
  if(path.find("~") == 0)
    {
    if (getenv("HOME"))
      {
      path = vtkstd::string(getenv("HOME")) + path.substr(1);
      }
    }

  // if there is a /tmp_mnt in a path get rid of it!
  if(path.find("/tmp_mnt") == 0)
    {
    path = path.substr(8);
    }
}

//----------------------------------------------------------------------------
/**
 * Return path of a full filename (no trailing slashes).
 * Warning: returned path is converted to Unix slashes format.
 */
vtkstd::string vtkMetaImageReaderInternal::GetFilenamePath(const vtkstd::string& filename)
{
  vtkstd::string fn = filename;
  vtkMetaImageReaderInternal::ConvertToUnixSlashes(fn);

  vtkstd::string::size_type slash_pos = fn.rfind("/");
  if(slash_pos != vtkstd::string::npos)
    {
    return fn.substr(0, slash_pos);
    }
  return "";
}

//----------------------------------------------------------------------------
// Due to a buggy stream library on the HP and another on Mac OSX, we
// need this very carefully written version of getline.  Returns true
// if any data were read before the end-of-file was reached.
int vtkMetaImageReaderInternal::GetLineFromStream(istream& is, vtkstd::string& line,
  bool *has_newline /* = 0 */, size_t maxlen /* = 0 */)
{
  const int bufferSize = 1024;
  char buffer[bufferSize];
  line = "";
  bool haveData = 0;
  if ( has_newline )
    {
    *has_newline = 0;
    }

  // If no characters are read from the stream, the end of file has
  // been reached.
  while((is.getline(buffer, bufferSize), is.gcount() > 0))
    {
    haveData = 1;
    if ( maxlen > 0 && is.gcount() + line.size() > maxlen )
      {
      line.append(buffer, maxlen - line.size());
      }
    else
      {
      line.append(buffer);
      }

    // If newline character was read, the gcount includes the
    // character, but the buffer does not.  The end of line has been
    // reached.
    if(strlen(buffer) < static_cast<size_t>(is.gcount()))
      {
      if ( has_newline )
        {
        *has_newline = 1;
        }
      break;
      }

    // The fail bit may be set.  Clear it.
    is.clear(is.rdstate() & ~ios::failbit);
    }
  return haveData;
}

//----------------------------------------------------------------------------
int vtkMetaImageReaderInternal::StringEquals(const char* s1, const char* s2, size_t maxlen)
{
  if ( s1 == s2 )
    {
    return 1;
    }
  if ( !s1 || !s2 )
    {
    return 0;
    }

  return strncmp(s1, s2, maxlen) == 0;
}

//----------------------------------------------------------------------------
void vtkMetaImageReader::ExecuteInformation()
{
  const char* fname = this->MHDFileName;
  if ( !fname )
    {
    return;
    }
  struct stat fs;
  if ( stat( fname, &fs) )
    {
    vtkErrorMacro(<< "Initialize: Could not open file " << fname);
    return;
    }

  ifstream ifs(fname);
  if ( !fname )
    {
    return;
    }

  vtkstd::string path = vtkMetaImageReaderInternal::GetFilenamePath(fname);
  int ndims = 0;
  int bigendian = 0;
  int dims[3] = { 0, 0, 0 };
  double origin[3] = { 0.0, 0.0, 0.0 };
  double spacing[3] = { 1.0, 1.0, 1.0 };
  int data_type = VTK_UNSIGNED_CHAR;
  int number_of_channels = 1;
  vtkstd::string datafile = "";

  vtkstd::string line;
  int count = -1;
  while(vtkMetaImageReaderInternal::GetLineFromStream(ifs, line, 0, 0) )
    {
    count ++;
    vtkstd::string::size_type pos = line.find("=");
    if ( pos != vtkstd::string::npos )
      {
      vtkstd::string::size_type keylen, valuelen;
      const char* key = line.c_str();
      const char* value = line.c_str()+pos+1;
      const char* endkey = line.c_str()+pos-1;
      const char* endvalue = line.c_str()+line.size();
      while ( *key!= 0 )
        {
        if ( *key!= ' ' && *key!= '\t' && *key!= '\r' )
          {
          break;
          }
        key++;
        }

      while ( endkey > key )
        {
        if ( *endkey != ' ' && *endkey != '\t' && *endkey != '\r' && *endkey != 0 )
          {
          break;
          }
        endkey--;
        }
      keylen = endkey - key + 1;

      while ( *value != 0 )
        {
        if ( *value != ' ' && *value != '\t' && *value != '\r' )
          {
          break;
          }
        value++;
        }
      while ( endvalue > value )
        {
        if ( *endvalue != ' ' && *endvalue != '\t' && *endvalue != '\r' && *endvalue != 0 )
          {
          break;
          }
        endvalue--;
        }
      valuelen = endvalue - value + 1;
      //cout << "Key: [";
      //cout.write(key, keylen);
      //cout << "] -- Value: [";
      //cout.write(value, valuelen);
      //cout << "]" << endl;
      if ( vtkMetaImageReaderInternal::StringEquals(key, "ObjectType", keylen) )
        {
        if ( !vtkMetaImageReaderInternal::StringEquals(value, "Image", valuelen) )
          {
          vtkErrorMacro(<<"Only understand image data. This is not an image data");
          return;
          }
        vtkDebugMacro(<< "* Have image data");
        }
      else if ( vtkMetaImageReaderInternal::StringEquals(key, "NDims", keylen) )
        {
        sscanf(value, "%d", &ndims);
        if ( ndims <= 0 || ndims >= 4)
          {
          vtkErrorMacro(<< "Only understands image data of 1, 2, or 3 dimensions. This image has "
            << ndims << " dimensions");
          return;
          }
        vtkDebugMacro(<< "* This image has " << ndims << " dimensions");
        }
      else if ( vtkMetaImageReaderInternal::StringEquals(key, "BinaryData", keylen) )
        {
        if ( !vtkMetaImageReaderInternal::StringEquals(value, "True", valuelen) && 
          vtkMetaImageReaderInternal::StringEquals(value, "true", valuelen) )
          {
          vtkErrorMacro(<< "Only understand binary image data. This one has BinaryData set to: " 
            << value);
          return;
          }
        vtkDebugMacro(<< "* This image has binary data");
        }
      else if ( vtkMetaImageReaderInternal::StringEquals(key, "BinaryDataByteOrderMSB", keylen) )
        {
        if ( !vtkMetaImageReaderInternal::StringEquals(value, "True", valuelen) && 
          vtkMetaImageReaderInternal::StringEquals(value, "true", valuelen) )
          {
          bigendian = 1;
          }
        bigendian = 0;
        vtkDebugMacro(<< "* This image has data which is " << (bigendian?"big":"little") << " endian");
        }
      else if ( vtkMetaImageReaderInternal::StringEquals(key, "DimSize", keylen) )
        {
        sscanf(value, "%d %d %d", dims, dims+1, dims+2);
        vtkDebugMacro(<< "* This image has dimensions " << dims[0] << " " << dims[1] << " " << dims[2]);
        }
      else if ( vtkMetaImageReaderInternal::StringEquals(key, "ElementSpacing", keylen) )
        {
        sscanf(value, "%lf %lf %lf", spacing, spacing+1, spacing+2);
        vtkDebugMacro(<< "* This image has spacing " << spacing[0] << " " << spacing[1] << " " << spacing[2]);
        }
      else if ( vtkMetaImageReaderInternal::StringEquals(key, "Position", keylen) )
        {
        sscanf(value, "%lf %lf %lf", origin, origin+1, origin+2);
        vtkDebugMacro(<< "* This image has origin " << origin[0] << " " << origin[1] << " " << origin[2]);
        }
      else if ( vtkMetaImageReaderInternal::StringEquals(key, "ElementNumberOfChannels", keylen) )
        {
        sscanf(value, "%d", &number_of_channels);
        if ( ndims <= 0 )
          {
          vtkErrorMacro(<< "Only understands image data of 1 or more channels. This image has "
            << number_of_channels << " dimensions");
          return;
          }
        vtkDebugMacro(<< "* This image has " << number_of_channels << " channels");
        }
      else if ( vtkMetaImageReaderInternal::StringEquals(key, "ElementType", keylen) )
        {
        if ( vtkMetaImageReaderInternal::StringEquals(value, "MET_CHAR", valuelen) || 
          vtkMetaImageReaderInternal::StringEquals(value, "MET_CHAR_ARRAY", valuelen) )
          {
          data_type = VTK_CHAR;
          }
        else if ( vtkMetaImageReaderInternal::StringEquals(value, "MET_UCHAR", valuelen) || 
          vtkMetaImageReaderInternal::StringEquals(value, "MET_UCHAR_ARRAY", valuelen) )
          {
          data_type = VTK_UNSIGNED_CHAR;
          }
        else if ( vtkMetaImageReaderInternal::StringEquals(value, "MET_SHORT", valuelen) || 
          vtkMetaImageReaderInternal::StringEquals(value, "MET_SHORT_ARRAY", valuelen) )
          {
          data_type = VTK_SHORT;
          }
        else if ( vtkMetaImageReaderInternal::StringEquals(value, "MET_USHORT", valuelen) || 
          vtkMetaImageReaderInternal::StringEquals(value, "MET_USHORT_ARRAY", valuelen) )
          {
          data_type = VTK_UNSIGNED_SHORT;
          }
        else if ( vtkMetaImageReaderInternal::StringEquals(value, "MET_INT", valuelen) || 
          vtkMetaImageReaderInternal::StringEquals(value, "MET_INT_ARRAY", valuelen) )
          {
          data_type = VTK_INT;
          }
        else if ( vtkMetaImageReaderInternal::StringEquals(value, "MET_UINT", valuelen) || 
          vtkMetaImageReaderInternal::StringEquals(value, "MET_UINT_ARRAY", valuelen) )
          {
          data_type = VTK_UNSIGNED_INT;
          }
        else if ( vtkMetaImageReaderInternal::StringEquals(value, "MET_LONG", valuelen) || 
          vtkMetaImageReaderInternal::StringEquals(value, "MET_LONG_ARRAY", valuelen) )
          {
          data_type = VTK_LONG;
          }
        else if ( vtkMetaImageReaderInternal::StringEquals(value, "MET_ULONG", valuelen) || 
          vtkMetaImageReaderInternal::StringEquals(value, "MET_ULONG_ARRAY", valuelen) )
          {
          data_type = VTK_UNSIGNED_LONG;
          }
        else if ( vtkMetaImageReaderInternal::StringEquals(value, "MET_FLOAT", valuelen) || 
          vtkMetaImageReaderInternal::StringEquals(value, "MET_FLOAT_ARRAY", valuelen) )
          {
          data_type = VTK_FLOAT;
          }
        else if ( vtkMetaImageReaderInternal::StringEquals(value, "MET_DOUBLE", valuelen) || 
          vtkMetaImageReaderInternal::StringEquals(value, "MET_DOUBLE_ARRAY", valuelen) )
          {
          data_type = VTK_DOUBLE;
          }
        else
          {
          vtkErrorMacro(<< "Unknown data type: " << value);
          return;
          }
        }
      else if ( vtkMetaImageReaderInternal::StringEquals(key, "ElementDataFile", keylen) )
        {
        if ( value[0] == '/' || 
          ( value[1] == ':' && ( value[2] == '/' || value[2] == '\\' ) ) ||
          ( value[0] == '\\' && value[1] == '\\' ) )
          {
          datafile = "";
          datafile.append(value, valuelen);
          vtkDebugMacro("Use absolute path");
          }
        else
          {
          datafile = path;
          datafile += "/";
          datafile.append(value, valuelen);
          vtkDebugMacro("Use relative path");
          }

        if ( stat( datafile.c_str(), &fs) )
          {
          vtkErrorMacro(<< "Initialize: Could not open file " << datafile.c_str());
          return;
          }
        vtkDebugMacro(<< "* Use data file: " << datafile.c_str());
        }
      }
    else
      {
      vtkErrorMacro(<< "Problem parsing line: " << count << " of file: " << fname);
      }
    }
  if ( ndims <= 0 )
    {
    vtkErrorMacro(<< "Number of dimensions not specified");
    return;
    }
  int cc;
  for ( cc = 0; cc < ndims; cc ++ )
    {
    if ( dims[cc] <= 0 )
      {
      vtkErrorMacro(<< "Dimension " << cc << " is " << dims[cc]);
      }
    if ( spacing[cc] == 0 )
      {
      vtkErrorMacro(<< "Spacing " << cc << " is 0.");
      }
    }
  for ( cc = ndims; cc < 3; cc ++ )
    {
    dims[cc] = 1;
    }

  this->SetDataScalarType(data_type);
  this->SetNumberOfScalarComponents(number_of_channels);
  this->SetDataExtent(0, dims[0]-1, 0, dims[1]-1, 0, dims[2]-1);
  this->SetFileDimensionality(3);
  this->SetDataSpacing(spacing);
  this->SetDataOrigin(origin);

  if ( bigendian )
    {
    this->SetDataByteOrderToBigEndian();
    }
  else
    {
    this->SetDataByteOrderToLittleEndian();
    }

  vtkDebugMacro("Read raw file: " << datafile.c_str());
  this->Superclass::SetFileName(datafile.c_str());
  this->Superclass::ExecuteInformation();
}

//----------------------------------------------------------------------------
int vtkMetaImageReader::CanReadFile(const char* fname)
{
  if ( !fname )
    {
    return 0;
    }
  struct stat fs;
  if ( stat( fname, &fs) )
    {
    return 0;
    }

  ifstream ifs(fname);
  if ( !ifs )
    {
    return 0;
    }
  int have_dims = 0;
  int have_ndims = 0;
  int have_datafile = 0;
  int count = 0;
  vtkstd::string line;
  while(vtkMetaImageReaderInternal::GetLineFromStream(ifs, line, 0, 1024) )
    {
    count ++;
    if ( count > 10 )
      {
      break;
      }
    vtkstd::string::size_type pos = line.find("=");
    if ( pos != vtkstd::string::npos )
      {
      vtkstd::string::size_type keylen;
      const char* key = line.c_str();
      const char* endkey = line.c_str()+pos-1;
      while ( *key!= 0 )
        {
        if ( *key!= ' ' && *key!= '\t' && *key!= '\r' )
          {
          break;
          }
        key++;
        }

      while ( endkey > key )
        {
        if ( *endkey != ' ' && *endkey != '\t' && *endkey != '\r' && *endkey != 0 )
          {
          break;
          }
        endkey--;
        }
      keylen = endkey - key + 1;

      if ( vtkMetaImageReaderInternal::StringEquals(key, "NDims", keylen) )
        {
        have_ndims = 1;
        }
      else if ( vtkMetaImageReaderInternal::StringEquals(key, "DimSize", keylen) )
        {
        have_dims = 1;
        }
      else if ( vtkMetaImageReaderInternal::StringEquals(key, "ElementDataFile", keylen) )
        {
        have_datafile = 1;
        }
      }
    else
      {
      return 0;
      }
    }
  if ( !have_dims || !have_ndims || !have_datafile )
    {
    return 0;
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkMetaImageReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "MHDFileName: " << (this->MHDFileName?this->MHDFileName:"(none)") << endl;
}
