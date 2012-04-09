/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoImageNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
// .NAME vtkGeoImageNode - A node in a multi-resolution image tree.
//
// .SECTION Description
// vtkGeoImageNode contains an image tile in a multi-resolution image tree,
// along with metadata about that image's extents.
//
// .SECTION See Also
// vtkGeoTreeNode vtkGeoTerrainNode
   
#ifndef __vtkGeoImageNode_h
#define __vtkGeoImageNode_h

#include "vtkGeoTreeNode.h"
#include "vtkSmartPointer.h" // for SP
#include "vtkImageData.h" // for SP

class vtkPolyData;
class vtkTexture;

class VTK_GEOVIS_EXPORT vtkGeoImageNode : public vtkGeoTreeNode
{
public:
  static vtkGeoImageNode *New();
  vtkTypeMacro(vtkGeoImageNode, vtkGeoTreeNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Every subclass implements these methods returning the specific type.
  // This is easier than templating.
  vtkGeoImageNode* GetChild(int idx);
  vtkGeoImageNode* GetParent();
  
  // Description:
  // Get the image tile.
  vtkImageData* GetImage();
  void SetImage(vtkImageData* image);

  // Description:
  // Get the image tile.
  vtkTexture* GetTexture();
  void SetTexture(vtkTexture* texture);

  // Description:
  // This crops the image as small as possible while still covering the 
  // patch.  The Longitude Latitude range may get bigger to reflect the
  // actual size of the image.
  // If prefix is specified, writes the tile to that location.
  void CropImageForTile(vtkImageData* image,double* imageLonLatExt,
    const char* prefix = 0);

  // Description:
  // This loads the image from a tile database at the specified location.
  void LoadAnImage(const char* prefix);

  // Description:
  // Shallow and Deep copy.
  virtual void ShallowCopy(vtkGeoTreeNode *src);  
  virtual void DeepCopy(vtkGeoTreeNode *src);

  // Returns whether this node has valid data associated
  // with it, or if it is an "empty" node.
  virtual bool HasData();

  // Description:
  // Deletes the data associated with the node to make this
  // an "empty" node. This is performed when the node has
  // been unused for a certain amount of time.
  virtual void DeleteData();
  
protected:
  vtkGeoImageNode();
  ~vtkGeoImageNode();

  int PowerOfTwo(int val);

//BTX
  vtkSmartPointer<vtkImageData> Image;
  vtkSmartPointer<vtkTexture> Texture;
//ETX

private:
  vtkGeoImageNode(const vtkGeoImageNode&);  // Not implemented.
  void operator=(const vtkGeoImageNode&);  // Not implemented.
};

#endif
