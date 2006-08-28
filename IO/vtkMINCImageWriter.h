/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMINCImageWriter.h

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
// .NAME vtkMINCImageWriter - A writer for MINC files.
// .SECTION Description
// MINC is a medical image file format that was developed at the Montreal
// Neurological Institute in 1992. It is based on the NetCDF format.
// The data is written slice-by-slice, and this writer is therefore
// suitable for streaming MINC data that is larger than the memory
// size through VTK.  This writer can also produce files with up to
// 4 dimensions, where the fourth dimension is provided by using
// AddInput() to specify multiple input data sets.
// .SECTION Thanks
// Thanks to Atamai Inc. for contributing this class to VTK.  Written
// by David Gobbi. 

#ifndef __vtkMINCImageWriter_h
#define __vtkMINCImageWriter_h

#include "vtkImageWriter.h"

class vtkStringArray;
class vtkIdTypeArray;
class vtkDoubleArray;
class vtkMatrix4x4;

// A special class that holds the attributes
class vtkMINCImageWriterAttributeMap;

class VTK_IO_EXPORT vtkMINCImageWriter : public vtkImageWriter
{
public:
  vtkTypeRevisionMacro(vtkMINCImageWriter,vtkImageWriter);

  static vtkMINCImageWriter *New();
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the entension for this file format. 
  virtual const char* GetFileExtensions() {
    return ".mnc"; }

  // Description:
  // Get the name of this file format.
  virtual const char* GetDescriptiveName() {
    return "MINC"; }

  // Description:
  // Set the file name.
  virtual void SetFileName(const char *name);

  // Description:
  // Write the data.  This will attempt to stream the data
  // slice-by-slice through the pipeline and out to the file,
  // unless the whole extent of the input has already been
  // updated.
  virtual void Write();

  // Description:
  // Set a matrix that describes the orientation of the data.  The
  // three columns of this matrix should give the unit-vector
  // directions for the VTK x, y and z dimensions respectively.  
  // The writer will use this information to determine how to map
  // the VTK dimensions to the canonical MINC dimensions, and if
  // necessary, the writer will re-order one or more dimensions
  // back-to-front to ensure that no MINC dimension ends up with
  // a direction cosines vector whose dot product with the canonical
  // unit vector for that dimension is negative.
  virtual void SetOrientationMatrix(vtkMatrix4x4 *matrix);
  vtkGetObjectMacro(OrientationMatrix, vtkMatrix4x4);

  // Description:
  // Set the slope and intercept for rescaling the intensities.  The
  // default values are zero, which indicates to the reader that no
  // rescaling is to be performed.
  vtkSetMacro(RescaleSlope, double);
  vtkGetMacro(RescaleSlope, double);
  vtkSetMacro(RescaleIntercept, double);
  vtkGetMacro(RescaleIntercept, double);

  // Description:
  // Set the valid_range to use for the data.  When the data is
  // written to disk, if the data is rescaled, it will be rescaled
  // to this range.  If you set a RescaleSlope by do not set
  // the ValidRange, then the full scalar range of the data type
  // will be used.  If you neither set a RescaleSlope or the
  // ValidRange, then the writer will automatically set the
  // valid_range to the scalar range of the data set.
  vtkSetVector2Macro(ValidRange, double);
  vtkGetVector2Macro(ValidRange, double);

  // Description:
  // Set the names of up to five dimensions. The ordering of these
  // dimensions will determine the dimension order of the file.  If
  // no DimensionNames are set, the writer will set the dimension
  // order of the file to be the same as the dimension order in memory.
  virtual void SetDimensionNames(vtkStringArray *);
  virtual vtkStringArray *GetDimensionNames() {
    return this->DimensionNames; };

  // Description:
  // Set attribute values for a variable as a vtkDataArray.
  // Set the variable to the empty string to set global attributes.
  // If StrictValidation is set, then you may only set valid minc
  // attributes for valid minc variables,
  virtual void SetAttributeValueAsArray(const char *variable,
                                        const char *attribute,
                                        vtkDataArray *array);

  // Description:
  // Set an attribute value as a string.  Set the variable
  // to the empty string to set global attributes.
  // If you specify a variable that does not exist, it will be
  // created.
  virtual void SetAttributeValueAsString(const char *variable,
                                         const char *attribute,
                                         const char *value);

  // Description:
  // Set an attribute value as an int. Set the variable
  // to the empty string to set global attributes.
  // If you specify a variable that does not exist, it will be
  // created.
  virtual void SetAttributeValueAsInt(const char *variable,
                                      const char *attribute,
                                      int value);

  // Description:
  // Set an attribute value as a double.  Set the variable
  // to the empty string to set global attributes.
  // If you specify a variable that does not exist, it will be
  // created.
  virtual void SetAttributeValueAsDouble(const char *variable,
                                         const char *attribute,
                                         double value);

  // Description:
  // Set whether to validate that all variable attributes that
  // have been set are ones that are listed in the MINC standard.
  vtkSetMacro(StrictValidation, int);
  vtkBooleanMacro(StrictValidation, int);
  vtkGetMacro(StrictValidation, int);

protected:
  vtkMINCImageWriter();
  ~vtkMINCImageWriter();  

  int MINCImageType;
  int MINCImageTypeSigned;
  double MINCValidRange[2];
  int MINCImageMinMaxDims;
  
  vtkMatrix4x4 *OrientationMatrix;
  double RescaleSlope;
  double RescaleIntercept;
  double ValidRange[2];
  int StrictValidation;
  int DataUpdateExtent[6];

  vtkStringArray *DimensionNames;
  vtkStringArray *InternalDimensionNames;
  vtkStringArray *VariableNames;
  vtkMINCImageWriterAttributeMap *AttributeNames;
  vtkMINCImageWriterAttributeMap *AttributeValues;

  int Permutation[3];
  int Flip[3];

  int MismatchedInputs;
  int MINCFileId;

  virtual int OpenNetCDFFile(const char *filename, int& ncid);
  virtual int CloseNetCDFFile(int ncid);

  virtual int VerifyGlobalAttribute(const char *attrib,
                                    vtkDataArray *array);
  virtual int VerifyGeneralAttribute(const char *varname,
                                     const char *attname,
                                     vtkDataArray *array);
  virtual int VerifyDimensionAttribute(const char *varname,
                                       const char *attname,
                                       vtkDataArray *array);
  virtual int VerifyImageAttribute(const char *varname,
                                   const char *attname,
                                   vtkDataArray *array);
  virtual int VerifyImageMinMaxAttribute(const char *varname,
                                         const char *attname,
                                         vtkDataArray *array);
  virtual int VerifyPatientAttribute(const char *varname,
                                     const char *attname,
                                     vtkDataArray *array);
  virtual int VerifyStudyAttribute(const char *varname,
                                   const char *attname,
                                   vtkDataArray *array);
  virtual int VerifyAcquisitionAttribute(const char *varname,
                                         const char *attname,
                                         vtkDataArray *array);

  virtual void ComputePermutationFromOrientation(int permutation[3],
                                                 int flip[3]);
  virtual int IndexFromDimensionName(const char *dimName);
  virtual void FindMINCValidRange(double range[2]);

  virtual int CreateMINCDimensions(int wholeExtent[6], int numComponents,
                                   int numFrames, int *dimids);
  virtual int CreateMINCVariables(int wholeExtent[6], int numComponents,
                                  double origin[3], double spacing[3],
                                  int *dimids);

  virtual int WriteMINCFileAttributes(vtkImageData *input, int numFrames);
  virtual int WriteMINCData(vtkImageData *input, int frameNumber);

  virtual int RequestInformation(vtkInformation *request,
                                 vtkInformationVector **inputVector,
                                 vtkInformationVector *outputVector);

  virtual int RequestUpdateExtent(vtkInformation *request,
                                  vtkInformationVector **inputVector,
                                  vtkInformationVector *outputVector);

  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

private:
  vtkMINCImageWriter(const vtkMINCImageWriter&); // Not implemented
  void operator=(const vtkMINCImageWriter&);  // Not implemented

};

#endif
