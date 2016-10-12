/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCollapseVerticesByArray.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCollapseVerticesByArray
 * @brief   Collapse the graph given a vertex array
 *
 *
 * vtkCollapseVerticesByArray is a class which collapses the graph using
 * a vertex array as the key. So if the graph has vertices sharing common
 * traits then this class combines all these vertices into one. This class
 * does not perform aggregation on vertex data but allow to do so for edge data.
 * Users can choose one or more edge data arrays for aggregation using
 * AddAggregateEdgeArray function.
 *
 *
*/

#ifndef vtkCollapseVerticesByArray_h
#define vtkCollapseVerticesByArray_h

#include "vtkInfovisCoreModule.h" // For export macro
#include "vtkGraphAlgorithm.h"

class vtkCollapseVerticesByArrayInternal;

class VTKINFOVISCORE_EXPORT vtkCollapseVerticesByArray : public vtkGraphAlgorithm
{
public:
    static vtkCollapseVerticesByArray* New();
    vtkTypeMacro(vtkCollapseVerticesByArray, vtkGraphAlgorithm);

    void PrintSelf(ostream &os, vtkIndent indent);

    //@{
    /**
     * Boolean to allow self loops during collapse.
     */
    vtkGetMacro(AllowSelfLoops, bool);
    vtkSetMacro(AllowSelfLoops, bool);
    vtkBooleanMacro(AllowSelfLoops, bool);
    //@}

    /**
     * Add arrays on which aggregation of data is allowed.
     * Default if replaced by the last value.
     */
    void AddAggregateEdgeArray(const char* arrName);


    /**
     * Clear the list of arrays on which aggregation was set to allow.
     */
    void ClearAggregateEdgeArray();

    //@{
    /**
     * Set the array using which perform the collapse.
     */
    vtkGetStringMacro(VertexArray);
    vtkSetStringMacro(VertexArray);
    //@}


    //@{
    /**
     * Set if count should be made of how many edges collapsed.
     */
    vtkGetMacro(CountEdgesCollapsed, bool);
    vtkSetMacro(CountEdgesCollapsed, bool);
    vtkBooleanMacro(CountEdgesCollapsed, bool);
    //@}

    //@{
    /**
     * Name of the array where the count of how many edges collapsed will
     * be stored.By default the name of array is "EdgesCollapsedCountArray".
     */
    vtkGetStringMacro(EdgesCollapsedArray);
    vtkSetStringMacro(EdgesCollapsedArray);
    //@}


    //@{
    /**
     * Get/Set if count should be made of how many vertices collapsed.
     */
    vtkGetMacro(CountVerticesCollapsed, bool);
    vtkSetMacro(CountVerticesCollapsed, bool);
    vtkBooleanMacro(CountVerticesCollapsed, bool);
    //@}

    //@{
    /**
     * Name of the array where the count of how many vertices collapsed will
     * be stored. By default name of the array is "VerticesCollapsedCountArray".
     */
    vtkGetStringMacro(VerticesCollapsedArray);
    vtkSetStringMacro(VerticesCollapsedArray);
    //@}

protected:

    vtkCollapseVerticesByArray();
   ~vtkCollapseVerticesByArray();

   /**
    * Pipeline function.
    */
   virtual int RequestData(vtkInformation* request,
                           vtkInformationVector** inputVector,
                           vtkInformationVector* outputVector);

   /**
    * Pipeline function.
    */
   virtual int FillOutputPortInformation(int port, vtkInformation* info);


   /**
    * Create output graph given all the parameters. Helper function.
    */
   vtkGraph* Create(vtkGraph* inGraph);

   /**
    * Helper function.
    */
   void FindEdge(vtkGraph* outGraph, vtkIdType source,
                 vtkIdType target, vtkIdType& edgeId);

private:
  //@{
  vtkCollapseVerticesByArray(const vtkCollapseVerticesByArray&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCollapseVerticesByArray&) VTK_DELETE_FUNCTION;
  //@}


protected:
  bool            AllowSelfLoops;
  char*           VertexArray;

  bool            CountEdgesCollapsed;
  char*           EdgesCollapsedArray;

  bool            CountVerticesCollapsed;
  char*           VerticesCollapsedArray;

  vtkCollapseVerticesByArrayInternal* Internal;
};

#endif // vtkCollapseVerticesByArray_h__
