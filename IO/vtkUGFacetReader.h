/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUGFacetReader.h
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
// .NAME vtkUGFacetReader - read EDS Unigraphics facet files
// .SECTION Description
// vtkUGFacetReader is a source object that reads Unigraphics facet files.
// Unigraphics is a solid modeling system; facet files are the polygonal
// plot files it uses to create 3D plots.

#ifndef __vtkUGFacetReader_h
#define __vtkUGFacetReader_h

#include <stdio.h>
#include "vtkPolyDataSource.h"
#include "vtkShortArray.h"

class VTK_IO_EXPORT vtkUGFacetReader : public vtkPolyDataSource 
{
public:
  vtkTypeMacro(vtkUGFacetReader,vtkPolyDataSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object to extract all parts, and with point merging
  // turned on.
  static vtkUGFacetReader *New();

  // Description:
  // Overload standard modified time function. If locator is modified,
  // then this object is modified as well.
  unsigned long GetMTime();

  // Description:
  // Specify Unigraphics file name.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Special methods for interrogating the data file.
  int GetNumberOfParts();

  // Description:
  // Retrieve color index for the parts in the file.
  short GetPartColorIndex(int partId);

  // Description:
  // Specify the desired part to extract. The part number must range between
  // [0,NumberOfParts-1]. If the value is =(-1), then all parts will be 
  // extracted. If the value is <(-1), then no parts will be  extracted but 
  // the part colors will be updated.
  vtkSetMacro(PartNumber,int);
  vtkGetMacro(PartNumber,int);

  // Description:
  // Turn on/off merging of points/triangles.
  vtkSetMacro(Merging,int);
  vtkGetMacro(Merging,int);
  vtkBooleanMacro(Merging,int);
  
  // Description:
  // Specify a spatial locator for merging points. By
  // default an instance of vtkMergePoints is used.
  void SetLocator(vtkPointLocator *locator);
  vtkGetObjectMacro(Locator,vtkPointLocator);

  // Description:
  // Create default locator. Used to create one when none is specified.
  void CreateDefaultLocator();

protected:
  vtkUGFacetReader();
  ~vtkUGFacetReader();

  void Execute();

  char *FileName;
  vtkShortArray *PartColors;
  int PartNumber;
  int Merging;
  vtkPointLocator *Locator;
private:
  vtkUGFacetReader(const vtkUGFacetReader&);  // Not implemented.
  void operator=(const vtkUGFacetReader&);  // Not implemented.
};

#endif


