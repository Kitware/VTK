/*=========================================================================
  
  Program:   Visualization Toolkit
  Module:    vtkImageBlockWriter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  
Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkImageBlockWriter - Breaks up image into blocks and save in files.
// .SECTION Description
// Experimenting with different file formats. This one saves an image in 
// multiple files.  I am allowing overlap between file for efficiency.

// .SECTION see also
// vtkImageBlockReader.

#ifndef __vtkImageBlockWriter_h
#define __vtkImageBlockWriter_h

#include "vtkProcessObject.h"
#include "vtkImageData.h"

class VTK_PARALLEL_EXPORT vtkImageBlockWriter : public vtkProcessObject
{
public:
  static vtkImageBlockWriter *New();
  vtkTypeMacro(vtkImageBlockWriter,vtkProcessObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The whole extent is broken up into this many divisions along each axis.
  vtkSetVector3Macro(Divisions, int);
  vtkGetVector3Macro(Divisions, int);

  // Description:
  // The number of points along any axis that belong to more than one piece.
  vtkSetMacro(Overlap, int);
  vtkGetMacro(Overlap, int);

  // Description:
  // This writer takes images as input.
  void SetInput(vtkImageData *input);
  vtkImageData *GetInput();
  
  // Description:
  // This printf pattern should take three integers, one for each axis.
  vtkSetStringMacro(FilePattern);
  vtkGetStringMacro(FilePattern);

  // Description:
  // Write the files.
  void Write();

  
protected:
  vtkImageBlockWriter();
  ~vtkImageBlockWriter();
  
  char *FilePattern;

  int Divisions[3];
  int Overlap;
private:
  vtkImageBlockWriter(const vtkImageBlockWriter&);  // Not implemented.
  void operator=(const vtkImageBlockWriter&);  // Not implemented.
};


#endif


