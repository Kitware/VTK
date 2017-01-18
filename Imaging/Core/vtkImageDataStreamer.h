/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDataStreamer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageDataStreamer
 * @brief   Initiates streaming on image data.
 *
 * To satisfy a request, this filter calls update on its input
 * many times with smaller update extents.  All processing up stream
 * streams smaller pieces.
*/

#ifndef vtkImageDataStreamer_h
#define vtkImageDataStreamer_h

#include "vtkImagingCoreModule.h" // For export macro
#include "vtkImageAlgorithm.h"

class vtkExtentTranslator;

class VTKIMAGINGCORE_EXPORT vtkImageDataStreamer : public vtkImageAlgorithm
{
public:
  static vtkImageDataStreamer *New();
  vtkTypeMacro(vtkImageDataStreamer,vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Set how many pieces to divide the input into.
   * void SetNumberOfStreamDivisions(int num);
   * int GetNumberOfStreamDivisions();
   */
  vtkSetMacro(NumberOfStreamDivisions,int);
  vtkGetMacro(NumberOfStreamDivisions,int);
  //@}

  //@{
  /**
   * Get the extent translator that will be used to split the requests
   */
  virtual void SetExtentTranslator(vtkExtentTranslator*);
  vtkGetObjectMacro(ExtentTranslator,vtkExtentTranslator);
  //@}

  // See the vtkAlgorithm for a desciption of what these do
  int ProcessRequest(vtkInformation*,
                     vtkInformationVector**,
                     vtkInformationVector*) VTK_OVERRIDE;

protected:
  vtkImageDataStreamer();
  ~vtkImageDataStreamer() VTK_OVERRIDE;

  vtkExtentTranslator *ExtentTranslator;
  int            NumberOfStreamDivisions;
  int            CurrentDivision;
private:
  vtkImageDataStreamer(const vtkImageDataStreamer&) VTK_DELETE_FUNCTION;
  void operator=(const vtkImageDataStreamer&) VTK_DELETE_FUNCTION;
};

#endif



