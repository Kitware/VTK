/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBYUWriter.h
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
// .NAME vtkBYUWriter - write MOVIE.BYU files
// .SECTION Description
// vtkBYUWriter writes MOVIE.BYU polygonal files. These files consist 
// of a geometry file (.g), a scalar file (.s), a displacement or 
// vector file (.d), and a 2D texture coordinate file (.t). These files 
// must be specified to the object, the appropriate boolean 
// variables must be true, and data must be available from the input
// for the files to be written.

#ifndef __vtkBYUWriter_h
#define __vtkBYUWriter_h

#include <stdio.h>
#include "vtkPolyWriter.h"
#include "vtkPolyData.h"

class VTK_EXPORT vtkBYUWriter : public vtkPolyWriter
{
public:
  vtkBYUWriter();
  ~vtkBYUWriter();
  vtkBYUWriter *New() {return new vtkBYUWriter;};
  char *GetClassName() {return "vtkBYUWriter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the name of the geometry file to write.
  vtkSetStringMacro(GeometryFilename);
  vtkGetStringMacro(GeometryFilename);
  void SetGeometryFileName(char *str){this->SetGeometryFilename(str);}
  char *GetGeometryFileName(){return this->GetGeometryFilename();}

  // Description:
  // Specify the name of the displacement file to write.
  vtkSetStringMacro(DisplacementFilename);
  vtkGetStringMacro(DisplacementFilename);
  void SetDisplacementFileName(char *str){this->SetDisplacementFilename(str);}
  char *GetDisplacementFileName(){return this->GetDisplacementFilename();}

  // Description:
  // Specify the name of the scalar file to write.
  vtkSetStringMacro(ScalarFilename);
  vtkGetStringMacro(ScalarFilename);
  void SetScalarFileName(char *str){this->SetScalarFilename(str);}
  char *GetScalarFileName(){return this->GetScalarFilename();}

  // Description:
  // Specify the name of the texture file to write.
  vtkSetStringMacro(TextureFilename);
  vtkGetStringMacro(TextureFilename);
  void SetTextureFileName(char *str){this->SetTextureFilename(str);}
  char *GetTextureFileName(){return this->GetTextureFilename();}

  // Description:
  // Turn on/off writing the displacement file.
  vtkSetMacro(WriteDisplacement,int);
  vtkGetMacro(WriteDisplacement,int);
  vtkBooleanMacro(WriteDisplacement,int);
  
  // Description:
  // Turn on/off writing the scalar file.
  vtkSetMacro(WriteScalar,int);
  vtkGetMacro(WriteScalar,int);
  vtkBooleanMacro(WriteScalar,int);
  
  // Description:
  // Turn on/off writing the texture file.
  vtkSetMacro(WriteTexture,int);
  vtkGetMacro(WriteTexture,int);
  vtkBooleanMacro(WriteTexture,int);

protected:
  void WriteData();

  char *GeometryFilename;
  char *DisplacementFilename;
  char *ScalarFilename;
  char *TextureFilename;
  int WriteDisplacement;
  int WriteScalar;
  int WriteTexture;

  void WriteGeometryFile(FILE *fp, int numPts);
  void WriteDisplacementFile(int numPts);
  void WriteScalarFile(int numPts);
  void WriteTextureFile(int numPts);
};

#endif

