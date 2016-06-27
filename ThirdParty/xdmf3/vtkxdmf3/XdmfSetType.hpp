#ifndef XDMFSETTYPE_HPP_
#define XDMFSETTYPE_HPP_

// C Compatible Includes
#include "Xdmf.hpp"

#ifdef __cplusplus

// Includes
#include "XdmfItemProperty.hpp"

/**
 * @brief Property describing the type of ids an XdmfSet contains.
 *
 * An XdmfSet holds ids for a collection of nodes, cells, faces, or
 * edges that are part of an XdmfGrid. This property indicates which
 * type the set contains.
 *
 * Example of use:
 *
 * C++
 *
 * @dontinclude ExampleXdmfSet.cpp
 * @skipline //#initialize
 * @until //#initialize
 * @skipline //#setType
 * @until //#setType
 * @skipline //#getType
 * @until //#getType
 *
 * Python
 *
 * @dontinclude XdmfExampleSet.py
 * @skipline #//initialize
 * @until #//initialize
 * @skipline #//setType
 * @until #//setType
 * @skipline #//getType
 * @until #//getType
 *
 * Xdmf supports the following set types:
 *   NoSetType
 *   Node
 *   Cell
 *   Face
 *   Edge
 */
class XDMF_EXPORT XdmfSetType : public XdmfItemProperty {

public:

  virtual ~XdmfSetType();

  friend class XdmfSet;

  // Supported Xdmf Set Types
  static shared_ptr<const XdmfSetType> NoSetType();
  static shared_ptr<const XdmfSetType> Node();
  static shared_ptr<const XdmfSetType> Cell();
  static shared_ptr<const XdmfSetType> Face();
  static shared_ptr<const XdmfSetType> Edge();

  void
  getProperties(std::map<std::string, std::string> & collectedProperties) const;

protected:

  /**
   * Protected constructor for XdmfSetType. The constructor is
   * protected because all set types supported by Xdmf should be
   * accessed through more specific static methods that construct
   * XdmfSetTypes - i.e. XdmfSetType::Node().
   *
   * @param     name    A std::string containing the name of the XdmfSetType.
   */
  XdmfSetType(const std::string & name);

  static std::map<std::string, shared_ptr<const XdmfSetType>(*)()> mSetDefinitions;

  static void InitTypes();

private:

  XdmfSetType(const XdmfSetType &); // Not implemented.
  void operator=(const XdmfSetType &); // Not implemented.

  static shared_ptr<const XdmfSetType>
  New(const std::map<std::string, std::string> & itemProperties);

  std::string mName;
};

#endif

#ifdef __cplusplus
extern "C" {
#endif

// C wrappers go here

#define XDMF_SET_TYPE_NO_SET_TYPE 600
#define XDMF_SET_TYPE_NODE        601
#define XDMF_SET_TYPE_CELL        602
#define XDMF_SET_TYPE_FACE        603
#define XDMF_SET_TYPE_EDGE        604

XDMF_EXPORT int XdmfSetTypeNoSetType();
XDMF_EXPORT int XdmfSetTypeNode();
XDMF_EXPORT int XdmfSetTypeCell();
XDMF_EXPORT int XdmfSetTypeFace();
XDMF_EXPORT int XdmfSetTypeEdge();

#ifdef __cplusplus
}
#endif

#endif /* XDMFSETTYPE_HPP_ */
