/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkActorNode.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkActorNode.h"

#include "vtkActor.h"
#include "vtkMapper.h"
#include "vtkObjectFactory.h"
#include "vtkProperty.h"

//============================================================================
vtkStandardNewMacro(vtkActorNode);

//----------------------------------------------------------------------------
vtkActorNode::vtkActorNode()
{
  //actor
  this->Visibility = false;
  //property
  this->Opacity = 1.0;
  this->Representation = VTK_POINTS;
  this->Lighting = true;
  this->Interpolation = VTK_FLAT;
  this->ScalarVisibility = false;
  this->Ambient = 0.0;
  this->AmbientColor[0] =
    this->AmbientColor[1] =
    this->AmbientColor[2] = 1.0;
  this->Diffuse = 1.0;
  this->DiffuseColor[0] =
    this->DiffuseColor[1] =
    this->DiffuseColor[2] = 1.0;
  this->LineWidth = 1.0;
  this->PointSize = 1.0;
  this->Specular = 0.0;
  this->SpecularColor[0] =
    this->SpecularColor[1] =
    this->SpecularColor[2] = 1.0;
  this->SpecularPower = 0.0;
  //mapper
  this->ScalarMode = VTK_SCALAR_MODE_DEFAULT;
  this->ColorMode = VTK_COLOR_MODE_DEFAULT;
  this->InterpolateScalarsBeforeMapping = false;
  this->ScalarRange[0] = 0.0;
  this->ScalarRange[1] = -1.0;
  this->UseLookupTableScalarRange = true;
  this->ScalarMaterialMode = VTK_MATERIALMODE_DEFAULT;
}

//----------------------------------------------------------------------------
vtkActorNode::~vtkActorNode()
{
}

//----------------------------------------------------------------------------
void vtkActorNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkActorNode::SynchronizeSelf()
{
  vtkActor *mine = vtkActor::SafeDownCast
    (this->GetRenderable());
  if (!mine)
    {
    return;
    }

  /* actor */
  // GetBackfaceProperty()       vtkActor        virtual
  // GetBounds() vtkActor        virtual
  // GetCenter() vtkProp3D
  // GetConsumer(int i)  vtkProp
  // GetDragable()       vtkProp virtual
  // GetEstimatedRenderTime()    vtkProp inlinevirtual
  // GetIsIdentity()     vtkProp3D       virtual
  // GetIsOpaque()       vtkActor        protected
  // GetLength() vtkProp3D
  // GetMapper() vtkActor        virtual
  // GetMatrix() vtkProp3D       inlinevirtual
  // GetNextPath()       vtkProp virtual
  // GetNumberOfConsumers()      vtkProp virtual
  // GetNumberOfPaths()  vtkProp inlinevirtual
  // GetOrientation()    vtkProp3D
  // GetOrigin() vtkProp3D       virtual
  // GetPickable()       vtkProp virtual
  // GetPosition()       vtkProp3D       virtual
  // GetPropertyKeys()   vtkProp virtual
  // GetRenderTimeMultiplier()   vtkProp virtual
  // GetScale()  vtkProp3D       virtual
  // GetTexture()        vtkActor        virtual
  // GetUseBounds()      vtkProp virtual
  // GetUserMatrix()     vtkProp3D
  // GetUserTransform()  vtkProp3D       virtual
  this->Visibility = mine->GetVisibility();
  // GetXRange() vtkProp3D
  // GetYRange() vtkProp3D
  // GetZRange() vtkProp3D

  /* property */
  this->Ambient = mine->GetProperty()->GetAmbient();
  mine->GetProperty()->GetAmbientColor(this->AmbientColor);
  // GetBackfaceCulling()        vtkProperty     virtual
  // GetColor()  vtkProperty
  this->Diffuse = mine->GetProperty()->GetDiffuse();
  mine->GetProperty()->GetDiffuseColor(this->DiffuseColor);
  // GetEdgeColor()      vtkProperty     virtual
  // GetEdgeVisibility() vtkProperty     virtual
  // GetFrontfaceCulling()       vtkProperty     virtual
  this->Interpolation = mine->GetProperty()->GetInterpolation();
  this->Lighting = mine->GetProperty()->GetLighting();
  // GetLineStipplePattern()     vtkProperty     virtual
  // GetLineStippleRepeatFactor()        vtkProperty     virtual
  this->LineWidth = mine->GetProperty()->GetLineWidth();
  // GetMaterialName()   vtkProperty     virtual
  // GetNumberOfTextures()       vtkProperty
  this->Opacity = mine->GetProperty()->GetOpacity();
  this->PointSize = mine->GetProperty()->GetPointSize();
  this->Representation = mine->GetProperty()->GetRepresentation();
  // GetShaderDeviceAdapter2()   vtkProperty     inlinevirtual
  // GetShading()        vtkProperty     virtual
  this->Specular = mine->GetProperty()->GetSpecular();
  mine->GetProperty()->GetSpecularColor(this->SpecularColor);
  this->SpecularPower = mine->GetProperty()->GetSpecularPower();
  // GetTexture(const char *name)        vtkProperty
  // GetTexture(int unit)        vtkProperty
  // GetTextureAtIndex(int index)        vtkProperty     protected
  // GetTextureUnit(const char *name)    vtkProperty     protected
  // GetTextureUnitAtIndex(int index)

  /* mapper */
  // GetArrayAccessMode()        vtkMapper       inline
  // GetArrayComponent() vtkMapper       inline
  // GetArrayId()        vtkMapper       inline
  // GetArrayName()      vtkMapper       inline
  // GetClippingPlanes() vtkAbstractMapper       virtual
  this->ColorMode = mine->GetMapper()->GetColorMode(); //direct or lut
  // GetFieldDataTupleId()       vtkMapper       virtual
  // GetImmediateModeRendering() vtkMapper       virtual
  this->InterpolateScalarsBeforeMapping =
    mine->GetMapper()->GetInterpolateScalarsBeforeMapping();
  // GetLookupTable()    vtkMapper
  // GetNumberOfClippingPlanes() vtkAbstractMapper3D
  // GetResolveCoincidentTopology()      vtkMapper       static
  // GetResolveCoincidentTopologyPolygonOffsetFaces()    vtkMapper       static
  // GetResolveCoincidentTopologyPolygonOffsetParameters(double &factor, double &units)  vtkMapper       static
  // GetResolveCoincidentTopologyZShift()        vtkMapper       static
  this->ScalarMaterialMode = mine->GetMapper()->GetScalarMaterialMode();
  this->ScalarMode = mine->GetMapper()->GetScalarMode(); //array location choice
  mine->GetMapper()->GetScalarRange(this->ScalarRange);
  // GetScalars(vtkDataSet *input, int scalarMode, int arrayAccessMode, int arrayId, const char *arrayName, int &cellFlag)       vtkAbstractMapper       static
  this->ScalarVisibility = mine->GetMapper()->GetScalarVisibility();
  // GetStatic() vtkMapper       virtual
  // GetSupportsSelection()      vtkMapper       inlinevirtual
  // GetUpdateExtent()   vtkAlgorithm    inline
  // GetUpdateGhostLevel()       vtkAlgorithm    inline
  // GetUpdateNumberOfPieces()   vtkAlgorithm    inline
  // GetUpdatePiece()    vtkAlgorithm    inline
  this->UseLookupTableScalarRange =
    mine->GetMapper()->GetUseLookupTableScalarRange();
}
