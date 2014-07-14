/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMINCImageWriter.cxx

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

#include "vtkMINCImageWriter.h"

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
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include "vtkType.h"

#include "vtkMINCImageAttributes.h"
#include "vtkMINC.h"
#include "vtk_netcdf.h"

#ifdef _WIN32
#include "vtkWindows.h"
#else
#include <sys/types.h>
#include <unistd.h>
#endif

#include <cstdlib>
#include <float.h>
#include <ctime>
#include <string>
#include <vector>
#include <map>

#define VTK_MINC_MAX_DIMS 8

//--------------------------------------------------------------------------
vtkStandardNewMacro(vtkMINCImageWriter);

vtkCxxSetObjectMacro(vtkMINCImageWriter,DirectionCosines,vtkMatrix4x4);
vtkCxxSetObjectMacro(vtkMINCImageWriter,ImageAttributes,
                     vtkMINCImageAttributes);

//-------------------------------------------------------------------------
vtkMINCImageWriter::vtkMINCImageWriter()
{
  this->DirectionCosines = 0;
  this->RescaleIntercept = 0.0;
  this->RescaleSlope = 0.0;
  this->InternalRescaleIntercept = 0.0;
  this->InternalRescaleSlope = 0.0;

  this->MINCImageType = 0;
  this->MINCImageTypeSigned = 1;
  this->MINCImageMinMaxDims = 0;

  this->FileDataType = 0;
  this->FileValidRange[0] = 0.0;
  this->FileValidRange[1] = 1.0;
  this->ComputeValidRangeFromScalarRange = 0;

  this->DataUpdateExtent[0] = 0;
  this->DataUpdateExtent[1] = 0;
  this->DataUpdateExtent[2] = 0;
  this->DataUpdateExtent[3] = 0;
  this->DataUpdateExtent[4] = 0;
  this->DataUpdateExtent[5] = 0;

  this->FileDimensionNames = vtkStringArray::New();

  this->ImageAttributes = 0;

  this->StrictValidation = 1;

  this->MismatchedInputs = 0;

  this->HistoryAddition = 0;
}

//-------------------------------------------------------------------------
vtkMINCImageWriter::~vtkMINCImageWriter()
{
  if (this->DirectionCosines)
    {
    this->DirectionCosines->Delete();
    this->DirectionCosines = 0;
    }
  if (this->FileDimensionNames)
    {
    this->FileDimensionNames->Delete();
    this->FileDimensionNames = 0;
    }
  if (this->ImageAttributes)
    {
    this->ImageAttributes->Delete();
    this->ImageAttributes = 0;
    }
  this->SetHistoryAddition(0);
}

//-------------------------------------------------------------------------
void vtkMINCImageWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "DirectionCosines: " << this->DirectionCosines << "\n";
  if (this->DirectionCosines)
    {
    this->DirectionCosines->PrintSelf(os, indent.GetNextIndent());
    }
  os << indent << "RescaleSlope: " << this->RescaleSlope << "\n";
  os << indent << "RescaleIntercept: " << this->RescaleIntercept << "\n";
  os << indent << "StrictValidation: " <<
    (this->StrictValidation ? "On\n" : "Off\n");
  os << indent << "HistoryAddition: " <<
    (this->HistoryAddition ? this->HistoryAddition : "(None)") << "\n";
}

//-------------------------------------------------------------------------
void vtkMINCImageWriter::SetFileName(const char *name)
{
  this->Superclass::SetFileName(name);
}

//-------------------------------------------------------------------------
int vtkMINCImageWriter::OpenNetCDFFile(const char *filename, int& ncid)
{
  int status = 0;

  if (filename == 0)
    {
    vtkErrorMacro("No filename was set");
    return 0;
    }

  status = nc_create(filename, 0, &ncid);
  if (status != NC_NOERR)
    {
    vtkErrorMacro("Could not open the MINC file:\n"
                  << nc_strerror(status));
    return 0;
    }

  return 1;
}

//-------------------------------------------------------------------------
int vtkMINCImageWriter::CloseNetCDFFile(int ncid)
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
#define vtkMINCImageWriterFailAndClose(ncid, status) \
{ \
  if (status != NC_NOERR) \
    { \
    vtkErrorMacro("There was an error with the MINC file \"" \
                  << this->GetFileName() << "\":\n" \
                  << nc_strerror(status)); \
    } \
  nc_close(ncid); \
}

//-------------------------------------------------------------------------
// Function for getting VTK dimension index from file name.
int vtkMINCImageWriter::IndexFromDimensionName(const char *dimName)
{
  switch(dimName[0])
    {
    case 'x':
      return this->Permutation[0];
    case 'y':
      return this->Permutation[1];
    case 'z':
      return this->Permutation[2];
    default:
      if (strcmp(dimName, MIvector_dimension) == 0)
        {
        return -1;
        }
      break;
    }

  // Any unrecognized dimensions are returned as index 3
  return 3;
}

//-------------------------------------------------------------------------
// Compute the default dimension order from the direction cosines,
// and look for flips.
// The way the permutation should be used is as follows:
// If permutation[0] == 0 then MIxspace is VTK's X dimension.
// If permutation[0] == 2 then MIxspace is VTK's Z dimension.
// If the "flip" is set for a VTK, then that VTK dimension
// and its dircos will have to be flipped before the MINC
// file is written.
// For example, if flip[2] == 1, then the MINC dimension that
// maps to the VTK Z dimension will to be flipped along with
// its dircos.
void vtkMINCImageWriter::ComputePermutationFromOrientation(
  int permutation[3], int flip[3])
{
  vtkMatrix4x4 *matrix = this->DirectionCosines;
  if (matrix == 0)
    {
    permutation[0] = 0;
    permutation[1] = 1;
    permutation[2] = 2;
    flip[0] = 0;
    flip[1] = 0;
    flip[2] = 0;

    return;
    }

  // There are 6 permutations for 3 dimensions.  In addition,
  // if each of those dimensions can be flipped, then there are
  // 8 (two to the power of three) possible flips.  That would
  // give 48 different possibilities, but since we don't consider
  // any combinations that result in left-handed rotations, the
  // total number of combinations that we test is 24.

  // Convert the matrix into three column vectors
  double vectors[3][4];
  int i = 0;
  int j = 0;
  for (i = 0; i < 3; i++)
    {
    double *v = vectors[i];
    for (j = 0; j < 4; j++)
      {
      v[j] = 0.0;
      }
    v[i] = 1.0;
    matrix->MultiplyPoint(v, v);
    }

  // Here's how the algorithm works.  We want to find a matrix
  // composed only of permutations and flips that has the closest
  // possible orientation (in terms of absolute orientation angle)
  // to our DirectionCosines.

  // The orientation angle for any matrix A is given by:
  //
  //   cos(angle/2) = sqrt(1 + trace(A))/2
  //
  // Therefore, the minimum angle occurs when the trace is
  // at its maximum.

  // So our method is to calculate the traces of all the various
  // permutations and flips, and just use the one with the largest
  // trace.

  // First check if the matrix includes an odd number of flips,
  // since if it does, it specifies a left-handed rotation.
  double d = vtkMath::Determinant3x3(vectors[0], vectors[1], vectors[2]);
  int oddPermutation = (d < 0);

  // Calculate all the traces, including any combination of
  // permutations and flips that represent right-handed
  // orientations.
  int imax = 0;
  int jmax = 0;
  int kmax = 0;
  int lmax = 0;
  double maxtrace = -1e30;

  for (i = 0; i < 3; i++)
    {
    for (j = 0; j < 2; j++)
      {
      double xval = vectors[i][0];
      double yval = vectors[(i + 1 + j) % 3][1];
      double zval = vectors[(i + 2 - j) % 3][2];
      for (int k = 0; k < 2; k++)
        {
        for (int l = 0; l < 2; l++)
          {
          // The (1 - 2*k) gives a sign from a boolean.
          // For z, we want to set the sign that will
          // not change the handedness ("^" is XOR).
          double xtmp = xval * (1 - 2*k);
          double ytmp = yval * (1 - 2*l);
          double ztmp = zval * (1 - 2*(j ^ k ^ l ^ oddPermutation));

          double trace = xtmp + ytmp + ztmp;

          // Find maximum trace
          if (trace > maxtrace)
            {
            maxtrace = trace;
            imax = i;
            jmax = j;
            kmax = k;
            lmax = l;
            }
          }
        }
      }
    }

  // Find the permutation to map each column of the orientation
  // matrix to a spatial dimension x, y, or z.
  int xidx = imax;
  int yidx = (imax + 1 + jmax) % 3;
  int zidx = (imax + 2 - jmax) % 3;

  permutation[0] = xidx;
  permutation[1] = yidx;
  permutation[2] = zidx;

  flip[xidx] = kmax;
  flip[yidx] = lmax;
  flip[zidx] = (jmax ^ kmax ^ lmax ^ oddPermutation);
}

//-------------------------------------------------------------------------
// Create an identity string for a file.
std::string vtkMINCImageWriterCreateIdentString()
{
  // A static counter for this process.
  static int identx = 1;

  // The separator between element.
  static const char *itemsep = ":";

  // Get username and hostname
  const char *username = 0;
  const char *hostname = 0;
#ifdef _WIN32
  char usernametext[100];
  DWORD numchars = sizeof(usernametext);
  if (GetUserName(usernametext, &numchars))
    {
    username = usernametext;
    }
  char hostnametext[100];
  numchars = sizeof(hostnametext);
  if (GetComputerName(hostnametext, &numchars))
    {
    hostname = hostnametext;
    }
#else
  username = getenv("LOGNAME");
  hostname = getenv("HOSTNAME");
#endif
  if (username == 0)
    {
    username = "nobody";
    }
  if (hostname == 0)
    {
    hostname = "unknown";
    }
  std::string ident = username;
  ident.append(itemsep);
  ident.append(hostname);
  ident.append(itemsep);

  // Get the local time
  char buf[1024];
  time_t t;
  time(&t);
  strftime(buf, sizeof(buf), "%Y.%m.%d.%H.%M.%S", localtime(&t));
  ident.append(buf);
  ident.append(itemsep);

  // Get the process ID and the counter for this process.
#ifdef _WIN32
  int processId = GetCurrentProcessId();
#else
  int processId = getpid();
#endif
  sprintf(buf, "%i%s%i", processId, itemsep, identx++);
  ident.append(buf);

  return ident;
}

//-------------------------------------------------------------------------
nc_type vtkMINCImageWriterConvertVTKTypeToMINCType(
  int dataType,
  int &mincsigned)
{
  nc_type minctype = NC_BYTE;

  // Get the vtk type of the data.
  switch (dataType)
    {
    case VTK_CHAR:
    case VTK_SIGNED_CHAR:
      minctype = NC_BYTE;
      mincsigned = 1;
      break;
    case VTK_UNSIGNED_CHAR:
      minctype = NC_BYTE;
      mincsigned = 0;
      break;
    case VTK_SHORT:
      minctype = NC_SHORT;
      mincsigned = 1;
      break;
    case VTK_UNSIGNED_SHORT:
      minctype = NC_SHORT;
      mincsigned = 0;
      break;
    case VTK_INT:
      minctype = NC_INT;
      mincsigned = 1;
      break;
    case VTK_UNSIGNED_INT:
      minctype = NC_INT;
      mincsigned = 0;
      break;
    case VTK_FLOAT:
      minctype = NC_FLOAT;
      mincsigned = 1;
      break;
    case VTK_DOUBLE:
      minctype = NC_DOUBLE;
      mincsigned = 1;
      break;
    default:
      break;
    }

  return minctype;
}

//-------------------------------------------------------------------------
// These macro is only for use in WriteMINCFileAttributes
#define vtkMINCImageWriterPutAttributeTextMacro(name, text) \
  if (status == NC_NOERR) \
    { \
    status = nc_put_att_text(ncid, varid, name, strlen(text)+1, text); \
    }

#define vtkMINCImageWriterPutAttributeDoubleMacro(name, count, ptr) \
  if (status == NC_NOERR) \
    { \
    status = nc_put_att_double(ncid, varid, name, NC_DOUBLE, count, ptr); \
    }

//-------------------------------------------------------------------------
// Allowed dimension variable names
static const char *vtkMINCDimVarNames[] = {
  MIxspace, MIyspace, MIzspace, MItime,
  MIxfrequency, MIyfrequency, MIzfrequency, MItfrequency,
  0
};

//-------------------------------------------------------------------------
int vtkMINCImageWriter::CreateMINCDimensions(
  vtkImageData *input, int numTimeSteps, int *dimids)
{
  int wholeExtent[6];
  vtkStreamingDemandDrivenPipeline::GetWholeExtent(
    this->GetInputInformation(0, 0), wholeExtent);
  int numComponents = input->GetNumberOfScalarComponents();

  // Create a default dimension order using the direction cosines.
  this->ComputePermutationFromOrientation(this->Permutation, this->Flip);
  const char *defaultdims[3];
  defaultdims[this->Permutation[0]] = MIxspace;
  defaultdims[this->Permutation[1]] = MIyspace;
  defaultdims[this->Permutation[2]] = MIzspace;

  int hasTimeDim = 0;
  std::vector<std::string> dimensions;
  int nuserdims = 0;
  vtkStringArray *dimensionNames = 0;
  if (this->ImageAttributes)
    {
    dimensionNames = this->ImageAttributes->GetDimensionNames();
    nuserdims = dimensionNames->GetNumberOfValues();
    }
  for (int iuserdims = 0; iuserdims < nuserdims; iuserdims++)
    {
    const char *dimname = dimensionNames->GetValue(iuserdims);
    // Remove vector_dimension, we'll add it back if it is needed
    if (strcmp(dimname, MIvector_dimension) == 0)
      {
      continue;
      }
    // Check for time or tfrequency
    if (dimname[0] == 't')
      {
      hasTimeDim = 1;
      }
    // Ensure the dimension name is valid
    const char **tryname = 0;
    for (tryname = vtkMINCDimVarNames; *tryname !=0; tryname++)
      {
      if (strcmp(dimname, *tryname) == 0)
        {
        break;
        }
      }
    if (*tryname == 0)
      {
      vtkErrorMacro("The dimension name " << dimname <<
                    " is not recognized.");
      return 0;
      }
    // Check for duplicated dimensions
    int ndim = static_cast<int>(dimensions.size());
    for (int idim = 0; idim < ndim; idim++)
      {
      if (dimensions[idim][0] == dimname[0])
        {
        vtkErrorMacro("Tried to create dimension " << dimname <<
                      " but " << dimensions[idim] << " already exists");
        return 0;
        }
      }

    // Add the dimension
    dimensions.push_back(dimname);
    }

  // Make sure number of dimensions matches the dimensionality
  int timeDimensions = ( numTimeSteps > 1);
  int spatialDimensions = ((wholeExtent[0] < wholeExtent[1]) +
                           (wholeExtent[2] < wholeExtent[3]) +
                           (wholeExtent[4] < wholeExtent[5]));
  if (spatialDimensions < 2)
    {
    spatialDimensions = 2;
    }
  // Insert dimension names until we have all spatial dimensions
  while (static_cast<int>(dimensions.size()) < spatialDimensions+hasTimeDim)
    {
    // Make sure we don't insert a dimension that is already there
    for (int i = 0; i < 3; i++)
      {
      int idim = 0;
      int ndims = static_cast<int>(dimensions.size());
      for (idim = 0; idim < ndims; idim++)
        {
        if (defaultdims[i][0] == dimensions[idim][0])
          {
          break;
          }
        }
      if (idim == ndims)
        {
        dimensions.insert(dimensions.begin(), defaultdims[i]);
        }
      }
    }
  // Make sure we have a time dimension if we need one
  if (timeDimensions == 1 && hasTimeDim == 0)
    {
    dimensions.insert(dimensions.begin(), MItime);
    }
  // Check for vector_dimension
  if (numComponents > 1)
    {
    dimensions.push_back(MIvector_dimension);
    }

  // ------------------------
  // Create the NetCDF dimensions

  int ncid = this->MINCFileId;
  int status = NC_NOERR;

  int ndim = static_cast<int>(dimensions.size());
  this->FileDimensionNames->SetNumberOfValues(ndim);
  for (int idim = 0; idim < ndim; idim++)
    {
    const char *dimname = dimensions[idim].c_str();
    this->FileDimensionNames->SetValue(idim, dimname);
    int dimIndex = this->IndexFromDimensionName(dimname);
    size_t length = numTimeSteps;
    if (dimIndex >= 0 && dimIndex < 3)
      {
      length = wholeExtent[2*dimIndex+1] - wholeExtent[2*dimIndex] + 1;
      }
    else if (strcmp(dimname, MIvector_dimension) == 0)
      {
      length = numComponents;
      }
    status = nc_def_dim(ncid, dimname, length, &dimids[idim]);
    if (status != NC_NOERR)
      {
      vtkMINCImageWriterFailAndClose(ncid, status);
      this->MINCFileId = 0;
      return 0;
      }
    }

  return 1;
}

//-------------------------------------------------------------------------
int vtkMINCImageWriter::CreateMINCVariables(
  vtkImageData *input, int vtkNotUsed(numTimeSteps), int *dimids)
{
  // Allowed standard variable names
  static const char *stdVarNames[] = {
    MIrootvariable, MIimage, MIimagemin, MIimagemax,
    MIpatient, MIstudy, MIacquisition,
    0
  };

  std::vector<std::string> variables;

  // Get the information from the input
  double spacing[3];
  double origin[3];
  int wholeExtent[6];
  int numComponents = input->GetNumberOfScalarComponents();
  int imageDataType = input->GetScalarType();
  input->GetSpacing(spacing);
  input->GetOrigin(origin);
  vtkStreamingDemandDrivenPipeline::GetWholeExtent(
    this->GetInputInformation(0, 0), wholeExtent);

  // Add all dimensions onto the list of variables
  int ndim = this->FileDimensionNames->GetNumberOfValues();
  for (int dimidx = 0; dimidx < ndim; dimidx++)
    {
    const char *dimname = this->FileDimensionNames->GetValue(dimidx);
    // vector_dimension isn't ever included as a variable
    if (strcmp(dimname, MIvector_dimension) != 0)
      {
      variables.push_back(this->FileDimensionNames->GetValue(dimidx));
      }
    }
  // Reset ndim so that it only includes dimensions with variables
  ndim = static_cast<int>(variables.size());

  variables.push_back(MIimage);
  variables.push_back(MIrootvariable);

  // Not all MINC images need image-min and image-max.
  this->MINCImageMinMaxDims = 0;
  if (this->InternalRescaleSlope != 0)
    {
    // Check whether slice-by-slice rescaling is needed
    if ((imageDataType == VTK_FLOAT ||
         imageDataType == VTK_DOUBLE) &&
        (this->MINCImageType != NC_FLOAT &&
         this->MINCImageType != NC_DOUBLE))
      {
      this->MINCImageMinMaxDims = ndim - 2;
      }
    variables.push_back(MIimagemin);
    variables.push_back(MIimagemax);
    }

  // Add user-defined variables
  int nuservars = 0;
  vtkStringArray *variableNames = 0;
  if (this->ImageAttributes)
    {
    variableNames = this->ImageAttributes->GetVariableNames();
    nuservars = variableNames->GetNumberOfValues();
    }
  for (int iuservars = 0; iuservars < nuservars; iuservars++)
    {
    const char *varname = variableNames->GetValue(iuservars);
    int ivar;
    int nvars = static_cast<int>(variables.size());
    for (ivar = 0; ivar < nvars; ivar++)
      {
      if (strcmp(variables[ivar].c_str(), varname) == 0)
        {
        break;
        }
      }
    if (ivar == nvars) // wasn't already in the list
      {
      // Check if the variable name is a dimension that isn't one
      // of the selected dimensions for this image
      for (const char **tryname = vtkMINCDimVarNames; *tryname !=0; tryname++)
        {
        if (strcmp(varname, *tryname) == 0)
          {
          vtkErrorMacro("The variable " << varname
                        << " is not a dimension of this image");
          return 0;
          }
        }
        variables.push_back(varname);
      }
    }

  // ------------------------
  // Find the children of the root variable
  std::string rootChildren = MI_EMPTY_STRING;

  int nvars = static_cast<int>(variables.size());
  int ivar = 0;
  for (ivar = 0; ivar < nvars; ivar++)
    {
    const char *varname = variables[ivar].c_str();
    if (strcmp(varname, MIrootvariable) == 0 ||
        strcmp(varname, MIimagemin) == 0 ||
        strcmp(varname, MIimagemax) == 0)
      {
      continue;
      }
    for (const char **tryname = stdVarNames; *tryname !=0; tryname++)
      {
      if (strcmp(varname, *tryname) == 0)
        {
        if (rootChildren != MI_EMPTY_STRING)
          {
          rootChildren.append(MI_CHILD_SEPARATOR);
          }
        rootChildren.append(varname);
        break;
        }
      }
    }

  // ------------------------
  // Create the variables and write the attributes.
  // Start at -1, which stands for global attributes.
  int ncid = this->MINCFileId;
  int status = NC_NOERR;
  nvars = static_cast<int>(variables.size());
  for (ivar = -1; ivar < nvars; ivar++)
    {
    const char *varname = MI_EMPTY_STRING;
    const char *vartype = MI_EMPTY_STRING;
    int varid = -1;

    if (ivar >= 0)
      {
      nc_type cdftype = NC_INT;
      varname = variables[ivar].c_str();
      const char *parent = MIrootvariable;
      const char *children = 0;
      int vardims = 0;

      // The dimensions are the first variables (note that ndim
      // does not include the vector_dimension)
      if (ivar < ndim)
        {
        vartype = MI_DIMENSION;
        }
      else
        {
        for (const char **tryname = stdVarNames; *tryname != 0; tryname++)
          {
          if (strcmp(varname, *tryname) == 0)
            {
            vartype = MI_GROUP;
            }
          }
        }

      // Check if this is an image-related variable
      if (strcmp(varname, MIimage) == 0)
        {
        cdftype = (nc_type)this->MINCImageType;
        vardims = ndim + (numComponents > 1);
        }
      else if (strcmp(varname, MIimagemin) == 0 ||
               strcmp(varname, MIimagemax) == 0)
        {
        parent = MIimage;
        vartype = MI_VARATT;
        cdftype = NC_DOUBLE;
        vardims = this->MINCImageMinMaxDims;
        }

      // Check if this is the rootvariable
      if (strcmp(varname, MIrootvariable) == 0)
        {
        parent = MI_EMPTY_STRING;
        children = rootChildren.c_str();
        }

      // Create the NetCDF variable
      status = nc_def_var(ncid, varname, cdftype, vardims, dimids,
                          &varid);

      if (status != NC_NOERR)
        {
        vtkMINCImageWriterFailAndClose(ncid, status);
        this->MINCFileId = 0;
        return 0;
        }

      // Variables of known type get standard MINC attributes
      if (strcmp(vartype, MI_EMPTY_STRING) != 0)
        {
        vtkMINCImageWriterPutAttributeTextMacro(MIvarid,    MI_STDVAR);
        vtkMINCImageWriterPutAttributeTextMacro(MIversion,  MI_VERSION_1_0);
        vtkMINCImageWriterPutAttributeTextMacro(MIvartype,  vartype);
        }

      int dimIndex = 0;
      if (strcmp(vartype, MI_DIMENSION) == 0)
        {
        static const char *dimensionComments[] = {
          "X increases from patient left to right",
          "Y increases from patient posterior to anterior",
          "Z increases from patient inferior to superior",
          0
        };

        dimIndex = this->IndexFromDimensionName(varname);
        double start = 0.0;
        double step = 1.0;
        if (dimIndex >= 0 && dimIndex < 3)
          {
          vtkMINCImageWriterPutAttributeTextMacro(
            MIcomments, dimensionComments[dimIndex]);
          start = origin[dimIndex];
          step = spacing[dimIndex];
          if (this->Flip[dimIndex])
            {
            // Switch the MIstart to the other end and change sign
            double length = (wholeExtent[2*dimIndex+1] -
                             wholeExtent[2*dimIndex] + 1);
            start = -(start + step*(length-1));
            }
          }

        vtkMINCImageWriterPutAttributeDoubleMacro(MIstart, 1, &start);
        vtkMINCImageWriterPutAttributeDoubleMacro(MIstep,  1, &step);
        vtkMINCImageWriterPutAttributeTextMacro(MIspacing,   MI_REGULAR);
        vtkMINCImageWriterPutAttributeTextMacro(MIspacetype, MI_NATIVE);
        vtkMINCImageWriterPutAttributeTextMacro(MIalignment, MI_CENTRE);

        // Extra attributes for spatial dimensions
        if (dimIndex >= 0 && dimIndex < 3)
          {
          vtkMatrix4x4 *matrix = this->GetDirectionCosines();
          if (matrix)
            {
            double dircos[3];
            // need to take permutation into account here
            dircos[0] = matrix->GetElement(0, dimIndex);
            dircos[1] = matrix->GetElement(1, dimIndex);
            dircos[2] = matrix->GetElement(2, dimIndex);
            if (this->Flip[dimIndex])
              {
              // Flip the dimension direction
              for (int idx = 0; idx < 3; idx++)
                {
                if (dircos[idx] != 0)
                  {
                  dircos[idx] = -dircos[idx];
                  }
                }
              }
            vtkMINCImageWriterPutAttributeDoubleMacro(MIdirection_cosines,
                                                      3, dircos);
            }
          }
        }
      else if (strcmp(vartype, MI_VARATT) == 0)
        {
        vtkMINCImageWriterPutAttributeTextMacro(MIparent,  parent);
        if (children)
          {
          vtkMINCImageWriterPutAttributeTextMacro(MIchildren, children);
          }
        if (strcmp(varname, MIimagemin) == 0)
          {
          double val = 0.0;
          vtkMINCImageWriterPutAttributeDoubleMacro(MI_FillValue, 1, &val);
          }
        else if (strcmp(varname, MIimagemax) == 0)
          {
          double val = 1.0;
          vtkMINCImageWriterPutAttributeDoubleMacro(MI_FillValue, 1, &val);
          }
        }
      else if (strcmp(vartype, MI_GROUP) == 0)
        {
        vtkMINCImageWriterPutAttributeTextMacro(MIparent,   parent);
        if (children)
          {
          vtkMINCImageWriterPutAttributeTextMacro(MIchildren, children);
          }

        if (strcmp(varname, MIimage) == 0)
          {
          const char *signType = MI_SIGNED;
          if (this->MINCImageTypeSigned == 0)
            {
            signType = MI_UNSIGNED;
            }
          double *validRange = this->FileValidRange;

          vtkMINCImageWriterPutAttributeTextMacro(MIcomplete,  MI_TRUE);

          // Only produce signtype and valid_range for integer data
          if (this->MINCImageType != NC_FLOAT &&
              this->MINCImageType != NC_DOUBLE)
            {
            vtkMINCImageWriterPutAttributeTextMacro(MIsigntype,  signType);

            // Don't set valid_range if the default is suitable
            if (this->ComputeValidRangeFromScalarRange ||
                (this->ImageAttributes &&
                 vtkDoubleArray::SafeDownCast(
                   this->ImageAttributes->GetAttributeValueAsArray(
                     MIimage, MIvalid_range))))
              {
              vtkMINCImageWriterPutAttributeDoubleMacro(MIvalid_range,2,
                                                   validRange);
              }
            }

          // The image-min, image-max will not always be present
          if (this->InternalRescaleSlope != 0)
            {
            vtkMINCImageWriterPutAttributeTextMacro(
              MIimagemin, MI_VARATT_POINTER_PREFIX MIimagemin);
            vtkMINCImageWriterPutAttributeTextMacro(
              MIimagemax, MI_VARATT_POINTER_PREFIX MIimagemax);
            }
          }
        }
      }
    else
      {
      // Set the varid for global variables
      varid = -1;

      // Global attributes: ident and history
      std::string ident = vtkMINCImageWriterCreateIdentString();
      vtkMINCImageWriterPutAttributeTextMacro(MIident, ident.c_str());

      // For history, include any previous history
      std::string history = MI_EMPTY_STRING;
      const char *previousHistory = 0;
      if (this->ImageAttributes)
        {
        previousHistory = this->ImageAttributes->GetAttributeValueAsString(
          MI_EMPTY_STRING, MIhistory);
        if (previousHistory)
          {
          history.append(previousHistory);
          }
        }

      if (history.size() > 1 && history[history.size()-1] != '\n')
        {
        history.append("\n");
        }

      time_t t;
      time(&t);
      std::string timestamp = ctime(&t);
      history.append(timestamp.substr(0, timestamp.size()-1) + ">>>");
      if (this->HistoryAddition)
        {
        history = history + this->HistoryAddition + "\n";
        }
      else
        {
        history = history + "Created by " + this->GetClassName() + "\n";
        }
      vtkMINCImageWriterPutAttributeTextMacro(MIhistory, history.c_str());
      }

    // Write out user-defined attributes for this variable
    vtkStringArray *attArray = 0;
    if (this->ImageAttributes)
      {
      attArray = this->ImageAttributes->GetAttributeNames(varname);
      }
    if (attArray)
      {
      std::string varpath = MI_GRPNAME MI_GRP_SEP;
      int natts = attArray->GetNumberOfValues();
      for (int iatt = 0; iatt < natts; iatt++)
        {
        const char *attname = attArray->GetValue(iatt);
        vtkDataArray *array =
          this->ImageAttributes->GetAttributeValueAsArray(
            varname, attname);

        int result = this->ImageAttributes->ValidateAttribute(
          varname, attname, array);

        if (result == 0)
          {
          // A result of zero means that this attribute has already
          // been automatically generated, or is mis-formatted
          continue;
          }
        else if (result > 1 && this->StrictValidation)
          {
          vtkWarningMacro("Attribute " << varname << ":" << attname
                          << " is not recognized");
          }
        else if (strcmp(attname, MIdirection_cosines) == 0 &&
                 this->DirectionCosines)
          {
          // Let DirectionCosines override the attributes setting
          continue;
          }
        else
          {
          // Write out the attribute
          int dataType = array->GetDataType();
          size_t size = array->GetNumberOfTuples();
          switch (dataType)
            {
            case VTK_CHAR:
              status = nc_put_att_text(ncid, varid, attname, size,
                ((vtkCharArray *)array)->GetPointer(0));
              break;
            case VTK_INT:
              status = nc_put_att_int(ncid, varid, attname, NC_INT, size,
                ((vtkIntArray *)array)->GetPointer(0));
              break;
            case VTK_DOUBLE:
              status = nc_put_att_double(ncid, varid, attname, NC_DOUBLE,
                size, ((vtkDoubleArray *)array)->GetPointer(0));
              break;
            default:
              {
              vtkWarningMacro("Attribute " << varname << ":" << attname
                              << " has bad data type " << dataType << ".");
              }
              break;
            }
          if (status != NC_NOERR)
            {
            vtkMINCImageWriterFailAndClose(ncid, status);
            this->MINCFileId = 0;
            return 0;
            }
          }
        }
      }
    }

  return 1;
}

//-------------------------------------------------------------------------
int vtkMINCImageWriter::WriteMINCFileAttributes(
  vtkImageData* input, int numTimeSteps)
{
  // Get the image data type
  int imageDataType = input->GetScalarType();
  this->FileDataType = imageDataType;

  // Get the rescale parameters (check the ImageAttributes if
  // they are not set explicitly)
  this->FindRescale(this->InternalRescaleSlope,
                    this->InternalRescaleIntercept);

  // If the data type of the input is floating point, assume that
  // the floating-point values represent the real data values
  if (imageDataType == VTK_FLOAT || imageDataType == VTK_DOUBLE)
    {
    // If the data type of the input is floating point, but the original
    // data type stored in ImageAttributes was an integer type, then
    // we will rescale the floating-point values to integer.
    if (this->ImageAttributes &&
        this->ImageAttributes->GetDataType() != VTK_VOID &&
        this->ImageAttributes->GetDataType() != VTK_FLOAT &&
        this->ImageAttributes->GetDataType() != VTK_DOUBLE)
      {
      this->FileDataType = this->ImageAttributes->GetDataType();
      }

    // Unless RescaleSlope was explicitly set, use unitary rescaling
    if (this->RescaleSlope == 0)
      {
      this->InternalRescaleSlope = 1.0;
      this->InternalRescaleIntercept = 0.0;
      }
    }

  // Convert VTK type to MINC type
  this->MINCImageType = vtkMINCImageWriterConvertVTKTypeToMINCType(
    this->FileDataType, this->MINCImageTypeSigned);

  // If the file type is the same as the image type, write the
  // data to disk with no rescaling and set the valid range to
  // the scalar range.
  if (this->FileDataType == imageDataType)
    {
    this->ComputeValidRangeFromScalarRange = 1;
    }
  else
    {
    this->ComputeValidRangeFromScalarRange = 0;
    this->FindMINCValidRange(this->FileValidRange);
    }

  // Create a list of dimensions (don't include vector_dimension)
  int dimids[VTK_MINC_MAX_DIMS];
  if (this->CreateMINCDimensions(input, numTimeSteps, dimids) == 0)
    {
    return 0;
    }

  // Create a list of variables and their attributes
  if (this->CreateMINCVariables(input, numTimeSteps, dimids) == 0)
    {
    return 0;
    }

  // Write the header so that we can use it for debugging, then reopen
  nc_close(this->MINCFileId);
  nc_open(this->GetFileName(), NC_WRITE, &this->MINCFileId);

  return 1;
}

//-------------------------------------------------------------------------
void vtkMINCImageWriter::FindMINCValidRange(double range[2])
{
  // Find the valid range. Start with the default.
  range[0] = 0.0;
  range[1] = 1.0;

  if (this->MINCImageType != NC_FLOAT &&
      this->MINCImageType != NC_DOUBLE)
    {
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

  // Look for the valid_range attribute of the data.
  vtkDoubleArray *rangearray = 0;
  if (this->ImageAttributes)
    {
    rangearray = vtkDoubleArray::SafeDownCast(
      this->ImageAttributes->GetAttributeValueAsArray(
        MIimage, MIvalid_range));
    }
  if (rangearray &&
      this->MINCImageType != NC_FLOAT &&
      this->MINCImageType != NC_DOUBLE &&
      this->MINCImageType != NC_BYTE &&
      !(rangearray->GetValue(0) == 0.0 &&
        rangearray->GetValue(1) == 1.0))
    {
    double tryrange[2];
    tryrange[0] = rangearray->GetValue(0);
    tryrange[1] = rangearray->GetValue(1);

    // Check to see if we should set the lower range to zero
    if (range[0] < 0 && tryrange[0] == 0 &&
        (tryrange[1] - tryrange[0]) < (range[1] - range[0]))
      {
      range[0] = 0.0;
      }

    // Check to see if we should reduce the upper range
    if (range[1] > tryrange[1] && range[0] <= tryrange[0] &&
        ((this->MINCImageType == NC_SHORT &&
          (tryrange[1] - tryrange[0]) >= 1023) ||
         (this->MINCImageType == NC_INT &&
          (tryrange[1] - tryrange[0]) > 65535)))
      {
      range[0] = tryrange[0];
      range[1] = tryrange[1];
      }
    }
}

//-------------------------------------------------------------------------
void vtkMINCImageWriter::FindRescale(
  double &rescaleSlope, double &rescaleIntercept)
{
  // If this->RescaleSlope was set, use it
  if (this->RescaleSlope != 0)
    {
    rescaleSlope = this->RescaleSlope;
    rescaleIntercept = this->RescaleIntercept;
    return;
    }

  if (// data type check against float and double
      this->ImageAttributes &&
      this->ImageAttributes->GetImageMin() &&
      this->ImageAttributes->GetImageMax())
    {
    // Compute the rescale parameters from the image attributes,
    // i.e. use the same rescaling as the original file.
    double imageRange[2];
    double validRange[2];

    this->ImageAttributes->FindImageRange(imageRange);
    this->ImageAttributes->FindValidRange(validRange);

    rescaleSlope = ((imageRange[1] - imageRange[0])/
                    (validRange[1] - validRange[0]));

    rescaleIntercept = (imageRange[0] -
                        rescaleSlope*validRange[0]);

    return;
    }

  // If the header contains no rescale information,
  // we will not rescale the image.
  rescaleSlope = 0;
  rescaleIntercept = 0;
}

//-------------------------------------------------------------------------
// Data conversion functions.  The rounding is done using the same
// method as in the MINC libraries.
#define vtkMINCImageWriterConvertMacro(F, T, MIN, MAX) \
inline void vtkMINCImageWriterConvert(const F& inVal, T& outVal) \
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

#define vtkMINCImageWriterConvertMacroFloat(F, T) \
inline void vtkMINCImageWriterConvert(const F &inVal, T &outVal) \
{ \
  outVal = static_cast<T>(inVal); \
}

vtkMINCImageWriterConvertMacro(double, signed char,
                               VTK_SIGNED_CHAR_MIN, VTK_SIGNED_CHAR_MAX);
vtkMINCImageWriterConvertMacro(double, unsigned char,
                               0, VTK_UNSIGNED_CHAR_MAX);
vtkMINCImageWriterConvertMacro(double, short,
                               VTK_SHORT_MIN, VTK_SHORT_MAX);
vtkMINCImageWriterConvertMacro(double, unsigned short,
                               0, VTK_UNSIGNED_SHORT_MAX);
vtkMINCImageWriterConvertMacro(double, int,
                               VTK_INT_MIN, VTK_INT_MAX);
vtkMINCImageWriterConvertMacro(double, unsigned int,
                               0, VTK_UNSIGNED_INT_MAX);
vtkMINCImageWriterConvertMacroFloat(double, float);
vtkMINCImageWriterConvertMacroFloat(double, double);

//-------------------------------------------------------------------------
// Overloaded functions for writing various data types.

#define vtkMINCImageWriterWriteChunkMacro(ncFunction, T) \
inline int vtkMINCImageWriterWriteChunk( \
  int ncid, int varid, size_t *start, size_t *count, T *buffer) \
{ \
  return ncFunction(ncid, varid, start, count, buffer); \
}

#define vtkMINCImageWriterWriteChunkMacro2(ncFunction, T1, T2) \
inline int vtkMINCImageWriterWriteChunk( \
  int ncid, int varid, size_t *start, size_t *count, T1 *buffer) \
{ \
  return ncFunction(ncid, varid, start, count, (T2 *)buffer); \
}

vtkMINCImageWriterWriteChunkMacro(nc_put_vara_schar, signed char);
vtkMINCImageWriterWriteChunkMacro(nc_put_vara_uchar, unsigned char);
vtkMINCImageWriterWriteChunkMacro(nc_put_vara_short, short);
vtkMINCImageWriterWriteChunkMacro2(nc_put_vara_short, unsigned short, short);
vtkMINCImageWriterWriteChunkMacro(nc_put_vara_int, int);
vtkMINCImageWriterWriteChunkMacro2(nc_put_vara_int, unsigned int, int);
vtkMINCImageWriterWriteChunkMacro(nc_put_vara_float, float);
vtkMINCImageWriterWriteChunkMacro(nc_put_vara_double, double);

//-------------------------------------------------------------------------
template<class T1, class T2>
void vtkMINCImageWriterExecuteChunk(
  T1 *inPtr, T2 *buffer,
  double chunkRange[2], double validRange[2],
  int ncid, int varid, int ndims, size_t *start, size_t *count,
  vtkIdType *permutedInc, int rescale)
{
  // See if there is a range of dimensions over which the
  // the MINC data and VTK data will be contiguous.  The
  // lastdim is the dimension after which all dimensions
  // are contiguous between the MINC file and the output.
  int lastdim = ndims-1;
  int idim = 0;
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

    // Also need to break if the spatial dimension
    // corresponding to idim has flipped.

    dimprod *= count[idim];
    }

  T2 *outPtr = buffer;

  // Initialize min and max values.
  T1 minval = *inPtr;
  T1 maxval = *inPtr;

  // Initialize shift and scale values
  double shift = 0.0;
  double scale = 1.0;

  // Need to do everything from here down _twice_: stage 0 is to
  // calculate the range, and stage 1 is to rescale the values
  // and write them out to disk.
  for (int stage = 0; stage < 2; stage++)
    {
    // Create space to save values during the copy loop.
    T1 *tmpInPtr = inPtr;
    T1 *saveInPtr[VTK_MINC_MAX_DIMS];
    size_t index[VTK_MINC_MAX_DIMS];
    for (idim = 0; idim < ndims; idim++)
      {
      index[idim] = 0;
      saveInPtr[idim] = tmpInPtr;
      }

    // Save the count and permuted increment of this dimension.
    size_t lastdimcount = count[lastdim];
    size_t lastdimindex = 0;
    vtkIdType lastdimInc = permutedInc[lastdim];
    T1 *lastdimInPtr = saveInPtr[lastdim];

    // Loop over all contiguous sections of the image.
    for (;;)
      {
      // Loop through one contiguous section
      if (stage == 0)
        {
        vtkIdType k = ncontiguous;
        do
          {
          // Calculate the range of data values.
          T1 val = *tmpInPtr++;
          if (val < minval)
            {
            minval = val;
            }
          if (val > maxval)
            {
            maxval = val;
            }
          }
        while (--k);
        }
      else
        {
        vtkIdType k = ncontiguous;
        do
          {
          // Use special function for type conversion.
          vtkMINCImageWriterConvert(((*tmpInPtr++) + shift)*scale, *outPtr++);
          }
        while (--k);
        }

      lastdimindex++;
      lastdimInPtr += lastdimInc;
      tmpInPtr = lastdimInPtr;

      // Continue until done lastdim.
      if (lastdimindex < lastdimcount)
        {
        continue;
        }

      // Copy these values back into the arrays
      index[lastdim] = lastdimindex;
      count[lastdim] = lastdimcount;

      // Handle all dimensions that are lower than lastdim.  Go down
      // the dimensions one at a time until we find one for which
      // the index is still less than the count.
      idim = lastdim;
      do
        {
        // We're done if the lowest dim's index has reached its count.
        if (idim == 0)
          {
          break;
          }

        // Reset the index to zero if it previously reached its count.
        index[idim--] = 0;

        // Now increase the index for the next lower dimension;
        index[idim]++;
        saveInPtr[idim] += permutedInc[idim];

        // Continue the loop if this dim's index has reached its count.
        }
      while (index[idim] >= count[idim]);

      // Break again (corresponds to the break above)
      if (idim == 0 && index[idim] >= count[idim])
        {
        break;
        }

      // Increment back up to the lastdim, resetting the pointers.
      tmpInPtr = saveInPtr[idim];
      do
        {
        saveInPtr[++idim] = tmpInPtr;
        }
      while (idim < lastdim);

      lastdimInPtr = tmpInPtr;
      lastdimindex = 0;
      }

    // We get here once we're finished going through the data.
    if (stage == 0 && rescale != 0)
      {
      // Calculate rescaling for this chunk.
      scale = 1.0;
      shift = - static_cast<double>(minval);
      if (minval != maxval)
        {
        scale = ((validRange[1] - validRange[0])/
                 (maxval - minval));
        shift = validRange[0]/scale - minval;
        }
      }
    }

  // Write the chunk of data to the MINC file.
  vtkMINCImageWriterWriteChunk(ncid, varid, start, count, buffer);

  // Return the min and max for this chunk
  chunkRange[0] = minval;
  chunkRange[1] = maxval;
}

//-------------------------------------------------------------------------
// Our own template that only includes MINC data types.

#define vtkMINCImageWriterTemplateMacro(call) \
  case VTK_DOUBLE:         { typedef double VTK_TT; call; };         break; \
  case VTK_FLOAT:          { typedef float VTK_TT; call; };          break; \
  case VTK_INT:            { typedef int VTK_TT; call; };            break; \
  case VTK_UNSIGNED_INT:   { typedef unsigned int VTK_TT; call; };   break; \
  case VTK_SHORT:          { typedef short VTK_TT; call; };          break; \
  case VTK_UNSIGNED_SHORT: { typedef unsigned short VTK_TT; call; }; break; \
  case VTK_SIGNED_CHAR:    { typedef signed char VTK_TT; call; };    break; \
  case VTK_UNSIGNED_CHAR:  { typedef unsigned char VTK_TT; call; };  break

//-------------------------------------------------------------------------
// Write out the UpdateExtent of the input data.  Note that MINC has
// to calculate the scalar range of each slice before writing it,
// therefore the UpdateExtent must contain whole slices, otherwise
// the range won't be properly calculated.
int vtkMINCImageWriter::WriteMINCData(
  vtkImageData *data, int timeStep, int inWholeExt[6], int inExt[6])
{
  int scalarType = data->GetScalarType();
  int scalarSize = data->GetScalarSize();
  int numComponents = data->GetNumberOfScalarComponents();
  int numTimeSteps = this->GetNumberOfInputConnections(0);
  vtkIdType inInc[3];
  data->GetIncrements(inInc);

  void *inPtr = data->GetScalarPointerForExtent(inExt);

  int status = 0;
  int ncid = this->MINCFileId;
  int varid = 0;
  int minid = 0;
  int maxid = 0;

  // Whether to rescale the data
  int rescale = !this->ComputeValidRangeFromScalarRange;

  // Get the image variable.
  status = nc_inq_varid(ncid, MIimage, &varid);
  // Get the image-min variable.
  if (rescale)
    {
    if (status == NC_NOERR)
      {
      status = nc_inq_varid(ncid, MIimagemin, &minid);
      }
    // Get the image-max variable.
    if (status == NC_NOERR)
      {
      status = nc_inq_varid(ncid, MIimagemax, &maxid);
      }
    }

  if (status != NC_NOERR)
    {
    vtkMINCImageWriterFailAndClose(ncid, status);
    this->MINCFileId = 0;
    return 0;
    }

  // Get the rescaling parameters
  double rescaleSlope = this->InternalRescaleSlope;
  double rescaleIntercept = this->InternalRescaleIntercept;

  // Get the dimensions.
  int ndims = this->FileDimensionNames->GetNumberOfValues();
  int idim = 0;
  int nminmaxdims = this->MINCImageMinMaxDims;

  // All of these values will be changed in the following loop
  vtkIdType nchunks = 1;
  vtkIdType chunkSize = 1;
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

    const char *dimName = this->FileDimensionNames->GetValue(idim);

    // Find the VTK dimension index.
    int dimIndex = this->IndexFromDimensionName(dimName);

    if (dimIndex >= 0 && dimIndex < 3)
      {
      // Set length according to the whole extent
      length[idim] = inWholeExt[2*dimIndex+1] - inWholeExt[2*dimIndex] + 1;
      // Set start and count according to the update extent.
      start[idim] = inExt[2*dimIndex];
      count[idim] = inExt[2*dimIndex+1] - inExt[2*dimIndex] + 1;
      permutedInc[idim] = inInc[dimIndex];
      // If flipped, march in opposite direction
      if (this->Flip[dimIndex])
        {
        // Adjust the pointer to opposite end of dimension
        if (idim >= nminmaxdims)
          {
          inPtr = (void *)((char *)inPtr +
                           (inExt[2*dimIndex+1] - inExt[2*dimIndex])
                           *inInc[dimIndex]*scalarSize);
          }
        start[idim] = (length[idim] - 1 - (start[idim] + count[idim] - 1));
        permutedInc[idim] = -permutedInc[idim];
        }
      }
    else if (strcmp(dimName, MIvector_dimension) == 0)
      {
      // Vector dimension size is also stored in numComponents.
      length[idim] = numComponents;
      start[idim] = 0;
      count[idim] = numComponents;
      permutedInc[idim] = 1;
      }
    else
      {
      // Use TimeStepNumber to compute the index into the remaining dimension
      length[idim] = numTimeSteps;
      start[idim] = timeStep;
      count[idim] = 1;
      permutedInc[idim] = 0;
      }

    // Calculate the number of chunks to use
    if (idim < nminmaxdims)
      {
      // Number of chunks is product of dimensions in minmax.
      nchunks *= count[idim];

      // After each chunk, we will increment inPtr by chunkInc.
      if (chunkInc == 0)
        {
        chunkInc = permutedInc[idim];
        // If flipped, march in opposite direction
        if (dimIndex >= 0 && dimIndex < 3 && this->Flip[dimIndex])
          {
          inPtr = (void *)((char *)inPtr +
                           (inExt[2*dimIndex+1] - inExt[2*dimIndex])*
                           chunkInc*scalarSize);
          chunkInc = -chunkInc;
          }
        }
      }
    else
      {
      chunkSize *= count[idim];
      }
    }

  // Create a buffer for intermediate results.
  int fileType = this->FileDataType;
  void *buffer = 0;
  switch (fileType)
    {
    vtkMINCImageWriterTemplateMacro(buffer=(void *)(new VTK_TT[chunkSize]));
    }

  // Create arrays for image-min and image-max
  double *minPtr = 0;
  double *maxPtr = 0;
  if (rescale)
    {
    minPtr = new double[nchunks];
    maxPtr = new double[nchunks];
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
      if (permutedInc[idim] != 0)
        {
        minmaxIdx += (start2[idim] - start[idim])*minmaxInc;
        minmaxInc *= count[idim];
        }
      chunkProd *= count[idim];
      }

    // Space to store the computed min and max of each chunk.
    double chunkRange[2];
    double validRange[2];
    validRange[0] = this->FileValidRange[0];
    validRange[1] = this->FileValidRange[1];

    // Permute the data and write out the chunk.
    if (scalarType == fileType)
      {
      // Write without type conversion
      switch (scalarType)
        {
        vtkMINCImageWriterTemplateMacro(
          vtkMINCImageWriterExecuteChunk(
            (VTK_TT *)inPtr, (VTK_TT *)buffer, chunkRange, validRange,
            ncid, varid, ndims, start2, count2, permutedInc, rescale));
        }
      }
    else if (scalarType == VTK_FLOAT)
      {
      // Write with type conversion from float
      switch (fileType)
        {
        vtkMINCImageWriterTemplateMacro(
          vtkMINCImageWriterExecuteChunk(
            (float *)inPtr, (VTK_TT *)buffer, chunkRange, validRange,
            ncid, varid, ndims, start2, count2, permutedInc, rescale));
        }
      }
    else if (scalarType == VTK_DOUBLE)
      {
      // Write with type conversion from double
      switch (fileType)
        {
        vtkMINCImageWriterTemplateMacro(
          vtkMINCImageWriterExecuteChunk(
            (double *)inPtr, (VTK_TT *)buffer, chunkRange, validRange,
            ncid, varid, ndims, start2, count2, permutedInc, rescale));
        }
      }

    // Set the min and max values from the chunk
    if (rescale)
      {
      minPtr[minmaxIdx] = chunkRange[0]*rescaleSlope + rescaleIntercept;
      maxPtr[minmaxIdx] = chunkRange[1]*rescaleSlope + rescaleIntercept;
      }
    else
      {
      if (chunkRange[0] < this->FileValidRange[0])
        {
        this->FileValidRange[0] = chunkRange[0];
        }
      if (chunkRange[1] > this->FileValidRange[1])
        {
        this->FileValidRange[1] = chunkRange[1];
        }
      }

    // Increment the inPtr for the next chunk.
    inPtr = (void *)(((char *)inPtr) + chunkInc*scalarSize);
    }

  switch (fileType)
    {
    vtkMINCImageWriterTemplateMacro(delete [] ((VTK_TT *)buffer));
    }

  // Sync the data to disk.
  status = nc_sync(ncid);

  // The trick with image-min and image-max is that if these
  // values are scalar, they are not be written out here.
  // Instead, they are computed from the valid_range via
  // the InternalRescaleIntercept and InternalRescaleSlope and
  // written out after all the data has been written.
  if (rescale)
    {
    // Write out to the image-min and image-max variables
    if (status == NC_NOERR)
      {
      status = nc_put_vara_double(ncid, minid, start, count, minPtr);
      }
    if (status == NC_NOERR)
      {
      nc_put_vara_double(ncid, maxid, start, count, maxPtr);
      }

    delete [] minPtr;
    delete [] maxPtr;
    }

  if (status != NC_NOERR)
    {
    vtkMINCImageWriterFailAndClose(ncid, status);
    this->MINCFileId = 0;
    }

  return 1;
}

//--------------------------------------------------------------------------
void vtkMINCImageWriter::Write()
{
  if (this->GetFileName() == 0)
    {
    vtkErrorMacro("Write: You must supply a file name.");
    return;
    }

  // Get the first input and update its information.
  vtkImageData *input = this->GetImageDataInput(0);

  if (input == 0)
    {
    vtkErrorMacro("Write: No input supplied.");
    return;
    }

  vtkDemandDrivenPipeline::SafeDownCast(
    this->GetInputExecutive(0, 0))->UpdateInformation();

  // Update the rest.
  this->UpdateInformation();
  if (this->MismatchedInputs)
    {
    vtkErrorMacro("Write: the input connects are not compatible with each"
                  " other");
    return;
    }

  // Open the file
  if (this->OpenNetCDFFile(this->GetFileName(), this->MINCFileId) == 0)
    {
    return;
    }

  // Create the variables and write the attributes.
  if (this->WriteMINCFileAttributes(
        input, this->GetNumberOfInputConnections(0)) == 0)
    {
    if (this->MINCFileId != 0)
      {
      vtkMINCImageWriterFailAndClose(this->MINCFileId, NC_NOERR);
      }
    return;
    }

  // Get the whole extent of the input
  vtkStreamingDemandDrivenPipeline::GetWholeExtent(
    this->GetInputInformation(0, 0), this->DataUpdateExtent);

  // If the image and file data types are the same, then we
  // write the data out directly and set the ValidRange to
  // the actual scalar range of the data.
  if (this->ComputeValidRangeFromScalarRange)
    {
    this->FileValidRange[0] = VTK_DOUBLE_MAX;
    this->FileValidRange[1] = VTK_DOUBLE_MIN;
    }

  // Find the VTK dimension index for output slices.
  int tryfounds[3];
  tryfounds[0] = tryfounds[1] = tryfounds[2] = 0;
  int nfound = 0;
  int dimIndex = 0;
  // Go through dimensions until 2 spatial dimension are found
  int idim = this->FileDimensionNames->GetNumberOfValues();
  while (idim)
    {
    idim--;
    const char *dimName = this->FileDimensionNames->GetValue(idim);
    dimIndex = this->IndexFromDimensionName(dimName);
    if (dimIndex >= 0 && dimIndex < 3)
      {
      nfound++;
      tryfounds[dimIndex] = 1;
      }
    if (nfound == 2)
      {
      break;
      }
    }
  // The spatial dimension that was not among the first two is the
  // slice dimension
  for (dimIndex = 0; dimIndex < 2; dimIndex++)
    {
    if (tryfounds[dimIndex] == 0)
      {
      break;
      }
    }

  int firstSlice = this->DataUpdateExtent[2*dimIndex];
  int lastSlice = this->DataUpdateExtent[2*dimIndex+1];

  // Go through data slice-by-slice using file-order slices
  for (int slice = firstSlice; slice <= lastSlice; slice++)
    {
    // Set the DataUpdateExtent to the slice extent we want to write
    this->DataUpdateExtent[2*dimIndex] = slice;
    this->DataUpdateExtent[2*dimIndex+1] = slice;
    this->Modified();

    // Call Update to execute pipeline and write slice to disk.
    this->Update();

    // If MINCFileID was set to zero, an error occurred
    if (this->MINCFileId == 0)
      {
      break;
      }
    }

  if (this->MINCFileId != 0)
    {
    int ncid = this->MINCFileId;

    // If file type is the same as image type, write the
    // scalar range as the valid_range unless the data
    // is floating-point
    if (this->FileDataType != VTK_FLOAT &&
        this->FileDataType != VTK_DOUBLE &&
        this->ComputeValidRangeFromScalarRange)
      {
      // If we calculated the valid_range from the data, write it
      int varid = 0;
      int status = nc_inq_varid(ncid, MIimage, &varid);
      if (status == NC_NOERR)
        {
        status = nc_put_att_double(ncid, varid, MIvalid_range, NC_DOUBLE, 2,
                                   this->FileValidRange);
        }
      if (status != NC_NOERR)
        {
        vtkMINCImageWriterFailAndClose(ncid, status);
        this->MINCFileId = 0;
        }
      }
    if (this->InternalRescaleSlope != 0 &&
        this->ComputeValidRangeFromScalarRange)
      {
      // Write out scalar image-min and image-max values
      double imageMin =
        this->FileValidRange[0]*this->InternalRescaleSlope +
        this->InternalRescaleIntercept;
      double imageMax =
        this->FileValidRange[1]*this->InternalRescaleSlope +
        this->InternalRescaleIntercept;

      static size_t start[] = { 0 };
      static size_t count[] = { 1 };

      int minid = 0;
      int maxid = 0;
      int status = nc_inq_varid(ncid, MIimagemin, &minid);
      if (status == NC_NOERR)
        {
        status = nc_put_vara_double(ncid, minid, start, count, &imageMin);
        }
      if (status == NC_NOERR)
        {
        status = nc_inq_varid(ncid, MIimagemax, &maxid);
        }
      if (status == NC_NOERR)
        {
        nc_put_vara_double(ncid, maxid, start, count, &imageMax);
        }
      }
    }

  // Close the file
  if (this->MINCFileId != 0)
    {
    if (this->CloseNetCDFFile(this->MINCFileId) == 0)
      {
      // If it failed, there might be a disk write error
      vtkErrorMacro("Writing " << this->GetFileName() <<
                    ":\nMINC file failed to close, might be corrupt");
      }
    this->MINCFileId = 0;
    }
}

//----------------------------------------------------------------------------
int vtkMINCImageWriter::FillInputPortInformation(
  int port, vtkInformation *info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
    {
    return 0;
    }
  info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  return 1;
}

//---------------------------------------------------------------------------
int vtkMINCImageWriter::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *vtkNotUsed(outputVector))
{
  // Check to make sure that all input information agrees
  this->MismatchedInputs = 0;

  double spacing[3];
  double origin[3];
  int extent[6];
  int components = 0;
  int dataType = 0;

  // For each connection on port 0, check against the first connection
  for (int i = 0; i < this->GetNumberOfInputConnections(0); i++)
    {
    vtkInformation *inInfo = inputVector[0]->GetInformationObject(i);
    if (i == 0)
      {
      inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent);
      inInfo->Get(vtkDataObject::SPACING(), spacing);
      inInfo->Get(vtkDataObject::ORIGIN(), origin);
      components = inInfo->Get(vtkDataObject::FIELD_NUMBER_OF_COMPONENTS());
      dataType = inInfo->Get(vtkDataObject::FIELD_ARRAY_TYPE());
      continue;
      }

    if (memcmp(inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()),
               extent, sizeof(extent)) ||
        memcmp(inInfo->Get(vtkDataObject::SPACING()), spacing,
               sizeof(spacing)) ||
        memcmp(inInfo->Get(vtkDataObject::ORIGIN()), origin,
               sizeof(origin)) ||
        inInfo->Get(vtkDataObject::FIELD_NUMBER_OF_COMPONENTS())
          != components ||
        inInfo->Get(vtkDataObject::FIELD_ARRAY_TYPE()) != dataType)
      {
      this->MismatchedInputs = 1;
      return 0;
      }
    }

  return 1;
}

//--------------------------------------------------------------------------
int vtkMINCImageWriter::RequestUpdateExtent(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *vtkNotUsed(outputVector))
{
  // Set the UpdateExtent from the DataUpdateExtent for the current slice
  int n = inputVector[0]->GetNumberOfInformationObjects();
  for (int i = 0; i < n; i++)
    {
    vtkInformation *inInfo = inputVector[0]->GetInformationObject(i);
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
                this->DataUpdateExtent, 6);
    }

  return 1;
}

//--------------------------------------------------------------------------
int vtkMINCImageWriter::RequestData(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* vtkNotUsed(outputVector))
{
  // Go through the inputs and write the data for each
  int numTimeSteps = inputVector[0]->GetNumberOfInformationObjects();

  for (int timeStep = 0; timeStep < numTimeSteps; timeStep++)
    {
    vtkInformation *inInfo =
      inputVector[0]->GetInformationObject(timeStep);
    vtkImageData *input =
      vtkImageData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

    // Error checking
    if (input == NULL)
      {
      // Close file, set MINCFileID to zero
      this->CloseNetCDFFile(this->MINCFileId);
      this->MINCFileId = 0;
      vtkErrorMacro(<<"Write: Please specify an input!");
      return 0;
      }

    // Call WriteMINCData for each input
    if (this->WriteMINCData(
          input,
          timeStep,
          vtkStreamingDemandDrivenPipeline::GetWholeExtent(inInfo),
          vtkStreamingDemandDrivenPipeline::GetUpdateExtent(inInfo)) == 0)
      {
      return 0;
      }
    }

  return 1;
}
