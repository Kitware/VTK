/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArrowSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkArrowSource
 * @brief   Appends a cylinder to a cone to form an arrow.
 *
 * vtkArrowSource was intended to be used as the source for a glyph.
 * The shaft base is always at (0,0,0). The arrow tip is always at (1,0,0). If
 * "Invert" is true, then the ends are flipped i.e. tip is at (0,0,0) while
 * base is at (1, 0, 0).
 * The resolution of the cone and shaft can be set and default to 6.
 * The radius of the cone and shaft can be set and default to 0.03 and 0.1.
 * The length of the tip can also be set, and defaults to 0.35.
*/

#ifndef vtkArrowSource_h
#define vtkArrowSource_h

#include "vtkFiltersSourcesModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class VTKFILTERSSOURCES_EXPORT vtkArrowSource : public vtkPolyDataAlgorithm
{
public:
  /**
   * Construct cone with angle of 45 degrees.
   */
  static vtkArrowSource *New();

  vtkTypeMacro(vtkArrowSource,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set the length, and radius of the tip.  They default to 0.35 and 0.1
   */
  vtkSetClampMacro(TipLength,double,0.0,1.0);
  vtkGetMacro(TipLength,double);
  vtkSetClampMacro(TipRadius,double,0.0,10.0);
  vtkGetMacro(TipRadius,double);
  //@}

  //@{
  /**
   * Set the resolution of the tip.  The tip behaves the same as a cone.
   * Resoultion 1 gives a single triangle, 2 gives two crossed triangles.
   */
  vtkSetClampMacro(TipResolution,int,1,128);
  vtkGetMacro(TipResolution,int);
  //@}

  //@{
  /**
   * Set the radius of the shaft.  Defaults to 0.03.
   */
  vtkSetClampMacro(ShaftRadius,double,0.0,5.0);
  vtkGetMacro(ShaftRadius,double);
  //@}

  //@{
  /**
   * Set the resolution of the shaft.  2 gives a rectangle.
   * I would like to extend the cone to produce a line,
   * but this is not an option now.
   */
  vtkSetClampMacro(ShaftResolution,int,0,128);
  vtkGetMacro(ShaftResolution,int);
  //@}

  //@{
  /**
   * Inverts the arrow direction. When set to true, base is at (1, 0, 0) while the
   * tip is at (0, 0, 0). The default is false, i.e. base at (0, 0, 0) and the tip
   * at (1, 0, 0).
   */
  vtkBooleanMacro(Invert, bool);
  vtkSetMacro(Invert, bool);
  vtkGetMacro(Invert, bool);
  //@}

protected:
  vtkArrowSource();
  ~vtkArrowSource() override {}

  int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;

  int TipResolution;
  double TipLength;
  double TipRadius;

  int ShaftResolution;
  double ShaftRadius;
  bool Invert;


private:
  vtkArrowSource(const vtkArrowSource&) = delete;
  void operator=(const vtkArrowSource&) = delete;
};

#endif


