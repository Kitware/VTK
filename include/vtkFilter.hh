/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFilter.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkFilter - abstract class for specifying filter behaviour
// .SECTION Description
// vtkFilter is an abstract class that specifies the interface for data 
// filters. Each filter must have an UpdateFilter() and Execute() method 
// that will cause the filter to execute if its input or the filter itself 
// has been modified since the last execution time.

#ifndef __vtkFilter_h
#define __vtkFilter_h

#include "vtkLWObject.hh"
#include "vtkDataSet.hh"

class vtkFilter : public vtkLWObject
{
public:
  vtkFilter();
  virtual ~vtkFilter() {};
  void _PrintSelf(ostream& os, vtkIndent indent);
  char *_GetClassName() {return "vtkFilter";};

  // Description:
  // All filters must provide a method to update the visualization 
  // pipeline.
  virtual void UpdateFilter();

  void SetStartMethod(void (*f)(void *), void *arg);
  void SetEndMethod(void (*f)(void *), void *arg);
  void SetStartMethodArgDelete(void (*f)(void *));
  void SetEndMethodArgDelete(void (*f)(void *));

protected:
  vtkDataSet *Input;
  char Updating;
  void (*StartMethod)(void *);
  void (*StartMethodArgDelete)(void *);
  void *StartMethodArg;
  void (*EndMethod)(void *);
  void (*EndMethodArgDelete)(void *);
  void *EndMethodArg;
  vtkTimeStamp ExecuteTime;

  // Every filter must have execute method.
  virtual void Execute();

  // Get flag indicating whether data has been released since last execution.
  // Used during update method to determin whether to execute or not.
  virtual int GetDataReleased();
  virtual void SetDataReleased(int flag);

};

#endif


