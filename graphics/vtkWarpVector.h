/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWarpVector.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkWarpVector - deform geometry with vector data
// .SECTION Description
// vtkWarpVector is a filter that modifies point coordinates by moving
// points along vector times the scale factor. Useful for showing flow
// profiles or mechanical deformation.
//
// The filter passes both its point data and cell data to its output.

#ifndef __vtkWarpVector_h
#define __vtkWarpVector_h

#include "vtkPointSetToPointSetFilter.h"
#include "vtkMultiThreader.h"

class VTK_EXPORT vtkWarpVector : public vtkPointSetToPointSetFilter
{
public:
  static vtkWarpVector *New() {return new vtkWarpVector;};
  const char *GetClassName() {return "vtkWarpVector";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify value to scale displacement.
  vtkSetMacro(ScaleFactor,float);
  vtkGetMacro(ScaleFactor,float);

  // Description:
  // Get/Set the number of threads to create when rendering
  vtkSetClampMacro( NumberOfThreads, int, 1, VTK_MAX_THREADS );
  vtkGetMacro( NumberOfThreads, int );

  // Description:
  // public so that C function can call it.
  void ThreadedExecute(int threadId, int numThreads);

protected:
  vtkWarpVector();
  ~vtkWarpVector();
  void Execute();
  int SplitPointRange(int threadId, int threadCount, int &min, int &max);
  float ScaleFactor;

  vtkMultiThreader *Threader;
  int NumberOfThreads;
};

#endif


