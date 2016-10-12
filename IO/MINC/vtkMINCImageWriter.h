/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMINCImageWriter.h

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
/**
 * @class   vtkMINCImageWriter
 * @brief   A writer for MINC files.
 *
 * MINC is a NetCDF-based medical image file format that was developed
 * at the Montreal Neurological Institute in 1992.
 * The data is written slice-by-slice, and this writer is therefore
 * suitable for streaming MINC data that is larger than the memory
 * size through VTK.  This writer can also produce files with up to
 * 4 dimensions, where the fourth dimension is provided by using
 * AddInput() to specify multiple input data sets.  If you want to
 * set header information for the file, you must supply a
 * vtkMINCImageAttributes
 * @sa
 * vtkMINCImageReader vtkMINCImageAttributes
 * @par Thanks:
 * Thanks to David Gobbi for writing this class and Atamai Inc. for
 * contributing it to VTK.
*/

#ifndef vtkMINCImageWriter_h
#define vtkMINCImageWriter_h

#include "vtkIOMINCModule.h" // For export macro
#include "vtkImageWriter.h"

class vtkStringArray;
class vtkIdTypeArray;
class vtkDoubleArray;
class vtkMatrix4x4;

// A special class that holds the attributes
class vtkMINCImageAttributes;

class VTKIOMINC_EXPORT vtkMINCImageWriter : public vtkImageWriter
{
public:
  vtkTypeMacro(vtkMINCImageWriter,vtkImageWriter);

  static vtkMINCImageWriter *New();
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Get the entension for this file format.
   */
  virtual const char* GetFileExtensions() {
    return ".mnc"; }

  /**
   * Get the name of this file format.
   */
  virtual const char* GetDescriptiveName() {
    return "MINC"; }

  /**
   * Set the file name.
   */
  virtual void SetFileName(const char *name);

  /**
   * Write the data.  This will attempt to stream the data
   * slice-by-slice through the pipeline and out to the file,
   * unless the whole extent of the input has already been
   * updated.
   */
  virtual void Write();

  //@{
  /**
   * Set a matrix that describes the orientation of the data.  The
   * three columns of this matrix should give the unit-vector
   * directions for the VTK x, y and z dimensions respectively.
   * The writer will use this information to determine how to map
   * the VTK dimensions to the canonical MINC dimensions, and if
   * necessary, the writer will re-order one or more dimensions
   * back-to-front to ensure that no MINC dimension ends up with
   * a direction cosines vector whose dot product with the canonical
   * unit vector for that dimension is negative.
   */
  virtual void SetDirectionCosines(vtkMatrix4x4 *matrix);
  vtkGetObjectMacro(DirectionCosines, vtkMatrix4x4);
  //@}

  //@{
  /**
   * Set the slope and intercept for rescaling the intensities.  The
   * default values are zero, which indicates to the reader that no
   * rescaling is to be performed.
   */
  vtkSetMacro(RescaleSlope, double);
  vtkGetMacro(RescaleSlope, double);
  vtkSetMacro(RescaleIntercept, double);
  vtkGetMacro(RescaleIntercept, double);
  //@}

  //@{
  /**
   * Set the image attributes, which contain patient information and
   * other useful metadata.
   */
  virtual void SetImageAttributes(vtkMINCImageAttributes *attributes);
  virtual vtkMINCImageAttributes *GetImageAttributes() {
    return this->ImageAttributes; };
  //@}

  //@{
  /**
   * Set whether to validate that all variable attributes that
   * have been set are ones that are listed in the MINC standard.
   */
  vtkSetMacro(StrictValidation, int);
  vtkBooleanMacro(StrictValidation, int);
  vtkGetMacro(StrictValidation, int);
  //@}

  //@{
  /**
   * Set a string value to append to the history of the file.  This
   * string should describe, briefly, how the file was processed.
   */
  vtkSetStringMacro(HistoryAddition);
  vtkGetStringMacro(HistoryAddition);
  //@}

protected:
  vtkMINCImageWriter();
  ~vtkMINCImageWriter();

  int MINCImageType;
  int MINCImageTypeSigned;
  int MINCImageMinMaxDims;

  vtkMatrix4x4 *DirectionCosines;
  double RescaleSlope;
  double RescaleIntercept;
  int StrictValidation;
  int DataUpdateExtent[6];

  int FileDataType;
  double FileValidRange[2];
  double InternalRescaleSlope;
  double InternalRescaleIntercept;

  int ComputeValidRangeFromScalarRange;

  vtkStringArray *FileDimensionNames;

  vtkMINCImageAttributes *ImageAttributes;

  int Permutation[3];
  int Flip[3];

  int MismatchedInputs;
  int MINCFileId;

  char *HistoryAddition;

  virtual int OpenNetCDFFile(const char *filename, int& ncid);
  virtual int CloseNetCDFFile(int ncid);

  virtual int IndexFromDimensionName(const char *dimName);
  virtual void ComputePermutationFromOrientation(int permutation[3],
                                                 int flip[3]);
  virtual int CreateMINCDimensions(vtkImageData *input, int timeStep,
                                   int *dimids);
  virtual int CreateMINCVariables(vtkImageData *input, int timeStep,
                                  int *dimids);
  virtual int WriteMINCFileAttributes(vtkImageData *input, int timeStep);
  virtual int WriteMINCData(vtkImageData *input,
                            int frameNumber,
                            int wholeExt[6],
                            int upExt[6]);
  virtual void FindRescale(double &rescaleSlope, double &rescaleIntercept);
  virtual void FindMINCValidRange(double range[2]);

  virtual int FillInputPortInformation(int port, vtkInformation *info);

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
  vtkMINCImageWriter(const vtkMINCImageWriter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkMINCImageWriter&) VTK_DELETE_FUNCTION;

};

#endif
