/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLabelSizeCalculator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkLabelSizeCalculator
// .SECTION Description
// This filter takes an input dataset, an array to process
// (which must be a string array), and a text property.
// It creates a new output array (named "LabelSize" by default) with
// 4 components per tuple that contain the width, height, horizontal
// offset, and descender height (in that order) of each string in
// the array.
//
// Use the inherited SelectInputArrayToProcess to indicate a string array.
// In no input array is specified, the first of the following that
// is a string array is used: point scalars, cell scalars, field scalars.
//
// The second input array to process is an array specifying the type of
// each label. Different label types may have different font properties.
// This array must be a vtkIntArray.
// Any type that does not map to a font property that was set will
// be set to the type 0's type property.

#ifndef vtkLabelSizeCalculator_h
#define vtkLabelSizeCalculator_h

#include "vtkRenderingLabelModule.h" // For export macro
#include "vtkPassInputTypeAlgorithm.h"

class vtkIntArray;
class vtkFreeTypeUtilities;
class vtkStringArray;
class vtkTextProperty;

class VTKRENDERINGLABEL_EXPORT vtkLabelSizeCalculator : public vtkPassInputTypeAlgorithm
{
public:
  static vtkLabelSizeCalculator* New();
  virtual void PrintSelf( ostream& os, vtkIndent indent );
  vtkTypeMacro(vtkLabelSizeCalculator,vtkPassInputTypeAlgorithm);

  // Description:
  // Get/Set the font used compute label sizes.
  // This defaults to "Arial" at 12 points.
  // If type is provided, it refers to the type of the text label provided
  // in the optional label type array. The default type is type 0.
  virtual void SetFontProperty(vtkTextProperty* fontProp, int type = 0);
  virtual vtkTextProperty* GetFontProperty(int type = 0);

  // Description:
  // The name of the output array containing text label sizes
  // This defaults to "LabelSize"
  vtkSetStringMacro(LabelSizeArrayName);
  vtkGetStringMacro(LabelSizeArrayName);

protected:
  vtkLabelSizeCalculator();
  virtual ~vtkLabelSizeCalculator();

  virtual int FillInputPortInformation( int port, vtkInformation* info );
  virtual int RequestData(
    vtkInformation* request,
    vtkInformationVector** inInfo,
    vtkInformationVector* outInfo );

  virtual vtkIntArray* LabelSizesForArray( vtkAbstractArray* labels, vtkIntArray* types );

  virtual void SetFontUtil( vtkFreeTypeUtilities* fontProp );
  vtkGetObjectMacro(FontUtil,vtkFreeTypeUtilities);

  vtkFreeTypeUtilities* FontUtil;
  char* LabelSizeArrayName;

  //BTX
  class Internals;
  Internals* Implementation;
  //ETX

private:
  vtkLabelSizeCalculator( const vtkLabelSizeCalculator& ); // Not implemented.
  void operator = ( const vtkLabelSizeCalculator& ); // Not implemented.
};

#endif // vtkLabelSizeCalculator_h
