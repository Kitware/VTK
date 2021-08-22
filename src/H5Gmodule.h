/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://www.hdfgroup.org/licenses.               *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * Programmer:	Quincey Koziol
 *		Saturday, September 12, 2015
 *
 * Purpose:	This file contains declarations which define macros for the
 *		H5G package.  Including this header means that the source file
 *		is part of the H5G package.
 */
#ifndef H5Gmodule_H
#define H5Gmodule_H

/* Define the proper control macros for the generic FUNC_ENTER/LEAVE and error
 *      reporting macros.
 */
#define H5G_MODULE
#define H5_MY_PKG      H5G
#define H5_MY_PKG_ERR  H5E_SYM
#define H5_MY_PKG_INIT YES

/**
 * \defgroup H5G H5G
 *
 * \details \Bold{Groups in HDF5:} A group associates names with objects and
 *          provides a mechanism for mapping a name to an object. Since all
 *          objects appear in at least one group (with the possible exception of
 *          the root object) and since objects can have names in more than one
 *          group, the set of all objects in an HDF5 file is a directed
 *          graph. The internal nodes (nodes with out-degree greater than zero)
 *          must be groups while the leaf nodes (nodes with out-degree zero) are
 *          either empty groups or objects of some other type. Exactly one
 *          object in every non-empty file is the root object. The root object
 *          always has a positive in-degree because it is pointed to by the file
 *          super block.
 *
 *          \Bold{Locating objects in the HDF5 file hierarchy:} An object name
 *          consists of one or more components separated from one another by
 *          slashes. An absolute name begins with a slash and the object is
 *          located by looking for the first component in the root object, then
 *          looking for the second component in the first object, etc., until
 *          the entire name is traversed. A relative name does not begin with a
 *          slash and the traversal begins at the location specified by the
 *          create or access function.
 *
 *          \Bold{Group implementations in HDF5:} The original HDF5 group
 *          implementation provided a single indexed structure for link
 *          storage. A new group implementation, in HDF5 Release 1.8.0, enables
 *          more efficient compact storage for very small groups, improved link
 *          indexing for large groups, and other advanced features.
 *
 *          \li The \Emph{original indexed} format remains the default. Links
 *              are stored in a B-tree in the group’s local heap.
 *          \li Groups created in the new \Emph{compact-or-indexed} format, the
 *              implementation introduced with Release 1.8.0, can be tuned for
 *              performance, switching between the compact and indexed formats
 *              at thresholds set in the user application.
 *              - The \Emph{compact} format will conserve file space and processing
 *                overhead when working with small groups and is particularly
 *                valuable when a group contains no links. Links are stored
 *                as a list of messages in the group’s header.
 *              - The \Emph{indexed} format will yield improved
 *                performance when working with large groups, e.g., groups
 *                containing thousands to millions of members. Links are stored in
 *                a fractal heap and indexed with an improved B-tree.
 *          \li The new implementation also enables the use of link names consisting of
 *              non-ASCII character sets (see H5Pset_char_encoding()) and is
 *              required for all link types other than hard or soft links, e.g.,
 *              external and user-defined links (see the \ref H5L APIs).
 *
 *          The original group structure and the newer structures are not
 *          directly interoperable. By default, a group will be created in the
 *          original indexed format. An existing group can be changed to a
 *          compact-or-indexed format if the need arises; there is no capability
 *          to change back. As stated above, once in the compact-or-indexed
 *          format, a group can switch between compact and indexed as needed.
 *
 *          Groups will be initially created in the compact-or-indexed format
 *          only when one or more of the following conditions is met:
 *          \li The low version bound value of the library version bounds property
 *              has been set to Release 1.8.0 or later in the file access property
 *              list (see H5Pset_libver_bounds()). Currently, that would require an
 *              H5Pset_libver_bounds() call with the low parameter set to
 *              #H5F_LIBVER_LATEST.\n When this property is set for an HDF5 file,
 *              all objects in the file will be created using the latest available
 *              format; no effort will be made to create a file that can be read by
 *              older libraries.
 *          \li The creation order tracking property, #H5P_CRT_ORDER_TRACKED, has been
 *              set in the group creation property list (see H5Pset_link_creation_order()).
 *
 *          An existing group, currently in the original indexed format, will be
 *          converted to the compact-or-indexed format upon the occurrence of
 *          any of the following events:
 *          \li An external or user-defined link is inserted into the group.
 *          \li A link named with a string composed of non-ASCII characters is
 *              inserted into the group.
 *
 *          The compact-or-indexed format offers performance improvements that
 *          will be most notable at the extremes, i.e., in groups with zero
 *          members and in groups with tens of thousands of members. But
 *          measurable differences may sometimes appear at a threshold as low as
 *          eight group members. Since these performance thresholds and criteria
 *          differ from application to application, tunable settings are
 *          provided to govern the switch between the compact and indexed
 *          formats (see H5Pset_link_phase_change()). Optimal thresholds will
 *          depend on the application and the operating environment.
 *
 *          Future versions of HDF5 will retain the ability to create, read,
 *          write, and manipulate all groups stored in either the original
 *          indexed format or the compact-or-indexed format.
 *
 */

#endif /* H5Gmodule_H */
