/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCornerAnnotation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCornerAnnotation - text annotation in four corners
// .SECTION Description
// This is an annotation object that manages four text actors / mappers
// to provide annotation in the four corners of a viewport
//
// .SECTION See Also
// vtkActor2D vtkTextMapper

#ifndef __vtkCornerAnnotation_h
#define __vtkCornerAnnotation_h

#include "vtkActor2D.h"

class vtkTextMapper;
class vtkImageMapToWindowLevelColors;
class vtkImageActor;
class vtkTextProperty;

class VTK_HYBRID_EXPORT vtkCornerAnnotation : public vtkActor2D
{
public:
  vtkTypeRevisionMacro(vtkCornerAnnotation,vtkActor2D);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Instantiate object with a rectangle in normaled view coordinates
  // of (0.2,0.85, 0.8, 0.95).
  static vtkCornerAnnotation *New();
  
  // Description:
  // Draw the scalar bar and annotation text to the screen.
  int RenderOpaqueGeometry(vtkViewport* viewport);
  int RenderTranslucentGeometry(vtkViewport* ) {return 0;};
  int RenderOverlay(vtkViewport* viewport);

  // Description:
  // Set/Get the maximum height of a line of text as a 
  // percentage of the vertical area allocated to this
  // scaled text actor. Defaults to 1.0
  vtkSetMacro(MaximumLineHeight,float);
  vtkGetMacro(MaximumLineHeight,float);
  
  // Description:
  // Set/Get the minimum size font that will be shown.
  // If the font drops below this size it will not be rendered.
  vtkSetMacro(MinimumFontSize,int);
  vtkGetMacro(MinimumFontSize,int);

  // Description:
  // Set/Get font scaling factors
  // The font size, f, is calculated as the largest possible value
  // such that the annotations for the given viewport do not overlap. 
  // This font size is scaled non-linearly with the viewport size,
  // to maintain an acceptable readable size at larger viewport sizes, 
  // without being too big.
  // f' = linearScale * pow(f,nonlinearScale)
  vtkSetMacro( LinearFontScaleFactor, float );
  vtkGetMacro( LinearFontScaleFactor, float );
  vtkSetMacro( NonlinearFontScaleFactor, float );
  vtkGetMacro( NonlinearFontScaleFactor, float );

  // Description:
  // Release any graphics resources that are being consumed by this actor.
  // The parameter window could be used to determine which graphic
  // resources to release.
  virtual void ReleaseGraphicsResources(vtkWindow *);

  // Description:
  // Set/Get the text to be displayed for each corner
  void SetText(int i, const char *text);
  char* GetText(int i);
  void ClearAllTexts();
  void CopyAllTextsFrom(vtkCornerAnnotation *ca);

  // Description:
  // Set an image actor to look at for slice information
  void SetImageActor(vtkImageActor*);
  vtkGetObjectMacro(ImageActor,vtkImageActor);
  
  // Description:
  // Set an instance of vtkImageMapToWindowLevelColors to use for
  // looking at window level changes
  void SetWindowLevel(vtkImageMapToWindowLevelColors*);
  vtkGetObjectMacro(WindowLevel,vtkImageMapToWindowLevelColors);

  // Description:
  // Set the value to shift the level by.
  vtkSetMacro(LevelShift, float);
  vtkGetMacro(LevelShift, float);
  
  // Description:
  // Set the value to scale the level by.
  vtkSetMacro(LevelScale, float);
  vtkGetMacro(LevelScale, float);
  
  // Description:
  // Set/Get the text property of all corners.
  virtual void SetTextProperty(vtkTextProperty *p);
  vtkGetObjectMacro(TextProperty,vtkTextProperty);

  // Description:
  // Even if there is an image actor, should <slice> and <image> be displayed?
  vtkBooleanMacro(ShowSliceAndImage, int);
  vtkSetMacro(ShowSliceAndImage, int);
  vtkGetMacro(ShowSliceAndImage, int);
  
protected:
  vtkCornerAnnotation();
  ~vtkCornerAnnotation();

  float MaximumLineHeight;

  vtkTextProperty *TextProperty;

  vtkImageMapToWindowLevelColors *WindowLevel;
  float LevelShift;
  float LevelScale;
  vtkImageActor *ImageActor;
  vtkImageActor *LastImageActor;

  char *CornerText[4];
  
  int FontSize;
  vtkActor2D    *TextActor[4];
  vtkTimeStamp   BuildTime;
  int            LastSize[2];
  vtkTextMapper *TextMapper[4];

  int   MinimumFontSize;
  float LinearFontScaleFactor;
  float NonlinearFontScaleFactor;
  
  int ShowSliceAndImage;
  
  // search for replacable tokens and replace
  void TextReplace(vtkImageActor *ia,  vtkImageMapToWindowLevelColors *wl);
private:
  vtkCornerAnnotation(const vtkCornerAnnotation&);  // Not implemented.
  void operator=(const vtkCornerAnnotation&);  // Not implemented.
};


#endif



