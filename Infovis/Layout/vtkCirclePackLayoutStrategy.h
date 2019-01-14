/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkCirclePackLayoutStrategy.h

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
/**
 * @class   vtkCirclePackLayoutStrategy
 * @brief   abstract superclass for all circle packing
 * layout strategies.
 *
 *
 * All subclasses of this class perform a circle packing layout on a vtkTree.
 * This involves assigning a circle to each vertex in the tree,
 * and placing that information in a data array with three components per
 * tuple representing (Xcenter, Ycenter, Radius).
 *
 * Instances of subclasses of this class may be assigned as the layout
 * strategy to vtkCirclePackLayout
 *
 * @par Thanks:
 * Thanks to Thomas Otahal from Sandia National Laboratories
 * for help developing this class.
*/

#ifndef vtkCirclePackLayoutStrategy_h
#define vtkCirclePackLayoutStrategy_h


#include "vtkInfovisLayoutModule.h" // For export macro
#include "vtkObject.h"

class vtkTree;
class vtkDataArray;

class VTKINFOVISLAYOUT_EXPORT vtkCirclePackLayoutStrategy : public vtkObject
{
public:
    vtkTypeMacro(vtkCirclePackLayoutStrategy,vtkObject);
    void PrintSelf(ostream& os, vtkIndent indent) override;

    /**
     * Perform the layout of the input tree, and store the circle
     * bounds of each vertex as a tuple in a data array.
     * (Xcenter, Ycenter, Radius).

     * The sizeArray may be nullptr, or may contain the desired
     * size of each vertex in the tree.
     */
    virtual void Layout(vtkTree *inputTree, vtkDataArray *areaArray,
                        vtkDataArray* sizeArray) = 0;

protected:
    vtkCirclePackLayoutStrategy();
    ~vtkCirclePackLayoutStrategy() override;

private:
    vtkCirclePackLayoutStrategy(const vtkCirclePackLayoutStrategy&) = delete;
    void operator=(const vtkCirclePackLayoutStrategy&) = delete;
};

#endif
