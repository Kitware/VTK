/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiGroupPolyDataMapper2.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMultiGroupPolyDataMapper2 
// .SECTION Description

#ifndef __vtkMultiGroupPolyDataMapper2_h
#define __vtkMultiGroupPolyDataMapper2_h

#include "vtkPainterPolyDataMapper.h"

class VTK_RENDERING_EXPORT vtkMultiGroupPolyDataMapper2 : public vtkPainterPolyDataMapper
{
public:
  static vtkMultiGroupPolyDataMapper2* New();
  vtkTypeRevisionMacro(vtkMultiGroupPolyDataMapper2, vtkPainterPolyDataMapper);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Implemented by sub classes. Actual rendering is done here.
  virtual void RenderPiece(vtkRenderer *ren, vtkActor *act);

  // Description:
  // Standard vtkProp method to get 3D bounds of a 3D prop
  double *GetBounds();
  void GetBounds(double bounds[6]) { this->Superclass::GetBounds( bounds ); };  


  // Description:
  // This calls RenderPiece (in a for loop is streaming is necessary).
  // Basically a reimplementation for vtkPolyDataMapper::Render() since we don't
  // want it to give up when vtkCompositeDataSet is encountered.
  virtual void Render(vtkRenderer *ren, vtkActor *act);

  // Description:
  // When set, each block is colored with a different color. Note that scalar
  // coloring will be ignored.
  vtkSetMacro(ColorBlocks, int);
  vtkGetMacro(ColorBlocks, int);

//BTX
protected:
  vtkMultiGroupPolyDataMapper2();
  ~vtkMultiGroupPolyDataMapper2();

  // Description:
  // We need to override this method because the standard streaming
  // demand driven pipeline is not what we want - we are expecting
  // hierarchical data as input
  vtkExecutive* CreateDefaultExecutive();

  // Description:
  // Need to define the type of data handled by this mapper.
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  // Description:
  // Need to loop over the hierarchy to compute bounds
  void ComputeBounds();

  // Description:
  // Called when the PainterInformation becomes obsolete. 
  // It is called before the Render is initiated on the Painter.
  virtual void UpdatePainterInformation();

  // Description:
  // Time stamp for computation of bounds.
  vtkTimeStamp BoundsMTime;

  int ColorBlocks;
private:
  vtkMultiGroupPolyDataMapper2(const vtkMultiGroupPolyDataMapper2&); // Not implemented.
  void operator=(const vtkMultiGroupPolyDataMapper2&); // Not implemented.
//ETX
};

#endif


