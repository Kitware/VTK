/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWindowLevelLookupTable.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkWindowLevelLookupTable
 * @brief   map scalar values into colors or colors to scalars; generate color table
 *
 * vtkWindowLevelLookupTable is an object that is used by mapper objects
 * to map scalar values into rgba (red-green-blue-alpha transparency)
 * color specification, or rgba into scalar values. The color table can
 * be created by direct insertion of color values, or by specifying a
 * window and level. Window / Level is used in medical imaging to specify
 * a linear greyscale ramp. The Level is the center of the ramp.  The
 * Window is the width of the ramp.
 *
 * @warning
 * vtkWindowLevelLookupTable is a reference counted object. Therefore, you
 * should always use operator "new" to construct new objects. This procedure
 * will avoid memory problems (see text).
 *
 * @sa
 * vtkLogLookupTable
*/

#ifndef vtkWindowLevelLookupTable_h
#define vtkWindowLevelLookupTable_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkLookupTable.h"

class VTKRENDERINGCORE_EXPORT vtkWindowLevelLookupTable : public vtkLookupTable
{
public:
  static vtkWindowLevelLookupTable *New();
  vtkTypeMacro(vtkWindowLevelLookupTable,vtkLookupTable);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Generate lookup table as a linear ramp between MinimumTableValue
   * and MaximumTableValue.
   */
  void Build() VTK_OVERRIDE;

  //@{
  /**
   * Set the window for the lookup table.  The window is the difference
   * between TableRange[0] and TableRange[1].
   */
  void SetWindow(double window) {
    if (window < 1e-5) { window = 1e-5; }
    this->Window = window;
    this->SetTableRange(this->Level - this->Window/2.0,
                        this->Level + this->Window/2.0); };
  vtkGetMacro(Window,double);
  //@}

  //@{
  /**
   * Set the Level for the lookup table.  The level is the average of
   * TableRange[0] and TableRange[1].
   */
  void SetLevel(double level) {
    this->Level = level;
    this->SetTableRange(this->Level - this->Window/2.0,
                        this->Level + this->Window/2.0); };
  vtkGetMacro(Level,double);
  //@}

  //@{
  /**
   * Set inverse video on or off.  You can achieve the same effect by
   * switching the MinimumTableValue and the MaximumTableValue.
   */
  void SetInverseVideo(int iv);
  vtkGetMacro(InverseVideo,int);
  vtkBooleanMacro(InverseVideo,int);
  //@}

  //@{
  /**
   * Set the minimum table value.  All lookup table entries below the
   * start of the ramp will be set to this color.  After you change
   * this value, you must re-build the lookup table.
   */
  vtkSetVector4Macro(MinimumTableValue,double);
  vtkGetVector4Macro(MinimumTableValue,double);
  //@}

  //@{
  /**
   * Set the maximum table value. All lookup table entries above the
   * end of the ramp will be set to this color.  After you change
   * this value, you must re-build the lookup table.
   */
  vtkSetVector4Macro(MaximumTableValue,double);
  vtkGetVector4Macro(MaximumTableValue,double);
  //@}

protected:
  vtkWindowLevelLookupTable(int sze=256, int ext=256);
  ~vtkWindowLevelLookupTable() VTK_OVERRIDE {}

  double Window;
  double Level;
  int InverseVideo;
  double MaximumTableValue[4];
  double MinimumTableValue[4];
private:
  vtkWindowLevelLookupTable(const vtkWindowLevelLookupTable&) VTK_DELETE_FUNCTION;
  void operator=(const vtkWindowLevelLookupTable&) VTK_DELETE_FUNCTION;
};

#endif
