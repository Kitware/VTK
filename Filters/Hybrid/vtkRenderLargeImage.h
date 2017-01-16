/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderLargeImage.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkRenderLargeImage
 * @brief   Use tiling to generate a large rendering
 *
 * vtkRenderLargeImage provides methods needed to read a region from a file.
*/

#ifndef vtkRenderLargeImage_h
#define vtkRenderLargeImage_h

#include "vtkFiltersHybridModule.h" // For export macro
#include "vtkAlgorithm.h"
#include "vtkImageData.h" // makes things a bit easier

class vtkRenderer;
class vtkActor2DCollection;
class vtkCollection;
class vtkRenderLargeImage2DHelperClass;

class VTKFILTERSHYBRID_EXPORT vtkRenderLargeImage : public vtkAlgorithm
{
public:
  static vtkRenderLargeImage *New();
  vtkTypeMacro(vtkRenderLargeImage,vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * The magnification of the current render window
   */
  vtkSetMacro(Magnification,int);
  vtkGetMacro(Magnification,int);
  //@}

  /**
   * Indicates what renderer to get the pixel data from.
   */
  virtual void SetInput(vtkRenderer*);

  //@{
  /**
   * Returns which renderer is being used as the source for the pixel data.
   */
  vtkGetObjectMacro(Input,vtkRenderer);
  //@}

  /**
   * Get the output data object for a port on this algorithm.
   */
  vtkImageData* GetOutput();

  /**
   * see vtkAlgorithm for details
   */
  int ProcessRequest(vtkInformation*,
                             vtkInformationVector**,
                             vtkInformationVector*) VTK_OVERRIDE;

protected:
  vtkRenderLargeImage();
  ~vtkRenderLargeImage() VTK_OVERRIDE;

  int Magnification;
  vtkRenderer *Input;
  void RequestData(vtkInformation *,
                   vtkInformationVector **, vtkInformationVector *);
  void RequestInformation (vtkInformation *,
                           vtkInformationVector **, vtkInformationVector *);

  // see algorithm for more info
  int FillOutputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;

  // Adjust the coordinates of all 2D actors to fit new window size
  void Rescale2DActors();
  // Shift each actor according to the tile we are rendering
  void Shift2DActors(int x, int y);
  // put them all back to their previous state when finished.
  void Restore2DActors();
  // 2D Actors need to be rescaled and shifted about for each tile
  // use this helper class to make life easier.
  vtkRenderLargeImage2DHelperClass *StoredData;

private:
  vtkRenderLargeImage(const vtkRenderLargeImage&) VTK_DELETE_FUNCTION;
  void operator=(const vtkRenderLargeImage&) VTK_DELETE_FUNCTION;
};

#endif
