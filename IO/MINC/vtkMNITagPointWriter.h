/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMNITagPointWriter.h

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
// .NAME vtkMNITagPointWriter - A writer for MNI tag point files.
// .SECTION Description
// The MNI .tag file format is used to store tag points, for use in
// either registration or labelling of data volumes.  This file
// format was developed at the McConnell Brain Imaging Centre at
// the Montreal Neurological Institute and is used by their software.
// Tag points can be stored for either one volume or two volumes,
// and this filter can take one or two inputs.  Alternatively, the
// points to be written can be specified by calling SetPoints().
// .SECTION See Also
// vtkMINCImageReader vtkMNIObjectReader vtkMNITransformReader
// .SECTION Thanks
// Thanks to David Gobbi for contributing this class to VTK.

#ifndef vtkMNITagPointWriter_h
#define vtkMNITagPointWriter_h

#include "vtkIOMINCModule.h" // For export macro
#include "vtkWriter.h"

class vtkDataSet;
class vtkPointSet;
class vtkStringArray;
class vtkDoubleArray;
class vtkIntArray;
class vtkPoints;

class VTKIOMINC_EXPORT vtkMNITagPointWriter : public vtkWriter
{
public:
  vtkTypeMacro(vtkMNITagPointWriter,vtkWriter);

  static vtkMNITagPointWriter *New();
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the entension for this file format.
  virtual const char* GetFileExtensions() {
    return ".tag"; }

  // Description:
  // Get the name of this file format.
  virtual const char* GetDescriptiveName() {
    return "MNI tags"; }

  // Description:
  // Set the points (unless you set them as inputs).
  virtual void SetPoints(int port, vtkPoints *points);
  virtual void SetPoints(vtkPoints *points) {
    this->SetPoints(0, points); }
  virtual vtkPoints *GetPoints(int port);
  virtual vtkPoints *GetPoints() {
    return this->GetPoints(0); }

  // Description:
  // Set the labels (unless the input PointData has an
  // array called LabelText). Labels are optional.
  virtual void SetLabelText(vtkStringArray *a);
  vtkGetObjectMacro(LabelText, vtkStringArray);

  // Description:
  // Set the weights (unless the input PointData has an
  // array called Weights).  Weights are optional.
  virtual void SetWeights(vtkDoubleArray *a);
  vtkGetObjectMacro(Weights, vtkDoubleArray);

  // Description:
  // Set the structure ids (unless the input PointData has
  // an array called StructureIds).  These are optional.
  virtual void SetStructureIds(vtkIntArray *a);
  vtkGetObjectMacro(StructureIds, vtkIntArray);

  // Description:
  // Set the structure ids (unless the input PointData has
  // an array called PatientIds).  These are optional.
  virtual void SetPatientIds(vtkIntArray *a);
  vtkGetObjectMacro(PatientIds, vtkIntArray);

  // Description:
  // Set comments to be added to the file.
  vtkSetStringMacro(Comments);
  vtkGetStringMacro(Comments);

  // Description:
  // Write the file.
  virtual int Write();

  // Description:
  // Get the MTime.
  virtual unsigned long GetMTime();

  // Description:
  // Specify file name of vtk polygon data file to write.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

protected:
  vtkMNITagPointWriter();
  ~vtkMNITagPointWriter();

  vtkPoints *Points[2];
  vtkStringArray *LabelText;
  vtkDoubleArray *Weights;
  vtkIntArray *StructureIds;
  vtkIntArray *PatientIds;
  char *Comments;

  virtual void WriteData() {}
  virtual void WriteData(vtkPointSet *inputs[2]);

  int FillInputPortInformation(int port, vtkInformation *info);

  int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *);

  char* FileName;

  int FileType;

  ostream *OpenFile();
  void CloseFile(ostream *fp);

private:
  vtkMNITagPointWriter(const vtkMNITagPointWriter&); // Not implemented
  void operator=(const vtkMNITagPointWriter&);  // Not implemented

};

#endif
