/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleUser.h
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

// .NAME vtkInteractorStyleUser - provides customizable interaction routines
// 
// .SECTION Description
// The most common way to customize user interaction is to write a subclass
// of vtkInteractorStyle: vtkInteractorStyleUser allows user interaction to
// be customized without subclassing.  

#ifndef __vtkInteractorStyleUser_h
#define __vtkInteractorStyleUser_h

#include "vtkInteractorStyleTrackball.h"

// new motion flag
#define VTKIS_USERINTERACTION 8 

class VTK_EXPORT vtkInteractorStyleUser : public vtkInteractorStyleTrackball 
{
public:
  static vtkInteractorStyleUser *New() {return new vtkInteractorStyleUser;}
  const char *GetClassName() {return "vtkInteractorStyleUser";};
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description: 
  // Hooks to set up a customized interaction mode
  void StartUserInteraction();
  void EndUserInteraction();
  void SetUserInteractionMethod(void (*f)(void *), void *arg);
  void SetUserInteractionMethodArgDelete(void (*f)(void *));

  // Description:
  // Get the most recent mouse position
  vtkGetVector2Macro(LastPos,int);

  // Description:
  // Get the previous mouse position
  vtkGetVector2Macro(OldPos,int);

  // Description:
  // Test whether modifiers were held down when mouse button was pressed
  vtkGetMacro(ShiftKey,int);
  vtkGetMacro(CtrlKey,int);

  // Description:
  // Other miscellaneous state variables:
  vtkGetMacro(ActorMode,int);
  vtkGetMacro(TrackballMode,int);
  vtkGetMacro(ControlMode,int);

protected:
  vtkInteractorStyleUser();
  ~vtkInteractorStyleUser();
  vtkInteractorStyleUser(const vtkInteractorStyleUser&) {};
  void operator=(const vtkInteractorStyleUser&) {};

  virtual void OnTimer(void);

  int OldPos[2];

  void (*UserInteractionMethod)(void *);
  void (*UserInteractionMethodArgDelete)(void *);
  void *UserInteractionMethodArg;
};

#endif
