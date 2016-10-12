/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfItem.hpp                                                        */
/*                                                                           */
/*  Author:                                                                  */
/*     Kenneth Leiter                                                        */
/*     kenneth.leiter@arl.army.mil                                           */
/*     US Army Research Laboratory                                           */
/*     Aberdeen Proving Ground, MD                                           */
/*                                                                           */
/*     Copyright @ 2011 US Army Research Laboratory                          */
/*     All Rights Reserved                                                   */
/*     See Copyright.txt for details                                         */
/*                                                                           */
/*     This software is distributed WITHOUT ANY WARRANTY; without            */
/*     even the implied warranty of MERCHANTABILITY or FITNESS               */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice             */
/*     for more information.                                                 */
/*                                                                           */
/*****************************************************************************/

#ifndef XDMFITEM_HPP_
#define XDMFITEM_HPP_

// C Compatible Includes
#include "XdmfCore.hpp"
#include "XdmfVisitor.hpp"

#ifdef __cplusplus

// Forward Declarations
class XdmfCoreReader;
class XdmfInformation;
class XdmfVisitor;

// Includes
#include <loki/Visitor.h>
#define vtk_libxml2_reference reference // Reversing VTK name mangling
#include <libxml/xmlexports.h>
#include <libxml/tree.h>
#include <libxml/uri.h>
#include <libxml/xpointer.h>
#include <libxml/xmlreader.h>
#include <map>
#include <set>
#include <string>
#include <vector>
#include "XdmfSharedPtr.hpp"


// Macro that allows children XdmfItems to be attached to a parent XdmfItem.
// -- For Header File
#define XDMF_CHILDREN(ParentClass, ChildClass, ChildName, SearchName)         \
                                                                              \
public:                                                                       \
                                                                              \
  /** Get a ChildClass attached to this item by index.<br>
      Example of use:<br>
      C++<br>
      //Assume that exampleItem is a shared pointer to the ParentClass object<br>
      //Using an XdmfInformation as an example because all XdmfItems have XdmfInformation as a child class<br>
      unsigned int getIndex = 0;<br>
      shared_ptr<XdmfInformation> exampleChild = exampleItem->getInformation(getIndex);<br>
      Python<br>
      '''<br>
      Assume that exampleItem is a shared pointer to the ParentClass object<br>
      Using an XdmfInformation as an example because all XdmfItems have XdmfInformation as a child class<br>
      '''<br>
      getIndex = 0<br>
      exampleChild = exampleItem.getInformation(getIndex)<br>
      @return requested ChildClass. If no ChildClass##s exist at the index,
      a NULL pointer is returned.
  */                                                                          \
  virtual shared_ptr<ChildClass>                                              \
  get##ChildName(const unsigned int index);                                   \
                                                                              \
  /** Get a ChildClass attached to this item by index (const version).<br>
      Example of use:<br>
      C++<br>
      //Assume that exampleItem is a shared pointer to the ParentClass object<br>
      //Using an XdmfInformation as an example because all XdmfItems have XdmfInformation as a child class<br>
      unsigned int getIndex = 0;<br>
      shared_ptr<const XdmfInformation> exampleChildConst = exampleItem->getInformation(getIndex);<br>
      Python: does not support a constant version of this function
      @param index of the ChildClass to retrieve.
      @return requested ChildClass. If no ChildClass##s exist at the index, a
      NULL pointer is returned.
  */                                                                          \
  virtual shared_ptr<const ChildClass>                                        \
  get##ChildName(const unsigned int index) const;                             \
                                                                              \
  /** Get a ChildClass attached to this item by SearchName.<br>
      Example of use:<br>
      C++<br>
      //Assume that exampleItem is a shared pointer to the ParentClass object<br>
      //Using an XdmfInformation as an example because all XdmfItems have XdmfInformation as a child class<br>
      std::string findingInfo = "Find this";<br>
      shared_ptr<XdmfInformation> exampleStringChild = exampleItem->getInformation(findingInfo);<br>
      Python<br>
      '''<br>
      Assume that exampleItem is a shared pointer to the ParentClass object<br>
      Using an XdmfInformation as an example because all XdmfItems have XdmfInformation as a child class<br>
      '''<br>
      findingInfo = "Find this"<br>
      exampleStringChild = exampleItem.getInformation(findingInfo)<br>
      @param SearchName of the ChildClass to retrieve.
      @return requested ChildClass. If no ChildClass##s are found with the
      correct SearchName, a NULL pointer is returned.
  */                                                                          \
  virtual shared_ptr<ChildClass>                                              \
  get##ChildName(const std::string & SearchName);                             \
                                                                              \
  /** Get a ChildClass attached to this item by SearchName (const version).<br>
      Example of use:<br>
      C++<br>
      //Assume that exampleItem is a shared pointer to the ParentClass object<br>
      //Using an XdmfInformation as an example because all XdmfItems have XdmfInformation as a child class<br>
      std::string findingInfo = "Find this";<br>
      shared_ptr<const XdmfInformation> exampleStringChildConst = exampleItem->getInformation(findingInfo);<br>
      Python: does not support a constant version of this function
      @param SearchName of the ChildClass to retrieve.
      @return requested ChildClass  If no ChildClass##s are found with the
      correct SearchName, a NULL pointer is returned.
  */                                                                          \
  virtual shared_ptr<const ChildClass>                                        \
  get##ChildName(const std::string & SearchName) const;                       \
                                                                              \
  /** Get the number of ChildClass##s attached to this item.<br>
      Example of use:<br>
      C++<br>
      //Assume that exampleItem is a shared pointer to the ParentClass object<br>
      //Using an XdmfInformation as an example because all XdmfItems have XdmfInformation as a child class<br>
      unsigned int exampleSize = exampleItem->getNumberInformations();<br>
      Python<br>
      '''<br>
      Assume that exampleItem is a shared pointer to the ParentClass object<br>
      Using an XdmfInformation as an example because all XdmfItems have XdmfInformation as a child class<br>
      '''<br>
      exampleSize = exampleItem.getNumberInformations()<br>
      @return number of ChildClass##s attached to this item.
  */                                                                          \
  virtual unsigned int getNumber##ChildName##s() const;                       \
                                                                              \
  /** Insert a ChildClass into to this item.<br>
      Example of use:<br>
      C++<br>
      shared_ptr<XdmfInformation> exampleItem = XdmfInformation::New("Parent", "This is a parent information");<br>
      shared_ptr<XdmfInformation> addChild = XdmfInformation::New("Child", "This is a child information");<br>
      exampleItem->insert(addChild);<br>
      Python<br>
      exampleItem = XdmfInformation.New("Parent", "This is a parent information")<br>
      addChild = XdmfInformation.New("Child", "This is a child information")<br>
      exampleItem.insert(addChild)<br>
      @param ChildName to attach to this item.
  */                                                                          \
  virtual void insert(const shared_ptr<ChildClass> ChildName);                \
                                                                              \
  /** Remove a ChildClass from this item by index. If no object exists
      at the index, nothing is removed.<br>
      Example of use:<br>
      C++<br>
      //Assume that exampleItem is a shared pointer to the ParentClass object<br>
      //Using an XdmfInformation as an example because all XdmfItems have XdmfInformation as a child class<br>
      unsigned int removeIndex = 0;<br>
      exampleItem->removeInformation(removeIndex);<br>
      Python<br>
      '''<br>
      Assume that exampleItem is a shared pointer to the ParentClass object<br>
      Using an XdmfInformation as an example because all XdmfItems have XdmfInformation as a child class<br>
      '''<br>
      removeIndex = 0<br>
      exampleItem.removeInformation(removeIndex)
      @param index of the ChildClass to remove.
  */                                                                          \
  virtual void remove##ChildName(const unsigned int index);                   \
                                                                              \
  /** Remove a ChildClass from this item by SearchName. If no ChildClass##s
      have the correct SearchName, nothing is removed.<br>
      Example of use:<br>
      C++<br>
      //Assume that exampleItem is a shared pointer to the ParentClass object<br>
      //Using an XdmfInformation as an example because all XdmfItems have XdmfInformation as a child class<br>
      std::string removeInfo = "Remove this";<br>
      exampleItem->removeInformation(removeInfo);<br>
      Python<br>
      '''<br>
      Assume that exampleItem is a shared pointer to the ParentClass object<br>
      Using an XdmfInformation as an example because all XdmfItems have XdmfInformation as a child class<br>
      '''<br>
      removeInfo = "Remove this"<br>
      exampleItem.removeInformation(removeInfo)<br>
      @param SearchName of the ChildClass to remove.
  */                                                                          \
  virtual void remove##ChildName(const std::string & SearchName);             \
                                                                              \
protected :                                                                   \
                                                                              \
  std::vector<shared_ptr<ChildClass> > m##ChildName##s;                       \
                                                                              \
public :

// Macro that allows children XdmfItems to be attached to a parent XdmfItem.
// -- For Implementation File
#define XDMF_CHILDREN_IMPLEMENTATION(ParentClass,                             \
                                     ChildClass,                              \
                                     ChildName,                               \
                                     SearchName)                              \
                                                                              \
  shared_ptr<ChildClass>                                                      \
  ParentClass::get##ChildName(const unsigned int index)                       \
  {                                                                           \
    return boost::const_pointer_cast<ChildClass>                              \
      (static_cast<const ParentClass &>(*this).get##ChildName(index));        \
  }                                                                           \
                                                                              \
  shared_ptr<const ChildClass>                                                \
  ParentClass::get##ChildName(const unsigned int index) const                 \
  {                                                                           \
    if(index < m##ChildName##s.size()) {                                      \
      return m##ChildName##s[index];                                          \
    }                                                                         \
    return shared_ptr<ChildClass>();                                          \
  }                                                                           \
                                                                              \
  shared_ptr<ChildClass>                                                      \
  ParentClass::get##ChildName(const std::string & SearchName)                 \
  {                                                                           \
    return boost::const_pointer_cast<ChildClass>                              \
      (static_cast<const ParentClass &>(*this).get##ChildName(SearchName));   \
  }                                                                           \
                                                                              \
  shared_ptr<const ChildClass>                                                \
  ParentClass::get##ChildName(const std::string & SearchName) const           \
  {                                                                           \
    for(std::vector<shared_ptr<ChildClass> >::const_iterator iter =           \
          m##ChildName##s.begin();                                            \
        iter != m##ChildName##s.end();                                        \
        ++iter) {                                                             \
      if((*iter)->get##SearchName().compare(SearchName) == 0) {               \
        return *iter;                                                         \
      }                                                                       \
    }                                                                         \
    return shared_ptr<ChildClass>();                                          \
  }                                                                           \
                                                                              \
  unsigned int                                                                \
  ParentClass::getNumber##ChildName##s() const                                \
  {                                                                           \
    return m##ChildName##s.size();                                            \
  }                                                                           \
                                                                              \
  void                                                                        \
  ParentClass::insert(const shared_ptr<ChildClass> ChildName)                 \
  {                                                                           \
    m##ChildName##s.push_back(ChildName);                                     \
    this->setIsChanged(true);                                                 \
  }                                                                           \
                                                                              \
  void                                                                        \
  ParentClass::remove##ChildName(const unsigned int index)                    \
  {                                                                           \
    if(index < m##ChildName##s.size()) {                                      \
      m##ChildName##s.erase(m##ChildName##s.begin() + index);                 \
    }                                                                         \
    this->setIsChanged(true);                                                 \
  }                                                                           \
                                                                              \
  void                                                                        \
  ParentClass::remove##ChildName(const std::string & SearchName)              \
  {                                                                           \
    for(std::vector<shared_ptr<ChildClass> >::iterator iter =                 \
          m##ChildName##s.begin();                                            \
        iter != m##ChildName##s.end();                                        \
        ++iter) {                                                             \
        if((*iter)->get##SearchName().compare(SearchName) == 0) {             \
          m##ChildName##s.erase(iter);                                        \
          return;                                                             \
        }                                                                     \
    }                                                                         \
    this->setIsChanged(true);                                                 \
  }

/**
 * @brief Base class of any object that is able to be added to an Xdmf
 * structure.
 *
 * XdmfItem is an abstract base class. An XdmfItem is a structure that
 * can be visited and traversed by an XdmfVisitor and have its
 * contents written to an Xdmf file.
 */

class XDMFCORE_EXPORT XdmfItem : public Loki::BaseVisitable<void> {

public:

  virtual ~XdmfItem() = 0;

  LOKI_DEFINE_VISITABLE_BASE()
  XDMF_CHILDREN(XdmfItem, XdmfInformation, Information, Key)
  friend class XdmfCoreReader;
  friend class XdmfWriter;
  friend class XdmfHeavyDataWriter;
  friend class XdmfHDF5Writer;

  /**
   * Get the tag for this item.  This is equivalent to tags in XML
   * parlance.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfItem.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getItemTag
   * @until //#getItemTag
   *
   * Python
   *
   * @dontinclude XdmfExampleItem.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//getItemTag
   * @until #//getItemTag
   *
   * @return    The tag for this XdmfItem.
   */
  virtual std::string getItemTag() const = 0;

  /**
   * Get the key/value property pairs for this item. These are
   * equivalent to attributes in XML parlance.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfItem.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getItemProperties
   * @until //#getItemProperties
   *
   * Python
   *
   * @dontinclude XdmfExampleItem.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//getItemProperties
   * @until #//getItemProperties
   *
   * @return    A map of key/value properties associated with this XdmfItem.
   */
  virtual std::map<std::string, std::string> getItemProperties() const = 0;

  /**
   * Traverse this item by passing the visitor to child items.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfItem.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#traverse
   * @until //#traverse
   *
   * Python
   *
   * @dontinclude XdmfExampleItem.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//traverse
   * @until #//traverse
   *
   * @param     visitor         The visitor to pass to child items.
   */
  virtual void traverse(const shared_ptr<XdmfBaseVisitor> visitor);

  XdmfItem(const XdmfItem &);

protected:

  XdmfItem();

  /**
   * Populates an item using a map of key/value property pairs and a
   * vector of its child items. This is used to support generic
   * reading of XdmfItems from disk.
   *
   * @param itemProperties a map of key/value properties associated with
   * this item.
   * @param childItems a vector of child items to be added to this item.
   * @param reader the current XdmfCoreReader being used to populate Xdmf
   * structures.
   */
  virtual void
  populateItem(const std::map<std::string, std::string> & itemProperties,
               const std::vector<shared_ptr<XdmfItem > > & childItems,
               const XdmfCoreReader * const reader);

  bool
  getIsChanged();

  void
  setIsChanged(bool status);

  std::set<XdmfItem *> mParents;

  bool mIsChanged;

private:

//  XdmfItem(const XdmfItem &);  // It is implemented for C wrappers.
  void operator=(const XdmfItem &);  // Not implemented.

};

#endif

#ifdef __cplusplus
extern "C" {
#endif

// C wrappers go here

struct XDMFITEM; // Simply as a typedef to ensure correct typing
typedef struct XDMFITEM XDMFITEM;

#ifndef XDMFINFORMATIONCDEFINE
#define XDMFINFORMATIONCDEFINE
struct XDMFINFORMATION; // Simply as a typedef to ensure correct typing
typedef struct XDMFINFORMATION XDMFINFORMATION;
#endif

XDMFCORE_EXPORT void XdmfItemAccept(XDMFITEM * item, XDMFVISITOR * visitor, int * status);

XDMFCORE_EXPORT void XdmfItemFree(void * item);

XDMFCORE_EXPORT XDMFINFORMATION * XdmfItemGetInformation(XDMFITEM * item, unsigned int index);

XDMFCORE_EXPORT XDMFINFORMATION * XdmfItemGetInformationByKey(XDMFITEM * item, char * key);

XDMFCORE_EXPORT unsigned int XdmfItemGetNumberInformations(XDMFITEM * item);

XDMFCORE_EXPORT void XdmfItemInsertInformation(XDMFITEM * item, XDMFINFORMATION * information, int passControl);

XDMFCORE_EXPORT void XdmfItemRemoveInformation(XDMFITEM * item, unsigned int index);

XDMFCORE_EXPORT void XdmfItemRemoveInformationByKey(XDMFITEM * item, char * key);

XDMFCORE_EXPORT char * XdmfItemGetItemTag(XDMFITEM * item);

#define XDMF_ITEM_C_CHILD_DECLARE(ClassName, CClassName, Level)                                                       \
                                                                                                                      \
Level##_EXPORT void ClassName##Accept ( CClassName * item, XDMFVISITOR * visitor, int * status);                      \
XDMFCORE_EXPORT void ClassName##Free(void * item);                                                                    \
Level##_EXPORT XDMFINFORMATION * ClassName##GetInformation( CClassName * item, unsigned int index);                   \
Level##_EXPORT XDMFINFORMATION * ClassName##GetInformationByKey( CClassName * item, char * key);                      \
Level##_EXPORT unsigned int ClassName##GetNumberInformations( CClassName * item);                                     \
Level##_EXPORT void ClassName##InsertInformation( CClassName * item, XDMFINFORMATION * information, int passControl); \
Level##_EXPORT void ClassName##RemoveInformation( CClassName * item, unsigned int index);                             \
Level##_EXPORT void ClassName##RemoveInformationByKey( CClassName * item, char * key);                                \
Level##_EXPORT char * ClassName##GetItemTag( CClassName * item);


#define XDMF_ITEM_C_CHILD_WRAPPER(ClassName, CClassName)                                                    \
void ClassName##Accept( CClassName * item, XDMFVISITOR * visitor, int * status)                             \
{                                                                                                           \
  XdmfItemAccept(((XDMFITEM *)item), visitor, status);                                                      \
}                                                                                                           \
                                                                                                            \
void ClassName##Free(void * item)                                                                           \
{                                                                                                           \
  XdmfItemFree(item);                                                                                       \
}                                                                                                           \
                                                                                                            \
XDMFINFORMATION * ClassName##GetInformation( CClassName * item, unsigned int index)                         \
{                                                                                                           \
  return XdmfItemGetInformation(((XDMFITEM *)item), index);                                                 \
}                                                                                                           \
                                                                                                            \
XDMFINFORMATION * ClassName##GetInformationByKey( CClassName * item, char * key)                            \
{                                                                                                           \
  return XdmfItemGetInformationByKey(((XDMFITEM *)item), key);                                              \
}                                                                                                           \
                                                                                                            \
unsigned int ClassName##GetNumberInformations( CClassName * item)                                           \
{                                                                                                           \
  return XdmfItemGetNumberInformations(((XDMFITEM *)item));                                                 \
}                                                                                                           \
                                                                                                            \
void ClassName##InsertInformation( CClassName * item, XDMFINFORMATION * information, int passControl)       \
{                                                                                                           \
  XdmfItemInsertInformation(((XDMFITEM *)item), information, passControl);                                  \
}                                                                                                           \
                                                                                                            \
void ClassName##RemoveInformation( CClassName * item, unsigned int index)                                   \
{                                                                                                           \
  XdmfItemRemoveInformation(((XDMFITEM *)item), index);                                                     \
}                                                                                                           \
                                                                                                            \
void ClassName##RemoveInformationByKey( CClassName * item, char * key)                                      \
{                                                                                                           \
  XdmfItemRemoveInformationByKey(((XDMFITEM *)item), key);                                                  \
}                                                                                                           \
                                                                                                            \
char * ClassName##GetItemTag( CClassName * item)                                                            \
{                                                                                                           \
  return XdmfItemGetItemTag(((XDMFITEM *)item));                                                            \
} 

#ifdef __cplusplus
}
#endif

#endif /* XDMFITEM_HPP_ */
