/*=========================================================================

   Program:   Visualization Toolkit
   Module:    vtkOSPRayMoleculeMapperNode.cxx

   Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
   All rights reserved.
   See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

      This software is distributed WITHOUT ANY WARRANTY; without even
      the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
      PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkOSPRayMoleculeMapperNode.h"

#include "vtkInformation.h"
#include "vtkMolecule.h"
#include "vtkMoleculeMapper.h"
#include "vtkOSPRayCache.h"
#include "vtkOSPRayRendererNode.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"

#include <algorithm>
#include <vector>

//============================================================================
vtkStandardNewMacro(vtkOSPRayMoleculeMapperNode);

//----------------------------------------------------------------------------
vtkOSPRayMoleculeMapperNode::vtkOSPRayMoleculeMapperNode()
{
  cerr << "CREATE OSPMOLMAPPERNODE " << this << endl;
}

//----------------------------------------------------------------------------
vtkOSPRayMoleculeMapperNode::~vtkOSPRayMoleculeMapperNode()
{
  cerr << "DESTROY OSPMOLMAPPERNODE " << this << endl;

  delete this->Cache;
}

//----------------------------------------------------------------------------
void vtkOSPRayMoleculeMapperNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkOSPRayMoleculeMapperNode::Render(bool prepass)
{
  if (prepass)
  {
    vtkMoleculeMapper* mapper = vtkMoleculeMapper::SafeDownCast(this->GetRenderable());
    vtkOSPRayRendererNode* orn =
      static_cast<vtkOSPRayRendererNode*>(this->GetFirstAncestorOfType("vtkOSPRayRendererNode"));
    vtkRenderer* ren = vtkRenderer::SafeDownCast(orn->GetRenderable());
    RTW::Backend* backend = orn->GetBackend();
    if (backend == nullptr)
    {
      return;
    }

    OSPModel OSPRayModel = orn->GetOModel();
  }
  else
  {
  }
}
