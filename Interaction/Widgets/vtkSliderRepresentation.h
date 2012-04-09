/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSliderRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSliderRepresentation - abstract class defines the representation for a vtkSliderWidget
// .SECTION Description
// This abstract class is used to specify how the vtkSliderWidget should
// interact with representations of the vtkSliderWidget. This class may be
// subclassed so that alternative representations can be created. The class
// defines an API, and a default implementation, that the vtkSliderWidget
// interacts with to render itself in the scene.

// .SECTION See Also
// vtkSliderWidget


#ifndef __vtkSliderRepresentation_h
#define __vtkSliderRepresentation_h

#include "vtkWidgetRepresentation.h"


class VTK_WIDGETS_EXPORT vtkSliderRepresentation : public vtkWidgetRepresentation
{
public:
  // Description:
  // Standard methods for the class.
  vtkTypeMacro(vtkSliderRepresentation,vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the current value for the widget. The value should lie between
  // the minimum and maximum values.
  void SetValue(double value);
  vtkGetMacro(Value,double);
  
  // Description:
  // Set the current minimum value that the slider can take. Setting the
  // minimum value greater than the maximum value will cause the maximum
  // value to grow to (minimum value + 1).
  void SetMinimumValue(double value);
  vtkGetMacro(MinimumValue,double);
  
  // Description:
  // Set the current maximum value that the slider can take. Setting the
  // maximum value less than the minimum value will cause the minimum
  // value to change to (maximum value - 1).
  void SetMaximumValue(double value);
  vtkGetMacro(MaximumValue,double);
  
  // Description: 
  // Specify the length of the slider shape (in normalized display coordinates
  // [0.01,0.5]). The slider length by default is 0.05.
  vtkSetClampMacro(SliderLength,double,0.01,0.5);
  vtkGetMacro(SliderLength,double);
  
  // Description:
  // Set the width of the slider in the directions orthogonal to the
  // slider axis. Using this it is possible to create ellipsoidal and hockey
  // puck sliders (in some subclasses). By default the width is 0.05.
  vtkSetClampMacro(SliderWidth,double,0.0,1.0);
  vtkGetMacro(SliderWidth,double);
  
  // Description:
  // Set the width of the tube (in normalized display coordinates) on which
  // the slider moves. By default the width is 0.05.
  vtkSetClampMacro(TubeWidth,double,0.0,1.0);
  vtkGetMacro(TubeWidth,double);
  
  // Description:
  // Specify the length of each end cap (in normalized coordinates
  // [0.0,0.25]). By default the length is 0.025. If the end cap length
  // is set to 0.0, then the end cap will not display at all.
  vtkSetClampMacro(EndCapLength,double,0.0,0.25);
  vtkGetMacro(EndCapLength,double);
  
  // Description:
  // Specify the width of each end cap (in normalized coordinates
  // [0.0,0.25]). By default the width is twice the tube width. 
  vtkSetClampMacro(EndCapWidth,double,0.0,0.25);
  vtkGetMacro(EndCapWidth,double);
  
  // Description:
  // Specify the label text for this widget. If the value is not set, or set
  // to the empty string "", then the label text is not displayed.
  virtual void SetTitleText(const char*) {}
  virtual const char* GetTitleText() {return NULL;}

  // Description:
  // Set/Get the format with which to print the slider value.
  vtkSetStringMacro(LabelFormat);
  vtkGetStringMacro(LabelFormat);

  // Description:
  // Specify the relative height of the label as compared to the length of the
  // slider. 
  vtkSetClampMacro(LabelHeight,double,0.0,2.0);
  vtkGetMacro(LabelHeight,double);

  // Description:
  // Specify the relative height of the title as compared to the length of the
  // slider. 
  vtkSetClampMacro(TitleHeight,double,0.0,2.0);
  vtkGetMacro(TitleHeight,double);

  // Description:
  // Indicate whether the slider text label should be displayed. This is
  // a number corresponding to the current Value of this widget.
  vtkSetMacro(ShowSliderLabel,int);
  vtkGetMacro(ShowSliderLabel,int);
  vtkBooleanMacro(ShowSliderLabel,int);

  // Description:
  // Methods to interface with the vtkSliderWidget. Subclasses of this class 
  // actually do something.
  virtual double GetCurrentT()
    {return this->CurrentT;}
  virtual double GetPickedT()
    {return this->PickedT;}

//BTX
  // Enums are used to describe what is selected
  enum _InteractionState
  {
    Outside=0,
    Tube,
    LeftCap,
    RightCap,
    Slider
  };
//ETX

protected:
  vtkSliderRepresentation();
  ~vtkSliderRepresentation();

  // Values
  double Value;  
  double MinimumValue;
  double MaximumValue;
  
  // More ivars controlling the appearance of the widget
  double SliderLength;
  double SliderWidth;
  double EndCapLength;
  double EndCapWidth;
  double TubeWidth;

  // The current parametric coordinate
  double CurrentT;
  double PickedT;

  // both the title and label
  int    ShowSliderLabel;
  char  *LabelFormat;
  double LabelHeight;
  double TitleHeight;

private:
  vtkSliderRepresentation(const vtkSliderRepresentation&);  //Not implemented
  void operator=(const vtkSliderRepresentation&);  //Not implemented
};

#endif
