/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredGridReader.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkStructuredGridReader - read vtk structured grid data file
// .SECTION Description
// vtkStructuredGridReader is a source object that reads ASCII or binary 
// structured grid data files in vtk format. See text for format details.
// .SECTION Caveats
// Binary files written on one system may not be readable on other systems.

#ifndef __vtkStructuredGridReader_h
#define __vtkStructuredGridReader_h

#include "vtkStructuredGridSource.h"
#include "vtkDataReader.h"

class VTK_EXPORT vtkStructuredGridReader : public vtkStructuredGridSource
{
public:
  vtkStructuredGridReader();
  vtkStructuredGridReader *New() {return new vtkStructuredGridReader;};
  char *GetClassName() {return "vtkStructuredGridReader";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // overload because of vtkDataReader ivar
  unsigned long int GetMTime();

  void SetFilename(char *name);
  char *GetFilename();
  void SetFileName(char *str){this->SetFilename(str);}
  char *GetFileName(){return this->GetFilename();}

  // Description:
  // Specify the InputString for use when reading from a character array.
  void SetInputString(char *in) {this->Reader.SetInputString(in);};
  void SetInputString(char *in,int len) {this->Reader.SetInputString(in,len);};
  char *GetInputString() { return this->Reader.GetInputString();};

  // Description:
  // Set/Get reading from an InputString instead of the default, a file.
  void SetReadFromInputString(int i) {this->Reader.SetReadFromInputString(i);};
  int GetReadFromInputString() {return this->Reader.GetReadFromInputString();};
  vtkBooleanMacro(ReadFromInputString,int);

  int GetFileType();

  void SetScalarsName(char *name);
  char *GetScalarsName();

  void SetVectorsName(char *name);
  char *GetVectorsName();

  void SetTensorsName(char *name);
  char *GetTensorsName();

  void SetNormalsName(char *name);
  char *GetNormalsName();

  void SetTCoordsName(char *name);
  char *GetTCoordsName();

  void SetLookupTableName(char *name);
  char *GetLookupTableName();

protected:
  void Execute();
  vtkDataReader Reader;

};

#endif


