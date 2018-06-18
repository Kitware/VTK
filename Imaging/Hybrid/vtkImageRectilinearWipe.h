/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageRectilinearWipe.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageRectilinearWipe
 * @brief   make a rectilinear combination of two images.
 *
 * vtkImageRectilinearWipe makes a rectilinear combination of two
 * images. The two input images must correspond in size, scalar type and
 * number of components.
 * The resulting image has four possible configurations
 * called:
 *   Quad - alternate input 0 and input 1 horizontally and
 *     vertically. Select this with SetWipeModeToQuad. The Position
 *     specifies the location of the quad intersection.
 *   Corner - 3 of one input and 1 of the other. Select the location of
 *     input 0 with with SetWipeModeToLowerLeft, SetWipeModeToLowerRight,
 *     SetWipeModeToUpperLeft and SetWipeModeToUpperRight. The Position
 *     selects the location of the corner.
 *   Horizontal - alternate input 0 and input 1 with a vertical
 *     split. Select this with SetWipeModeToHorizontal. Position[0]
 *     specifies the location of the vertical transition between input 0
 *     and input 1.
 *   Vertical - alternate input 0 and input 1 with a horizontal
 *     split. Only the y The intersection point of the rectilinear points
 *     is controlled with the Point ivar.
 *
 * @par Thanks:
 * This work was supported by PHS Research Grant No. 1 P41 RR13218-01
 * from the National Center for Research Resources.
 *
 * @sa
 * vtkImageCheckerboard
*/

#ifndef vtkImageRectilinearWipe_h
#define vtkImageRectilinearWipe_h

#include "vtkImagingHybridModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

#define VTK_WIPE_QUAD 0
#define VTK_WIPE_HORIZONTAL 1
#define VTK_WIPE_VERTICAL 2
#define VTK_WIPE_LOWER_LEFT 3
#define VTK_WIPE_LOWER_RIGHT 4
#define VTK_WIPE_UPPER_LEFT 5
#define VTK_WIPE_UPPER_RIGHT 6

class VTKIMAGINGHYBRID_EXPORT vtkImageRectilinearWipe : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageRectilinearWipe *New();
  vtkTypeMacro(vtkImageRectilinearWipe,vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set/Get the location of the image transition. Note that position is
   * specified in pixels.
   */
  vtkSetVector2Macro(Position,int);
  vtkGetVectorMacro(Position,int,2);
  //@}

  //@{
  /**
   * Set/Get the location of the wipe axes. The default is X,Y (ie vector
   * values of 0 and 1).
   */
  vtkSetVector2Macro(Axis,int);
  vtkGetVectorMacro(Axis,int,2);
  //@}

  /**
   * Set the two inputs to this filter.
   */
  virtual void SetInput1Data(vtkDataObject *in) { this->SetInputData(0,in); }
  virtual void SetInput2Data(vtkDataObject *in) { this->SetInputData(1,in); }

  //@{
  /**
   * Specify the wipe mode. This mode determnis how input 0 and input
   * 1 are combined to produce the output. Each mode uses one or both
   * of the values stored in Position.
   * SetWipeToQuad - alternate input 0 and input 1 horizontally and
   * vertically. The Position specifies the location of the quad
   * intersection.
   * SetWipeToLowerLeft{LowerRight,UpperLeft.UpperRight} - 3 of one
   * input and 1 of the other. Select the location of input 0 to the
   * LowerLeft{LowerRight,UpperLeft,UpperRight}. Position
   * selects the location of the corner.
   * SetWipeToHorizontal - alternate input 0 and input 1 with a vertical
   * split. Position[0] specifies the location of the vertical
   * transition between input 0 and input 1.
   * SetWipeToVertical - alternate input 0 and input 1 with a
   * horizontal split. Position[1] specifies the location of the
   * horizontal transition between input 0 and input 1.
   */
  vtkSetClampMacro(Wipe,int,VTK_WIPE_QUAD,VTK_WIPE_UPPER_RIGHT);
  vtkGetMacro(Wipe,int);
  void SetWipeToQuad()
    {this->SetWipe(VTK_WIPE_QUAD);}
  void SetWipeToHorizontal()
    {this->SetWipe(VTK_WIPE_HORIZONTAL);}
  void SetWipeToVertical()
    {this->SetWipe(VTK_WIPE_VERTICAL);}
  void SetWipeToLowerLeft()
    {this->SetWipe(VTK_WIPE_LOWER_LEFT);}
  void SetWipeToLowerRight()
    {this->SetWipe(VTK_WIPE_LOWER_RIGHT);}
  void SetWipeToUpperLeft()
    {this->SetWipe(VTK_WIPE_UPPER_LEFT);}
  void SetWipeToUpperRight()
    {this->SetWipe(VTK_WIPE_UPPER_RIGHT);}
  //@}

protected:
  vtkImageRectilinearWipe();
  ~vtkImageRectilinearWipe() override {}

  void ThreadedRequestData(vtkInformation *request,
                                   vtkInformationVector **inputVector,
                                   vtkInformationVector *outputVector,
                                   vtkImageData ***inData,
                                   vtkImageData **outData,
                                   int extent[6], int threadId) override;

  int Position[2];
  int Wipe;
  int Axis[2];

private:
  vtkImageRectilinearWipe(const vtkImageRectilinearWipe&) = delete;
  void operator=(const vtkImageRectilinearWipe&) = delete;
};

#endif
