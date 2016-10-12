/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHedgeHog.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkHedgeHog
 * @brief   create oriented lines from vector data
 *
 * vtkHedgeHog creates oriented lines from the input data set. Line
 * length is controlled by vector (or normal) magnitude times scale
 * factor. If VectorMode is UseNormal, normals determine the orientation
 * of the lines. Lines are colored by scalar data, if available.
*/

#ifndef vtkHedgeHog_h
#define vtkHedgeHog_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

#define VTK_USE_VECTOR 0
#define VTK_USE_NORMAL 1

class VTKFILTERSCORE_EXPORT vtkHedgeHog : public vtkPolyDataAlgorithm
{
public:
  static vtkHedgeHog *New();
  vtkTypeMacro(vtkHedgeHog,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Set scale factor to control size of oriented lines.
   */
  vtkSetMacro(ScaleFactor,double);
  vtkGetMacro(ScaleFactor,double);
  //@}

  //@{
  /**
   * Specify whether to use vector or normal to perform vector operations.
   */
  vtkSetMacro(VectorMode,int);
  vtkGetMacro(VectorMode,int);
  void SetVectorModeToUseVector() {this->SetVectorMode(VTK_USE_VECTOR);};
  void SetVectorModeToUseNormal() {this->SetVectorMode(VTK_USE_NORMAL);};
  const char *GetVectorModeAsString();
  //@}

  //@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the vtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings.
   */
  vtkSetMacro(OutputPointsPrecision,int);
  vtkGetMacro(OutputPointsPrecision,int);
  //@}

protected:
  vtkHedgeHog();
  ~vtkHedgeHog() VTK_OVERRIDE {}

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;
  int FillInputPortInformation(int port, vtkInformation *info) VTK_OVERRIDE;
  double ScaleFactor;
  int VectorMode; // Orient/scale via normal or via vector data
  int OutputPointsPrecision;

private:
  vtkHedgeHog(const vtkHedgeHog&) VTK_DELETE_FUNCTION;
  void operator=(const vtkHedgeHog&) VTK_DELETE_FUNCTION;
};

//@{
/**
 * Return the vector mode as a character string.
 */
inline const char *vtkHedgeHog::GetVectorModeAsString(void)
{
  if ( this->VectorMode == VTK_USE_VECTOR)
  {
    return "UseVector";
  }
  else if ( this->VectorMode == VTK_USE_NORMAL)
  {
    return "UseNormal";
  }
  else
  {
    return "Unknown";
  }
}
#endif
//@}
