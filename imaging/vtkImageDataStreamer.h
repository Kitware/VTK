/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDataStreamer.h
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
// .NAME vtkImageDataStreamer - Initiates streaming on image data.
// .SECTION Description
// To satisfy a request, this filter calls update on its input
// many times with smaller update extents.  All processing up stream
// streams smaller pieces.

#ifndef __vtkImageDataStreamer_h
#define __vtkImageDataStreamer_h

#include "vtkImageToImageFilter.h"

//BTX
// Define a helper class to keep track of a stack of extents 
class VTK_EXPORT vtkImageDataStreamerExtentStack
{
public:
  vtkImageDataStreamerExtentStack()
  { this->StackTop = NULL; 
    this->StackStorageSize = 0;
    this->StackSize = 0;
    this->Stack = NULL; };

  ~vtkImageDataStreamerExtentStack()
  { if ( this->Stack ) { delete [] this->Stack; }; };

  void Push( int extent[6] );
  void Pop( int extent[6] );
  void PopAll();
  
  int  GetStackSize() { return this->StackSize; };

protected:
  int    *Stack;
  int    *StackTop;
  int    StackSize;
  int    StackStorageSize;
};
//ETX


class VTK_EXPORT vtkImageDataStreamer : public vtkImageToImageFilter
{
public:
  static vtkImageDataStreamer *New();
  vtkTypeMacro(vtkImageDataStreamer,vtkImageToImageFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set / Get the memory limit in kilobytes.
  vtkSetMacro(MemoryLimit, unsigned long);
  vtkGetMacro(MemoryLimit, unsigned long);
  
  // Description:
  // How should the streamer break up extents. Block mode
  // tries to break an extent up into cube blocks.  It always chooses
  // the largest axis to split.
  // Slab mode first breaks up the Z axis.  If it gets to one slice,
  // then it starts breaking up other axes.
  void SetSplitModeToBlock()
    {this->SplitMode = VTK_IMAGE_DATA_STREAMER_BLOCK_MODE;}
  void SetSplitModeToXSlab()
    {this->SplitMode = VTK_IMAGE_DATA_STREAMER_X_SLAB_MODE;}
 void SetSplitModeToYSlab()
    {this->SplitMode = VTK_IMAGE_DATA_STREAMER_Y_SLAB_MODE;}
 void SetSplitModeToZSlab()
    {this->SplitMode = VTK_IMAGE_DATA_STREAMER_Z_SLAB_MODE;}
  
  // Description:
  // Need to override since this is where the check for incremental will
  // be done
  void UpdateInformation();
  
  // Description:
  // Need to override since this is where streaming will be done
  void UpdateData( vtkDataObject *out );

  // Description:
  // Need to override and do nothing since it should be triggered during
  // the update data pass due to streaming
  void TriggerAsynchronousUpdate();

  // Description:
  // Is this an incremental streamer? If yes, then each update of this
  // filter will produce another chunk of the output - update must be called
  // multiple times to generate the whole output. If no, then one update 
  // produces all of the output.
  vtkSetClampMacro( IncrementalUpdate, int, 0, 1 );
  vtkGetMacro( IncrementalUpdate, int );
  vtkBooleanMacro( IncrementalUpdate, int );


protected:
  vtkImageDataStreamer();
  ~vtkImageDataStreamer() {};
  vtkImageDataStreamer(const vtkImageDataStreamer&) {};
  void operator=(const vtkImageDataStreamer&) {};

  unsigned long  MemoryLimit;
  int            SplitMode;
  int            IncrementalUpdate;
  int            ProcessExtent[6];
  int            DataWasPassed;
    
  vtkImageDataStreamerExtentStack ExtentStack;

//BTX

// Don't change the numbers here - they are used in the code
// to indicate array indices.

  enum Modes {
    VTK_IMAGE_DATA_STREAMER_X_SLAB_MODE=0,
    VTK_IMAGE_DATA_STREAMER_Y_SLAB_MODE=1,
    VTK_IMAGE_DATA_STREAMER_Z_SLAB_MODE=2,
    VTK_IMAGE_DATA_STREAMER_BLOCK_MODE= 3
  };

//ETX

};




#endif



