/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfInformation.hpp                                                 */
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

#ifndef XDMFINFORMATION_HPP_
#define XDMFINFORMATION_HPP_

// C Compatible Includes
#include "XdmfCore.hpp"
#include "XdmfItem.hpp"
#include "XdmfArray.hpp"

#ifdef __cplusplus

// Forward declarations
class XdmfArray;

/**
 * @brief Holds a key/value pair that can be attached to an Xdmf
 * structure.
 *
 * XdmfInformation stores two strings as a key value pair. These can
 * be used to store input parameters to a code or for simple result
 * data like wall time.
 */
class XDMFCORE_EXPORT XdmfInformation : public XdmfItem {

public:

  /**
   * Create a new XdmfInformation.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfInformation.cpp
   * @skipline //#initialization
   * @until //#initialization
   *
   * Python
   *
   * @dontinclude XdmfExampleInformation.py
   * @skipline #//initialization
   * @until #//initialization
   *
   * @return    Constructed XdmfInformation.
   */
  static shared_ptr<XdmfInformation> New();

  /**
   * Create a new XdmfInformation.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfInformation.cpp
   * @skipline //#initializationfull
   * @until //#initializationfull
   *
   * Python
   *
   * @dontinclude XdmfExampleInformation.py
   * @skipline #//initializationfull
   * @until #//initializationfull
   *
   * @param     key     A string containing the key of the XdmfInformation to create.
   * @param     value   A string containing the value of the XdmfInformation to
   * create.
   *
   * @return    Constructed XdmfInformation
   */
  static shared_ptr<XdmfInformation> New(const std::string & key,
                                         const std::string & value);

  virtual ~XdmfInformation();

  LOKI_DEFINE_VISITABLE(XdmfInformation, XdmfItem)
  XDMF_CHILDREN(XdmfInformation, XdmfArray, Array, Name)
  static const std::string ItemTag;

  std::map<std::string, std::string> getItemProperties() const;

  virtual std::string getItemTag() const;

  /**
   * Get the key for this information item.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfInformation.cpp
   * @skipline //#initializationfull
   * @until //#initializationfull
   * @skipline //#getKey
   * @until //#getKey
   *
   * Python
   *
   * @dontinclude XdmfExampleInformation.py
   * @skipline #//initializationfull
   * @until #//initializationfull
   * @skipline #//getKey
   * @until #//getKey
   *
   * @return    A string containing the key.
   */
  std::string getKey() const;

  /**
   * Get the value for this information item.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfInformation.cpp
   * @skipline //#initializationfull
   * @until //#initializationfull
   * @skipline //#getValue
   * @until //#getValue
   *
   * Python
   *
   * @dontinclude XdmfExampleInformation.py
   * @skipline #//initializationfull
   * @until #//initializationfull
   * @skipline #//getValue
   * @until #//getValue
   *
   * @return    A string containing the value.
   */
  std::string getValue() const;

  using XdmfItem::insert;

  /**
   * Set the key for this information item.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfInformation.cpp
   * @skipline //#initializationfull
   * @until //#initializationfull
   * @skipline //#setKey
   * @until //#setKey
   *
   * Python
   *
   * @dontinclude XdmfExampleInformation.py
   * @skipline #//initializationfull
   * @until #//initializationfull
   * @skipline #//setKey
   * @until #//setKey
   *
   * @param     key     A string containing the key to set.
   */
  void setKey(const std::string & key);

  /**
   * Set the value for this information item.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfInformation.cpp
   * @skipline //#initializationfull
   * @until //#initializationfull
   * @skipline //#setValue
   * @until //#setValue
   *
   * Python
   *
   * @dontinclude XdmfExampleInformation.py
   * @skipline #//initializationfull
   * @until #//initializationfull
   * @skipline #//setValue
   * @until #//setValue
   *
   * @param     value   A string containing the value to set.
   */
  void setValue(const std::string & value);

  virtual void traverse(const shared_ptr<XdmfBaseVisitor> visitor);

  XdmfInformation(XdmfInformation &);

protected:

  XdmfInformation(const std::string & key = "",
                  const std::string & value = "");

  virtual void
  populateItem(const std::map<std::string, std::string> & itemProperties,
               const std::vector<shared_ptr<XdmfItem> > & childItems,
               const XdmfCoreReader * const reader);

private:

  XdmfInformation(const XdmfInformation &);
  void operator=(const XdmfInformation &);  // Not implemented.

  std::string mKey;
  std::string mValue;
};

#endif

#ifdef __cplusplus
extern "C" {
#endif

// C wrappers go here

#ifndef XDMFINFORMATIONCDEFINE
#define XDMFINFORMATIONCDEFINE
struct XDMFINFORMATION; // Simply as a typedef to ensure correct typing
typedef struct XDMFINFORMATION XDMFINFORMATION;
#endif

XDMFCORE_EXPORT XDMFINFORMATION * XdmfInformationNew(char * key, char * value);

XDMFCORE_EXPORT XDMFARRAY * XdmfInformationGetArray(XDMFINFORMATION * information, unsigned int index);

XDMFCORE_EXPORT XDMFARRAY * XdmfInformationGetArrayByName(XDMFINFORMATION * information, char * name);

XDMFCORE_EXPORT char * XdmfInformationGetKey(XDMFINFORMATION * information);

XDMFCORE_EXPORT unsigned int XdmfInformationGetNumberArrays(XDMFINFORMATION * information);

XDMFCORE_EXPORT char * XdmfInformationGetValue(XDMFINFORMATION * information);

XDMFCORE_EXPORT void XdmfInformationInsertArray(XDMFINFORMATION * information, XDMFARRAY * array, int transferOwnership);

XDMFCORE_EXPORT void XdmfInformationRemoveArray(XDMFINFORMATION * information, unsigned int index);

XDMFCORE_EXPORT void XdmfInformationRemoveArrayByName(XDMFINFORMATION * information, char * name);

XDMFCORE_EXPORT void XdmfInformationSetKey(XDMFINFORMATION * information, char * key, int * status);

XDMFCORE_EXPORT void XdmfInformationSetValue(XDMFINFORMATION * information, char * value, int * status);

// C Wrappers for parent classes are generated by macros

XDMF_ITEM_C_CHILD_DECLARE(XdmfInformation, XDMFINFORMATION, XDMFCORE)

#ifdef __cplusplus
}
#endif

#endif /* XDMFINFORMATION_HPP_ */
