/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMNITagPointReader.h

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
// .NAME vtkMNITagPointReader - A reader for MNI tag files.
// .SECTION Description
// The MNI .tag file format is used to store labeled points, it can
// store either one or two point sets.  All point sets must have the
// same number of points and they will share the same labels.  This
// file format was developed at the McConnell Brain Imaging Centre at
// the Montreal Neurological Institute and is used by their software.
// The labels are stored as a vtkStringArray in the PointData of the
// output dataset, which is a vtkPolyData.
// .SECTION See Also
// vtkMINCImageReader vtkMNIObjectReader vtkMNITransformReader
// .SECTION Thanks
// Thanks to David Gobbi for contributing this class.

#ifndef __vtkMNITagPointReader_h
#define __vtkMNITagPointReader_h

#include "vtkPolyDataAlgorithm.h"
#include "vtkStdString.h" // needed for std::string

class vtkPolyData;
class vtkPoints;
class vtkStringArray;
class vtkDoubleArray;
class vtkIntArray;

class VTK_HYBRID_EXPORT vtkMNITagPointReader : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkMNITagPointReader,vtkPolyDataAlgorithm);

  static vtkMNITagPointReader *New();
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the file name.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Get the entension for this file format.
  virtual const char* GetFileExtensions() {
    return ".tag"; }

  // Description:
  // Get the name of this file format.
  virtual const char* GetDescriptiveName() {
    return "MNI tags"; }

  // Description:
  // Test whether the specified file can be read.
  virtual int CanReadFile(const char* name);

  // Description:
  // Get the number of volumes specified by the file, which will be
  // equal to one or two.  There will be an output point set for each
  // volume, so really, this parameter just tells you the number of
  // outputs to expect from this reader.
  virtual int GetNumberOfVolumes();

  // Description:
  // Get the points.  These are also provided in the first and
  // second output ports of the reader.  This method will return
  // NULL if there is no data.
  virtual vtkPoints *GetPoints(int port);
  virtual vtkPoints *GetPoints() { return this->GetPoints(0); }

  // Description:
  // Get the labels.  These same labels are provided in the output
  // point sets, as the PointData data array named "LabelText".
  // This will return NULL if there were no labels in the file.
  virtual vtkStringArray *GetLabelText();

  // Description:
  // Get the weights.  These are also provided in the output
  // point sets, as the PointData data array named "Weights".
  // This will return NULL if there were no weights in the file.
  virtual vtkDoubleArray *GetWeights();

  // Description:
  // Get the structure ids.  These are also provided in the output
  // point sets, as the PointData data array named "StructureIds".
  // This will return NULL if there were no ids in the file.
  virtual vtkIntArray *GetStructureIds();

  // Description:
  // Get the patient ids.  These are also provided in the output
  // point sets, as the PointData data array named "PatientIds".
  // This will return NULL if there were no ids in the file.
  virtual vtkIntArray *GetPatientIds();

  // Description:
  // Get any comments that are included in the file.
  virtual const char *GetComments();

protected:
  vtkMNITagPointReader();
  ~vtkMNITagPointReader();

  char *FileName;
  int NumberOfVolumes;

  int LineNumber;
  char *Comments;

  int ReadLine(istream &infile, std::string &linetext,
               std::string::iterator &pos);
  int ReadLineAfterComments(istream &infile, std::string &linetext,
                            std::string::iterator &pos);
  int SkipWhitespace(istream &infile,  std::string &linetext,
                     std::string::iterator &pos, int nl);
  int ParseLeftHandSide(istream &infile, std::string &linetext,
                        std::string::iterator &pos,
                        std::string &identifier);
  int ParseStringValue(istream &infile, std::string &linetext,
                       std::string::iterator &pos,
                       std::string &data);
  int ParseIntValues(istream &infile, std::string &linetext,
                     std::string::iterator &pos,
                     int *values, int count);
  int ParseFloatValues(istream &infile, std::string &linetext,
                       std::string::iterator &pos,
                       double *values, int count);

  virtual int ReadFile(vtkPolyData *output1, vtkPolyData *output2);

  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inInfo,
                          vtkInformationVector* outInfo);

private:
  vtkMNITagPointReader(const vtkMNITagPointReader&); // Not implemented
  void operator=(const vtkMNITagPointReader&);  // Not implemented

};

#endif
