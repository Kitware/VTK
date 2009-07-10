
/*
* Copyright (c) 2007, Sandia Corporation
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of the Sandia Corporation nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY Sandia Corporation ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL Sandia Corporation BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _vtkMimeTypes_h
#define _vtkMimeTypes_h

#include <vtkObject.h>
#include <vtkStdString.h> //Needed for lookup

/// Helper class for determining the MIME-type of files at runtime.  To use,
/// create an instance of vtkMimeTypes, then call the Lookup() method to
/// determine the MIME-type of each file of-interest.
class VTK_TEXT_ANALYSIS_EXPORT vtkMimeTypes :
  public vtkObject
{
public:
  static vtkMimeTypes* New();
  vtkTypeRevisionMacro(vtkMimeTypes, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Returns the MIME-type of a file, or empty-string if the type cannot be identified
  vtkStdString Lookup(const vtkStdString& path);

private:
  vtkMimeTypes();
  ~vtkMimeTypes();

  vtkMimeTypes(const vtkMimeTypes&); //Not implemented.
  void operator=(const vtkMimeTypes&); //Not implemented.

//BTX
  class implementation;
  implementation* const Implementation;
//ETX
};

#endif // !_vtkMimeTypes_h

