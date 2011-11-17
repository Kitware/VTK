/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkCirclePackLayout.h

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
// .NAME vtkCirclePackLayout - layout a vtkTree as a circle packing.
//
// .SECTION Description
// vtkCirclePackLayout assigns circle shaped regions to each vertex
// in the tree, creating a circle packing layout.  The data is added
// as a data array with three components per tuple representing the
// center and radius of the circle using the format (Xcenter, Ycenter, Radius).
//
// This algorithm relies on a helper class to perform the actual layout.
// This helper class is a subclass of vtkCirclePackLayoutStrategy.
//
// WARNING: A size array must be in the input vtkTree that specifies the size
// for each vertex in the vtkTree.  The default name for this array is "size".
//
// .SECTION Thanks
// Thanks to Thomas Otahal from Sandia National Laboratories
// for help developing this class.
//

#ifndef __vtkCirclePackLayout_h
#define __vtkCirclePackLayout_h

#include "vtkTreeAlgorithm.h"

class vtkCirclePackLayoutStrategy;

class VTK_INFOVIS_EXPORT vtkCirclePackLayout : public vtkTreeAlgorithm
{
public:
    static vtkCirclePackLayout *New();

    vtkTypeMacro(vtkCirclePackLayout,vtkTreeAlgorithm);
    void PrintSelf(ostream& os, vtkIndent indent);

    // Description:
    // The field name to use for storing the circles for each vertex.
    // The rectangles are stored in a triple float array
    // (Xcenter, Ycenter, Radius).
    vtkGetStringMacro(CirclesFieldName);
    vtkSetStringMacro(CirclesFieldName);

    // Description:
    // The array to use for the size of each vertex.
    // Default name is "size".
    virtual void SetSizeArrayName(const char* name)
    { this->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_VERTICES, name); }

    // Description:
    // The strategy to use when laying out the tree map.
    vtkGetObjectMacro(LayoutStrategy, vtkCirclePackLayoutStrategy);
    void SetLayoutStrategy(vtkCirclePackLayoutStrategy * strategy);

    // Description:
    // Returns the vertex id that contains pnt (or -1 if no one contains it)
    // pnt[0] is x, and pnt[1] is y.
    // If cinfo[3] is provided, then (Xcenter, Ycenter, Radius) of the circle
    // containing pnt[2] will be returned.
    vtkIdType FindVertex(float pnt[2], float *cinfo=0);

    // Description:
    // Return the Xcenter, Ycenter, and Radius of the
    // vertex's bounding circle
    void GetBoundingCircle(vtkIdType id, float *cinfo);

    // Description:
    // Get the modification time of the layout algorithm.
    virtual unsigned long GetMTime();

protected:
    vtkCirclePackLayout();
    ~vtkCirclePackLayout();

    char * CirclesFieldName;
    vtkCirclePackLayoutStrategy* LayoutStrategy;

    int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

private:

    vtkCirclePackLayout(const vtkCirclePackLayout&);  // Not implemented.
    void operator=(const vtkCirclePackLayout&);  // Not implemented.
};

#endif
