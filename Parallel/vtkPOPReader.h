/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPOPReader.h
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
// .NAME vtkPOPReader - read POP data files
// .SECTION Description
// vtkPOPReader Just converts from images to a structured grid for now.


#ifndef __vtkPOPReader_h
#define __vtkPOPReader_h

#include <stdio.h>
#include "vtkImageData.h"
#include "vtkStructuredGridSource.h"

class VTK_PARALLEL_EXPORT vtkPOPReader : public vtkStructuredGridSource 
{
public:
  static vtkPOPReader *New();
  vtkTypeMacro(vtkPOPReader,vtkStructuredGridSource);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // This is the longitude and latitude dimensions of the structured grid.
  vtkGetVector2Macro(Dimensions, int);  
    
  // Description:
  // This file contains the latitude and longitude of the grid.  
  // It must be double with no header.
  vtkGetStringMacro(GridFileName);

  // Description:
  // These files contains the u and v components of the flow.
  vtkGetStringMacro(UFlowFileName);
  vtkGetStringMacro(VFlowFileName);
  
  // Description:
  // This file contains information about all the files.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  
  // Description:
  // Radius of the earth.
  vtkSetMacro(Radius, float);
  vtkGetMacro(Radius, float);
  
  // Description:
  // Because the data can be so large, here is an option to clip
  // while reading.
  vtkSetVector6Macro(ClipExtent, int);
  vtkGetVector6Macro(ClipExtent, int);

protected:
  vtkPOPReader();
  ~vtkPOPReader();
  vtkPOPReader(const vtkPOPReader&) {};
  void operator=(const vtkPOPReader&) {};

  void ExecuteInformation();
  void Execute();
  
  void ReadInformationFile();
  vtkPoints *ReadPoints(vtkImageData *image);
  void ReadFlow();
  // NOT USED
  vtkPoints *GeneratePoints();
  
  char *FileName;
  
  int Dimensions[2];
  vtkSetStringMacro(GridFileName);
  void SetGridName(char *name);
  char *GridFileName;

  float Radius;
  vtkFloatArray *DepthValues;
  
  void DeleteArrays();
  void AddArray(char *arrayName, char *fileName, unsigned long offset);
  void AddArrayName(char *arrayName, char *fileName, unsigned long offset);
  int NumberOfArrays;
  int MaximumNumberOfArrays;
  char **ArrayNames;
  char **ArrayFileNames;  
  unsigned long *ArrayOffsets;
  int ArrayFileDimensionality;


  char *UFlowFileName;
  vtkSetStringMacro(UFlowFileName);
  unsigned long UFlowFileOffset;
  char *VFlowFileName;
  vtkSetStringMacro(VFlowFileName);
  unsigned long VFlowFileOffset;
  

  int IsFileName(char *name);
  char *MakeFileName(char *name);

  int ClipExtent[6];
};

#endif


