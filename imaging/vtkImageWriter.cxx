/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageWriter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "vtkImageWriter.h"



//----------------------------------------------------------------------------
vtkImageWriter::vtkImageWriter()
{
  this->FilePrefix = NULL;
  this->FilePattern = NULL;
  this->FileName = NULL;
  this->FileDimensionality = 2;

  this->SetFilePrefix("");
  this->SetFilePattern("%s.%d");
  
  this->Input = NULL;
  this->FileLowerLeft = 0;
}



//----------------------------------------------------------------------------
vtkImageWriter::~vtkImageWriter()
{
  // get rid of memory allocated for file names
  if (this->FilePrefix)
    {
    delete [] this->FilePrefix;
    this->FilePrefix = NULL;
    }
  if (this->FilePattern)
    {
    delete [] this->FilePattern;
    this->FilePattern = NULL;
    }
  if (this->FileName)
    {
    delete [] this->FileName;
    this->FileName = NULL;
    }
}


//----------------------------------------------------------------------------
void vtkImageWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  os << indent << "Input: (" << this->Input << ")\n";

  os << indent << "FileName: " <<
    (this->FileName ? this->FileName : "(none)") << "\n";
  os << indent << "FilePrefix: " << 
    (this->FilePrefix ? this->FilePrefix : "(none)") << "\n";
  os << indent << "FilePattern: " << 
    (this->FilePattern ? this->FilePattern : "(none)") << "\n";
}

//----------------------------------------------------------------------------
// Description:
// This function sets the name of the file. 
void vtkImageWriter::SetFileName(char *name)
{
  if ( this->FileName && name && (!strcmp(this->FileName,name))) return; 
  if (!name && !this->FileName) return;
  if (this->FileName)
    {
    delete [] this->FileName;
    }
  if (this->FilePrefix)
    {
    delete [] this->FilePrefix;
    this->FilePrefix = NULL;
    }  
  this->FileName = new char[strlen(name) + 1];
  strcpy(this->FileName, name);
  this->Modified();
}

//----------------------------------------------------------------------------
// Description:
// This function sets the prefix of the file name. "image" would be the
// name of a series: image.1, image.2 ...
void vtkImageWriter::SetFilePrefix(char *prefix)
{
  if ( this->FilePrefix && prefix && (!strcmp(this->FilePrefix,prefix))) return; 
  if (!prefix && !this->FilePrefix) return;
  if (this->FilePrefix)
    {
    delete [] this->FilePrefix;
    }
  if (this->FileName)
    {
    delete [] this->FileName;
    this->FileName = NULL;
    }  
  this->FilePrefix = new char[strlen(prefix) + 1];
  strcpy(this->FilePrefix, prefix);
  this->Modified();
}

//----------------------------------------------------------------------------
// Description:
// This function sets the pattern of the file name which turn a prefix
// into a file name. "%s.%3d" would be the
// pattern of a series: image.001, image.002 ...
void vtkImageWriter::SetFilePattern(char *pattern)
{
  if ( this->FilePattern && pattern && 
       (!strcmp(this->FilePattern,pattern))) return; 
  if (!pattern && !this->FilePattern) return;
  if (this->FilePattern)
    {
    delete [] this->FilePattern;
    }
  if (this->FileName)
    {
    delete [] this->FileName;
    this->FileName = NULL;
    }
  this->FilePattern = new char[strlen(pattern) + 1];
  strcpy(this->FilePattern, pattern);
  this->Modified();
}

//----------------------------------------------------------------------------
// Description:
// Writes all the data from the input.
void vtkImageWriter::Write()
{
  // Error checking
  if ( this->Input == NULL )
    {
    vtkErrorMacro(<<"Write:Please specify an input!");
    return;
    }
  if ( ! this->FileName && (! this->FilePrefix || ! this->FilePattern))
    {
    vtkErrorMacro(<<"Write:Please specify either a FileName or a file prefix and pattern");
    return;
    }
  
  // Make sure the file name is allocated
  this->InternalFileName = 
    new char[(this->FileName ? strlen(this->FileName) : 1) +
	    (this->FilePrefix ? strlen(this->FilePrefix) : 1) +
	    (this->FilePattern ? strlen(this->FilePattern) : 1) + 10];
  
  // Fill in image information.
  this->Input->UpdateImageInformation();
  this->Input->SetUpdateExtent(this->Input->GetWholeExtent());
  this->FileNumber = 1;
  this->RecursiveWrite(2, this->Input, NULL);
  delete [] this->InternalFileName;
  this->InternalFileName = NULL;
}

//----------------------------------------------------------------------------
// Breaks region itto pieces with correct dimensionality.
void vtkImageWriter::RecursiveWrite(int axis, vtkImageCache *cache,
				    ofstream *file)
{
  int min, max, mid;
  vtkImageData *data;
  int fileOpenedHere = 0;
  
  // if we need to open another slice, do it
  if (!file && (axis + 1) == this->FileDimensionality)
    {
    // determine the name
    if (this->FilePrefix)
      {
      sprintf(this->InternalFileName, this->FilePattern, 
	      this->FilePrefix, this->FileNumber);
      }
    else
      {
      sprintf(this->InternalFileName, "%s", this->FileName);
      }
    // Open the file
#ifdef _WIN32
    file = new ofstream(this->InternalFileName, ios::out | ios::binary);
#else
    file = new ofstream(this->InternalFileName, ios::out);
#endif
    fileOpenedHere = 1;
    if (! file)
      {
      vtkErrorMacro("RecursiveWrite: Could not open file " << 
		    this->InternalFileName);
      return;
      }

    // Subclasses can write a header with this method call.
    this->WriteFileHeader(file, cache);
    ++this->FileNumber;
    }
  
  // will the current request fit into memory
  // if so the just get the data and write it out
  if (cache->GetUpdateExtentMemorySize() < cache->GetMemoryLimit())
    {
    data = cache->UpdateAndReturnData();
    this->RecursiveWrite(axis,cache,data,file);
    return;
    }

  // if the current request did not fit into memory
  // the we will split the current axis
  this->Input->GetAxisUpdateExtent(axis, min, max);
  if (min == max)
    {
    if (axis > 0)
      {
      this->RecursiveWrite(axis - 1,cache, file);
      }
    else
      {
      vtkWarningMacro("Cache to small to hold one row of pixels!!");
      }
    return;
    }
  
  mid = (min + max) / 2;
  vtkDebugMacro ("Split axis " << axis << " ("
		 << min << "->" << mid << ") and (" << mid+1 << "->"
		 << max << ")");

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
  if (file && fileOpenedHere)
    {
    file->close();
    delete file;
    file = NULL;
    }
}


//----------------------------------------------------------------------------
// same idea as the previous method, but it knows that the data is ready
void vtkImageWriter::RecursiveWrite(int axis, vtkImageCache *cache,
				    vtkImageData *data, ofstream *file)
{
  int idx, min, max;
  
  // if the file is already open then just write to it
  if (file)
    {
    this->WriteFile(file,data,cache->GetUpdateExtent());
    return;
    }
  
  // if we need to open another slice, do it
  if (!file && (axis + 1) == this->FileDimensionality)
    {
    // determine the name
    if (this->FilePrefix)
      {
      sprintf(this->InternalFileName, this->FilePattern, 
	      this->FilePrefix, this->FileNumber);
      }
    else
      {
      sprintf(this->InternalFileName, "%s", this->FileName);
      }
    // Open the file
#ifdef _WIN32
    file = new ofstream(this->InternalFileName, ios::out | ios::binary);
#else
    file = new ofstream(this->InternalFileName, ios::out);
#endif
    if (! file)
      {
      vtkErrorMacro("RecursiveWrite: Could not open file " << 
		    this->InternalFileName);
      return;
      }

    // Subclasses can write a header with this method call.
    this->WriteFileHeader(file, cache);
    this->WriteFile(file,data,cache->GetUpdateExtent());
    ++this->FileNumber;
    file->close();
    delete file;
    return;
    }
  
  // if the current region is too high a dimension forthe file
  // the we will split the current axis
  cache->GetAxisUpdateExtent(axis, min, max);
  
  // if it is the y axis then flip by default
  if (axis == 1 && !this->FileLowerLeft)
    {
    for(idx = max; idx >= min; idx--)
      {
      cache->SetAxisUpdateExtent(axis, idx, idx);
      this->RecursiveWrite(axis - 1, cache, data, file);
      }
    }
  else
    {
    for(idx = min; idx <= max; idx++)
      {
      cache->SetAxisUpdateExtent(axis, idx, idx);
      this->RecursiveWrite(axis - 1, cache, data, file);
      }
    }
  
  // restore original extent
  cache->SetAxisUpdateExtent(axis, min, max);
}
  

//----------------------------------------------------------------------------
// Writes a region in a file.  Subclasses can override this method
// to produce a header. This method only hanldes 3d data (plus components).
void vtkImageWriter::WriteFile(ofstream *file, vtkImageData *data,
			       int extent[6])
{
  int idxY, idxZ;
  int rowLength; // in bytes
  void *ptr;
  
  // Make sure we actually have data.
  if ( !data->GetPointData()->GetScalars())
    {
    vtkErrorMacro(<< "Could not get data from input.");
    return;
    }

  // take into consideration the scalar type
  switch (data->GetScalarType())
    {
    case VTK_FLOAT:
      rowLength = sizeof(float);
      break;
    case VTK_INT:
      rowLength = sizeof(int);
      break;
    case VTK_SHORT:
      rowLength = sizeof(short);
      break;
    case VTK_UNSIGNED_SHORT:
      rowLength = sizeof(unsigned short); 
      break;
    case VTK_UNSIGNED_CHAR:
      rowLength = sizeof(unsigned char); 
      break;
    default:
      cerr << "Execute: Unknown output ScalarType";
      return; 
    }
  rowLength *= data->GetNumberOfScalarComponents();
  rowLength *= (extent[1] - extent[0] + 1);

  for (idxZ = extent[4]; idxZ <= extent[5]; ++idxZ)
    {
    for (idxY = extent[3]; idxY >= extent[2]; idxY--)
      {
      ptr = data->GetScalarPointer(extent[0], idxY, idxZ);
      if ( ! file->write((char *)ptr, rowLength))
	{
	vtkErrorMacro("WriteFile: write failed");
	file->close();
	delete file;
	}
      }
    }
}


