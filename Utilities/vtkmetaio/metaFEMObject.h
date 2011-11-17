/*=========================================================================
 *
 *  Copyright Insight Software Consortium
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/
#include "metaTypes.h"

#ifndef ITKMetaIO_METAFEMOBJECT_H
#define ITKMetaIO_METAFEMOBJECT_H

#include "metaUtils.h"
#include "metaObject.h"

#ifdef _MSC_VER
#pragma warning ( disable: 4251 )
#endif

#include <list>
#include <vector>

/*!    metaFEMObject (.h and .cxx)
 *
 * Description:
 *    Reads and Writes Meta FEM Objects. These essentially describe
 *    a complete FE Model. This class was derived from metaMesh.
 *
 * \author Kiran Shivanna
 *
 * \date January, 2011
 *
 * Depends on:
 *    MetaUtils.h
 */

#if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE {
#endif

// helper classes to store the information read in for the FEM mesh
/** Define a fem node */
class METAIO_EXPORT FEMObjectNode
{
public:

  FEMObjectNode(int dim);
  ~FEMObjectNode();

  unsigned int m_Dim; //Element Dimension
  float* m_X;         // Node Coordinates
  int m_GN;               // global number used in FEM namespace
};


/** Define a mesh Element
 *  An element contains the following items:
 *    1) CLass name
 *    2) Number of dimensions
 *    3) Number of nodes used to define the element
 *    4) Id for the associated material property
 *    5) list of Ids defining the nodes
 */
class METAIO_EXPORT FEMObjectElement
{
public:

  FEMObjectElement(int dim);
  ~FEMObjectElement();

  int m_GN;
  char m_ElementName[256]; // class name
  unsigned int m_Dim;
  unsigned int m_NumNodes;
  unsigned int m_MaterialGN;
  int* m_NodesId;
};

/** Define a FE Mesh Material
 *  The material contains the following items:
 *    1) Global ID number
 *    2) Material Class Name
 *    3) Elasticity
 *    4) VAM
 *    3) VAM
 *    4) VAM
 *    5) VAM
 */
class METAIO_EXPORT FEMObjectMaterial
{
public:

  FEMObjectMaterial()
    {
    }
  ~FEMObjectMaterial()
    {
    }

  int m_GN;
  char m_MaterialName[256]; // material name
  double E;
  double A;
  double I;
  double nu;
  double h;
  double RhoC;
};

/** Define a FE Mesh FEMObjectMFCTerm
 *  The material contains the following items:
 *    1) VAM
 */
class METAIO_EXPORT FEMObjectMFCTerm
{
public:
  /**
     * Pointer to element, which holds the DOF that is affected by MFC
     */
  unsigned int m_ElementGN;

  /**
     * DOF number within the Element object
     */
  unsigned int m_DOF;

  /**
     * Value with which this displacement is multiplied on the lhs of MFC equation
     */
  float m_Value;

  /**
     * Constructor for easy object creation.
     */
  FEMObjectMFCTerm(unsigned int element_, unsigned int dof_,
                   float value_):m_ElementGN(element_), m_DOF(dof_), m_Value(value_) {}
};

/** Define a FE Mesh Load - This is a general purpose container
 *  able to hold information for any Load Type. The Load contains
 *  the following items:
 *    1) Global Model number
 *    2) Load Class Name (i.e. Type)
 *    3) Element global Number on which load is applied
 *    4) Number of dimensions
 *    5) Force vector
 *    6) Degrees of freedom
 *    7) Node Number
 *    8) Number of Right Hand Side components
 *    9) Right Hand Side
 *    10) Number of Left Hand Side components
 *    11) Left Hand Side
 *    12) Number of Elements
 *    13) Element Ids
 *    14) Force Matrix
 *    15) Edge Number
 *    16) Undeformed point (for landmark load)
 *    17) Deformed point (for landmark load)
 *    18) Variance
 */
class METAIO_EXPORT FEMObjectLoad
{
public:

  FEMObjectLoad();
  ~FEMObjectLoad();

  int m_GN;
  char m_LoadName[256]; // load name
  int m_ElementGN;
  int m_Dim;
  METAIO_STL::vector<float> m_ForceVector;
  int m_DOF;
  int m_NodeNumber;
  int m_NumRHS;
  METAIO_STL::vector<float> m_RHS;
  int m_NumLHS;
  METAIO_STL::vector<FEMObjectMFCTerm*> m_LHS;
  int m_NumElements;
  METAIO_STL::vector<int> m_Elements;
  METAIO_STL::vector< METAIO_STL::vector<float> > m_ForceMatrix;
  int m_EdgeNumber;
  METAIO_STL::vector<float> m_Undeformed;
  METAIO_STL::vector<float> m_Deformed;
  float m_Variance;
};


/** MetaFEMObject - This is the class to hold and write the
 *  FE Model.
 */
class METAIO_EXPORT MetaFEMObject : public MetaObject
{
public:

  MetaFEMObject(void);

  MetaFEMObject(const char *_headerName);

  MetaFEMObject(const MetaFEMObject *_femobject);

  MetaFEMObject(unsigned int dim);

  ~MetaFEMObject(void);

  void PrintInfo(void) const;

  void CopyInfo(const MetaObject * _object);

  /** Clear the MetaFEMObject */
  void  Clear(void);

  /** List of valid class name types from FEM namespace*/
  typedef METAIO_STL::list<std::string> ClassNameListType;

  /** List of Node, Element, Material and Load*/
  typedef METAIO_STL::list<FEMObjectNode*> NodeListType;
  typedef METAIO_STL::list<FEMObjectElement*>  ElementListType;
  typedef METAIO_STL::list<FEMObjectMaterial*>  MaterialListType;
  typedef METAIO_STL::list<FEMObjectLoad*>  LoadListType;

  /** Access methods*/
  NodeListType & GetNodeList(void) {return m_NodeList;}
  const NodeListType & GetNodeList(void) const {return m_NodeList;}

  ElementListType & GetElementList(void) {return m_ElementList;}
  const ElementListType & GetElementList(void) const {return m_ElementList;}

  MaterialListType & GetMaterialList(void) {return m_MaterialList;}
  const MaterialListType & GetMaterialList(void) const {return m_MaterialList;}

  LoadListType & GetLoadList(void) {return m_LoadList;}
  const LoadListType & GetLoadList(void) const {return m_LoadList;}

protected:

  void  M_Destroy(void);

  void  M_SetupReadFields(void);

  void  M_SetupWriteFields(void);

  bool  M_Read(void);

  bool  M_Write(void);

  /** For reading and writing in node details */
  bool  M_Read_Node();

  void M_Write_Node(FEMObjectNode *Node);

  void M_Write_Material(FEMObjectMaterial *Material);

  void M_Write_Element(FEMObjectElement *Element);

  void M_Write_Load(FEMObjectLoad *Load);

  /** For reading in element details. The input is the name of the element from FEM namespace */
  bool  M_Read_Element(std::string element_name);

  /** For reading in element details. The input is the name of the element from FEM namespace */
  bool  M_Read_Material(std::string material_name);

  /** For reading in element details. The input is the name of the element from FEM namespace */
  bool  M_Read_Load(std::string load_name);

  /** Read in only the keywords that are in the  'ClassNameListType' list container*/
  bool IsClassNamePresent(std::string c_string);

  /** Global number is common for all entity lists of FEM*/
  int ReadGlobalNumber();

  void  SkipWhiteSpace();

  /** Based on the element name get the number of nodes and the dimension*/
  int* GetElementDimensionAndNumberOfNodes(std::string c_string, int info[2]);

  // variables
  static const std::string whitespaces;

  ClassNameListType m_ClassNameList;
  ElementListType m_ElementList;
  NodeListType m_NodeList;
  MaterialListType m_MaterialList;
  LoadListType m_LoadList;

  std::string  m_ElementDataFileName;

  // to keep track of the type element created
  enum
  {
    NODE            = 0,
    MATERIAL        = 1,
    ELEMENT         = 2,
    LOAD            = 3
  };

};

#if (METAIO_USE_NAMESPACE)
}
#endif


#endif
