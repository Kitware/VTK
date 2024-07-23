// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHyperTreeGridMapper
 * @brief   map vtkHyperTreeGrid to graphics primitives

 * vtkHyperTreeGridMapper is a class that maps polygonal data (i.e., vtkHyperTreeGrid)
 * to graphics primitives. vtkHyperTreeGridMapper serves as a superclass for
 * device-specific poly data mappers, that actually do the mapping to the
 * rendering/graphics hardware/software.

 * By default, this class use an Adaptive GeometryFilter that extract only
 * the part of the geometry to render. Be careful as this implies that new
 * render may trigger an update of the pipeline to get the new part of the
 * geometry to render.

 * Note: this class has its own module to avoid cyclic dependency between Rendering Core
 * and Filters Hybrid
 * * It need Filters Hybrid for Adaptive2DGeometryFilter
 * * Filters Hybrid need Rendering Core because of Adaptive2DGeometryFilter
 */

#ifndef vtkHyperTreeGridMapper_h
#define vtkHyperTreeGridMapper_h

#include "vtkMapper.h"
#include "vtkSetGet.h"       // Get macro
#include "vtkSmartPointer.h" // For vtkSmartPointer

#include "vtkRenderingHyperTreeGridModule.h" // For export macro

#include <set> // For std::set

VTK_ABI_NAMESPACE_BEGIN
class vtkHyperTreeGrid;
class vtkCompositeDataSet;
class vtkCompositeDataDisplayAttributes;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkRenderWindow;
class vtkRenderer;

class VTKRENDERINGHYPERTREEGRID_EXPORT vtkHyperTreeGridMapper : public vtkMapper
{
public:
  static vtkHyperTreeGridMapper* New();
  vtkTypeMacro(vtkHyperTreeGridMapper, vtkMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set the connection for the given input port index.  Each input
   * port of a filter has a specific purpose.  A port may have zero or
   * more connections and the required number is specified by each
   * filter.  Setting the connection with this method removes all
   * other connections from the port.  To add more than one connection
   * use AddInputConnection().
   * The input for the connection is the output port of another
   * filter, which is obtained with GetOutputPort().  Typical usage is
   * filter2->SetInputConnection(0, filter1->GetOutputPort(0)).
   */
  using Superclass::SetInputConnection;
  void SetInputDataObject(int port, vtkDataObject* input) override;
  void SetInputDataObject(vtkDataObject* input) override;
  ///@}

  ///@{
  /**
   * For this mapper, the bounds correspond to the output for the
   * internal surface filter which may be restricted to the Camera frustum
   * if UseCameraFrustum is on.
   * Bounds take block visibility into account for composite inputs.
   */
  double* GetBounds() override;
  void GetBounds(double bounds[6]) override;
  void GetBoundsComposite(double bounds[6]);
  ///@}

  ///@{
  /**
   * This boolean control whether or not the mapping should adapt
   * to the Camera frustum during the rendering. Setting this variable
   * to true (default) should provide increased performances.
   */
  vtkGetMacro(UseAdaptiveDecimation, bool);
  vtkSetMacro(UseAdaptiveDecimation, bool);
  vtkBooleanMacro(UseAdaptiveDecimation, bool);
  ///@}

  ///@{
  /**
   * Set/get the composite data set attributes.
   * This is forwarded to the internal composite polydata mapper if we're using one.
   */
  void SetCompositeDataDisplayAttributes(vtkCompositeDataDisplayAttributes* attributes);
  vtkCompositeDataDisplayAttributes* GetCompositeDataDisplayAttributes();
  ///@}

  ///@{
  /**
   * Set/get the visibility for a block given its flat index.
   * Only works for subclasses whose mapper is composite.
   * CompositeDataDisplayAttributes needs to be set for visibilities to be applied.
   */
  void SetBlockVisibility(unsigned int index, bool visible);
  bool GetBlockVisibility(unsigned int index);
  void RemoveBlockVisibility(unsigned int index);
  void RemoveBlockVisibilities();
  ///@}

  /**
   * Use the internal PolyData Mapper to do the rendering
   * of the HTG transformed by the current SurfaceFilter:
   * * Adaptive2DGeometryFilter if UseCameraFrustum
   * * GeometryFilter otherwise
   */
  void Render(vtkRenderer* ren, vtkActor* act) override;

  /**
   * Fill the input port information objects for this algorithm.  This
   * is invoked by the first call to GetInputPortInformation for each
   * port so subclasses can specify what they can handle.
   */
  int FillInputPortInformation(int port, vtkInformation* info) override;

protected:
  vtkHyperTreeGridMapper();
  ~vtkHyperTreeGridMapper() override;

  /**
   * Generate a new composite were each leave is decimated if required
   */
  vtkSmartPointer<vtkCompositeDataSet> UpdateWithDecimation(
    vtkCompositeDataSet* htg, vtkRenderer* ren);

  // In 2D mode, these variables control the mapper optimisations
  bool UseAdaptiveDecimation = false;

  // render the extracted surface,
  // need to be created in device specific subclass
  vtkSmartPointer<vtkPolyDataMapper> Mapper;

  // Internal object to render
  vtkSmartPointer<vtkCompositeDataSet> Input;

private:
  vtkHyperTreeGridMapper(const vtkHyperTreeGridMapper&) = delete;
  void operator=(const vtkHyperTreeGridMapper&) = delete;

  std::set<unsigned int> BlocksShown;
  std::set<unsigned int> BlocksHidden;

  /**
   * Apply internally-stored block visibility settings to the composite mapper, if any.
   */
  void ApplyBlockVisibilities();
};

VTK_ABI_NAMESPACE_END
#endif
