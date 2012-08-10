/*******************************************************************/
/*                               XDMF                              */
/*                   eXtensible Data Model and Format              */
/*                                                                 */
/*  Id : Id  */
/*  Date : $Date$ */
/*  Version : $Revision$ */
/*                                                                 */
/*  Author:                                                        */
/*     Jerry A. Clarke                                             */
/*     clarke@arl.army.mil                                         */
/*     US Army Research Laboratory                                 */
/*     Aberdeen Proving Ground, MD                                 */
/*                                                                 */
/*     Copyright @ 2006 US Army Research Laboratory                */
/*     All Rights Reserved                                         */
/*     See Copyright.txt or http://www.arl.hpc.mil/ice for details */
/*                                                                 */
/*     This software is distributed WITHOUT ANY WARRANTY; without  */
/*     even the implied warranty of MERCHANTABILITY or FITNESS     */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice   */
/*     for more information.                                       */
/*                                                                 */
/*******************************************************************/
#ifndef __XdmfElement_h
#define __XdmfElement_h

#include "XdmfLightData.h"

#define XDMF_ELEMENT_STATE_UNINITIALIZED 0
#define XDMF_ELEMENT_STATE_LIGHT_PARSED  1
#define XDMF_ELEMENT_STATE_HEAVY_READ    2

class XdmfDOM;
class XdmfDsmBuffer;

//! Basic XML Based Element
/*!
    XdmfElement represents an Element in the LightData. For example,
    XdmfInformation, XdmfGrid, XdmfTopology etc. are all elements.
    Elements have "Attributes" which are the Name=Value pairs in the
    XML. Elements can also have children elements and CDATA which is the
    Character Data of the Element. Consider :
\verbatim

    <Element Name="Stuff">
        November 28, 1962
        <Element Name="XBounds" Value="0.0 100.0" />
    </Element>

\endverbatim
    In the first Element, Name is the one and only Attribute. The second
    Element is a child of the first and has the Attributes : Name and Value.
    November 28, 1962 is the CDATA of the first Element.

    Elements can also reference other elements. This is particularly useful for
    "DataStructure" elements to minimize the amount of I/O that is performed.
    References are accomplished via the XML facility known as XPath. Consider :
\verbatim

<DataStructure Reference="/Xdmf/Domain/Grid[@Name='Shot Points']/Geometry/DataStructure[2]"/>
                    OR
<DataStructure Reference="XML">
    /Xdmf/Domain/Grid[@Name="Shot Points"]/Geometry/DataStructure[2]
</DataStructure>

\endverbatim
    This says that this DataStructure information can be found in the 2nd DataStructure (1 based) under
    the "Geometry" element, under the "Grid" element who's Name='Shot Points', under "Domain"
    under "Xdmf", in the current file. There are many expressions in XPath, see XPath documentation
    and Web pages for more.

    If an XDMF Object has already the target node via UpdateInformation() / Update(), a Reference
    node will point to the data of the original object. So each XML node that is Parsed will be
    owned by some object.

    References can point to other references to form a chain.
*/
class XDMF_EXPORT XdmfElement : public XdmfLightData {

public:
    XdmfElement();
    virtual ~XdmfElement();
    virtual XdmfConstString GetClassName() { return("XdmfElement"); } ;

    //! Set the DOM to use
    XdmfSetValueMacro(DOM, XdmfDOM *);
    //! Get the current DOM
    XdmfGetValueMacro(DOM, XdmfDOM *);

    //! Set the current State
    XdmfSetValueMacro(State, XdmfInt32 );
    //! Get the current State
    XdmfGetValueMacro(State, XdmfInt32 );

    //! Does this Element Reference Some Other Element
    XdmfSetValueMacro(IsReference, XdmfInt32);
    //! Does this Element Reference Some Other Element
    XdmfGetValueMacro(IsReference, XdmfInt32);

    //! Check to see if Element references another XML node (which will then be returned)
    /*!
    \param Element is the head XML node of a potential reference chain
    */
    virtual XdmfXmlNode CheckForReference(XdmfXmlNode Element);

    //! Add a child Node
    XdmfInt32 InsertChildElement(XdmfXmlNode Child);

    //! Return the XML representation
    XdmfConstString Serialize();

//! Set the XML Node from which to parse
/*! 
        \param Element is the low level node returned from XdmfDOM->FindElement() etc.
        \param AssociateElement Should the node in the DOM be associated with the XdmfElement (1)
*/
    XdmfInt32   SetElement(XdmfXmlNode Element, XdmfInt32 AssociateElement=1);
//! Get the XML Node
    XdmfGetValueMacro(Element, XdmfXmlNode);

    //! Tells the Element if data is to be copied out of a reference.
    /*!
        If an element is a reference and another element has already accessed the data there
        are two choices. First, copy the data out of the element that contains the data. This
        is the default and the safest, but leads to additional memory requirements. The second
        option is to just point to the original data. For this to work in the general caes, a 
        refernce counting / garbage collection scheme needs to be used. Otherwise, the destruction
        of the original element will leave an invalid reference in the new element. In short, setting
        CopyReferenceData to 0 should be done with care.
    */
    XdmfSetValueMacro(CopyReferenceData, XdmfInt32);
    //! Get the flag if data is to be copied out of a reference.
    XdmfGetValueMacro(CopyReferenceData, XdmfInt32);

    //! Get the ElementName. i.e. ElementName for <Information ... = "Information"
    XdmfGetStringMacro(ElementName);
    //! Set the ElementName. i.e. ElementName for <Information ... = "Information"
    XdmfSetStringMacro(ElementName);

    //! Get the DataXml. This is used by Build() when HeavyData has already been written
    XdmfGetStringMacro(DataXml);
    //! Set the DataXml. This will override the Build() method and inject the raw XML
    XdmfSetStringMacro(DataXml);

    //! Get the DataXml. This is used by Build() when HeavyData has already been written
    XdmfGetStringMacro(InsertedDataXml);
    //! Set the DataXml. This will override the Build() method and inject the raw XML
    XdmfInt32 SetInsertedDataXml(XdmfString Inserted){this->InsertedDataXml = Inserted; return(XDMF_SUCCESS);};

    //! Build XML from user supplied DataXml
    XdmfInt32 BuildFromDataXml(XdmfInt32 AllowCData = 0);

    //! Get the Element type : Grid, Topology, etc.
    XdmfConstString GetElementType();

    //! Initialize basic structure from XML (INPUT)
    virtual XdmfInt32 UpdateInformation();

    //! Initialize all information. Possibly acessing Heavy Data. (INPUT)
    virtual XdmfInt32 Update();

    //! Update the DOM from the Basic Structure
    virtual XdmfInt32 Build();

    //! Set the Value of an Attribute (OUTPUT)
    XdmfInt32 Set(XdmfConstString Name, XdmfConstString Value);

    //! Get the Value of An Attribute (INPUT)
    XdmfConstString Get(XdmfConstString Name);

    //! Copy Information from Another Element. Overridden in Child Class
    virtual XdmfInt32 Copy(XdmfElement *Source);

    //! Insert a child Element. Overridden in child Class to check ElementName
    virtual XdmfInt32   Insert(XdmfElement *Child = 0);

    //! Follow a Refernce Chain one step, if it exists
    XdmfXmlNode FollowReference(XdmfXmlNode Element);
    //! Get DSM Buffer
    XdmfGetValueMacro(DsmBuffer, XdmfDsmBuffer *);
    //! Set DSM Buffer
    XdmfSetValueMacro(DsmBuffer, XdmfDsmBuffer *);
    //! Delete the Element when the XdmfGrid container is deleted. Default = 0
    XdmfSetValueMacro(DeleteOnGridDelete, XdmfInt32);
    //! Delete the Element when the XdmfGrid container is deleted. Default = 0
    XdmfGetValueMacro(DeleteOnGridDelete, XdmfInt32);

protected:
    void        SetReferenceObject(XdmfXmlNode Element, void *p);
    void        *GetReferenceObject(XdmfXmlNode Element);
    //! Set the XdmfElement Object Assigned to the XML Element in the DOM
    void        SetCurrentXdmfElement(XdmfXmlNode Element, void *p);
    //! Get the XdmfElement Object Assigned to the XML Element in the DOM
    void        *GetCurrentXdmfElement(XdmfXmlNode Element);
    XdmfDOM     *DOM;
    XdmfInt32   State;
    XdmfString  ElementName;
    XdmfString  DataXml;
    XdmfString  InsertedDataXml;
    //! Target XML That Represents this. In the case of a reference, this is the target XML not the Reference="XX" node.
    XdmfXmlNode Element;
    //! If this is a Reference XML, this is the head of the Reference chain (the Original XML node).
    XdmfXmlNode ReferenceElement;
    XdmfInt32   IsReference;
    // Copy, don't copy data out of reference element. Default is to copy.
    XdmfInt32   CopyReferenceData;
    XdmfDsmBuffer *DsmBuffer;
    XdmfXmlNode RootWhenParsed;
    XdmfInt32   DeleteOnGridDelete;
};

class XDMF_EXPORT XdmfElementData : public XdmfObject {
    public :
        XdmfElementData();
        ~XdmfElementData();

        XdmfSetValueMacro(ReferenceElement, XdmfElement *);
        XdmfGetValueMacro(ReferenceElement, XdmfElement *);
        XdmfSetValueMacro(CurrentXdmfElement, XdmfElement *);
        XdmfGetValueMacro(CurrentXdmfElement, XdmfElement *);
    protected :
        XdmfElement     *ReferenceElement;
        XdmfElement     *CurrentXdmfElement;
};

#endif // __XdmfElement_h
