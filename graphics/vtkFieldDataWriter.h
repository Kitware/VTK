/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFieldDataWriter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME vtkFieldDataWriter - write vtk field data
// .SECTION Description
// vtkFieldDataWriter is a source object that writes ASCII or binary 
// field data files in vtk format. Field data is a general form of data in
// matrix form.
// .SECTION Caveats
// Binary files written on one system may not be readable on other systems.
// .SECTION See Also
// vtkFieldData vtkFieldDataReader

#ifndef __vtkFieldDataWriter_h
#define __vtkFieldDataWriter_h

#include "vtkDataWriter.h"
#include "vtkFieldData.h"

class VTK_EXPORT vtkFieldDataWriter : public vtkWriter
{
public:
  // Description:
  // Instantiate object with no input.
  vtkFieldDataWriter();

  static vtkFieldDataWriter *New() {return new vtkFieldDataWriter;};
  const char *GetClassName() {return "vtkFieldDataWriter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the input data or filter.
  void SetInput(vtkDataObject *input);

  void SetInput(vtkDataObject &input) {this->SetInput(&input);};
  vtkDataObject *GetInput() {return this->Input;};
                               
  // Methods delegated to vtkDataWriter
  void SetFileName(char *filename) {this->Writer.SetFileName(filename);};
  char *GetFileName() {return this->Writer.GetFileName();};

  void SetHeader(char *header) {this->Writer.SetHeader(header);};
  char *GetHeader() {return this->Writer.GetHeader();};
  
  void SetFileType(int type) {this->Writer.SetFileType(type);};
  int GetFileType() {return this->Writer.GetFileType();};
  void SetFileTypeToASCII() {this->Writer.SetFileType(VTK_ASCII);};
  void SetFileTypeToBinary() {this->Writer.SetFileType(VTK_BINARY);};

  void SetFieldDataName(char *fieldname) {this->Writer.SetFieldDataName(fieldname);};
  char *GetFieldDataName() {return this->Writer.GetFieldDataName();};

protected:
  void WriteData();
  vtkDataWriter Writer;
  
};

#endif


