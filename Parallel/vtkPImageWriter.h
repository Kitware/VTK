/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPImageWriter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.


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
// .NAME vtkImageWriter - Writes images to files.
// .SECTION Description
// vtkImageWriter writes images to files with any data type. The data type of
// the file is the same scalar type as the input.  The dimensionality
// determines whether the data will be written in one or multiple files.
// This class is used as the superclass of most image writing classes 
// such as vtkBMPWriter etc. It supports streaming.

#ifndef __vtkPImageWriter_h
#define __vtkPImageWriter_h

#include "vtkImageWriter.h"
class vtkPipelineSize;

class VTK_PARALLEL_EXPORT vtkPImageWriter : public vtkImageWriter
{
public:
  static vtkPImageWriter *New();
  vtkTypeMacro(vtkPImageWriter,vtkImageWriter);
  void PrintSelf(ostream& os, vtkIndent indent);  

  // Description:
  // Set / Get the memory limit in kilobytes. The writer will
  // stream to attempt to keep the pipeline size within this limit
  vtkSetMacro(MemoryLimit, unsigned long);
  vtkGetMacro(MemoryLimit, unsigned long);

protected:
  vtkPImageWriter();
  ~vtkPImageWriter();

  unsigned long MemoryLimit;
  
  virtual void RecursiveWrite(int dim, vtkImageData *region, ofstream *file);
  virtual void RecursiveWrite(int dim, vtkImageData *cache, 
                              vtkImageData *data, ofstream *file) 
    {this->vtkImageWriter::RecursiveWrite(dim,cache,data,file);};
  
  vtkPipelineSize *SizeEstimator;
private:
  vtkPImageWriter(const vtkPImageWriter&);  // Not implemented.
  void operator=(const vtkPImageWriter&);  // Not implemented.
};

#endif


