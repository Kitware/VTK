/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtLabelSizeCalculator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef __vtkQtLabelSizeCalculator_h
#define __vtkQtLabelSizeCalculator_h

#include "vtkLabelSizeCalculator.h"

// .NAME vtkQtLabelSizeCalculator
// .SECTION Description
// This filter takes an input dataset, an array to process
// (which must be a string array), and a text property.
// It creates a new output array (named "LabelSize" by default) with
// 4 components per tuple that contain the width, height, horizontal
// offset, and descender height (in that order) of each string in
// the array based on placing the labels on a Qimage.
//
// Use the inherited SelectInputArrayToProcess to indicate a string array.
// In no input array is specified, the first of the following that
// is a string array is used: point scalars, cell scalars, field scalars.

class VTK_RENDERING_EXPORT vtkQtLabelSizeCalculator : public vtkLabelSizeCalculator
{
public:
  static vtkQtLabelSizeCalculator* New();
  virtual void PrintSelf( ostream& os, vtkIndent indent );
  vtkTypeRevisionMacro(vtkQtLabelSizeCalculator,vtkLabelSizeCalculator);

protected:
  vtkQtLabelSizeCalculator();
  virtual ~vtkQtLabelSizeCalculator();

  virtual vtkIntArray* LabelSizesForArray( vtkAbstractArray* labels, 
                                           vtkIntArray* types );
  //BTX
  class Internals;
  Internals* Implementation;
  //ETX

private:
  vtkQtLabelSizeCalculator( const vtkQtLabelSizeCalculator& ); // Not implemented.
  void operator = ( const vtkQtLabelSizeCalculator& ); // Not implemented.
};

#endif // __vtkQtLabelSizeCalculator_h
