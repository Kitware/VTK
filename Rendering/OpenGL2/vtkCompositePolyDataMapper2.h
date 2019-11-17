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

#include "vtkOpenGLPolyDataMapper.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkSmartPointer.h"           // for vtkSmartPointer

#include "vtkColor.h" // used for ivars
#include <map>        // use for ivars
#include <stack>      // used for ivars
#include <vector>     // used for ivars

class vtkCompositeDataDisplayAttributes;
class vtkCompositeMapperHelper2;
class vtkCompositeMapperHelperData;

class VTKRENDERINGOPENGL2_EXPORT vtkCompositePolyDataMapper2 : public vtkOpenGLPolyDataMapper
{
public:
  static vtkCompositePolyDataMapper2* New();
  vtkTypeMacro(vtkCompositePolyDataMapper2, vtkOpenGLPolyDataMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Some introspection on the type of data the mapper will render
   * used by props to determine if they should invoke the mapper
   * on a specific rendering pass.
   */
  bool HasOpaqueGeometry() override;
  bool HasTranslucentPolygonalGeometry() override;
  //@}

  //@{
  /**
   * Set/get the composite data set attributes.
   */
  void SetCompositeDataDisplayAttributes(vtkCompositeDataDisplayAttributes* attributes);
  vtkCompositeDataDisplayAttributes* GetCompositeDataDisplayAttributes();
  //@}

  //@{
  /**
   * Set/get the visibility for a block given its flat index.
   */
  void SetBlockVisibility(unsigned int index, bool visible);
  bool GetBlockVisibility(unsigned int index);
  void RemoveBlockVisibility(unsigned int index);
  void RemoveBlockVisibilities();
  // This method is deprecated and will be removed in VTK 8.2. It is misspelled.
  VTK_LEGACY(void RemoveBlockVisibilites());
  //@}

  //@{
  /**
   * Set/get the color for a block given its flat index.
   */
  void SetBlockColor(unsigned int index, double color[3]);
  void SetBlockColor(unsigned int index, double r, double g, double b)
  {
    double color[3] = { r, g, b };
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

  /**
   * If the current 'color by' array is missing on some datasets, color these
   * dataset by the LookupTable's NaN color, if the lookup table supports it.
   * Default is false.
   * @{
   */
  vtkSetMacro(ColorMissingArraysWithNanColor, bool);
  vtkGetMacro(ColorMissingArraysWithNanColor, bool);
  vtkBooleanMacro(ColorMissingArraysWithNanColor, bool);
  /**@}*/

  /**
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow*) override;

  /**
   * This calls RenderPiece (in a for loop if streaming is necessary).
   */
  void Render(vtkRenderer* ren, vtkActor* act) override;

  //@{
  /**
   * Call SetInputArrayToProcess on helpers.
   */
  using vtkAlgorithm::SetInputArrayToProcess;
  void SetInputArrayToProcess(
    int idx, int port, int connection, int fieldAssociation, const char* name) override;
  void SetInputArrayToProcess(
    int idx, int port, int connection, int fieldAssociation, int fieldAttributeType) override;
  void SetInputArrayToProcess(int idx, vtkInformation* info) override;
  //@}

  /**
   * Accessor to the ordered list of PolyData that we end last drew.
   */
  std::vector<vtkPolyData*> GetRenderedList() { return this->RenderedList; }

  /**
   * allows a mapper to update a selections color buffers
   * Called from a prop which in turn is called from the selector
   */
  void ProcessSelectorPixelBuffers(
    vtkHardwareSelector* sel, std::vector<unsigned int>& pixeloffsets, vtkProp* prop) override;

protected:
  vtkCompositePolyDataMapper2();
  ~vtkCompositePolyDataMapper2() override;

  /**
   * We need to override this method because the standard streaming
   * demand driven pipeline is not what we want - we are expecting
   * hierarchical data as input
   */
  vtkExecutive* CreateDefaultExecutive() override;

  /**
   * Need to define the type of data handled by this mapper.
   */
  int FillInputPortInformation(int port, vtkInformation* info) override;

  /**
   * Need to loop over the hierarchy to compute bounds
   */
  void ComputeBounds() override;

  /**
   * This method is called before RenderPiece is called on helpers.
   * One can override it to initialize the helpers.
   */
  virtual void InitializeHelpersBeforeRendering(
    vtkRenderer* vtkNotUsed(ren), vtkActor* vtkNotUsed(act))
  {
  }

  /**
   * Time stamp for computation of bounds.
   */
  vtkTimeStamp BoundsMTime;

  // what "index" are we currently rendering, -1 means none
  int CurrentFlatIndex;
  std::map<const std::string, vtkCompositeMapperHelper2*> Helpers;
  std::map<vtkPolyData*, vtkCompositeMapperHelperData*> HelperDataMap;
  vtkTimeStamp HelperMTime;

  virtual vtkCompositeMapperHelper2* CreateHelper();

  // copy values to the helpers
  virtual void CopyMapperValuesToHelper(vtkCompositeMapperHelper2* helper);

  class RenderBlockState
  {
  public:
    std::stack<bool> Visibility;
    std::stack<bool> Pickability;
    std::stack<double> Opacity;
    std::stack<vtkColor3d> AmbientColor;
    std::stack<vtkColor3d> DiffuseColor;
    std::stack<vtkColor3d> SpecularColor;
  };

  bool RecursiveHasTranslucentGeometry(vtkDataObject* dobj, unsigned int& flat_index);
  vtkStateStorage TranslucentState;
  bool HasTranslucentGeometry;

  void BuildRenderValues(
    vtkRenderer* renderer, vtkActor* actor, vtkDataObject* dobj, unsigned int& flat_index);
  vtkStateStorage RenderValuesState;

  RenderBlockState BlockState;
  void RenderBlock(
    vtkRenderer* renderer, vtkActor* actor, vtkDataObject* dobj, unsigned int& flat_index);

  /**
   * Composite data set attributes.
   */
  vtkSmartPointer<vtkCompositeDataDisplayAttributes> CompositeAttributes;

  friend class vtkCompositeMapperHelper2;

  /**
   * If the current 'color by' array is missing on some datasets, color these
   * dataset by the LookupTable's NaN color, if the lookup table supports it.
   */
  bool ColorMissingArraysWithNanColor;

  std::vector<vtkPolyData*> RenderedList;

private:
  double ColorResult[3];

  vtkCompositePolyDataMapper2(const vtkCompositePolyDataMapper2&) = delete;
  void operator=(const vtkCompositePolyDataMapper2&) = delete;
};

#endif
