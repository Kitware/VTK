/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMINCImageAttributes.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

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

#include "vtkMINCImageAttributes.h"

#include "vtkObjectFactory.h"

#include "vtkStringArray.h"
#include "vtkCharArray.h"
#include "vtkSignedCharArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkShortArray.h"
#include "vtkIntArray.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkSmartPointer.h"

#include "vtkMINC.h"
#include "vtk_netcdf.h"

#include <stdlib.h>
#include <ctype.h>
#include <float.h>
#include <vtkstd/string>
#include <vtkstd/map>
#include <vtksys/ios/sstream>

//-------------------------------------------------------------------------
// A container for mapping attribute names to arrays
class vtkMINCImageAttributeMap
{
public:
  typedef vtkstd::map<vtkstd::string, vtkSmartPointer<vtkObject> > MapType;

  static vtkMINCImageAttributeMap *New() {
    return new vtkMINCImageAttributeMap; };

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
  vtkMINCImageAttributeMap() : Map() {};
  ~vtkMINCImageAttributeMap() {};

  MapType Map;
};

//--------------------------------------------------------------------------
vtkStandardNewMacro(vtkMINCImageAttributes);

vtkCxxSetObjectMacro(vtkMINCImageAttributes,ImageMin,vtkDoubleArray);
vtkCxxSetObjectMacro(vtkMINCImageAttributes,ImageMax,vtkDoubleArray);

//-------------------------------------------------------------------------
vtkMINCImageAttributes::vtkMINCImageAttributes()
{
  this->DimensionNames = vtkStringArray::New();
  this->DimensionLengths = vtkIdTypeArray::New();

  this->VariableNames = vtkStringArray::New();

  this->AttributeNames = vtkMINCImageAttributeMap::New();
  // Add global attribute name array
  vtkStringArray *tmparray = vtkStringArray::New();
  tmparray->SetName("");
  this->AttributeNames->AddArray(tmparray);
  tmparray->Delete();

  this->AttributeValues = vtkMINCImageAttributeMap::New();
  this->StringStore = 0;

  this->NumberOfImageMinMaxDimensions = 0;
  this->ImageMin = 0;
  this->ImageMax = 0;

  this->Name = 0;
  this->DataType = VTK_VOID;

  this->ValidateAttributes = 1;
}

//-------------------------------------------------------------------------
vtkMINCImageAttributes::~vtkMINCImageAttributes()
{
  this->SetName(0);
  
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
  if (this->ImageMin)
    {
    this->ImageMin->Delete();
    this->ImageMin = 0;
    }
  if (this->ImageMax)
    {
    this->ImageMax->Delete();
    this->ImageMax = 0;
    }
  if (this->StringStore)
    {
    this->StringStore->Delete();
    this->StringStore = 0;
    }
}

//-------------------------------------------------------------------------
void vtkMINCImageAttributes::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Name: " 
     << (this->Name ? this->Name : "(none)") << "\n";
  os << indent << "DataType: " << this->DataType << "\n";
  os << indent << "DimensionNames: " << this->DimensionNames << "\n";
  os << indent << "DimensionLengths: " << this->DimensionLengths << "\n";
  os << indent << "VariableNames: " << this->VariableNames << "\n";
  os << indent << "ImageMin: " << this->ImageMin << "\n";
  os << indent << "ImageMax: " << this->ImageMax << "\n";
  os << indent << "NumberOfImageMinMaxDimensions: " 
     << this->NumberOfImageMinMaxDimensions << "\n";
  os << indent << "ValidateAttributes: "
     << (this->ValidateAttributes ? "On\n" : "Off\n");
}

//-------------------------------------------------------------------------
void vtkMINCImageAttributes::Reset()
{
  this->SetName(0);
  this->SetDataType(VTK_VOID);
  this->SetImageMin(0);
  this->SetImageMax(0);
  this->AttributeValues->Clear();
  this->AttributeNames->Clear();
  this->VariableNames->Reset();
  this->DimensionNames->Reset();
  this->DimensionLengths->Reset();
  if (this->StringStore)
    {
    this->StringStore->Reset();
    }
  this->NumberOfImageMinMaxDimensions = 0;

  // Add global attribute name array
  vtkStringArray *tmparray = vtkStringArray::New();
  tmparray->SetName("");
  this->AttributeNames->AddArray(tmparray);
  tmparray->Delete();
}

//-------------------------------------------------------------------------
// Allowed dimension variable names
static const char *vtkMINCDimVarNames[] = {
  MIxspace, MIyspace, MIzspace, MItime,
  MIxfrequency, MIyfrequency, MIzfrequency, MItfrequency,
  0
};

//-------------------------------------------------------------------------
void vtkMINCImageAttributes::AddDimension(const char *dimension,
                                          vtkIdType length)
{
  // Check for duplicates
  int n = this->DimensionNames->GetNumberOfValues();
  for (int i = 0; i < n; i++)
    {
    if (strcmp(dimension, this->DimensionNames->GetValue(i)) == 0)
      {
      vtkErrorMacro("The dimension " << dimension <<
                    " has already been created.");
      return;
      }
    }

  // Ensure the dimension name is valid
  const char **tryname = 0;
  for (tryname = vtkMINCDimVarNames; *tryname != 0; tryname++)
    {
    if (strcmp(dimension, *tryname) == 0)
      {
      break;
      }
    }
  if (*tryname == 0 && strcmp(dimension, MIvector_dimension) != 0)
    {
    vtkWarningMacro("The dimension name " << dimension <<
                    " is not recognized.");
    }

  this->DimensionNames->InsertNextValue(dimension);
  this->DimensionLengths->InsertNextTuple1(length);
}

//-------------------------------------------------------------------------
// This method also has to store the resulting string internally.
const char *vtkMINCImageAttributes::ConvertDataArrayToString(
  vtkDataArray *array)
{
  int dataType = array->GetDataType();

  if (dataType == VTK_CHAR)
    {
    vtkCharArray *charArray = vtkCharArray::SafeDownCast(array);
    return charArray->GetPointer(0);
    }

  vtksys_ios::ostringstream os;

  int n = array->GetNumberOfTuples();
  int i = 0;
  for (i = 0; i < n; i++)
    {
    double val = array->GetComponent(i, 0);
    if (dataType == VTK_DOUBLE || dataType == VTK_FLOAT)
      {
      // Use NetCDF's usual precision for printing the values
      char storage[128];
      if (dataType == VTK_DOUBLE)
        {
        sprintf(storage, "%0.15g", val);
        }
      else
        {
        sprintf(storage, "%0.7g", val);
        }
      // Add a decimal if there isn't one, to distinguish from int
      for (char *cp = storage; *cp != '.'; cp++)
        {
        if (*cp == '\0')
          {
          *cp++ = '.';
          *cp = '\0';
          break;
          } 
        }
      os << storage;
      }
    else
      {
      os << val;
      }
    if (i < n-1)
      {
      os << ", ";
      }
    }

    // Store the string
    vtkstd::string str = os.str();
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
      if (strcmp(str.c_str(), result) == 0)
        {
        break;
        }
      }
    // If not, add it to the array.
    if (i == n)
      {
      i = this->StringStore->InsertNextValue(str.c_str());
      result = this->StringStore->GetValue(i);
      }

    return result;
}

//-------------------------------------------------------------------------
void vtkMINCImageAttributes::PrintFileHeader()
{
  this->PrintFileHeader(cout);
}

//-------------------------------------------------------------------------
void vtkMINCImageAttributes::PrintFileHeader(ostream &os)
{
  const char *name = "unknown";
  if (this->Name)
    {
    name = this->Name;
    }

  // Get the data type
  const char *imageDataType;
  switch (this->DataType)
    {
    case VTK_CHAR:
    case VTK_SIGNED_CHAR:
    case VTK_UNSIGNED_CHAR:
      imageDataType = "byte";
      break;
    case VTK_SHORT:
    case VTK_UNSIGNED_SHORT:
      imageDataType = "short";
      break;
    case VTK_INT:
    case VTK_UNSIGNED_INT:
      imageDataType = "int";
      break;
    case VTK_FLOAT:
      imageDataType = "float";
      break;
    case VTK_DOUBLE:
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
  int ivar = 0;
  if (this->VariableNames)
    {
    nvar = this->VariableNames->GetNumberOfValues();
    }
  for (ivar = 0; ivar < nvar+1; ivar++)
    {
    const char *varname = MI_EMPTY_STRING;
    if (ivar == nvar)
      {
      os << "\n// global attributes:\n";
      }
    else
      {
      varname = this->VariableNames->GetValue(ivar);
      if (strcmp(varname, MIimage) == 0 ||
          strcmp(varname, MIimagemax) == 0 ||
          strcmp(varname, MIimagemin) == 0)
        {
        os << "\t" << imageDataType << " " << varname;
        int nvardim = this->DimensionNames->GetNumberOfValues();
        // If this is image-min or image-max, only print the
        // dimensions for these variables
        if (varname[5] == '-')
          {
          if (this->NumberOfImageMinMaxDimensions < nvardim)
            {
            nvardim = this->NumberOfImageMinMaxDimensions;
            }
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
                  case '\b':
                    text[j++] = '\\';
                    text[j] = '\b';
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
                  case '\v':
                    text[j++] = '\\';
                    text[j] = 'v';
                    break;
                  case '\'':
                    text[j++] = '\\';
                    text[j] = '\'';
                    break;
                  case '\"':
                    text[j++] = '\\';
                    text[j] = '\"';
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

  os << "data:\n";

  if (this->VariableNames)
    {
    nvar = this->VariableNames->GetNumberOfValues();
    }
  for (ivar = 0; ivar < nvar; ivar++)
    {
    const char *varname = this->VariableNames->GetValue(ivar);

    if (strcmp(varname, MIimage) == 0)
      {
      continue;
      }

    os << "\n " << varname << " = ";

    if (strcmp(varname, MIimagemin) == 0)
      {
      if (this->ImageMin)
        {
        os << this->ConvertDataArrayToString(this->ImageMin) << " ;\n";
        }
      else
        {
        os << "0. ;\n";
        }
      }
    else if (strcmp(varname, MIimagemax) == 0)
      {
      if (this->ImageMax)
        {
        os << this->ConvertDataArrayToString(this->ImageMax) << " ;\n";
        }
      else
        {
        os << "1. ;\n";
        }
      }
    else
      {
      os << "_ ;\n";
      }
    }

  os << "}\n";
}

//-------------------------------------------------------------------------
vtkStringArray *vtkMINCImageAttributes::GetAttributeNames(
  const char *variable)
{
  // If variable is null, use empty string to get global attributes
  if (variable == 0)
    {
    variable = MI_EMPTY_STRING;
    }

  return this->AttributeNames->GetStringArray(variable);
}

//-------------------------------------------------------------------------
int vtkMINCImageAttributes::HasAttribute(
  const char *variable,
  const char *attribute)
{
  return (this->GetAttributeValueAsArray(variable, attribute) != 0);
}

//-------------------------------------------------------------------------
vtkDataArray *vtkMINCImageAttributes::GetAttributeValueAsArray(
  const char *variable,
  const char *attribute)
{
  vtkstd::string path = MI_GRPNAME;
  if (variable && variable[0] != '\0')
    {
    path += MI_GRP_SEP;
    path += variable;
    }
  path += MI_ATT_SEP;
  path += attribute;

  return this->AttributeValues->GetDataArray(path.c_str());
}

//-------------------------------------------------------------------------
const char *vtkMINCImageAttributes::GetAttributeValueAsString(
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
int vtkMINCImageAttributes::GetAttributeValueAsInt(
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
double vtkMINCImageAttributes::GetAttributeValueAsDouble(
  const char *variable,
  const char *attribute)
{
  if (variable == 0)
    {
    variable = MI_EMPTY_STRING;
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
void vtkMINCImageAttributes::SetAttributeValueAsArray(
  const char *variable,
  const char *attribute,
  vtkDataArray *array)
{
  vtkstd::string path = MI_GRPNAME;
  if (variable && variable[0] != '\0')
    {
    path += MI_GRP_SEP;
    path += variable;
    }
  path += MI_ATT_SEP;
  path += attribute;

  array->SetName(path.c_str());
  this->AttributeValues->AddArray(array);

  // Add to variable to VariableNames
  int n = this->VariableNames->GetNumberOfValues();
  int i = 0;
  for (i = 0; i < n; i++)
    {
    if (strcmp(this->VariableNames->GetValue(i), variable) == 0)
      {
      break;
      }
    }
  if (i == n && variable[0] != '\0')
    {
    this->VariableNames->InsertNextValue(variable);
    }

  // Add to attribute to AttributeNames
  vtkStringArray *attribs = this->AttributeNames->GetStringArray(variable);
  // Create a new array if necessary
  if (attribs == 0)
    {
    attribs = vtkStringArray::New();
    attribs->SetName(variable);
    this->AttributeNames->AddArray(attribs);
    attribs->Delete();
    }

  n = attribs->GetNumberOfValues();
  i = 0;
  for (i = 0; i < n; i++)
    {
    if (strcmp(attribs->GetValue(i), attribute) == 0)
      {
      break;
      }
    }
  if (i == n)
    {
    attribs->InsertNextValue(attribute);
    }

  if (this->ValidateAttributes)
    {
    // Print warning if there is something wrong with the attribute
    int result = this->ValidateAttribute(variable, attribute, array);

    if (result > 1)
      {
      vtkWarningMacro("Attribute " << variable << ":" << attribute
                      << " is not a valid attribute.");
      }
    }
}

//-------------------------------------------------------------------------
void vtkMINCImageAttributes::SetAttributeValueAsString(
  const char *variable,
  const char *attribute,
  const char *value)
{
  size_t length = strlen(value)+1;

  vtkCharArray *array = vtkCharArray::New();
  array->SetNumberOfValues(length);
  strcpy(array->GetPointer(0), value);

  this->SetAttributeValueAsArray(variable, attribute, array);

  array->Delete();
}

//-------------------------------------------------------------------------
void vtkMINCImageAttributes::SetAttributeValueAsInt(
  const char *variable,
  const char *attribute,
  int value)
{
  vtkIntArray *array = vtkIntArray::New();
  array->SetNumberOfValues(1);
  array->SetValue(0, value);

  this->SetAttributeValueAsArray(variable, attribute, array);

  array->Delete();
}

//-------------------------------------------------------------------------
void vtkMINCImageAttributes::SetAttributeValueAsDouble(
  const char *variable,
  const char *attribute,
  double value)
{
  vtkDoubleArray *array = vtkDoubleArray::New();
  array->SetNumberOfValues(1);
  array->SetValue(0, value);

  this->SetAttributeValueAsArray(variable, attribute, array);

  array->Delete();
}

//-------------------------------------------------------------------------
// These validation methods have three return values:
// 0 means that the attribute should be skipped
// 1 means that the attribute should be set
// 2 means that the attribute wasn't recognized

//-------------------------------------------------------------------------
int vtkMINCImageAttributes::ValidateGlobalAttribute(
  const char *attname, vtkDataArray *vtkNotUsed(array))
{
  // Global attributes
  static const char *globalAttributes[] = {
    MIident,
    MIhistory,
    MItitle,
    0
  };
  const int autoGlobalAttributes = 2;

  int itry = 0;
  for (itry = 0; globalAttributes[itry] != 0; itry++)
    {
    if (strcmp(attname, globalAttributes[itry]) == 0)
      {
      break;
      }
    }
  if (itry < autoGlobalAttributes)
    {
    // Skip to the next attribute
    return 0;
    }
  else if (globalAttributes[itry] == 0)
    {
    return 2;
    }

  return 1;
}

//-------------------------------------------------------------------------
int vtkMINCImageAttributes::ValidateGeneralAttribute(
  const char *varname, const char *attname, vtkDataArray *array)
{
  // Attributes that all MINC variables have
  static const char *generalAttributes[] = {
    MIvartype,      // MI_GROUP, MI_DIMENSION, MI_VARATT
    MIvarid,        // MI_STDVAR
    MIversion,      // MI_VERSION_1_0
    MIparent,       // parent variable for this variable
    MIchildren,     // newline-separated list of child variables
    MIcomments,     // each variable has specific comments to go with it
    0
  };
  const int autoGeneralAttributes = 5;

  int dataType = array->GetDataType();

  // Check to see if the attribute is one that we automatically generate.
  int itry = 0;
  for (itry = 0; generalAttributes[itry] != 0; itry++)
    {
    if (strcmp(attname, generalAttributes[itry]) == 0)
      {
      break;
      }
    }
  if (itry < autoGeneralAttributes)
    {
    // Skip to the next attribute
    return 0;
    }
  else if (generalAttributes[itry] != 0)
    {
    if (dataType != VTK_CHAR)
      {
      vtkWarningMacro("The attribute " << varname << ":"
                      << attname << " has the wrong type ("
                      << dataType << ").");
      return 0;
      }
    }
  else
    {
    return 2;
    }

  return 1;
}

//-------------------------------------------------------------------------
int vtkMINCImageAttributes::ValidateDimensionAttribute(
  const char *varname, const char *attname, vtkDataArray *array)
{
  // Attributes for dimension variables (vartype = MI_DIMENSION)
  static const char *dimensionAttributes[] = {
    MIstep,
    MIstart,
    MIspacing,      // MI_REGULAR ("irregular" not supported)
    MIspacetype,    // "native____", "talairach_", "calossal__"
    MIalignment,    // MI_CENTRE ("start_", "end___" not supported)
    MIunits,        // "mm"
    MIdirection_cosines,  // three doubles
    0
  };
  const int autoDimensionAttributes = 3;

  vtkIdType size = (array->GetNumberOfTuples()*
                    array->GetNumberOfComponents());
  int dataType = array->GetDataType();

  int itry = 0;
  for (itry = 0; dimensionAttributes[itry] != 0; itry++)
    {
    if (strcmp(attname, dimensionAttributes[itry]) == 0)
      {
      break;
      }
    }
  if (itry < autoDimensionAttributes)
    {
    // Skip to the next attribute
    return 0;
    }
  else if (strcmp(attname, MIdirection_cosines) == 0)
    {
    if (varname[0] == 'x' || varname[0] == 'y' || varname[0] == 'z')
      {
      if (dataType != VTK_DOUBLE || size != 3)
        {
        vtkWarningMacro("The attribute " << varname << ":"
                        << attname << " has the wrong type ("
                        << dataType << ") or size ("
                        << size << ").");
        return 0;
        }
      }
    else
      {
      vtkWarningMacro("Dimension " << varname << " cannot have"
                      " a direction_cosines attribute");
      return 0;
      }
    }
  else if (dimensionAttributes[itry] != 0)
    {
    if (dataType != VTK_CHAR)
      {
      vtkWarningMacro("The attribute " << varname << ":"
                      << attname << " has the wrong type ("
                      << dataType << ").");
      return 0;
      }
    }
  else
    {
    return 2;
    }

  return 1;
}

//-------------------------------------------------------------------------
int vtkMINCImageAttributes::ValidateImageAttribute(
  const char *vtkNotUsed(varname), const char *attname,
  vtkDataArray *vtkNotUsed(array))
{
  // Attributes for the MIimage variable (vartype = MI_GROUP)
  static const char *imageAttributes[] = {
    MIcomplete,    // MI_TRUE (MI_FALSE means not yet all written)
    MIimagemin     // "--->image-min" variable attribute pointer
    MIimagemax     // "--->image-max" variable attribute pointer
    MIsigntype,     // MI_SIGNED or MI_UNSIGNED
    MIvalid_range,  // min and max scalar values as doubles
    0
  };
  const int autoImageAttributes = 5;

  int itry = 0;
  for (itry = 0; imageAttributes[itry] != 0; itry++)
    {
    if (strcmp(attname, imageAttributes[itry]) == 0)
      {
      break;
      }
    }
  if (itry < autoImageAttributes)
    {
    // Skip to the next attribute
    return 0;
    }
  else if (imageAttributes[itry] == 0)
    {
    return 2;
    }

  return 1;
}

//-------------------------------------------------------------------------
int vtkMINCImageAttributes::ValidateImageMinMaxAttribute(
  const char *varname, const char *attname, vtkDataArray *array)
{
  // Attributes for MIimagemin, MIimagemax (vartype = MI_VARATT)
  static const char *imageMinMaxAttributes[] = {
     MI_FillValue,  // 0.0 for image-min, 1.0 for image-max
     MIunits,       // "normalized", "Hounsfields", etc.
     0
  };
  const int autoImageMinMaxAttributes = 1;

  int itry = 0;
  for (itry = 0; imageMinMaxAttributes[itry] != 0; itry++)
    {
    if (strcmp(attname, imageMinMaxAttributes[itry]) == 0)
      {
      break;
      }
    }
  if (itry < autoImageMinMaxAttributes)
    {
    // Skip to the next attribute
    return 0;
    }
  else if (imageMinMaxAttributes[itry] != 0)
    {
    int dataType = array->GetDataType();
    if (dataType != VTK_CHAR)
      {
      vtkWarningMacro("The attribute " << varname << ":"
                      << attname << " has the wrong type ("
                      << dataType << ").");
      return 0;
      }
    }
  else
    {
    return 2;
    }

  return 1;
}

//-------------------------------------------------------------------------
int vtkMINCImageAttributes::ValidatePatientAttribute(
  const char *vtkNotUsed(varname), const char *attname,
  vtkDataArray *vtkNotUsed(array))
{
  // Attributes for MIpatient variable (vartype = MI_GROUP)
  static const char *patientAttributes[] = {
    MIfull_name,     // "LASTNAME^FIRSTNAME SECONDNAME"
    MIother_names,   // newline-separated string
    MIidentification,
    MIother_ids,
    MIbirthdate,     // "YYYYMMDD"
    MIsex,           // "male__", "female", "other_"
    MIage,           // "XXXD", "XXXM", or "XXXY" (days, months, years)
    MIweight,        // "XXkg", "X.Xkg" (assume kg if no units given)
    MIsize,          // "XXXcm" (assume metres if no units given)
    MIaddress,       // newline-separated string
    MIinsurance_id,
    0
  };

  int itry = 0;
  for (itry = 0; patientAttributes[itry] != 0; itry++)
    {
    if (strcmp(attname, patientAttributes[itry]) == 0)
      {
      break;
      }
    }
  if (patientAttributes[itry] != 0)
    {
    // Add checks for correct data type?
    }
  else
    {
    return 2;
    }

  return 1;
}

//-------------------------------------------------------------------------
int vtkMINCImageAttributes::ValidateStudyAttribute(
  const char *vtkNotUsed(varname), const char *attname,
  vtkDataArray *vtkNotUsed(array))
{
  // Attributes for MIstudy variable (vartype = MI_GROUP)
  static const char *studyAttributes[] = {
    MIstudy_id,
    MIstart_time,    // "YYYYMMDDHHMMSS.SS"
    MIstart_year,    // as int (use start_time instead)
    MIstart_month,   // as int (use start_time instead)
    MIstart_day,     // as int (use start_time instead)
    MIstart_hour,    // as int (use start_time instead)
    MIstart_minute,  // as int (use start_time instead)
    MIstart_seconds, // as double or int (use start_time instead)
    MImodality,      // "PET__", "SPECT", "GAMMA", "MRI__", "MRS__",
                     // "MRA__", "CT___", "DSA__", "DR___", "label"
    MImanufacturer,
    MIdevice_model,
    MIinstitution,
    MIdepartment,
    MIstation_id,
    MIreferring_physician,
    MIattending_physician,
    MIradiologist,
    MIoperator,
    MIadmitting_diagnosis,
    MIprocedure,
    0
  };

  int itry = 0;
  for (itry = 0; studyAttributes[itry] != 0; itry++)
    {
    if (strcmp(attname, studyAttributes[itry]) == 0)
      {
      break;
      }
    }
  if (studyAttributes[itry] != 0)
    {
    // Add checks for correct data type?
    }
  else
    {
    return 2;
    }

  return 1;
}

//-------------------------------------------------------------------------
int vtkMINCImageAttributes::ValidateAcquisitionAttribute(
  const char *vtkNotUsed(varname), const char *attname,
  vtkDataArray *vtkNotUsed(array))
{
  // Attributes for MIacquisition variable (vartype = MI_GROUP)
  static const char *acquisitionAttributes[] = {
    MIprotocol,
    MIscanning_sequence, // "GR", "SPGR", etc.
    MIrepetition_time,   // as double, milliseconds
    MIecho_time,         // as double, milliseconds
    MIinversion_time,    // as double, milliseconds
    MInum_averages,      // as int
    MIimaging_frequency, // in Hz, as double
    MIimaged_nucleus,    // "H1", "C13", etc. for MRI
    MIradionuclide,      // for PET and SPECT
    MIradionuclide_halflife,
    MIcontrast_agent,
    MItracer,
    MIinjection_time,
    MIinjection_year,
    MIinjection_month,
    MIinjection_day,
    MIinjection_hour,
    MIinjection_minute,
    MIinjection_seconds,
    MIinjection_length,
    MIinjection_dose,
    MIdose_units,
    MIinjection_volume,
    MIinjection_route,
    0
  };

  int itry = 0;
  for (itry = 0; acquisitionAttributes[itry] != 0; itry++)
    {
    if (strcmp(attname, acquisitionAttributes[itry]) == 0)
      {
      break;
      }
    }
  if (acquisitionAttributes[itry] != 0)
    {
    // Add checks for correct data type?
    }
  else
    {
    return 2;
    }

  return 1;
}

//-------------------------------------------------------------------------
int vtkMINCImageAttributes::ValidateAttribute(
  const char *varname, const char *attname, vtkDataArray *array)
{
  // Standard variable names
  static const char *stdVarNames[] = {
    MIrootvariable, MIimage, MIimagemin, MIimagemax,
    MIpatient, MIstudy, MIacquisition,
    0
  };

  int result = 1;
  int vartype = 0;

  const char **tryname = 0;
  for (tryname = stdVarNames; *tryname != 0; tryname++)
    {
    if (strcmp(varname, *tryname) == 0)
      {
      vartype = 1;
      break;
      }
    }
  for (tryname = vtkMINCDimVarNames; *tryname != 0; tryname++)
    {
    if (strcmp(varname, *tryname) == 0)
      {
      vartype = 2;
      break;
      }
    }

  if (strcmp(varname, MI_EMPTY_STRING) == 0)
    {
    // Check global attributes
    result = this->ValidateGlobalAttribute(attname, array);
    // Allow users to create their own global attributes
    if (result == 2)
      {
      result = 1;
      }
    }
  else if (vartype != 0)
    {
    // Check general attributes
    result = this->ValidateGeneralAttribute(varname, attname, array);
    }
  if (result == 2)
    {
    if (vartype == 2)
      {
      result = this->ValidateDimensionAttribute(varname, attname, array);
      }
    else if (strcmp(varname, MIimage) == 0)
      {
      result = this->ValidateImageAttribute(varname, attname, array);
      }
    else if (strcmp(varname, MIimagemin) == 0 ||
             strcmp(varname, MIimagemax) == 0)
      {
      result = this->ValidateImageMinMaxAttribute(varname, attname, array);
      }
    else if (strcmp(varname, MIpatient) == 0)
      {
      result = this->ValidatePatientAttribute(varname, attname, array);
      }
    else if (strcmp(varname, MIstudy) == 0)
      {
      result = this->ValidateStudyAttribute(varname, attname, array);
      }
    else if (strcmp(varname, MIacquisition) == 0)
      {
      result = this->ValidateAcquisitionAttribute(varname, attname, array);
      }
    }
  
  return result;
}

//-------------------------------------------------------------------------
void vtkMINCImageAttributes::FindValidRange(double range[2])
{
  // Find the valid range. Start with the default.
  range[0] = 0.0;
  range[1] = 1.0;

  // Look for the valid_range attribute of the data. 
  vtkDoubleArray *rangearray = 
    vtkDoubleArray::SafeDownCast(this->GetAttributeValueAsArray(
                                   MIimage, MIvalid_range));
  if (rangearray)
    {
    range[0] = rangearray->GetValue(0);
    range[1] = rangearray->GetValue(1);
    if (range[0] > range[1])
      {
      double tmpval = range[0];
      range[0] = range[1];
      range[1] = tmpval;
      }

    if (this->DataType == VTK_FLOAT)
      {
      // use float precision if VTK_FLOAT
      range[0] = (float)range[0];
      range[1] = (float)range[1];
      }
    }
  else
    {
    // If there is no valid_range attribute, use maximum range.
    switch (this->DataType)
      {
      case VTK_CHAR:
      case VTK_SIGNED_CHAR:
        range[0] = VTK_SIGNED_CHAR_MIN;
        range[1] = VTK_SIGNED_CHAR_MAX;
        break;
      case VTK_UNSIGNED_CHAR:
        range[0] = VTK_UNSIGNED_CHAR_MIN;
        range[1] = VTK_UNSIGNED_CHAR_MAX;
        break;
      case VTK_SHORT:
        range[0] = VTK_SHORT_MIN;
        range[1] = VTK_SHORT_MAX;
        break;
      case VTK_UNSIGNED_SHORT:
        range[0] = VTK_UNSIGNED_SHORT_MIN;
        range[1] = VTK_UNSIGNED_SHORT_MAX;
        break;
      case VTK_INT:
        range[0] = VTK_INT_MIN;
        range[1] = VTK_INT_MAX;
        break;
      case VTK_UNSIGNED_INT:
        range[0] = VTK_UNSIGNED_INT_MIN;
        range[1] = VTK_UNSIGNED_INT_MAX;
        break;
      case VTK_FLOAT:
        range[0] = -FLT_MAX;
        range[1] = FLT_MAX;
        break;
      case VTK_DOUBLE:
        range[0] = -DBL_MAX;
        range[1] = DBL_MAX;
        break;
      }
    }

  // If the valid_range is set to the full float range, replace
  // with the image range.
  if ((this->DataType == VTK_FLOAT && range[1] == FLT_MAX) ||
      (this->DataType == VTK_DOUBLE && range[1] == DBL_MAX))
    {
    if (this->ImageMin && this->ImageMax &&
        this->ImageMin->GetNumberOfTuples() > 0 &&
        this->ImageMax->GetNumberOfTuples() > 0)
      {
      range[0] = this->ImageMin->GetRange()[0];
      range[1] = this->ImageMax->GetRange()[1];
      }
    else
      {
      range[0] = 0.0;
      range[1] = 1.0;
      }
    }
}

//-------------------------------------------------------------------------
void vtkMINCImageAttributes::FindImageRange(double range[2])
{
  // Initialize to the default values
  range[0] = 0.0;
  range[1] = 1.0;

  // If image-min and image-max variables exist, use them.
  // Otherwise, use the valid_range of the data instead.
  if (this->ImageMin && this->ImageMax &&
      this->ImageMin->GetNumberOfTuples() > 0 &&
      this->ImageMax->GetNumberOfTuples() > 0)
    {
    range[0] = this->ImageMin->GetRange()[0];
    range[1] = this->ImageMax->GetRange()[1];
    }
  else
    {
    this->FindValidRange(range);
    }
}

//-------------------------------------------------------------------------
void vtkMINCImageAttributes::ShallowCopy(vtkMINCImageAttributes *source)
{
  this->SetName(source->GetName());
  this->SetDataType(source->GetDataType());

  this->SetImageMin(source->GetImageMin());
  this->SetImageMax(source->GetImageMax());
  this->SetNumberOfImageMinMaxDimensions( 
    source->GetNumberOfImageMinMaxDimensions());

  this->DimensionNames->DeepCopy(source->GetDimensionNames());
  this->DimensionLengths->DeepCopy(source->GetDimensionLengths());
  
  this->VariableNames->Reset();
  this->AttributeValues->Clear();
  this->AttributeNames->Clear();

  vtkStringArray *varnames = source->GetVariableNames();
  int nvar = varnames->GetNumberOfValues();
  for (int ivar = 0; ivar <= nvar; ivar++)
    {
    // set varname to emtpy last time around to get global attributes
    const char *varname = MI_EMPTY_STRING;
    if (ivar < nvar)
      {
      varname = varnames->GetValue(ivar);
      }
    vtkStringArray *attnames = source->GetAttributeNames(varname);
    int natt = attnames->GetNumberOfValues();
    for (int iatt = 0; iatt < natt; iatt++)
      {
      const char *attname = attnames->GetValue(iatt);
      this->SetAttributeValueAsArray(
        varname, attname, source->GetAttributeValueAsArray(varname, attname));
      }
    }

  if (this->StringStore)
    {
    this->StringStore->Reset();
    }
}

