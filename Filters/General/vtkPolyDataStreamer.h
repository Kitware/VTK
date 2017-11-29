/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataStreamer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPolyDataStreamer
 * @brief   Streamer appends input pieces to the output.
 *
 * vtkPolyDataStreamer initiates streaming by requesting pieces from its
 * single input it appends these pieces to the requested output.
 * Note that since vtkPolyDataStreamer uses an append filter, all the
 * polygons generated have to be kept in memory before rendering. If
 * these do not fit in the memory, it is possible to make the vtkPolyDataMapper
 * stream. Since the mapper will render each piece separately, all the
 * polygons do not have to stored in memory.
 * @attention
 * The output may be slightly different if the pipeline does not handle
 * ghost cells properly (i.e. you might see seames between the pieces).
 * @sa
 * vtkAppendFilter
*/

#ifndef vtkPolyDataStreamer_h
#define vtkPolyDataStreamer_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkStreamerBase.h"

class vtkAppendPolyData;

class VTKFILTERSGENERAL_EXPORT vtkPolyDataStreamer : public vtkStreamerBase
{
public:
  static vtkPolyDataStreamer *New();

  vtkTypeMacro(vtkPolyDataStreamer,vtkStreamerBase);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set the number of pieces to divide the problem into.
   */
  void SetNumberOfStreamDivisions(int num);
  int GetNumberOfStreamDivisions()
  {
    return this->NumberOfPasses;
  }
  //@}

  //@{
  /**
   * By default, this option is off.  When it is on, cell scalars are generated
   * based on which piece they are in.
   */
  vtkSetMacro(ColorByPiece, vtkTypeBool);
  vtkGetMacro(ColorByPiece, vtkTypeBool);
  vtkBooleanMacro(ColorByPiece, vtkTypeBool);
  //@}


protected:
  vtkPolyDataStreamer();
  ~vtkPolyDataStreamer() override;

  // see algorithm for more info
  int FillOutputPortInformation(int port, vtkInformation* info) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  int RequestUpdateExtent(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;

  int ExecutePass(vtkInformationVector **inputVector,
                  vtkInformationVector *outputVector) override;

  int PostExecute(vtkInformationVector **inputVector,
                  vtkInformationVector *outputVector) override;

  vtkTypeBool ColorByPiece;
private:
  vtkPolyDataStreamer(const vtkPolyDataStreamer&) = delete;
  void operator=(const vtkPolyDataStreamer&) = delete;

  vtkAppendPolyData* Append;
};

#endif
