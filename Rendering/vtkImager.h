/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImager.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImager - Renders into part of a ImageWindow
// .SECTION Description
// vtkImager is the 2D counterpart to vtkRenderer. An Imager renders
// 2D actors into a viewport of an image window. 

// .SECTION See Also
//  vtkImageWindow vtkViewport
   

#ifndef __vtkImager_h
#define __vtkImager_h

#include "vtkObject.h"
#include "vtkActor2DCollection.h"
#include "vtkActor2D.h"
#include "vtkViewport.h"
#include "vtkWindow.h"

class vtkImageWindow;

class VTK_RENDERING_EXPORT vtkImager : public vtkViewport
{ 
public:
  static vtkImager *New();
  vtkTypeRevisionMacro(vtkImager,vtkViewport);

  // Description:
  // Renders an imager.  Passes Render message on the 
  // the imager's actor2D collection.
  virtual int RenderOpaqueGeometry();
  virtual int RenderTranslucentGeometry();
  virtual int RenderOverlay();

  // Description:
  // Get the image window that an imager is attached to.
  vtkImageWindow* GetImageWindow();
  vtkWindow *GetVTKWindow() {return static_cast<vtkWindow*>(this->VTKWindow);};

  
  //BTX
  // Description:
  // These set methods are used by the image window, and should not be
  // used by anyone else.  They do not reference count the window.
  void SetImageWindow (vtkImageWindow* win);
  void SetVTKWindow (vtkWindow* win);  
  //ETX
  
  // Description:
  // Erase the contents of the imager in the window.
  virtual void Erase(){vtkErrorMacro(<<"vtkImager::Erase - Not implemented!");};

  virtual vtkAssemblyPath* PickProp(float selectionX, float selectionY);
  virtual float GetPickedZ();

protected:
  vtkImager();
  ~vtkImager();

  virtual void StartPick(unsigned int pickFromSize);
  virtual void UpdatePickId();
  virtual void DonePick(); 
  virtual unsigned int GetPickedId();
  virtual void DevicePickRender();
private:
  vtkImager(const vtkImager&);  // Not implemented.
  void operator=(const vtkImager&);  // Not implemented.
};


#endif




