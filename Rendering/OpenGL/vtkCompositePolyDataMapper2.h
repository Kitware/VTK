/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositePolyDataMapper2.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCompositePolyDataMapper2
 * @brief   mapper for composite dataset consisting
 * of polygonal data.
 *
 * vtkCompositePolyDataMapper2 is similar to vtkCompositePolyDataMapper except
 * that instead of creating individual mapper for each block in the composite
 * dataset, it iterates over the blocks internally.
*/

#ifndef vtkCompositePolyDataMapper2_h
#define vtkCompositePolyDataMapper2_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkPainterPolyDataMapper.h"
#include "vtkSmartPointer.h" // for vtkSmartPointer

class vtkCompositeDataDisplayAttributes;

class VTKRENDERINGOPENGL_EXPORT vtkCompositePolyDataMapper2 : public vtkPainterPolyDataMapper
{
public:
  static vtkCompositePolyDataMapper2* New();
  vtkTypeMacro(vtkCompositePolyDataMapper2, vtkPainterPolyDataMapper);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Returns if the mapper does not expect to have translucent geometry. This
   * may happen when using ScalarMode is set to not map scalars i.e. render the
   * scalar array directly as colors and the scalar array has opacity i.e. alpha
   * component. Note that even if this method returns true, an actor may treat
   * the geometry as translucent since a constant translucency is set on the
   * property, for example.
   * Overridden to use the actual data and ScalarMode to determine if we have
   * opaque geometry.
   */
  bool GetIsOpaque() VTK_OVERRIDE;

  //@{
  /**
   * Set/get the composite data set attributes.
   */
  void SetCompositeDataDisplayAttributes(vtkCompositeDataDisplayAttributes *attributes);
  vtkCompositeDataDisplayAttributes* GetCompositeDataDisplayAttributes();
  //@}

  //@{
  /**
   * Set/get the visibility for a block given its flat index.
   */
  void SetBlockVisibility(unsigned int index, bool visible);
  bool GetBlockVisibility(unsigned int index) const;
  void RemoveBlockVisibility(unsigned int index);
  void RemoveBlockVisibilites();
  //@}

  //@{
  /**
   * Set/get the color for a block given its flat index.
   */
  void SetBlockColor(unsigned int index, double color[3]);
  void SetBlockColor(unsigned int index, double r, double g, double b)
  {
    double color[3] = {r, g, b};
    this->SetBlockColor(index, color);
  }
  double* GetBlockColor(unsigned int index);
  void RemoveBlockColor(unsigned int index);
  void RemoveBlockColors();
  //@}

  //@{
  /**
   * Set/get the opacity for a block given its flat index.
   */
  void SetBlockOpacity(unsigned int index, double opacity);
  double GetBlockOpacity(unsigned int index);
  void RemoveBlockOpacity(unsigned int index);
  void RemoveBlockOpacities();
  //@}

protected:
  vtkCompositePolyDataMapper2();
  ~vtkCompositePolyDataMapper2() VTK_OVERRIDE;

  /**
   * We need to override this method because the standard streaming
   * demand driven pipeline is not what we want - we are expecting
   * hierarchical data as input
   */
  vtkExecutive* CreateDefaultExecutive() VTK_OVERRIDE;

  /**
   * Need to define the type of data handled by this mapper.
   */
  int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;

  /**
   * Need to loop over the hierarchy to compute bounds
   */
  void ComputeBounds() VTK_OVERRIDE;

  /**
   * Called when the PainterInformation becomes obsolete. Overridden to pass
   * CompositeDataDisplayAttributes to the painters.
   */
  void UpdatePainterInformation() VTK_OVERRIDE;

  /**
   * Time stamp for computation of bounds.
   */
  vtkTimeStamp BoundsMTime;

  /**
   * Composite data set attributes.
   */
  vtkSmartPointer<vtkCompositeDataDisplayAttributes> CompositeAttributes;

  vtkPainter* SelectionCompositePainter;

private:
  vtkMTimeType LastOpaqueCheckTime;
  bool LastOpaqueCheckValue;

private:
  vtkCompositePolyDataMapper2(const vtkCompositePolyDataMapper2&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCompositePolyDataMapper2&) VTK_DELETE_FUNCTION;

};

#endif
