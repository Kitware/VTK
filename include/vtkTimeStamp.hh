/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTimeStamp.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkTimeStamp - record modification and/or execution time
// .SECTION Description
// vtkTimeStamp records a unique time when the method Modified() is 
// executed. This time is guaranteed to be monotonically increasing.
// Classes use this object to record modified and/or execution time.
// There is built in support for the binary < and > comparison
// operators between two vtkTimeStamp objects.

#ifndef __vtkTimeStamp_h
#define __vtkTimeStamp_h

class vtkTimeStamp 
{
public:
  vtkTimeStamp() : ModifiedTime(0) {};

  // Description:
  // Set this objects time to the current time. The current time is
  // just a monotonically increasing unsigned long integer. It is
  // possible for this number to wrap around back to zero.
  // This should only happen for processes that have been running
  // for a very long time, while constantly changing objects
  // within the program. When this does occur, the typical consequence
  // should be that some filters will update themselves when really
  // they don't need to.
  void Modified() {this->ModifiedTime = ++vtkTime;};

  // Description:
  // Return this object's Modified time.
  unsigned long int GetMTime() {return ModifiedTime;};

  int operator>(vtkTimeStamp& ts) {return (this->ModifiedTime > ts.ModifiedTime);};
  int operator<(vtkTimeStamp& ts) {return (this->ModifiedTime < ts.ModifiedTime);};
  operator unsigned long int() {return this->ModifiedTime;};

private:
  unsigned long ModifiedTime;
  static unsigned long vtkTime;
};

#endif
