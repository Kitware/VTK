/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractPropPicker.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkAbstractPropPicker.h"
#include "vtkAssemblyNode.h"
#include "vtkActor.h"
#include "vtkActor2D.h"
#include "vtkAssembly.h"
#include "vtkVolume.h"
#include "vtkPropAssembly.h"
#include "vtkObjectFactory.h"


vtkAbstractPropPicker::vtkAbstractPropPicker()
{
  this->Path = NULL;
}

vtkAbstractPropPicker::~vtkAbstractPropPicker()
{
  if ( this->Path )
    {
    this->Path->Delete();
    }
}

// set up for a pick
void vtkAbstractPropPicker::Initialize()
{
  this->vtkAbstractPicker::Initialize();
  if ( this->Path )
    {
    this->Path->Delete();
    this->Path = NULL;
    }
}

vtkProp *vtkAbstractPropPicker::GetProp()
{
  if ( this->Path != NULL )
    {
    return this->Path->GetFirstNode()->GetProp();
    }
  else
    {
    return NULL;
    }
}

vtkProp3D *vtkAbstractPropPicker::GetProp3D()
{
  if ( this->Path != NULL )
    {
    vtkProp *prop = this->Path->GetFirstNode()->GetProp();
    return vtkProp3D::SafeDownCast(prop);
    }
  else
    {
    return NULL;
    }
}

vtkActor *vtkAbstractPropPicker::GetActor()
{
  if ( this->Path != NULL )
    {
    vtkProp *prop = this->Path->GetFirstNode()->GetProp();
    return vtkActor::SafeDownCast(prop);
    }
  else
    {
    return NULL;
    }
}

vtkActor2D *vtkAbstractPropPicker::GetActor2D()
{
  if ( this->Path != NULL )
    {
    vtkProp *prop = this->Path->GetFirstNode()->GetProp();
    return vtkActor2D::SafeDownCast(prop);
    }
  else
    {
    return NULL;
    }
}

vtkVolume *vtkAbstractPropPicker::GetVolume()
{
  if ( this->Path != NULL )
    {
    vtkProp *prop = this->Path->GetFirstNode()->GetProp();
    return vtkVolume::SafeDownCast(prop);
    }
  else
    {
    return NULL;
    }
}

vtkAssembly *vtkAbstractPropPicker::GetAssembly()
{
  if ( this->Path != NULL )
    {
    vtkProp *prop = this->Path->GetFirstNode()->GetProp();
    return vtkAssembly::SafeDownCast(prop);
    }
  else
    {
    return NULL;
    }
}

vtkPropAssembly *vtkAbstractPropPicker::GetPropAssembly()
{
  if ( this->Path != NULL )
    {
    vtkProp *prop = this->Path->GetFirstNode()->GetProp();
    return vtkPropAssembly::SafeDownCast(prop);
    }
  else
    {
    return NULL;
    }
}
  
void vtkAbstractPropPicker::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkAbstractPicker::PrintSelf(os, indent);

  if ( this->Path )
    {
    os << indent << "Path: " << this->Path << endl;
    }
  else
    {
    os << indent << "Path: (none)" << endl;
    }
}
