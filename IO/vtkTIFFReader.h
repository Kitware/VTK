/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTIFFReader.h
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
// .NAME vtkTIFFReader - read TIFF files
// .SECTION Description
// vtkTIFFReader is a source object that reads TIFF files.
// It should be able to read most any TIFF file
//
// .SECTION See Also
// vtkTIFFWriter

#ifndef __vtkTIFFReader_h
#define __vtkTIFFReader_h

#include <stdio.h>
#include "vtkImageReader2.h"

//BTX
class TIFFInternal;
//ETX

class VTK_IO_EXPORT vtkTIFFReader : public vtkImageReader2
{
  public:
  static vtkTIFFReader *New();
  vtkTypeMacro(vtkTIFFReader,vtkImageReader2);

//BTX
  enum { NOFORMAT, RGB, GRAYSCALE, PALETTE_RGB, PALETTE_GRAYSCALE, OTHER };

  void GetColor( int index, 
                 unsigned short *r, unsigned short *g, unsigned short *b );
  unsigned int  GetFormat();
  void InitializeColors();

  void ReadImageInternal( void *, void *outPtr,  
                          int *outExt, unsigned int size );

  //Description: create a clone of this object.
  virtual vtkImageReader2* MakeObject() { return vtkTIFFReader::New(); }

  // Description: is the given file name a png file?
  virtual int CanReadFile(const char* fname);

  // Description:
  // Get the file extensions for this format.
  // Returns a string with a space separated list of extensions in 
  // the format .extension
  virtual const char* GetFileExensions()
    {
    return ".tif";
    }

  // Description: 
  // Return a descriptive name for the file format that might be useful in a GUI.
  virtual const char* GetDescriptiveName()
    {
    return "TIFF";
    }

  TIFFInternal *GetInternalImage()
    { return this->InternalImage; }
 
//ETX

  protected:
  vtkTIFFReader();
  ~vtkTIFFReader();

  virtual void ExecuteInformation();
  virtual void ExecuteData(vtkDataObject *out);

  void ReadGenericImage( void *out, 
                         unsigned int width, unsigned int height,
                         unsigned int size );
  
  int EvaluateImageAt( void*, void* ); 
  private:
  vtkTIFFReader(const vtkTIFFReader&);  // Not implemented.
  void operator=(const vtkTIFFReader&);  // Not implemented.

  unsigned short *ColorRed;
  unsigned short *ColorGreen;
  unsigned short *ColorBlue;
  int TotalColors;
  unsigned int ImageFormat;
  TIFFInternal *InternalImage;
  int *InternalExtents;
};
#endif


