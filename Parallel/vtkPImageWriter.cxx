/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPImageWriter.cxx
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
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "vtkPImageWriter.h"
#include "vtkObjectFactory.h"
#include "vtkPipelineSize.h"

#define vtkPIWCloseFile \
    if (file && fileOpenedHere) \
      { \
      this->WriteFileTrailer(file,cache); \
      file->close(); \
      delete file; \
      file = NULL; \
      } \

//------------------------------------------------------------------------------
vtkPImageWriter* vtkPImageWriter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPImageWriter");
  if(ret)
    {
    return (vtkPImageWriter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPImageWriter;
}

#ifdef write
#undef write
#endif

#ifdef close
#undef close
#endif


//----------------------------------------------------------------------------
vtkPImageWriter::vtkPImageWriter()
{
  // Set a default memory limit of a gigabyte
  this->MemoryLimit = 1000000; 

  this->SizeEstimator = vtkPipelineSize::New();
}



//----------------------------------------------------------------------------
vtkPImageWriter::~vtkPImageWriter()
{
  if (this->SizeEstimator)
    {
    this->SizeEstimator->Delete();
    }
}


//----------------------------------------------------------------------------
void vtkPImageWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageWriter::PrintSelf(os,indent);

  os << indent << "MemoryLimit: " << this->MemoryLimit << "\n";
}


//----------------------------------------------------------------------------
// Breaks region into pieces with correct dimensionality.
void vtkPImageWriter::RecursiveWrite(int axis, vtkImageData *cache,
				    ofstream *file)
{
  int             min, max, mid;
  vtkImageData    *data;
  int             fileOpenedHere = 0;
  int             *ext;
  unsigned long   inputMemorySize;

  // if we need to open another slice, do it
  if (!file && (axis + 1) == this->FileDimensionality)
    {
    // determine the name
    if (this->FileName)
      {
      sprintf(this->InternalFileName,"%s",this->FileName);
      }
    else 
      {
      if (this->FilePrefix)
        {
        sprintf(this->InternalFileName, this->FilePattern, 
                this->FilePrefix, this->FileNumber);
        }
      else
        {
        sprintf(this->InternalFileName, this->FilePattern,this->FileNumber);
        }
      }
    // Open the file
#ifdef _WIN32
    file = new ofstream(this->InternalFileName, ios::out | ios::binary);
#else
    file = new ofstream(this->InternalFileName, ios::out);
#endif
    fileOpenedHere = 1;
    if (file->fail())
      {
      vtkErrorMacro("RecursiveWrite: Could not open file " << 
		    this->InternalFileName);
      delete file;
      return;
      }

    // Subclasses can write a header with this method call.
    this->WriteFileHeader(file, cache);
    ++this->FileNumber;
    }
  
  // Propagate the update extent so we can determine pipeline size
  this->GetInput()->PropagateUpdateExtent();

  // Now we can ask how big the pipeline will be
  inputMemorySize = this->SizeEstimator->GetEstimatedSize(this->GetInput());

  // will the current request fit into memory
  // if so the just get the data and write it out
  if ( inputMemorySize < this->MemoryLimit )
    {
    ext = cache->GetUpdateExtent();
    vtkDebugMacro("Getting input extent: " << ext[0] << ", " << ext[1] << ", " << ext[2] << ", " << ext[3] << ", " << ext[4] << ", " << ext[5] << endl);
    cache->Update();
    data = cache;
    this->RecursiveWrite(axis,cache,data,file);
    vtkPIWCloseFile;
    return;
    }

  // if the current request did not fit into memory
  // the we will split the current axis
  this->GetInput()->GetAxisUpdateExtent(axis, min, max);
  
  vtkDebugMacro("Axes: " << axis << "(" << min << ", " << max 
  	<< "), UpdateMemory: " << inputMemorySize 
  	<< ", Limit: " << this->MemoryLimit << endl);
  
  if (min == max)
    {
    if (axis > 0)
      {
      this->RecursiveWrite(axis - 1,cache, file);
      }
    else
      {
      vtkWarningMacro("MemoryLimit too small for one pixel of information!!");
      }
    vtkPIWCloseFile;
    return;
    }
  
  mid = (min + max) / 2;

  // if it is the y axis then flip by default
  if (axis == 1 && !this->FileLowerLeft)
    {
    // first half
    cache->SetAxisUpdateExtent(axis, mid+1, max);
    this->RecursiveWrite(axis,cache,file);
    
    // second half
    cache->SetAxisUpdateExtent(axis, min, mid);
    this->RecursiveWrite(axis,cache,file);
    }
  else
    {
    // first half
    cache->SetAxisUpdateExtent(axis, min, mid);
    this->RecursiveWrite(axis,cache,file);
    
    // second half
    cache->SetAxisUpdateExtent(axis, mid+1, max);
    this->RecursiveWrite(axis,cache,file);
    }
    
  // restore original extent
  cache->SetAxisUpdateExtent(axis, min, max);

  // if we opened the file here, then we need to close it up
  vtkPIWCloseFile;
}



