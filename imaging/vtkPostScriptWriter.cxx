/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPostScriptWriter.cxx
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
#include "vtkPostScriptWriter.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkPostScriptWriter* vtkPostScriptWriter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPostScriptWriter");
  if(ret)
    {
    return (vtkPostScriptWriter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPostScriptWriter;
}



#define VTK_MARGIN 0.95

void vtkPostScriptWriter::WriteFileTrailer(ofstream *file, 
					   vtkImageData *vtkNotUsed(cache))
{
  *file << "\ngrestore\nshowpage\n%%%%Trailer\n";
}

void vtkPostScriptWriter::WriteFileHeader(ofstream *file, 
                                          vtkImageData *cache)
{
  int min1, max1, min2, max2, min3, max3;
  int bpp;
  int cols, rows, scols, srows;
  float scale = 1;
  int pagewid = (int) (8.5*72);
  int pagehgt = 11*72;
  
  // Find the length of the rows to write.
  cache->GetWholeExtent(min1, max1, min2, max2, min3, max3);
  bpp = cache->GetNumberOfScalarComponents();
  
  cols = max1 - min1 + 1;
  rows = max2 - min2 + 1;
  
  float pixfac = 0.96;	/* 1, approx. */
  scols = (int)(cols * pixfac);
  srows = (int)(rows * pixfac);
  if ( scols > pagewid * VTK_MARGIN || srows > pagehgt * VTK_MARGIN )
    {
    if ( scols > pagewid * VTK_MARGIN )
      {
      scale = scale*(pagewid * VTK_MARGIN / scols);
      scols = (int)(scale * cols * pixfac);
      srows = (int)(scale * rows * pixfac);
      }
    if ( srows > pagehgt * VTK_MARGIN )
      {
      scale = scale * (pagehgt * VTK_MARGIN / srows);
      scols = (int)(scale * cols * pixfac);
      srows = (int)(scale * rows * pixfac);
      }
    }
  float llx = ( pagewid - scols ) / 2;
  float lly = ( pagehgt - srows ) / 2;
  
  // spit out the PostScript header
  *file << "%!PS-Adobe-2.0 EPSF-2.0\n";
  *file << "%%Creator: Visualization Toolkit\n";
  *file << "%%Title: " << this->InternalFileName << endl;
  *file << "%%Pages: 1\n";
  *file << "%%BoundingBox: " << (int) llx << " "  << (int) lly
       << " " << (int) ( llx + scols + 0.5 ) << " " << 
    (int) ( lly + srows + 0.5 ) << endl;
  *file << "%%EndComments\n";
  *file << "/readstring {\n";
  *file << "  currentfile exch readhexstring pop\n";
  *file << "} bind def\n";
  
  if ( bpp == 3)
    {
    *file << "/rpicstr " << cols << " string def\n";
    *file << "/gpicstr " << cols << " string def\n";
    *file << "/bpicstr " << cols << " string def\n";
    }
  else if (bpp == 1)
    {
    *file << "/picstr " << cols << " string def\n";
    }
  else 
    {
    vtkWarningMacro( " vtkPostScriptWriter only supports 1 and 3 component images");
    }
  
  *file << "%%EndProlog\n";
  *file << "%%Page: 1 1\n";
  *file << "gsave\n";
  *file << llx << " " << lly << " translate\n";
  *file << scols << " " << srows << " scale\n";
  *file << cols << " " << rows << " 8\n";
  *file << "[ " << cols << " 0 0 " << -rows << " 0 " << rows << " ]\n";
  if (bpp == 3)
    {
    *file << "{ rpicstr readstring }\n";
    *file << "{ gpicstr readstring }\n";
    *file << "{ bpicstr readstring }\n";
    *file << "true 3\n";
    *file << "colorimage\n";
    }
  else
    {
    *file << "{ picstr readstring }\n";
    *file << "image\n";
    }
}


void vtkPostScriptWriter::WriteFile(ofstream *file, vtkImageData *data,
                                    int extent[6])
{
  int idxC, idx0, idx1, idx2;
  unsigned char *ptr;
  unsigned long count = 0;
  unsigned long target;
  float progress = this->Progress;
  float area;
  int *wExtent;
  static int itemsperline = 0;
  char* hexits = (char *) "0123456789abcdef";
  
  // Make sure we actually have data.
  if ( !data->GetPointData()->GetScalars())
    {
    vtkErrorMacro(<< "Could not get data from input.");
    return;
    }

  // take into consideration the scalar type
  switch (data->GetScalarType())
    {
    case VTK_UNSIGNED_CHAR:
      break;
    default:
      vtkErrorMacro("PostScriptWriter only accepts unsigned char scalars!");
      return; 
    }
  
  wExtent = this->GetInput()->GetWholeExtent();
  area = ((extent[5] - extent[4] + 1)*(extent[3] - extent[2] + 1)*
	  (extent[1] - extent[0] + 1)) / 
    ((wExtent[5] -wExtent[4] + 1)*(wExtent[3] -wExtent[2] + 1)*
     (wExtent[1] -wExtent[0] + 1));
    

  int numComponents = data->GetNumberOfScalarComponents();
  // ignore alpha
  int maxComponent = numComponents;
  if (numComponents == 2) 
    {
    maxComponent = 1;
    }
  if (numComponents == 4) 
    {
    maxComponent = 3;
    }
  target = (unsigned long)((extent[5]-extent[4]+1)*
			   (extent[3]-extent[2]+1)/(50.0*area));
  target++;
  
  for (idx2 = extent[4]; idx2 <= extent[5]; ++idx2)
    {
    for (idx1 = extent[3]; idx1 >= extent[2]; idx1--)
      {
      if (!(count%target))
	{
	this->UpdateProgress(progress + count/(50.0*target));
	}
      count++;
      // write out components one at a time because
      for (idxC = 0; idxC < maxComponent; idxC++)
	{
	ptr = (unsigned char*)data->GetScalarPointer(extent[0],idx1, idx2);
	ptr += idxC;
	for ( idx0 = extent[0]; idx0 <= extent[1]; idx0++ )
	  {
	  if ( itemsperline == 30 )
	    {
	    *file << endl;
	    itemsperline = 0;
	    }
	  *file << hexits[*ptr >> 4] << hexits[*ptr & 15];
	  ++itemsperline;
	  ptr += numComponents;
	  }
	}
      }
    }
  
}



