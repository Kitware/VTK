/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAnnotationLayers.cxx
  
-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkAnnotationLayers.h"

#include "vtkAnnotation.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

#include <vtkstd/algorithm>
#include <vtkstd/vector>

vtkCxxRevisionMacro(vtkAnnotationLayers, "1.3");
vtkStandardNewMacro(vtkAnnotationLayers);

class vtkAnnotationLayers::Internals
{
public:
  vtkstd::vector<vtksys_stl::vector<vtkSmartPointer<vtkAnnotation> > >
    Layers;
};

vtkAnnotationLayers::vtkAnnotationLayers() :
  Implementation(new Internals())
{
}

vtkAnnotationLayers::~vtkAnnotationLayers()
{
  delete this->Implementation;
}

unsigned int vtkAnnotationLayers::GetNumberOfLayers()
{
  return static_cast<unsigned int>(this->Implementation->Layers.size());
}

unsigned int vtkAnnotationLayers::GetNumberOfAnnotations(
  unsigned int layer)
{
  if (layer >= this->Implementation->Layers.size())
    {
    vtkErrorMacro("Index out of bounds");
    return 0;
    }
  return static_cast<unsigned int>(this->Implementation->Layers[layer].size());
}

vtkAnnotation* vtkAnnotationLayers::GetAnnotation(
  unsigned int layer, unsigned int idx)
{
  if (layer >= this->Implementation->Layers.size())
    {
    return 0;
    }
  if (idx >= this->Implementation->Layers[layer].size())
    {
    return 0;
    }
  return this->Implementation->Layers[layer][idx];
}

void vtkAnnotationLayers::AddAnnotation(
  unsigned int layer, vtkAnnotation* annotation)
{
  if (layer >= this->GetNumberOfLayers())
    {
    return;
    }
  this->Implementation->Layers[layer].push_back(annotation);
  this->Modified();
}

void vtkAnnotationLayers::RemoveAnnotation(
  unsigned int layer, vtkAnnotation* annotation)
{
  if (layer >= this->GetNumberOfLayers())
    {
    return;
    }
  this->Implementation->Layers[layer].erase(
    vtkstd::remove(
      this->Implementation->Layers[layer].begin(),
      this->Implementation->Layers[layer].end(),
      annotation),
    this->Implementation->Layers[layer].end());
}

void vtkAnnotationLayers::InsertLayer(
  unsigned int layer)
{
  if (layer >= this->GetNumberOfLayers())
    {
    while (layer >= this->GetNumberOfLayers())
      {
      this->Implementation->Layers.push_back(
        vtkstd::vector<vtkSmartPointer<vtkAnnotation> >());
      }
    return;
    }
  this->Implementation->Layers.insert(
    this->Implementation->Layers.begin() + layer,
    vtkstd::vector<vtkSmartPointer<vtkAnnotation> >());
}

void vtkAnnotationLayers::RemoveLayer(
  unsigned int layer)
{
  if (layer >= this->GetNumberOfLayers())
    {
    return;
    }
  this->Implementation->Layers.erase(
    this->Implementation->Layers.begin() + layer);
}

void vtkAnnotationLayers::Initialize()
{
  this->Implementation->Layers.clear();
}

void vtkAnnotationLayers::ShallowCopy(vtkDataObject* other)
{
  this->Superclass::ShallowCopy(other);
  vtkAnnotationLayers* obj = vtkAnnotationLayers::SafeDownCast(other);
  if (!obj)
    {
    return;
    }
  this->Implementation->Layers.clear();
  for (unsigned int l = 0; l < obj->GetNumberOfLayers(); ++l)
    {
    this->AddLayer();
    for (unsigned int a = 0; a < obj->GetNumberOfAnnotations(a); ++a)
      {
      vtkAnnotation* ann = obj->GetAnnotation(l, a);
      this->AddAnnotation(l, ann);
      }
    }
}

void vtkAnnotationLayers::DeepCopy(vtkDataObject* other)
{
  this->Superclass::DeepCopy(other);
  vtkAnnotationLayers* obj = vtkAnnotationLayers::SafeDownCast(other);
  if (!obj)
    {
    return;
    }
  this->Implementation->Layers.clear();
  for (unsigned int l = 0; l < obj->GetNumberOfLayers(); ++l)
    {
    this->AddLayer();
    for (unsigned int a = 0; a < obj->GetNumberOfAnnotations(a); ++a)
      {
      vtkSmartPointer<vtkAnnotation> ann =
        vtkSmartPointer<vtkAnnotation>::New();
      ann->DeepCopy(obj->GetAnnotation(l, a));
      this->AddAnnotation(l, ann);
      }
    }
}

void vtkAnnotationLayers::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  vtkIndent next = indent.GetNextIndent();
  for (unsigned int l = 0; l < this->GetNumberOfLayers(); ++l)
    {
    os << indent << "Layer " << l << ":\n";
    for (unsigned int a = 0; a < this->GetNumberOfAnnotations(a); ++a)
      {
      os << next << "Annotation " << a << ":";
      vtkAnnotation* ann = this->GetAnnotation(l, a);
      if (ann)
        {
        os << "\n";
        ann->PrintSelf(os, next.GetNextIndent());
        }
      else
        {
        os << "(none)\n";
        }
      }
    }
}

vtkAnnotationLayers* vtkAnnotationLayers::GetData(vtkInformation* info)
{
  return info ? vtkAnnotationLayers::SafeDownCast(info->Get(DATA_OBJECT())) : 0;
}

vtkAnnotationLayers* vtkAnnotationLayers::GetData(vtkInformationVector* v, int i)
{
  return vtkAnnotationLayers::GetData(v->GetInformationObject(i));
}
