// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkGLSLModCoincidentTopology.h"
#include "vtkAbstractMapper.h"
#include "vtkActor.h"
#include "vtkHardwareSelector.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkLogger.h"
#include "vtkMapper.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLActor.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkProperty.h"
#include "vtkShaderProgram.h"

#include "vtk_glad.h"

#include <algorithm>
#include <cmath>

VTK_ABI_NAMESPACE_BEGIN

namespace
{
//------------------------------------------------------------------------------
float getDepthResolution(vtkRenderer* ren)
{
  int bits = 0;
  if (auto* renWin = vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow()))
  {
    bits = renWin->GetDepthBufferSize();
  }
  // near 1.0, a 32-bit depth attachment has 24-bits for significand
  // and resolves the same as a 24-bit integer buffer. Fall back to 16 bits
  // if renderWindow does not say anything about the depth buffer size.
  bits = bits > 0 ? std::min(bits, 24) : 16;
  return std::ldexp(1.0f, -bits);
}
}

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkGLSLModCoincidentTopology);

//------------------------------------------------------------------------------
vtkGLSLModCoincidentTopology::vtkGLSLModCoincidentTopology() = default;

//------------------------------------------------------------------------------
vtkGLSLModCoincidentTopology::~vtkGLSLModCoincidentTopology() = default;

//------------------------------------------------------------------------------
void vtkGLSLModCoincidentTopology::PrintSelf(ostream& os, vtkIndent indent)
{
  os << "ReplacementsDone: " << this->ReplacementsDone << "\n";
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
bool vtkGLSLModCoincidentTopology::ReplaceShaderValues(vtkOpenGLRenderer* renderer,
  std::string& vtkNotUsed(vertexShader), std::string& vtkNotUsed(tessControlShader),
  std::string& vtkNotUsed(tessEvalShader), std::string& vtkNotUsed(geometryShader),
  std::string& fragmentShader, vtkAbstractMapper* mapper, vtkActor* actor)
{
  if (this->ReplacementsDone)
  {
    return true;
  }
  float factor = 0.0;
  float offset = 0.0;
  this->GetCoincidentParameters(renderer, vtkMapper::SafeDownCast(mapper), actor, factor, offset);

  // if we need an offset handle it here
  // cOffset is in units of the smallest resolvable depth difference,
  // cDepthRes, which is set from the bound depth buffer at draw time.
  if (factor != 0.0 || offset != 0.0)
  {
    vtkShaderProgram::Substitute(fragmentShader, "//VTK::Coincident::Dec",
      "uniform float cOffset;\n"
      "uniform float cFactor;\n"
      "uniform float cDepthRes;\n");
    vtkShaderProgram::Substitute(fragmentShader, "//VTK::UniformFlow::Impl",
      "float cscale = length(vec2(dFdx(gl_FragCoord.z), dFdy(gl_FragCoord.z)));\n"
      "  //VTK::UniformFlow::Impl\n" // for other replacements
    );
    vtkShaderProgram::Substitute(fragmentShader, "//VTK::Depth::Impl",
      "gl_FragDepth = gl_FragCoord.z + cFactor*cscale + cOffset*cDepthRes;\n");
  }
  this->ReplacementsDone = true;
  return true;
}

//------------------------------------------------------------------------------
bool vtkGLSLModCoincidentTopology::SetShaderParameters(vtkOpenGLRenderer* renderer,
  vtkShaderProgram* program, vtkAbstractMapper* mapper, vtkActor* actor,
  vtkOpenGLVertexArrayObject* vtkNotUsed(VAO) /*=nullptr*/)
{
  if (!this->ReplacementsDone)
  {
    return true;
  }
  // handle coincident
  float factor = 0.0;
  float offset = 0.0;
  this->GetCoincidentParameters(renderer, vtkMapper::SafeDownCast(mapper), actor, factor, offset);
  if (program->IsUniformUsed("cOffset") && program->IsUniformUsed("cFactor"))
  {
    program->SetUniformf("cOffset", offset);
    program->SetUniformf("cFactor", factor);
    program->SetUniformf("cDepthRes", getDepthResolution(renderer));
  }

  return true;
}

//------------------------------------------------------------------------------
void vtkGLSLModCoincidentTopology::GetCoincidentParameters(
  vtkOpenGLRenderer* ren, vtkMapper* mapper, vtkActor* actor, float& factor, float& offset)
{
  if (!mapper)
  {
    return;
  }
  factor = 0.0;
  offset = 0.0;
  if (vtkMapper::GetResolveCoincidentTopology() == VTK_RESOLVE_SHIFT_ZBUFFER)
  {
    // do something rough is better than nothing
    double zRes = vtkMapper::GetResolveCoincidentTopologyZShift(); // 0 is no shift 1 is big shift
    double f = zRes * 4.0;
    offset = f;
  }

  vtkProperty* ppty = actor->GetProperty();
  if ((vtkMapper::GetResolveCoincidentTopology() == VTK_RESOLVE_POLYGON_OFFSET) ||
    (ppty->GetEdgeVisibility() && ppty->GetRepresentation() == VTK_SURFACE))
  {
    double f = 0.0;
    double u = 0.0;
    if (this->PrimitiveType == GL_POINTS || ppty->GetRepresentation() == VTK_POINTS)
    {
      mapper->GetCoincidentTopologyPointOffsetParameter(u);
    }
    else if (this->PrimitiveType == GL_LINES || this->PrimitiveType == GL_LINE_STRIP ||
      ppty->GetRepresentation() == VTK_WIREFRAME)
    {
      mapper->GetCoincidentTopologyLineOffsetParameters(f, u);
    }
    else if (this->PrimitiveType == GL_TRIANGLES || this->PrimitiveType == GL_TRIANGLE_STRIP)
    {
      mapper->GetCoincidentTopologyPolygonOffsetParameters(f, u);
    }
    factor = f;
    offset = u;
  }

  // TODO: Somehow get whether we are drawing selections.
  // // always move selections a bit closer to the camera
  // // but not as close as point picking would move
  // if (this->DrawingSelection)
  // {
  //   offset -= 1.0;
  // }

  // hardware picking always offset due to saved zbuffer
  // This gets you above the saved surface depth buffer.
  vtkHardwareSelector* selector = ren->GetSelector();
  if (selector && selector->GetFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    offset -= 2.0;
  }
}

VTK_ABI_NAMESPACE_END
