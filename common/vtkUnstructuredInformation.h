/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredInformation.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkUnstructuredInformation - Only MaximumNumberOfPieces for now..
// .SECTION Description
// Note:  This object is under development an might change in the future.
// This class contains all the information specific to unstructured
// data sets.  The only thing it contians now is the
// mamximum number of pieces streaming can request.


#ifndef __vtkUnstructuredInformation_h
#define __vtkUnstructuredInformation_h

#include "vtkDataInformation.h"


class VTK_EXPORT vtkUnstructuredInformation : public vtkDataInformation
{
public:
  static vtkUnstructuredInformation *New();

  vtkTypeMacro(vtkUnstructuredInformation,vtkDataInformation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Makes an empty similar type object.
  vtkDataInformation *MakeObject() 
    {return vtkUnstructuredInformation::New();}
  
  // Description:
  // Subclasses override this method, and try to be smart
  // if the types are different.
  void Copy(vtkDataInformation *in);

  // Description:
  // This is the maximum number of pieces that can be requested.
  // Requesting more than this value will give you no data.
  // This variable is going to be moved into vtkUnstructuredInformation.
  vtkSetMacro(MaximumNumberOfPieces, unsigned long);
  vtkGetMacro(MaximumNumberOfPieces, unsigned long);
  
  // Description:
  // This method is passed a ClassName and returns 1 if the object is
  // a subclass of the class arg.  It is an attempt at making a smarter copy.
  int GetClassCheck(char *className);
  
  // Description:
  // Serialization provided for the multi-process ports.
  void ReadSelf(istream& is);
  void WriteSelf(ostream& os);

protected:
  vtkUnstructuredInformation();
  ~vtkUnstructuredInformation() {};
  vtkUnstructuredInformation(vtkUnstructuredInformation&) {};
  void operator=(vtkUnstructuredInformation&) {};

  // This tells down stream filters the smallest resolution available 
  // for streaming/spliting.  Now this is sort of a whole extent
  // for unstructured data, and should not be part 
  // of the information of vtkDataObject...
  unsigned long MaximumNumberOfPieces;

};


#endif
