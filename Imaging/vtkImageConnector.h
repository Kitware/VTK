/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageConnector.h
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
// .NAME vtkImageConnector - Create a binary image of a sphere.
// .SECTION Description
// vtkImageConnector is a helper class for connectivity filters.  
// It is not meant to be used directly.
// It implements a stack and breadth first search necessary for
// some connectivity filters.  Filtered axes sets the dimensionality 
// of the neighbor comparison, and
// cannot be more than three dimensions.  
// As implemented, only voxels which share faces are considered
// neighbors. 


#ifndef __vtkImageConnector_h
#define __vtkImageConnector_h

#include "vtkObject.h"
#include "vtkImageData.h"

//
// Special classes for manipulating data
//
//BTX - begin tcl exclude
//
// For the breadth first search
class vtkImageConnectorSeed { //;prevent man page generation
public:
  static vtkImageConnectorSeed *New() { return new vtkImageConnectorSeed;}
  void *Pointer;
  int  Index[3];
  vtkImageConnectorSeed *Next;
};
//ETX - end tcl exclude
//


class VTK_IMAGING_EXPORT vtkImageConnector : public vtkObject
{
public:
  static vtkImageConnector *New();

  vtkTypeMacro(vtkImageConnector,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
  vtkImageConnectorSeed *NewSeed(int index[3], void *ptr);
  void AddSeed(vtkImageConnectorSeed *seed);
  void AddSeedToEnd(vtkImageConnectorSeed *seed);
  //ETX
  void RemoveAllSeeds();

  // Description:
  // Values used by the MarkRegion method
  vtkSetMacro(ConnectedValue, unsigned char); 
  vtkGetMacro(ConnectedValue, unsigned char);
  vtkSetMacro(UnconnectedValue, unsigned char);
  vtkGetMacro(UnconnectedValue, unsigned char);


  // Description:
  // Input a data of 0's and "UnconnectedValue"s. Seeds of this object are
  // used to find connected pixels.  All pixels connected to seeds are set to
  // ConnectedValue.  The data has to be unsigned char.
  void MarkData(vtkImageData *data, int dimensionality, int ext[6]);


protected:
  vtkImageConnector();
  ~vtkImageConnector();
  vtkImageConnector(const vtkImageConnector&);
  void operator=(const vtkImageConnector&);

  unsigned char ConnectedValue;
  unsigned char UnconnectedValue;

  vtkImageConnectorSeed *PopSeed();

  vtkImageConnectorSeed *Seeds;
  vtkImageConnectorSeed *LastSeed;
};



#endif

  
