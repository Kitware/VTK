/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDashedStreamLine.h
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
// .NAME vtkDashedStreamLine - generate constant-time dashed streamline in arbitrary dataset
// .SECTION Description
// vtkDashedStreamLine is a filter that generates a "dashed" streamline for 
// an arbitrary dataset. The streamline consists of a series of dashes, each 
// of which represents (approximately) a constant time increment. Thus, in the
// resulting visual representation, relatively long dashes represent areas of 
// high velocity, and small dashes represent areas of low velocity.
//
// vtkDashedStreamLine introduces the instance variable DashFactor. 
// DashFactor interacts with its superclass' instance variable StepLength to
// create the dashes. DashFactor is the percentage of the StepLength line 
// segment that is visible. Thus, if the DashFactor=0.75, the dashes will be 
// "three-quarters on" and "one-quarter off".
// .SECTION See Also
// vtkStreamer vtkStreamLine vtkStreamPoints

#ifndef __vtkDashedStreamLine_h
#define __vtkDashedStreamLine_h

#include "vtkStreamLine.hh"

class vtkDashedStreamLine : public vtkStreamLine
{
public:
  vtkDashedStreamLine();
  char *GetClassName() {return "vtkDashedStreamLine";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // For each dash, specify the fraction of the dash that is "on". A factor
  // of 1.0 will result in a continuous line, a factor of 0.5 will result in 
  // dashed that are half on and half off.
  vtkSetClampMacro(DashFactor,float,0.01,1.0);
  vtkGetMacro(DashFactor,float);

protected:
  // Convert streamer array into vtkPolyData
  void Execute();

  // the fraction of on versus off in dash
  float DashFactor;
  
};

#endif


