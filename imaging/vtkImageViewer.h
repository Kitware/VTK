/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageViewer.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.


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
// .NAME vtkImageViewer - Display a 2d image.
// .SECTION Description
// vtkImageViewer  is a generic viewer superclass.  The "New" method
// creates a viewer of the correct type.

#ifndef __vtkImageViewer_h
#define __vtkImageViewer_h

#include    	<fstream.h>
#include 	<stdlib.h>
#include 	<iostream.h>

#include 	"vtkObject.h"
#include 	"vtkImageSource.h"
#include 	"vtkImageRegion.h"
#include  "vtkStructuredPointsToImage.h"

class VTK_EXPORT vtkImageViewer : public vtkObject {
public:
  vtkImageViewer();
  ~vtkImageViewer();
  char *GetClassName() {return "vtkImageViewer";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get name of rendering window
  vtkGetStringMacro(WindowName);

  // Description:
  // Object factory for this class.
  static vtkImageViewer *New();

  // Description:
  // Subclass must define this method.
  virtual void Render(void) {};
  
  // Description:
  // Set/Get the input to the viewer.
  vtkSetObjectMacro(Input,vtkImageSource);
  vtkGetObjectMacro(Input,vtkImageSource);
  void SetInput(vtkStructuredPoints *spts)
    {this->SetInput(spts->GetStructuredPointsToImage()->GetOutput());}
  
  // Description:
  // Display the wole image or just the region specified by extent.
  vtkSetMacro(WholeImage,int);
  vtkGetMacro(WholeImage,int);
  vtkBooleanMacro(WholeImage,int);
  
  // Description:
  // Messages that get forwarded to this viewers "Region".
  void SetExtent(int *extent)
  {this->Region.SetExtent(2,extent); this->Modified(); this->WholeImageOff();};
  void SetExtent(int min0, int max0, int min1, int max1)
  {this->Region.SetExtent(min0,max0, min1,max1); this->Modified();this->WholeImageOff();};
  int *GetExtent(){return this->Region.GetExtent();};
  void GetExtent(int *extent){this->Region.GetExtent(2,extent);};
  void GetExtent(int &min0,int &max0,int &min1,int &max1)
  {this->Region.GetExtent(min0,max0,min1,max1);};

  // Description:
  // Coordinate2 and Coordiante3 specify which 2d image to show.
  vtkSetMacro(Coordinate2,int);
  vtkGetMacro(Coordinate2,int);
  vtkSetMacro(Coordinate3,int);
  vtkGetMacro(Coordinate3,int);
  
  // Description:
  // Sets The coordinate system of the displayed region.  The first
  // Two dimensions are the ones displayed.  The others are provided
  // to set default values (i.e. slice of a volume.
  void SetAxes(int axis0, int axis1)
  {this->Region.SetAxes(axis0,axis1); this->Modified();};
  void SetAxes(int axis0, int axis1, int axis2)
  {this->Region.SetAxes(axis0,axis1,axis2); this->Modified();};
  void SetAxes(int axis0, int axis1, int axis2, int axis3)
  {this->Region.SetAxes(axis0,axis1,axis2,axis3); this->Modified();};

  // Description:
  // Sets window/level for mapping pixels to colors.
  vtkSetMacro(ColorWindow,float);
  vtkGetMacro(ColorWindow,float);
  vtkSetMacro(ColorLevel,float);
  vtkGetMacro(ColorLevel,float);

  // Description:
  // Turn color interpretation on/off.
  vtkSetMacro(ColorFlag, int);
  vtkGetMacro(ColorFlag, int);
  vtkBooleanMacro(ColorFlag, int);
  // Description:
  // Which components should be used for RGB.
  vtkSetMacro(Red, int);
  vtkGetMacro(Red, int);
  vtkSetMacro(Green, int);
  vtkGetMacro(Green, int);
  vtkSetMacro(Blue, int);
  vtkGetMacro(Blue, int);
  
  // Description:
  // Set/Get the upper left location of the viewer in the window.
  vtkSetMacro(XOffset, int);
  vtkGetMacro(XOffset, int);
  vtkSetMacro(YOffset, int);
  vtkGetMacro(YOffset, int);

  // Description:
  // These are here for using a tk window.
  virtual void SetDisplayId(void *) {};
  virtual void SetWindowId(void *) {};
  virtual void SetParentId(void *) {};
  
  // Description:
  // Keep track of whether the rendering window has been mapped to screen.
  vtkSetMacro(Mapped,int);
  vtkGetMacro(Mapped,int);
  vtkBooleanMacro(Mapped,int);

  // Description:
  // By default this is a color viewer. GrayScaleHintOn will improve the appearance
  // of gray scale images on some systems.
  vtkSetMacro(GrayScaleHint,int);
  vtkGetMacro(GrayScaleHint,int);
  vtkBooleanMacro(GrayScaleHint,int);

  // Description:
  // Set/Get the position in screen coordinates of the rendering window.
  virtual int *GetPosition() {return (int *)NULL;};
  virtual void SetPosition(int,int);
  virtual void SetPosition(int a[2]);

  // Description:
  // Set/Get the size of the window in screen coordinates.
  virtual int *GetSize() {return (int *)NULL;};
  virtual void SetSize(int,int) {};
  virtual void SetSize(int a[2]);

protected:
  // location of upper left corner in window.
  int                  XOffset;
  int                  YOffset;
  int Mapped;
  vtkImageSource *Input;
  int WholeImage;
  // Contains the extent of the region to be displayed.
  vtkImageRegion Region;
  int Coordinate2;
  int Coordinate3;  
  // For converting image pixels to X pixels.
  float ColorWindow;
  float ColorLevel;
  // Stuff for mapping color (i.e. Components to RGB)
  int ColorFlag;
  int Red;
  int Green;
  int Blue;
  char *WindowName;
  int Size[2];
  int Position[2];
  int GrayScaleHint;
};

#endif


