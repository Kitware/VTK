/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTimeStamp.h
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
// .NAME vtkTimeStamp - record modification and/or execution time
// .SECTION Description
// vtkTimeStamp records a unique time when the method Modified() is 
// executed. This time is guaranteed to be monotonically increasing.
// Classes use this object to record modified and/or execution time.
// There is built in support for the binary < and > comparison
// operators between two vtkTimeStamp objects.

#ifndef __vtkTimeStamp_h
#define __vtkTimeStamp_h

#include "vtkWin32Header.h"

class VTK_EXPORT vtkTimeStamp 
{
public:
  vtkTimeStamp() {this->ModifiedTime = 0;};
  static vtkTimeStamp *New();
  void Delete() {delete this;};

  virtual const char *GetClassName() {return "vtkTimeStamp";};

  // Description:
  // Set this objects time to the current time. The current time is
  // just a monotonically increasing unsigned long integer. It is
  // possible for this number to wrap around back to zero.
  // This should only happen for processes that have been running
  // for a very long time, while constantly changing objects
  // within the program. When this does occur, the typical consequence
  // should be that some filters will update themselves when really
  // they don't need to.
  void Modified();

  // Description:
  // Return this object's Modified time.
  unsigned long int GetMTime() {return this->ModifiedTime;};

  // Description:
  // Support comparisons of time stamp objects directly.
  int operator>(vtkTimeStamp& ts) {
    return (this->ModifiedTime > ts.ModifiedTime);};
  int operator<(vtkTimeStamp& ts) {
    return (this->ModifiedTime < ts.ModifiedTime);};

  // Description:
  // Allow for typecasting to unsigned long.
  operator unsigned long() {return this->ModifiedTime;};

private:
  unsigned long ModifiedTime;
};

#endif
