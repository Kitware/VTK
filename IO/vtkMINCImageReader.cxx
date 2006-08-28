/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMINCImageReader.cxx

Copyright (c) 2006 Atamai, Inc.

Use, modification and redistribution of the software, in source or
binary forms, are permitted provided that the following terms and
conditions are met:

1) Redistribution of the source code, in verbatim or modified
   form, must retain the above copyright notice, this license,
   the following disclaimer, and any notices that refer to this
   license and/or the following disclaimer.

2) Redistribution in binary form must include the above copyright
   notice, a copy of this license and the following disclaimer
   in the documentation or with other materials provided with the
   distribution.

3) Modified copies of the source code must be clearly marked as such,
   and must not be misrepresented as verbatim copies of the source code.

THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES PROVIDE THE SOFTWARE "AS IS"
WITHOUT EXPRESSED OR IMPLIED WARRANTY INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE.  IN NO EVENT SHALL ANY COPYRIGHT HOLDER OR OTHER PARTY WHO MAY
MODIFY AND/OR REDISTRIBUTE THE SOFTWARE UNDER THE TERMS OF THIS LICENSE
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, LOSS OF DATA OR DATA BECOMING INACCURATE
OR LOSS OF PROFIT OR BUSINESS INTERRUPTION) ARISING IN ANY WAY OUT OF
THE USE OR INABILITY TO USE THE SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGES.

=========================================================================*/

#include "vtkMINCImageReader.h"

#include "vtkObjectFactory.h"

#include "vtkImageData.h"
#include "vtkStringArray.h"
#include "vtkCharArray.h"
#include "vtkSignedCharArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkShortArray.h"
#include "vtkIntArray.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkMatrix4x4.h"
#include "vtkSmartPointer.h"
#include "vtkMath.h"

#include "vtkType.h"

#include <stdlib.h>
#include <ctype.h>
#include <float.h>
#include <vtknetcdf/netcdf.h>
#include <vtkstd/string>
#include <vtkstd/map>

#define VTK_MINC_MAX_DIMS 8

//--------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkMINCImageReader, "1.6");
vtkStandardNewMacro(vtkMINCImageReader);

//-------------------------------------------------------------------------
// A container for mapping attribute names to arrays
class vtkMINCImageReaderAttributeMap
{
public:
  typedef vtkstd::map<vtkstd::string, vtkSmartPointer<vtkObject> > MapType;

  static vtkMINCImageReaderAttributeMap *New() {
    return new vtkMINCImageReaderAttributeMap; };

  void Delete() {
    delete this; };

  void Clear() {
    this->Map.clear(); };

  void AddArray(vtkDataArray *array) {
    this->AddObject(array->GetName(), array); };

  void AddArray(vtkStringArray *array) {
    this->AddObject(array->GetName(), array); };

  vtkDataArray *GetDataArray(const char *name) const {
    return vtkDataArray::SafeDownCast(this->GetObject(name)); };

  vtkCharArray *GetCharArray(const char *name) const {
    return vtkCharArray::SafeDownCast(this->GetObject(name)); };

  vtkDoubleArray *GetDoubleArray(const char *name) const {
    return vtkDoubleArray::SafeDownCast(this->GetObject(name)); };

  vtkStringArray *GetStringArray(const char *name) const {
    return vtkStringArray::SafeDownCast(this->GetObject(name)); };

protected:
  void AddObject(const char *name, vtkObject *object) {
    this->Map[name] = object; };

  vtkObject *GetObject(const char *name) const {
    MapType::const_iterator iter = this->Map.find(name);
    if (iter != this->Map.end()) { return iter->second; };
    return 0; };

private:
  vtkMINCImageReaderAttributeMap() : Map() {};
  ~vtkMINCImageReaderAttributeMap() {};

  MapType Map;
};

//-------------------------------------------------------------------------
vtkMINCImageReader::vtkMINCImageReader()
{
  this->NumberOfFrames = 1;
  this->FrameNumber = 0;
  this->OrientationMatrix = vtkMatrix4x4::New();
  this->RescaleIntercept = 0.0;
  this->RescaleSlope = 1.0;

  this->MINCImageType = 0;
  this->MINCImageTypeSigned = 1;

  this->MINCValidRange[0] = 0.0;
  this->MINCValidRange[1] = 1.0;

  this->MINCImageRange[0] = 0.0;
  this->MINCImageRange[1] = 1.0;

  this->MINCImageMinMaxDims = 0;
  this->MINCImageMin = vtkDoubleArray::New();
  this->MINCImageMax = vtkDoubleArray::New();

  this->DimensionNames = vtkStringArray::New();
  this->DimensionLengths = vtkIdTypeArray::New();
  this->VariableNames = vtkStringArray::New();
  this->AttributeNames = vtkMINCImageReaderAttributeMap::New();
  this->AttributeValues = vtkMINCImageReaderAttributeMap::New();
  this->StringStore = 0;

  this->FileNameHasChanged = 0;
}

//-------------------------------------------------------------------------
vtkMINCImageReader::~vtkMINCImageReader()
{
  if (this->OrientationMatrix)
    {
    this->OrientationMatrix->Delete();
    this->OrientationMatrix = 0;
    }
  if (this->DimensionNames)
    {
    this->DimensionNames->Delete();
    this->DimensionNames = 0;
    }
  if (this->DimensionLengths)
    {
    this->DimensionLengths->Delete();
    this->DimensionLengths = 0;
    }
  if (this->VariableNames)
    {
    this->VariableNames->Delete();
    this->VariableNames = 0;
    }
  if (this->AttributeNames)
    {
    this->AttributeNames->Delete();
    this->AttributeNames = 0;
    }
  if (this->AttributeValues)
    {
    this->AttributeValues->Delete();
    this->AttributeValues = 0;
    }
  if (this->MINCImageMin)
    {
    this->MINCImageMin->Delete();
    this->MINCImageMin = 0;
    }
  if (this->MINCImageMax)
    {
    this->MINCImageMax->Delete();
    this->MINCImageMax = 0;
    }
  if (this->StringStore)
    {
    this->StringStore->Delete();
    this->StringStore = 0;
    }
}

//-------------------------------------------------------------------------
void vtkMINCImageReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "OrientationMatrix: " << this->OrientationMatrix << "\n";
  if (this->OrientationMatrix)
    {
    this->OrientationMatrix->PrintSelf(os, indent.GetNextIndent());
    }
  os << indent << "RescaleSlope: " << this->RescaleSlope << "\n";
  os << indent << "RescaleIntercept: " << this->RescaleIntercept << "\n";
  os << indent << "NumberOfFrames: " << this->NumberOfFrames << "\n";
  os << indent << "FrameNumber: " << this->FrameNumber << "\n";
  os << indent << "DimensionNames: " << this->DimensionNames << "\n";
  os << indent << "DimensionLengths: " << this->DimensionLengths << "\n";
  os << indent << "VariableNames: " << this->VariableNames << "\n";
}

//-------------------------------------------------------------------------
// This method also has to store the resulting string internally.
const char *vtkMINCImageReader::ConvertDataArrayToString(
  vtkDataArray *array)
{
  int dataType = array->GetDataType();

  if (dataType == VTK_CHAR)
    {
    vtkCharArray *charArray = vtkCharArray::SafeDownCast(array);
    return charArray->GetPointer(0);
    }

  ostrstream os;

  int n = array->GetNumberOfTuples();
  int i = 0;
  for (i = 0; i < n; i++)
    {
    double val = array->GetComponent(i, 0);
    os << val;
    if (dataType == VTK_DOUBLE || dataType == VTK_FLOAT)
      {
      if (val == static_cast<int>(val))
        { // add a decimal to distinguish floats from ints
        os << ".";
        }
      }
    if (i < n-1)
      {
      os << ", ";
      }
    }

    os << ends;

    // Store the string
    const char *str = os.str();
    const char *result = 0;

    if (this->StringStore == 0)
      {
      this->StringStore = vtkStringArray::New();
      }

    // See if the string is already stored
    n = this->StringStore->GetNumberOfValues();
    for (i = 0; i < n; i++)
      {
      result = this->StringStore->GetValue(i);
      if (strcmp(str, result) == 0)
        {
        break;
        }
      }
    // If not, add it to the array.
    if (i == n)
      {
      i = this->StringStore->InsertNextValue(str);
      result = this->StringStore->GetValue(i);
      }

    os.rdbuf()->freeze(0);

    return result;
}

//-------------------------------------------------------------------------
void vtkMINCImageReader::PrintFileHeader()
{
  this->ReadMINCFileAttributes();

  ostream& os = cout;

  // Get the name to print out with the dump
  char name[128];
  name[0] = '\0';
  int startChar = 0;
  int endChar = strlen(this->FileName);

  for (startChar = endChar-1; startChar > 0; startChar--)
    {
    if (this->FileName[startChar] == '.')
      {
      endChar = startChar;
      }
    if (this->FileName[startChar-1] == '/')
      {
      break;
      }
    }
  if (endChar - startChar > 127)
    {
    endChar = startChar + 128;
    }
  if (endChar > startChar)
    {
    strncpy(name, &this->FileName[startChar], endChar-startChar);
    name[endChar - startChar] = '\0';
    }

  // Get the data type
  const char *imageDataType;
  switch (this->MINCImageType)
    {
    case NC_BYTE:
      imageDataType = "byte";
      break;
    case NC_SHORT:
      imageDataType = "short";
      break;
    case NC_INT:
      imageDataType = "int";
      break;
    case NC_FLOAT:
      imageDataType = "float";
      break;
    case NC_DOUBLE:
      imageDataType = "double";
      break;
    default:
      imageDataType = "void";
    }

  os << "netcdf " << name << " {\n";
  os << "dimensions:\n";
  
  int ndim = 0;
  if (this->DimensionNames)
    {
    ndim = this->DimensionNames->GetNumberOfValues();
    }
  for (int idim = 0; idim < ndim; idim++)
    {
    os << "\t" << this->DimensionNames->GetValue(idim) << " = "
       << this->DimensionLengths->GetValue(idim) << " ;\n";
    }

  os << "variables:\n";

  int nvar = 0;
  if (this->VariableNames)
    {
    nvar = this->VariableNames->GetNumberOfValues();
    }
  for (int ivar = 0; ivar < nvar+1; ivar++)
    {
    const char *varname = "";
    if (ivar == nvar)
      {
      os << "\n// global attributes:\n";
      }
    else
      {
      varname = this->VariableNames->GetValue(ivar);

      if (strcmp(varname, "image") == 0 ||
          strcmp(varname, "image-max") == 0 ||
          strcmp(varname, "image-min") == 0)
        {
        os << "\t" << imageDataType << " " << varname;
        int nvardim = this->DimensionNames->GetNumberOfValues();
        if (varname[5] == '-')
          {
          nvardim = this->MINCImageMinMaxDims;
          }

        if (nvardim > 0)
          {
          os << "(";
          for (int ivardim = 0; ivardim < nvardim; ivardim++)
            {
            os << this->DimensionNames->GetValue(ivardim);
            if (ivardim < nvardim - 1)
              {
              os << ", ";
              }
            }
          os << ")";
          }
        os << " ;\n";
        }
      else
        {
        os << "\t" << "int " << varname << " ;\n";
        }
      }
    vtkStringArray *attArray =
      this->AttributeNames->GetStringArray(varname);
    if (attArray)
      {
      int natt = attArray->GetNumberOfValues();
      for (int iatt = 0; iatt < natt; iatt++)
        {
        const char *attname = attArray->GetValue(iatt);
        vtkDataArray *array =
          this->GetAttributeValueAsArray(varname, attname);
        os << "\t\t" << varname << ":" << attname << " = ";
        if (array->GetDataType() == VTK_CHAR)
          {
          vtkCharArray *charArray =
            vtkCharArray::SafeDownCast(array);
          os << "\"";
          const char *cp = charArray->GetPointer(0);
          const char *endcp = cp + charArray->GetNumberOfTuples();
          char text[512];
          text[0] = '\0';
          while (cp < endcp)
            {
            int c = 0;
            int j;
            for (j = 0; j < 508 && cp < endcp; j++, cp++)
              {
              c = *cp;
              if (c == '\0' && (cp + 1) == endcp)
                {
                // break if at terminal null
                cp++;
                break;
                }
              if (isprint(c) && c != '\\')
                {
                text[j] = c;
                }
              else
                {
                // quote the non-printing characters
                switch (c)
                  {
                  case '\\':
                    text[j++] = '\\';
                    text[j] = '\\';
                    break;
                  case '\n':
                    text[j++] = '\\';
                    text[j] = 'n';
                    break;
                  case '\r':
                    text[j++] = '\\';
                    text[j] = 'r';
                    break;
                  case '\f':
                    text[j++] = '\\';
                    text[j] = 'f';
                    break;
                  case '\t':
                    text[j++] = '\\';
                    text[j] = 't';
                    break;
                  default:
                    text[j++] = '\\';
                    text[j++] = '0' + ((c & 0xc0) >> 6);
                    text[j++] = '0' + ((c & 0x38) >> 3);
                    text[j] = '0' + (c & 0x7);
                    break;
                  }
                }
              if (c == '\n')
                {
                j++;
                cp++;
                break;
                }
              }

            text[j] = '\0';
            os << text;
            text[0] = '\0';

            // Start a new string after each newline, unless this
            // newline is the final character.
            if (c == '\n' && cp < endcp)
              {
              os << "\",\n\t\t\t\"";
              }
            }
          os << "\" ;\n";
          }
        else
          {
          // Use handy conversion method
          os << this->ConvertDataArrayToString(array) << " ;\n";
          }
        }
      }
    }

  os << "}\n";
}

//-------------------------------------------------------------------------
void vtkMINCImageReader::SetFileName(const char *name)
{
  // Set FileNameHasChanged even if the file name hasn't changed,
  // because it is possible that the user is re-reading a file after
  // changing it.
  if (!(name == 0 && this->GetFileName() == 0))
    { 
    this->FileNameHasChanged = 1;
    }

  this->Superclass::SetFileName(name);
}

//-------------------------------------------------------------------------
int vtkMINCImageReader::CanReadFile(const char* fname)
{
  // First do a very rapid check of the magic number
  FILE *fp = fopen(fname, "rb");
  if (!fp)
    {
    return 0;
    }

  char magic[4];
  fread(magic, 4, 1, fp);
  fclose(fp);

  if (magic[0] != 'C' ||
      magic[1] != 'D' ||
      magic[2] != 'F' ||
      magic[3] != '\001')
    {
    return 0;
    }

  // Do a more thorough check of the image:version attribute, since
  // there are lots of NetCDF files out there that aren't minc files.
  int status = NC_NOERR;
  int ncid = 0;
  status = nc_open(fname, 0, &ncid);
  if (status != NC_NOERR)
    {
    return 0;
    }

  int ndims = 0;
  int nvars = 0;
  int ngatts = 0;
  int unlimdimid = 0;
  status = nc_inq(ncid, &ndims, &nvars, &ngatts, &unlimdimid);
  if (status != NC_NOERR)
    {
    return 0;
    }

  int varid = 0;
  char varname[NC_MAX_NAME+1];
  nc_type vartype = NC_INT;
  int nvardims;
  int dimids[VTK_MINC_MAX_DIMS];
  int nvaratts = 0;
  for (varid = 0; varid < nvars && status == NC_NOERR; varid++)
    {
    status = nc_inq_var(ncid, varid, varname, &vartype, &nvardims,
                        dimids, &nvaratts);
    if (status == NC_NOERR && strcmp(varname, "image") == 0)
      {
      nc_type atttype = NC_INT;
      size_t attlength = 0;
      status = nc_inq_att(ncid, varid, "version", &atttype, &attlength);
      if (status == NC_NOERR && atttype == NC_CHAR && attlength < 32)
        {
        char verstring[32];
        status = nc_get_att_text(ncid, varid, "version", verstring);
        if (status == NC_NOERR && strncmp(verstring, "MINC ", 5) == 0)
          {
          nc_close(ncid);
          return 1;
          }
        }
      break;
      }
    }

  nc_close(ncid);

  return 0;
}

//-------------------------------------------------------------------------
vtkMatrix4x4 *vtkMINCImageReader::GetOrientationMatrix()
{
  this->ReadMINCFileAttributes();
  return this->OrientationMatrix;
}

//-------------------------------------------------------------------------
double vtkMINCImageReader::GetRescaleSlope()
{
  this->ReadMINCFileAttributes();
  return this->RescaleSlope;
}

//-------------------------------------------------------------------------
double vtkMINCImageReader::GetRescaleIntercept()
{
  this->ReadMINCFileAttributes();
  return this->RescaleIntercept;
}

//-------------------------------------------------------------------------
double *vtkMINCImageReader::GetValidRange()
{
  this->ReadMINCFileAttributes();
  this->FindMINCValidRange();
  return this->MINCValidRange;
}

//-------------------------------------------------------------------------
int vtkMINCImageReader::GetNumberOfFrames()
{
  this->ReadMINCFileAttributes();
  return this->NumberOfFrames;
}

//-------------------------------------------------------------------------
vtkStringArray *vtkMINCImageReader::GetDimensionNames()
{
  this->ReadMINCFileAttributes();
  return this->DimensionNames;
}

//-------------------------------------------------------------------------
vtkIdTypeArray *vtkMINCImageReader::GetDimensionLengths()
{
  this->ReadMINCFileAttributes();
  return this->DimensionLengths;
}

//-------------------------------------------------------------------------
vtkStringArray *vtkMINCImageReader::GetVariableNames()
{
  this->ReadMINCFileAttributes();
  return this->VariableNames;
}

//-------------------------------------------------------------------------
vtkStringArray *vtkMINCImageReader::GetAttributeNames(
  const char *variable)
{
  // If variable is null, use empty string to get global attributes
  if (variable == 0)
    {
    variable = "";
    }

  this->ReadMINCFileAttributes();
  return this->AttributeNames->GetStringArray(variable);
}

//-------------------------------------------------------------------------
int vtkMINCImageReader::HasAttribute(
  const char *variable,
  const char *attribute)
{
  return (this->GetAttributeValueAsArray(variable, attribute) != 0);
}

//-------------------------------------------------------------------------
vtkDataArray *vtkMINCImageReader::GetAttributeValueAsArray(
  const char *variable,
  const char *attribute)
{
  this->ReadMINCFileAttributes();

  vtkstd::string path = "/minc";
  if (variable && variable[0] != '\0')
    {
    path += "/";
    path += variable;
    }
  path += "/";
  path += attribute;

  return this->AttributeValues->GetDataArray(path.c_str());
}

//-------------------------------------------------------------------------
const char *vtkMINCImageReader::GetAttributeValueAsString(
  const char *variable,
  const char *attribute)
{
  vtkDataArray *array =
    this->GetAttributeValueAsArray(variable, attribute);

  // Return NULL if not found
  if (array == 0)
    {
    return 0;
    }

  // Convert any other array to a a string.
  return this->ConvertDataArrayToString(array);
}

//-------------------------------------------------------------------------
int vtkMINCImageReader::GetAttributeValueAsInt(
  const char *variable,
  const char *attribute)
{
  vtkDataArray *array = this->GetAttributeValueAsArray(variable, attribute);

  if (array == 0)
    {
    vtkErrorMacro("The attribute " << variable << ":"
                  << attribute << " was not found.");
    return 0;
    }

  if (array->GetDataType() == VTK_CHAR)
    {
    char *text = vtkCharArray::SafeDownCast(array)->GetPointer(0);
    char *endp = text;
    long result = strtol(text, &endp, 10);
    // Check for complete conversion
    if (*endp == '\0' && *text != '\0')
      {
      return static_cast<int>(result);
      }
    }
  else if (array->GetNumberOfTuples() == 1)
    {
    switch(array->GetDataType())
      {
      case VTK_SIGNED_CHAR:
      case VTK_UNSIGNED_CHAR:
      case VTK_SHORT:
      case VTK_INT:
        return static_cast<int>(array->GetComponent(0,0));
        break;
      default:
        break;
      }
    }

  vtkErrorMacro("GetAttributeValueAsInt() used on non-integer attribute "
                << variable << ":" << attribute <<".");
  return static_cast<int>(array->GetComponent(0,0));
}

//-------------------------------------------------------------------------
double vtkMINCImageReader::GetAttributeValueAsDouble(
  const char *variable,
  const char *attribute)
{
  if (variable == 0)
    {
    variable = "";
    }

  vtkDataArray *array = this->GetAttributeValueAsArray(variable, attribute);

  if (array == 0)
    {
    vtkErrorMacro("The attribute " << variable << ":"
                  << attribute << " was not found.");
    return 0;
    }

  if (array->GetDataType() == VTK_CHAR)
    {
    char *text = vtkCharArray::SafeDownCast(array)->GetPointer(0);
    char *endp = text;
    double result = strtod(text, &endp);
    // Check for complete conversion
    if (*endp == '\0' && *text != '\0')
      {
      return result;
      }
    }
  else if (array->GetNumberOfTuples() == 1)
    {
    switch(array->GetDataType())
      {
      case VTK_SIGNED_CHAR:
      case VTK_UNSIGNED_CHAR:
      case VTK_SHORT:
      case VTK_INT:
      case VTK_FLOAT:
      case VTK_DOUBLE:
        return array->GetComponent(0,0);
        break;
      default:
        break;
      }
    }

  vtkErrorMacro("GetAttributeValueAsDouble() used on non-real attribute "
                << variable << ":" << attribute <<".");
  return array->GetComponent(0,0);
}

//-------------------------------------------------------------------------
int vtkMINCImageReader::OpenNetCDFFile(const char *filename, int& ncid)
{
  int status = 0;

  if (filename == 0)
    {
    vtkErrorMacro("No filename was set");
    return 0;
    }

  status = nc_open(filename, 0, &ncid);
  if (status != NC_NOERR)
    {
    vtkErrorMacro("Could not open the MINC file:\n"
                  << nc_strerror(status));
    return 0;
    }

  return 1;
}

//-------------------------------------------------------------------------
int vtkMINCImageReader::CloseNetCDFFile(int ncid)
{
  int status = 0;
  status = nc_close(ncid);
  if (status != NC_NOERR)
    {
    vtkErrorMacro("Could not close the MINC file:\n"
                  << nc_strerror(status));
    return 0;
    }

  return 1;
}

//-------------------------------------------------------------------------
// this is a macro so the vtkErrorMacro will report a useful line number
#define vtkMINCImageReaderFailAndClose(ncid, status) \
{ \
  if (status != NC_NOERR) \
    { \
    vtkErrorMacro("There was an error with the MINC file:\n" \
                  << this->GetFileName() << "\n" \
                  << nc_strerror(status)); \
    } \
  nc_close(ncid); \
}


//-------------------------------------------------------------------------
// Function for getting VTK dimension index from file name.
int vtkMINCImageReader::IndexFromDimensionName(const char *dimName)
{
  switch(dimName[0])
    {
    case 'x':
      return 0;
    case 'y':
      return 1;
    case 'z':
      return 2;
    default:
      if (strcmp(dimName, "vector_dimension") == 0)
        {
        return -1;
        }
      break;
    }

  // Any unrecognized dimensions are returned as index 3
  return 3;
}

//-------------------------------------------------------------------------
int vtkMINCImageReader::ReadMINCFileAttributes()
{
  // If the filename hasn't changed since the last time the attributes
  // were read, don't read them again.
  if (!this->FileNameHasChanged)
    {
    return 1;
    }

  // Reset the MINC information for the file.
  this->MINCImageType = 0;
  this->MINCImageTypeSigned = 1;

  this->NumberOfFrames = 1;
  this->OrientationMatrix->Identity();

  // Orientation set tells us which direction cosines were found
  int orientationSet[3];
  orientationSet[0] = 0;
  orientationSet[1] = 0;
  orientationSet[2] = 0;

  this->MINCImageMinMaxDims = 0;
  this->MINCImageMin->SetNumberOfTuples(0);
  this->MINCImageMax->SetNumberOfTuples(0);

  this->AttributeNames->Clear();
  this->AttributeValues->Clear();

  // Miscellaneous NetCDF variables
  int status = 0;
  int ncid = 0;
  int dimid = 0;
  int varid = 0;
  int ndims = 0;
  int nvars = 0;
  int ngatts = 0;
  int unlimdimid = 0;

  if (this->OpenNetCDFFile(this->GetFileName(), ncid) == 0)
    {
    return 0;
    }

  // Get the basic information for the file.  The ndims are
  // ignored here, because we only want the dimensions that
  // belong to the image variable.
  status = nc_inq(ncid, &ndims, &nvars, &ngatts, &unlimdimid);
  if (status != NC_NOERR)
    {
    vtkMINCImageReaderFailAndClose(ncid, status);
    return 0;
    }
  if (ndims > VTK_MINC_MAX_DIMS)
    {
    vtkErrorMacro("MINC file has " << ndims << ", but this reader"
                  " only supports " << VTK_MINC_MAX_DIMS << ".");
    return 0;
    }

  // Reset the variable name array.
  this->VariableNames->SetNumberOfValues(nvars);

  // Go through all the variables in the MINC file.  A varid of -1
  // is used to signal global attributes.
  for (varid = -1; varid < nvars; varid++)
    {
    char varname[NC_MAX_NAME+1];
    int dimids[VTK_MINC_MAX_DIMS];
    nc_type vartype = NC_SHORT;
    int nvardims = 0;
    int nvaratts = 0;

    if (varid == -1)  // for global attributes
      {
      nvaratts = ngatts;
      varname[0] = '\0';
      }
    else
      {
      status = nc_inq_var(ncid, varid, varname, &vartype, &nvardims,
                          dimids, &nvaratts);
      if (status != NC_NOERR)
        {
        vtkMINCImageReaderFailAndClose(ncid, status);
        return 0;
        }

      this->VariableNames->SetValue(varid, varname);
      }

    // Get all the variable attributes and store them in field data.
    vtkStringArray *attributeNames = vtkStringArray::New();
    for (int j = 0; j < nvaratts; j++)
      {
      char attname[NC_MAX_NAME+1];
      nc_type atttype;
      size_t attlength = 0;

      status = nc_inq_attname(ncid, varid, j, attname);
      if (status != NC_NOERR)
        {
        vtkMINCImageReaderFailAndClose(ncid, status);
        return 0;
        }
      status = nc_inq_att(ncid, varid, attname, &atttype, &attlength);
      if (status != NC_NOERR)
        {
        vtkMINCImageReaderFailAndClose(ncid, status);
        return 0;
        }

      // Get the attribute values as a vtkDataArray.
      vtkDataArray *dataArray = 0;
      switch (atttype)
        {
        case NC_BYTE:
          {
          // NetCDF leaves it up to us to decide whether NC_BYTE
          // should be signed.
          vtkUnsignedCharArray *ucharArray = vtkUnsignedCharArray::New();
          ucharArray->SetNumberOfValues(attlength);
          nc_get_att_uchar(ncid, varid, attname,
                           ucharArray->GetPointer(0));
          dataArray = ucharArray;
          }
          break;
        case NC_CHAR:
          {
          // The NC_CHAR type is for text.
          vtkCharArray *charArray = vtkCharArray::New();
          // The netcdf standard doesn't enforce null-termination
          // of string attributes, so we add a null here.
          charArray->SetNumberOfValues(attlength+1);
          charArray->SetValue(attlength, 0);
          charArray->SetNumberOfValues(attlength);
          nc_get_att_text(ncid, varid, attname,
                          charArray->GetPointer(0));
          dataArray = charArray;
          }
          break;
        case NC_SHORT:
          {
          vtkShortArray *shortArray = vtkShortArray::New();
          shortArray->SetNumberOfValues(attlength);
          nc_get_att_short(ncid, varid, attname,
                           shortArray->GetPointer(0));
          dataArray = shortArray;
          }
          break;
        case NC_INT:
          {
          vtkIntArray *intArray = vtkIntArray::New();
          intArray->SetNumberOfValues(attlength);
          nc_get_att_int(ncid, varid, attname,
                         intArray->GetPointer(0));
          dataArray = intArray;
          }
          break;
        case NC_FLOAT:
          {
          vtkFloatArray *floatArray = vtkFloatArray::New();
          floatArray->SetNumberOfValues(attlength);
          nc_get_att_float(ncid, varid, attname,
                           floatArray->GetPointer(0));
          dataArray = floatArray;
          }
          break;
        case NC_DOUBLE:
          {
          vtkDoubleArray *doubleArray = vtkDoubleArray::New();
          doubleArray->SetNumberOfValues(attlength);
          nc_get_att_double(ncid, varid, attname,
                            doubleArray->GetPointer(0));
          dataArray = doubleArray;
          }
          break;
        default:
          break;
        }
      if (dataArray)
        {
        // The array names are formatted as /minc/varname/attname paths.
        vtkstd::string path = "/minc";
        if (varid != -1)
          {
          path += "/";
          path += varname;
          }
        path += "/";
        path += attname;

        dataArray->SetName(path.c_str());
        this->AttributeValues->AddArray(dataArray);
        dataArray->Delete();

        attributeNames->InsertNextValue(attname);
        }
      }

    // Add this variable's attribute names to AttributeNames
    attributeNames->SetName(varname);
    this->AttributeNames->AddArray(attributeNames);
    attributeNames->Delete();

    // Special treatment of image variable.
    if (strcmp(varname, "image") == 0)
      {
      // Set the type of the data.
      this->MINCImageType = vartype;

      // Find the sign of the data.
      int signedType = 1;
      vtkCharArray *signtype =
        this->AttributeValues->GetCharArray("/minc/image/signtype");
      if (signtype)
        {
        if (strcmp(signtype->GetPointer(0), "unsigned") == 0)
          {
          signedType = 0;
          }
        }
      this->MINCImageTypeSigned = signedType;

      // Get the names and lengths of all image dimensions.
      this->DimensionNames->SetNumberOfValues(nvardims);
      this->DimensionLengths->SetNumberOfValues(nvardims);

      for (int i = 0; i < nvardims; i++)
        {
        char dimname[NC_MAX_NAME+1];
        size_t dimlength = 0;

        dimid = dimids[i];

        status = nc_inq_dim(ncid, dimid, dimname, &dimlength);
        if (status != NC_NOERR)
          {
          vtkMINCImageReaderFailAndClose(ncid, status);
          return 0;
          }

        this->DimensionNames->SetValue(i, dimname);
        this->DimensionLengths->SetValue(i,
                                         static_cast<vtkIdType>(dimlength));

        int dimIndex = this->IndexFromDimensionName(dimname);

        if (dimIndex >= 0 && dimIndex < 3)
          {
          // Set the orientation matrix from the direction_cosines
          vtkstd::string basepath = "/minc/";
          vtkstd::string path = basepath + dimname + "/direction_cosines";
          vtkDoubleArray *doubleArray =
            this->AttributeValues->GetDoubleArray(path.c_str());
          if (doubleArray && doubleArray->GetNumberOfTuples() == 3)
            {
            double *dimDirCos = doubleArray->GetPointer(0);
            this->OrientationMatrix->SetElement(0, dimIndex, dimDirCos[0]);
            this->OrientationMatrix->SetElement(1, dimIndex, dimDirCos[1]);
            this->OrientationMatrix->SetElement(2, dimIndex, dimDirCos[2]);
            orientationSet[dimIndex] = 1;
            }
          }
        else if (strcmp(dimname, "vector_dimension") != 0)
          {
          // Set the NumberOfFrames to the product of all dimensions
          // that are neither spatial dimensions nor vector dimensions.
          this->NumberOfFrames *= dimlength;
          }
        }
      }
    else if (strcmp(varname, "image-min") == 0 ||
             strcmp(varname, "image-max") == 0)
      {
      // Read the image-min and image-max.
      this->MINCImageMinMaxDims = nvardims;

      vtkDoubleArray *doubleArray = this->MINCImageMax;
      if (strcmp(varname, "image-min") == 0)
        {
        doubleArray = this->MINCImageMin;
        }

      vtkIdType size = 1;
      size_t start[VTK_MINC_MAX_DIMS];
      size_t count[VTK_MINC_MAX_DIMS];

      for (int i = 0; i < nvardims; i++)
        {
        char dimname[NC_MAX_NAME+1];
        size_t dimlength = 0;

        dimid = dimids[i];

        status = nc_inq_dim(ncid, dimid, dimname, &dimlength);
        if (status != NC_NOERR)
          {
          vtkMINCImageReaderFailAndClose(ncid, status);
          return 0;
          }

        start[i] = 0;
        count[i] = dimlength;

        size *= dimlength;
        }

      doubleArray->SetNumberOfValues(size);
      status = nc_get_vara_double(ncid, varid, start, count,
                                  doubleArray->GetPointer(0));
      if (status != NC_NOERR)
        {
        vtkMINCImageReaderFailAndClose(ncid, status);
        return 0;
        }
      }
    }

  // Check to see if only 2 spatial dimensions were included,
  // since we'll have to make up the third dircos if that is the case
  int numDirCos = 0;
  int notSetIndex = 0;
  for (int dcount = 0; dcount < 3; dcount++)
    {
    if (orientationSet[dcount])
      {
      numDirCos++;
      }
    else
      {
      notSetIndex = dcount;
      }
    }
  // If only two were set, use cross product to get the third
  if (numDirCos == 2)
    {
    int idx1 = (notSetIndex + 1) % 3;
    int idx2 = (notSetIndex + 2) % 3;
    double v1[4];
    double v2[4];
    double v3[3];
    for (int tmpi = 0; tmpi < 4; tmpi++)
      {
      v1[tmpi] = v2[tmpi] = 0.0;
      }
    v1[idx1] = 1.0;
    v2[idx2] = 1.0;
    this->OrientationMatrix->MultiplyPoint(v1, v1);
    this->OrientationMatrix->MultiplyPoint(v2, v2);
    vtkMath::Cross(v1, v2, v3);
    this->OrientationMatrix->SetElement(0, notSetIndex, v3[0]);
    this->OrientationMatrix->SetElement(1, notSetIndex, v3[1]);
    this->OrientationMatrix->SetElement(2, notSetIndex, v3[2]);
    }

  // Get the MINCValidRange and MINCImageRange.
  this->FindMINCValidRange();

  this->FindMINCImageRange();

  this->RescaleSlope = ((this->MINCImageRange[1] - this->MINCImageRange[0])/
                        (this->MINCValidRange[1] - this->MINCValidRange[0]));

  this->RescaleIntercept = (this->MINCImageRange[0] -
                            this->RescaleSlope*this->MINCValidRange[0]);

  // We're done reading the attributes, so close the file.
  if (this->CloseNetCDFFile(ncid) == 0)
    {
    return 0;
    }

  // Don't have to do this again until the file name changes.
  this->FileNameHasChanged = 0;

  return 1;
}

//-------------------------------------------------------------------------
void vtkMINCImageReader::FindMINCValidRange()
{
  // Find the valid range. Start with the default.
  double range[2];
  range[0] = 0.0;
  range[1] = 1.0;

  // Look for the valid_range attribute of the data.
  vtkDoubleArray *rangearray =
    this->AttributeValues->GetDoubleArray("/minc/image/valid_range");
  if (rangearray)
    {
    range[0] = rangearray->GetValue(0);
    range[1] = rangearray->GetValue(1);

    if (this->MINCImageType == NC_FLOAT)
      {
      range[0] = (float)range[0];
      range[1] = (float)range[1];
      }

    // Sometimes the range is accidentally set to the full
    // float range.  In that case, we ignore the valid_range.
    if (this->MINCImageType == NC_FLOAT && range[1] == FLT_MAX ||
        this->MINCImageType == NC_DOUBLE && range[1] == DBL_MAX)
      {
      range[0] = 0.0;
      range[1] = 1.0;
      }
    }
  else
    {
    // If there is no valid_range attribute, use maximum range.
    if (this->MINCImageTypeSigned)
      {
      switch (this->MINCImageType)
        {
        case NC_BYTE:
          range[0] = VTK_SIGNED_CHAR_MIN;
          range[1] = VTK_SIGNED_CHAR_MAX;
          break;
        case NC_SHORT:
          range[0] = VTK_SHORT_MIN;
          range[1] = VTK_SHORT_MAX;
          break;
        case NC_INT:
          range[0] = VTK_INT_MIN;
          range[1] = VTK_INT_MAX;
          break;
        }
      }
    else
      {
      switch (this->MINCImageType)
        {
        case NC_BYTE:
          range[0] = VTK_UNSIGNED_CHAR_MIN;
          range[1] = VTK_UNSIGNED_CHAR_MAX;
          break;
        case NC_SHORT:
          range[0] = VTK_UNSIGNED_SHORT_MIN;
          range[1] = VTK_UNSIGNED_SHORT_MAX;
          break;
        case NC_INT:
          range[0] = VTK_UNSIGNED_INT_MIN;
          range[1] = VTK_UNSIGNED_INT_MAX;
          break;
        }
      }
    }

  this->MINCValidRange[0] = range[0];
  this->MINCValidRange[1] = range[1];
}

//-------------------------------------------------------------------------
void vtkMINCImageReader::FindMINCImageRange()
{
  // Initialize to the default values
  double range[2];
  range[0] = 0.0;
  range[1] = 1.0;

  // If image-min and image-max variables exist, use them.
  // Otherwise, use the valid_range of the data instead.
  if (this->MINCImageMin->GetNumberOfTuples() > 0 &&
      this->MINCImageMax->GetNumberOfTuples() > 0)
    {
    range[0] = this->MINCImageMin->GetRange()[0];
    range[1] = this->MINCImageMax->GetRange()[1];
    }
  else
    {
    range[0] = this->MINCValidRange[0];
    range[1] = this->MINCValidRange[1];
    }

  this->MINCImageRange[0] = range[0];
  this->MINCImageRange[1] = range[1];
}

//-------------------------------------------------------------------------
int vtkMINCImageReader::ConvertMINCTypeToVTKType(
  int minctype,
  int mincsigned)
{
  int dataType = 0;

  // Get the vtk type of the data.
  switch (minctype)
    {
    case NC_BYTE:
      dataType = VTK_UNSIGNED_CHAR;
      if (mincsigned)
        {
        dataType = VTK_SIGNED_CHAR;
        }
      break;
    case NC_SHORT:
      dataType = VTK_UNSIGNED_SHORT;
      if (mincsigned)
        {
        dataType = VTK_SHORT;
        }
      break;
    case NC_INT:
      dataType = VTK_UNSIGNED_INT;
      if (mincsigned)
        {
        dataType = VTK_INT;
        }
      break;
    case NC_FLOAT:
      dataType = VTK_FLOAT;
      break;
    case NC_DOUBLE:
      dataType = VTK_DOUBLE;
      break;
    default:
      break;
    }

  return dataType;
}

//-------------------------------------------------------------------------
void vtkMINCImageReader::ExecuteInformation()
{
  // Read the MINC attributes from the file.
  if (this->ReadMINCFileAttributes() == 0)
    {
    return;
    }

  // Set the VTK information from the MINC information.
  int dataExtent[6];
  dataExtent[0] = dataExtent[1] = 0;
  dataExtent[2] = dataExtent[3] = 0;
  dataExtent[4] = dataExtent[5] = 0;

  double dataSpacing[3];
  dataSpacing[0] = dataSpacing[1] = dataSpacing[2] = 1.0;

  double dataOrigin[3];
  dataOrigin[0] = dataOrigin[1] = dataOrigin[2] = 0.0;

  int numberOfComponents = 1;

  int dataType = this->ConvertMINCTypeToVTKType(this->MINCImageType,
                                                this->MINCImageTypeSigned);

  if (dataType == 0)
    {
    vtkErrorMacro("Couldn't convert NetCDF data type " << this->MINCImageType
                  << (this->MINCImageTypeSigned ? " signed" : " unsigned")
                  << " to a VTK data type.");
    return;
    }

  // Go through the image dimensions to discover data information.
  int numberOfDimensions = this->DimensionNames->GetNumberOfValues();
  for (int i = 0; i < numberOfDimensions; i++)
    {
    const char *dimName = this->DimensionNames->GetValue(i);
    vtkIdType dimLength = this->DimensionLengths->GetValue(i);

    // Set the VTK dimension index.
    int dimIndex = this->IndexFromDimensionName(dimName);

    // Do special things with the spatial dimensions.
    if (dimIndex >= 0 && dimIndex < 3)
      {
      vtkstd::string basepath = "/minc/";
      vtkstd::string path;
      vtkDoubleArray *doubleArray = 0;

      // Set the spacing from the 'step' attribute.
      path = basepath + dimName + "/step";
      doubleArray = this->AttributeValues->GetDoubleArray(path.c_str());
      if (doubleArray)
        {
        dataSpacing[dimIndex] = doubleArray->GetValue(0);
        }

      // Set the origin from the 'start' attribute.
      path = basepath + dimName + "/start";
      doubleArray = this->AttributeValues->GetDoubleArray(path.c_str());
      if (doubleArray)
        {
        dataOrigin[dimIndex] = doubleArray->GetValue(0);
        }

      // Set the extent from the dimension length.
      dataExtent[2*dimIndex + 1] = static_cast<int>(dimLength - 1);
      }

    // Check for vector_dimension.
    else if (strcmp(dimName, "vector_dimension") == 0)
      {
      numberOfComponents = dimLength;
      }
    }

  this->SetDataExtent(dataExtent);
  this->SetDataSpacing(dataSpacing[0], dataSpacing[1], dataSpacing[2]);
  this->SetDataOrigin(dataOrigin[0], dataOrigin[1], dataOrigin[2]);
  this->SetDataScalarType(dataType);
  this->SetNumberOfScalarComponents(numberOfComponents);
}

//-------------------------------------------------------------------------
// Data conversion functions.  The rounding is done using the same
// method as in the MINC libraries.
#define vtkMINCImageReaderConvertMacro(F, T, MIN, MAX) \
inline void vtkMINCImageReaderConvert(const F& inVal, T& outVal) \
{ \
  double val = inVal; \
  if (val >= static_cast<double>(MIN)) \
    { \
    if (val <= static_cast<double>(MAX)) \
      { \
      outVal = static_cast<T>((val < 0) ? (val - 0.5) : (val + 0.5)); \
      return; \
      } \
    outVal = static_cast<T>(MAX); \
    return; \
    } \
  outVal = static_cast<T>(MIN); \
}

vtkMINCImageReaderConvertMacro(double, signed char, -128, 127);
vtkMINCImageReaderConvertMacro(double, unsigned char, 0, 255);
vtkMINCImageReaderConvertMacro(double, short, -32768, 32767);
vtkMINCImageReaderConvertMacro(double, unsigned short, 0, 65535);
vtkMINCImageReaderConvertMacro(double, int, -2147483647 - 1, 2147483647);
vtkMINCImageReaderConvertMacro(double, unsigned int, 0u, 4294967295u);

inline void vtkMINCImageReaderConvert(const double &inVal, float &outVal)
{
  outVal = static_cast<float>(inVal);
}

inline void vtkMINCImageReaderConvert(const double &inVal, double &outVal)
{
  outVal = inVal;
}

//-------------------------------------------------------------------------
// Overloaded functions for reading various data types.

// Handle most with a macro.
#define vtkMINCImageReaderReadChunkMacro(ncFunction, T) \
inline int vtkMINCImageReaderReadChunk( \
  int ncid, int varid, size_t *start, size_t *count, T *buffer) \
{ \
  return ncFunction(ncid, varid, start, count, buffer); \
}

vtkMINCImageReaderReadChunkMacro(nc_get_vara_schar, signed char);
vtkMINCImageReaderReadChunkMacro(nc_get_vara_uchar, unsigned char);
vtkMINCImageReaderReadChunkMacro(nc_get_vara_short, short);
vtkMINCImageReaderReadChunkMacro(nc_get_vara_int, int);
vtkMINCImageReaderReadChunkMacro(nc_get_vara_float, float);
vtkMINCImageReaderReadChunkMacro(nc_get_vara_double, double);

// The unsigned short and unsigned int need an additional typecast.
inline int vtkMINCImageReaderReadChunk(
  int ncid, int varid, size_t *start, size_t *count,
  unsigned short *buffer)
{
  return nc_get_vara_short(ncid, varid, start, count, (short *)buffer);
}

inline int vtkMINCImageReaderReadChunk(
  int ncid, int varid, size_t *start, size_t *count,
  unsigned int *buffer)
{
  return nc_get_vara_int(ncid, varid, start, count, (int *)buffer);
}

//-------------------------------------------------------------------------
template<class T1, class T2>
void vtkMINCImageReaderExecuteChunk(
  T1 *outPtr, T2 *buffer, double slope, double intercept,
  int ncid, int varid, int ndims, size_t *start, size_t *count,
  vtkIdType *permutedInc)
{
  // Read the chunk of data from the MINC file.
  vtkMINCImageReaderReadChunk(ncid, varid, start, count, buffer);

  // Create space to save values during the copy loop.
  T1 *saveOutPtr[VTK_MINC_MAX_DIMS];
  size_t index[VTK_MINC_MAX_DIMS];
  int idim = 0;
  for (idim = 0; idim < ndims; idim++)
    {
    index[idim] = 0;
    saveOutPtr[idim] = outPtr;
    }

  // See if there is a range of dimensions over which the
  // the MINC data and VTK data will be contiguous.  The
  // lastdim is the dimension after which all dimensions
  // are contiguous between the MINC file and the output.
  int lastdim = ndims-1;
  int ncontiguous = 1;
  vtkIdType dimprod = 1;
  for (idim = ndims; idim > 0; )
    {
    idim--;

    lastdim = idim;
    ncontiguous = dimprod;

    if (dimprod != permutedInc[idim])
      {
      break;
      }

    dimprod *= count[idim];
    }

  // Save the count and permuted increment of this dimension.
  size_t lastdimcount = count[lastdim];
  size_t lastdimindex = 0;
  vtkIdType lastdimInc = permutedInc[lastdim];
  T1 *lastdimOutPtr = saveOutPtr[lastdim];

  // Loop over all contiguous sections of the image.
  for (;;)
    {
    // Loop through one contiguous section
    vtkIdType k = ncontiguous;
    do
      {
      // Use special function for type conversion.
      vtkMINCImageReaderConvert((*buffer++)*slope + intercept, *outPtr++);
      }
    while (--k);

    lastdimindex++;
    lastdimOutPtr += lastdimInc;
    outPtr = lastdimOutPtr;

    // Continue until done lastdim.
    if (lastdimindex < lastdimcount)
      {
      continue;
      }

    // Handle all dimensions that are lower than lastdim.  Go down
    // the dimensions one at a time until we find one for which
    // the index is still less than the count.
    idim = lastdim;
    do
      {
      // We're done if the lowest dim's index has reached its count.
      if (idim == 0)
        {
        return;
        }
      // Reset the index to zero if it previously reached its count.
      index[idim--] = 0;

      // Now increase the index for the next lower dimension;
      index[idim]++;
      saveOutPtr[idim] += permutedInc[idim];

      // Continue the loop if this dim's index has reached its count.
      }
    while (index[idim] >= count[idim]);

    // Increment back up to the lastdim, resetting the pointers.
    outPtr = saveOutPtr[idim];
    do
      {
      saveOutPtr[++idim] = outPtr;
      }
    while (idim < lastdim);

    lastdimOutPtr = outPtr;
    lastdimindex = 0;
    }
}

//-------------------------------------------------------------------------
// Our own template that only includes MINC data types.

#define vtkMINCImageReaderTemplateMacro(call) \
  case VTK_DOUBLE:         { typedef double VTK_TT; call; };         break; \
  case VTK_FLOAT:          { typedef float VTK_TT; call; };          break; \
  case VTK_INT:            { typedef int VTK_TT; call; };            break; \
  case VTK_UNSIGNED_INT:   { typedef unsigned int VTK_TT; call; };   break; \
  case VTK_SHORT:          { typedef short VTK_TT; call; };          break; \
  case VTK_UNSIGNED_SHORT: { typedef unsigned short VTK_TT; call; }; break; \
  case VTK_SIGNED_CHAR:    { typedef signed char VTK_TT; call; };    break; \
  case VTK_UNSIGNED_CHAR:  { typedef unsigned char VTK_TT; call; };  break

//-------------------------------------------------------------------------
void vtkMINCImageReader::ExecuteData(vtkDataObject *output)
{
  vtkImageData *data = this->AllocateOutputData(output);
  int scalarType = data->GetScalarType();
  int scalarSize = data->GetScalarSize();
  int numComponents = data->GetNumberOfScalarComponents();
  int outExt[6];
  data->GetUpdateExtent(outExt);
  vtkIdType outInc[3];
  data->GetIncrements(outInc);
  int outSize[3];
  data->GetDimensions(outSize);

  void *outPtr = data->GetScalarPointerForExtent(outExt);

  int frameNumber = this->FrameNumber;
  if (frameNumber < 0 || frameNumber >= this->NumberOfFrames)
    {
    vtkWarningMacro("FrameNumber is set to " << this->FrameNumber <<
                    " but there are only " << this->NumberOfFrames <<
                    " frames.");
    frameNumber = frameNumber % this->NumberOfFrames;
    }

  int status = 0;
  int ncid = 0;
  int varid = 0;

  if (this->OpenNetCDFFile(this->GetFileName(), ncid) == 0)
    {
    return;
    }

  // Get the image variable.
  status = nc_inq_varid(ncid, "image", &varid);
  if (status != NC_NOERR)
    {
    vtkMINCImageReaderFailAndClose(ncid, status);
    return;
    }

  // Get the dimensions.
  int ndims = this->DimensionNames->GetNumberOfValues();
  int idim = 0;
  int nminmaxdims = this->MINCImageMinMaxDims;
  vtkIdType minmaxSize = this->MINCImageMin->GetNumberOfTuples();

  // All of these values will be changed in the following loop
  vtkIdType nchunks = 1;
  vtkIdType numFrames = 1;
  vtkIdType chunkSize = numComponents;
  vtkIdType chunkInc = 0;

  // These arrays will be filled in by the following loop
  vtkIdType permutedInc[VTK_MINC_MAX_DIMS];
  size_t start[VTK_MINC_MAX_DIMS];
  size_t count[VTK_MINC_MAX_DIMS];
  size_t length[VTK_MINC_MAX_DIMS];

  // Loop over the dimensions starting with the fastest-varying.
  for (idim = ndims; idim > 0; )
    {
    idim--;

    const char *dimName = this->DimensionNames->GetValue(idim);
    vtkIdType dimLength = this->DimensionLengths->GetValue(idim);
    length[idim] = dimLength;

    // Find the VTK dimension index.
    int dimIndex = this->IndexFromDimensionName(dimName);

    if (dimIndex >= 0 && dimIndex < 3)
      {
      // Set start and count according to the update extent.
      start[idim] = outExt[2*dimIndex];
      count[idim] = outExt[2*dimIndex+1] - outExt[2*dimIndex] + 1;
      permutedInc[idim] = outInc[dimIndex];
      if (idim < nminmaxdims)
        {
        // Number of chunks is product of dimensions in minmax.
        nchunks *= count[idim];

        // After each chunk, we will increment outPtr by chunkInc.
        if (chunkInc == 0)
          {
          chunkInc = outInc[dimIndex];
          }
        }
      else
        {
        chunkSize *= count[idim];
        }
      }
    else if (strcmp(dimName, "vector_dimension") == 0)
      {
      // Vector dimension size is also stored in numComponents.
      start[idim] = 0;
      count[idim] = numComponents;
      permutedInc[idim] = 1;
      }
    else
      {
      // Use FrameNumber to compute the index into the remaining dimensions.
      start[idim] = (frameNumber / numFrames) % dimLength;
      count[idim] = 1;
      numFrames *= dimLength;
      permutedInc[idim] = 0;
      }
    }

  // Create a buffer for intermediate results.
  void *buffer = 0;
  switch (scalarType)
    {
    vtkMINCImageReaderTemplateMacro(buffer=(void *)(new VTK_TT[chunkSize]));
    }

  // Initialize the min and max to the global min max.
  double *minPtr = &this->MINCImageRange[0];
  double *maxPtr = &this->MINCImageRange[1];

  // If min and max arrays are not empty, use them instead.
  if (minmaxSize > 0)
    {
    minPtr = this->MINCImageMin->GetPointer(0);
    maxPtr = this->MINCImageMax->GetPointer(0);
    }

  // Initialize the start and count to use for each chunk.
  size_t start2[VTK_MINC_MAX_DIMS];
  size_t count2[VTK_MINC_MAX_DIMS];
  for (idim = 0; idim < ndims; idim++)
    {
    start2[idim] = start[idim];
    count2[idim] = count[idim];
    }

  // Go through all the chunks
  for (vtkIdType ichunk = 0; ichunk < nchunks; ichunk++)
    {
    // Find the start and count to use for each chunk.
    vtkIdType minmaxIdx = 0;
    vtkIdType minmaxInc = 1;
    vtkIdType chunkProd = 1;
    for (idim = nminmaxdims; idim > 0; )
      {
      idim--;
      start2[idim] = start[idim] + (ichunk / chunkProd) % count[idim];
      count2[idim] = 1;
      minmaxIdx += start2[idim]*minmaxInc;
      minmaxInc *= length[idim];
      chunkProd *= count[idim];
      }

    // Get the min and max values to apply to this chunk
    double chunkRange[2];
    chunkRange[0] = minPtr[minmaxIdx];
    chunkRange[1] = maxPtr[minmaxIdx];

    // Use the range to calculate a linear transformation
    // to apply to the data values of this chunk.
    double slope = ((chunkRange[1] - chunkRange[0])/
                    (this->MINCImageRange[1] - this->MINCImageRange[0]));
    double intercept = ((chunkRange[0] - this->RescaleIntercept)/
                        this->RescaleSlope) - slope*this->MINCValidRange[0];

    // Read in the chunks and permute them.
    switch (scalarType)
      {
      vtkMINCImageReaderTemplateMacro(
        vtkMINCImageReaderExecuteChunk(
          (VTK_TT *)outPtr, (VTK_TT *)buffer, slope, intercept,
          ncid, varid, ndims, start2, count2, permutedInc));
      }

    // Increment the outPtr for the next chunk.
    outPtr = (void *)(((char *)outPtr) + chunkInc*scalarSize);
    }

  switch (scalarType)
    {
    vtkMINCImageReaderTemplateMacro(delete [] ((VTK_TT *)buffer));
    }

  this->CloseNetCDFFile(ncid);
}
