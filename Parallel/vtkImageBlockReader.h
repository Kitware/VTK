/*=========================================================================
  
  Program:   Visualization Toolkit
  Module:    vtkImageBlockReader.h
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
// .NAME vtkImageBlockReader - Breaks up image into blocks and save in files.
// .SECTION Description
// Experimenting with different file formats. This one saves an image in 
// multiple files.  I am allowing overlap between file for efficiency.

// .SECTION see also
// vtkImageBlockReader.

#ifndef __vtkImageBlockReader_h
#define __vtkImageBlockReader_h

#include "vtkImageSource.h"


class VTK_PARALLEL_EXPORT vtkImageBlockReader : public vtkImageSource
{
public:
  static vtkImageBlockReader *New();
  vtkTypeMacro(vtkImageBlockReader,vtkImageSource);
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
  // Although this information could be gotten from the files, this is easy.
  vtkSetVector6Macro(WholeExtent, int);
  vtkGetVector6Macro(WholeExtent, int);

  // Description:
  // Although this information could be gotten from the files, this is easy.
  vtkSetMacro(NumberOfScalarComponents, int);  
  vtkGetMacro(NumberOfScalarComponents, int);  
  
  // Description:
  // Although this information could be gotten from the files, this is easy.
  vtkSetMacro(ScalarType, int);  
  vtkGetMacro(ScalarType, int);  
  
  // Description:
  // This printf pattern should take three integers, one for each axis.
  vtkSetStringMacro(FilePattern);
  vtkGetStringMacro(FilePattern);


  
protected:
  vtkImageBlockReader();
  ~vtkImageBlockReader();
  
  char *FilePattern;

  int WholeExtent[6];
  int NumberOfScalarComponents;
  int ScalarType;
  int Divisions[3];
  int Overlap;


  // Description:
  // Write the files.
  void Execute(vtkImageData *data);
  void ExecuteInformation();

  // This method computes the XYZExtents.
  void ComputeBlockExtents();
  void DeleteBlockExtents();

  // helper methods
  void Read(vtkImageData *data, int *ext);
  void ReadRemainder(vtkImageData *data, int *ext, int *doneExt);
  void ReadBlock(int xIdx, int yIdx, int zIdx, 
                 vtkImageData *data, int *ext);

  // Description:
  // Generate more than requested.  Called by the superclass before
  // an execute, and before output memory is allocated.
  void ModifyOutputUpdateExtent();

  // extents (min, max) of the divisions.
  int *XExtents;
  int *YExtents;
  int *ZExtents;
private:
  vtkImageBlockReader(const vtkImageBlockReader&);  // Not implemented.
  void operator=(const vtkImageBlockReader&);  // Not implemented.
};


#endif


