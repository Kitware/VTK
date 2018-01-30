/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://support.hdfgroup.org/ftp/HDF5/releases.  *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* TERNARY SEARCH TREE ALGS
  This code is described in "Ternary Search Trees" by Jon
Bentley and Robert Sedgewick in the April, 1998, Dr. Dobb's Journal.
*/


#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5FLprivate.h"	/* Free lists                           */
#include "H5STprivate.h"        /* Ternary search trees                 */

#ifdef H5ST_DEBUG
static herr_t H5ST__dump_internal(H5ST_ptr_t p);
#endif /* H5ST_DEBUG */

/* Declare a free list to manage the H5ST_node_t struct */
H5FL_DEFINE_STATIC(H5ST_node_t);

/* Declare a free list to manage the H5ST_tree_t struct */
H5FL_DEFINE_STATIC(H5ST_tree_t);


/*--------------------------------------------------------------------------
 NAME
    H5ST_create
 PURPOSE
    Create a TST
 USAGE
    H5ST_ptr_t H5ST_create()

 RETURNS
    Returns a pointer to the new TST tree on success, NULL on failure.
 DESCRIPTION
    Create a TST.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
H5ST_tree_t *
H5ST_create(void)
{
    H5ST_tree_t *ret_value;   /* Return value */

    FUNC_ENTER_NOAPI(NULL)

    /* Allocate wrapper for TST */
    if(NULL == (ret_value = H5FL_MALLOC(H5ST_tree_t)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")

    /* Set the internal fields */
    ret_value->root = NULL;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5ST_create() */


/*--------------------------------------------------------------------------
 NAME
    H5ST_close_internal
 PURPOSE
    Close a TST, deallocating it.
 USAGE
    herr_t H5ST_close(p)
        H5ST_ptr_t p;        IN/OUT: Root of TST to free

 RETURNS
    Returns non-negative on success, negative on failure.
 DESCRIPTION
    Close a TST, freeing all nodes.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5ST_close_internal(H5ST_ptr_t p)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Recursively free TST */
    if(p) {
        H5ST_close_internal(p->lokid);
        if(p->splitchar)
            H5ST_close_internal(p->eqkid);
        H5ST_close_internal(p->hikid);
        p = H5FL_FREE(H5ST_node_t, p);
    } /* end if */

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5ST_close_internal() */


/*--------------------------------------------------------------------------
 NAME
    H5ST_close
 PURPOSE
    Close a TST, deallocating it.
 USAGE
    herr_t H5ST_close(tree)
        H5ST_tree_t *tree;        IN/OUT: TST tree to free

 RETURNS
    Returns non-negative on success, negative on failure.
 DESCRIPTION
    Close a TST, freeing all nodes.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5ST_close(H5ST_tree_t *tree)
{
    herr_t ret_value = SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Check arguments */
    if(NULL == tree)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid TST")

    /* Free the TST itself */
    if(H5ST_close_internal(tree->root) < 0)
        HGOTO_ERROR(H5E_TST, H5E_CANTFREE, FAIL, "can't free TST")

    /* Free root node itself */
    tree = H5FL_FREE(H5ST_tree_t, tree);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5ST_close() */


/*--------------------------------------------------------------------------
 NAME
    H5ST_insert
 PURPOSE
    Insert a string/object pair into a TST
 USAGE
    herr_t H5ST_insert(tree,s,obj)
        H5ST_tree_t *tree;      IN/OUT: TST to insert string into
        const char *s;          IN: String to use as key for object
        void *obj;              IN: Pointer to object to insert

 RETURNS
    Returns non-negative on success, negative on failure.
 DESCRIPTION
    Insert a key (string)/object pair into a TST
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5ST_insert(H5ST_tree_t *tree, const char *s, void *obj)
{
    int d;                      /* Comparison value */
    H5ST_ptr_t pp, *p;          /* Pointer to current node and pointer to that */
    H5ST_ptr_t parent=NULL;     /* Pointer to parent node */
    H5ST_ptr_t up=NULL;         /* Pointer to up node */
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Find the correct location to insert object */
    p = &tree->root;
    while((pp = *p)) {
        /* If this node matches the character in the key, then drop down to the lower tree */
        if(0 == (d = *s - pp->splitchar)) {
            if(*s++ == 0)
                HGOTO_ERROR(H5E_TST, H5E_EXISTS, FAIL, "key already in tree")
            up=pp;
            p = &(pp->eqkid);
        } /* end if */
        else {
            /* Walk through the current tree, searching for the matching character */
            parent = pp;
            if(d < 0)
                p = &(pp->lokid);
            else
                p = &(pp->hikid);
        } /* end else */
    } /* end while */

    /* Finish walking through the key string, adding nodes until the end */
    for (;;) {
        if(NULL == (*p = H5FL_MALLOC(H5ST_node_t)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")
        pp = *p;
        pp->splitchar = *s;
        pp->up = up;
        pp->parent = parent;
        pp->lokid = pp->eqkid = pp->hikid = NULL;

        /* If this is the end of the key string, break out */
        if(*s++ == 0) {
            pp->eqkid = (H5ST_ptr_t)obj;
            break;
        } /* end if */

        /* Continue to next character */
        parent = NULL;
        up = pp;
        p = &(pp->eqkid);
    } /* end for */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5ST_insert() */


/*--------------------------------------------------------------------------
 NAME
    H5ST_search
 PURPOSE
    Determine if a key is in the TST
 USAGE
    hbool_t H5ST_search(tree,s)
        H5ST_tree_t *tree;      IN: TST to find string in
        const char *s;          IN: String to use as key to locate

 RETURNS
    Success: TRUE if key string in TST, FALSE if not
    Failure: negative
 DESCRIPTION
    Locate a key (string) in a TST
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
htri_t
H5ST_search(H5ST_tree_t *tree, const char *s)
{
    H5ST_ptr_t p;               /* Temporary pointer to TST node */
    htri_t ret_value=FALSE;     /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    p = tree->root;
    while (p) {
        if (*s < p->splitchar)
            p = p->lokid;
        else if (*s == p->splitchar)  {
            if (*s++ == 0)
                HGOTO_DONE(TRUE);
            p = p->eqkid;
        } else
            p = p->hikid;
    } /* end while */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5ST_search() */


/*--------------------------------------------------------------------------
 NAME
    H5ST_find_internal
 PURPOSE
    Find the node matching a particular key string
 USAGE
    H5ST_ptr_t H5ST_find(p,s)
        H5ST_ptr_t p;           IN: TST to find string in
        const char *s;          IN: String to use as key to locate

 RETURNS
    Success: Non-NULL
    Failure: NULL
 DESCRIPTION
    Locate a key (string) in a TST
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static H5ST_ptr_t
H5ST_find_internal(H5ST_ptr_t p, const char *s)
{
    H5ST_ptr_t ret_value = NULL;  /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    while (p) {
        if (*s < p->splitchar)
            p = p->lokid;
        else if (*s == p->splitchar)  {
            if (*s++ == 0)
                HGOTO_DONE(p);
            p = p->eqkid;
        } else
            p = p->hikid;
    } /* end while */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5ST_find_internal() */


/*--------------------------------------------------------------------------
 NAME
    H5ST_find
 PURPOSE
    Find the node matching a particular key string
 USAGE
    H5ST_ptr_t H5ST_find(tree,s)
        H5ST_tree_t *tree;      IN: TST to find string in
        const char *s;          IN: String to use as key to locate

 RETURNS
    Success: Non-NULL
    Failure: NULL
 DESCRIPTION
    Locate a key (string) in a TST
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
H5ST_ptr_t
H5ST_find(H5ST_tree_t *tree, const char *s)
{
    H5ST_ptr_t ret_value;  /* Return value */

    FUNC_ENTER_NOAPI(NULL)

    if(NULL == (ret_value = H5ST_find_internal(tree->root, s)))
        HGOTO_ERROR(H5E_TST, H5E_NOTFOUND, NULL, "key not found in TST")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5ST_find() */


/*--------------------------------------------------------------------------
 NAME
    H5ST_locate
 PURPOSE
    Find an object in a TST
 USAGE
    void *H5ST_locate(tree,s)
        H5ST_tree_t *tree;  IN: TST to locate object within
        const char *s;      IN: String of key for object to locate
 RETURNS
    Success: Non-NULL, pointer to object stored for key
    Failure: Negative
 DESCRIPTION
    Locate a node in a TST, returning the object from the node.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
void *
H5ST_locate(H5ST_tree_t *tree, const char *s)
{
    H5ST_ptr_t node;    /* Pointer to node located */
    void *ret_value;    /* Return value */

    FUNC_ENTER_NOAPI(NULL)

    /* Locate the node to remove */
    if(NULL == (node = H5ST_find_internal(tree->root, s)))
        HGOTO_ERROR(H5E_TST, H5E_NOTFOUND, NULL, "key not found in TST")

    /* Get the pointer to the object to return */
    ret_value = node->eqkid;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5ST_locate() */


/*--------------------------------------------------------------------------
 NAME
    H5ST_findfirst_internal
 PURPOSE
    Find the first node in a TST
 USAGE
    H5ST_ptr_t H5ST_findfirst_internal(p)
        H5ST_ptr_t p;      IN: TST to locate first node within
 RETURNS
    Success: Non-NULL
    Failure: NULL
 DESCRIPTION
    Get the first (lexicographically) node in a TST
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static H5ST_ptr_t
H5ST_findfirst_internal(H5ST_ptr_t p)
{
    H5ST_ptr_t ret_value = NULL;  /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    while(p) {
        /* Find least node in current tree */
        while(p->lokid)
            p = p->lokid;

        /* Is least node '\0'? */
        if(p->splitchar == '\0') {
            /* Return it */
            HGOTO_DONE(p);
        } /* end if */
        else {
            /* Go down to next level of tree */
            p = p->eqkid;
        } /* end else */
    } /* end while */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5ST_findfirst_internal() */


/*--------------------------------------------------------------------------
 NAME
    H5ST_findfirst
 PURPOSE
    Find the first node in a TST
 USAGE
    H5ST_ptr_t H5ST_findfirst(tree)
        H5ST_tree_t *tree;      IN: TST to locate first node within
 RETURNS
    Success: Non-NULL
    Failure: NULL
 DESCRIPTION
    Get the first (lexicographically) node in a TST
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
H5ST_ptr_t
H5ST_findfirst(H5ST_tree_t *tree)
{
    H5ST_ptr_t ret_value;       /* Return value */

    FUNC_ENTER_NOAPI(NULL)

    if(NULL == (ret_value = H5ST_findfirst_internal(tree->root)))
        HGOTO_ERROR(H5E_TST,H5E_NOTFOUND,NULL,"no nodes in TST");

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5ST_findfirst() */


/*--------------------------------------------------------------------------
 NAME
    H5ST_getnext
 PURPOSE
    Internal routine to find the next node in a given level of a TST
 USAGE
    H5ST_ptr_t H5ST_getnext(p)
        H5ST_ptr_t *p;       IN: Pointer to node to find next node from
 RETURNS
    Success: Non-NULL
    Failure: NULL
 DESCRIPTION
    Get the next (lexicographically) node in the current level of a TST
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static H5ST_ptr_t
H5ST_getnext(H5ST_ptr_t p)
{
    H5ST_ptr_t ret_value = NULL;  /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* If the node to continue from has higher-valued nodes attached */
    if(p->hikid) {
        /* Go to first higher-valued node */
        p = p->hikid;

        /* Find least node from here */
        while(p->lokid)
            p = p->lokid;
        HGOTO_DONE(p);
    } /* end if */
    else {
        H5ST_ptr_t q;           /* Temporary TST node pointer */

        /* Go up one level in current tree */
        q = p->parent;
        if(q == NULL)
            HGOTO_DONE(NULL);

        /* While the previous node was the higher-valued node, keep backing up the tree */
        while(q->hikid == p) {
            p = q;
            q = p->parent;
            if(NULL == q)
                HGOTO_DONE(NULL);
        } /* end while */
        HGOTO_DONE(q);
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5ST_getnext() */


/*--------------------------------------------------------------------------
 NAME
    H5ST_findnext
 PURPOSE
    Find the next node from a node in a TST
 USAGE
    H5ST_ptr_t H5ST_findnext(p)
        H5ST_ptr_t p;       IN: Current node to continue from
 RETURNS
    Success: Non-NULL
    Failure: NULL
 DESCRIPTION
    Get the next (lexicographically) node in a TST
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
H5ST_ptr_t
H5ST_findnext(H5ST_ptr_t p)
{
    H5ST_ptr_t q;               /* Temporary pointer to TST node */
    H5ST_ptr_t ret_value = NULL;  /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Find the next node at the current level, or go back up the tree */
    do {
        q = H5ST_getnext(p);
        if(q) {
            HGOTO_DONE(H5ST_findfirst_internal(q->eqkid));
        } /* end if */
        else
            p = p->up;
    } while(p);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5ST_findnext() */


/*--------------------------------------------------------------------------
 NAME
    H5ST_delete_internal
 PURPOSE
    Delete a node from a TST
 USAGE
    herr_t H5ST_delete_internal(root,p)
        H5ST_ptr_t *root;       IN/OUT: Root of TST to delete node from
        H5ST_ptr_t p;       IN: Node to delete
 RETURNS
    Success: Non-negative
    Failure: Negative
 DESCRIPTION
    Delete a node from a TST.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    This should be the final node for a string.
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5ST_delete_internal(H5ST_ptr_t *root, H5ST_ptr_t p)
{
    H5ST_ptr_t q,               /* Temporary pointer to TST node */
        newp;                   /* Pointer to node which will replace deleted node in tree */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Find node to replace one being deleted */
    if(p->lokid) {
        /* If the deleted node has lo & hi kids, attach them together */
        if(p->hikid) {
            q = p->lokid;
            while(q->hikid)
                q = q->hikid;
            q->hikid = p->hikid;
            p->hikid->parent = q;
        } /* end if */
        newp = p->lokid;
    } /* end if */
    else if(p->hikid) {
        newp = p->hikid;
    } /* end if */
    else {
        newp = NULL;
    } /* end else */

    /* Deleted node is in middle of tree */
    if(p->parent) {
        /* Attach new node to correct side of parent */
        if(p == p->parent->lokid)
            p->parent->lokid = newp;
        else
            p->parent->hikid = newp;
        if(newp)
            newp->parent = p->parent;
    } /* end if */
    else {
        if(newp)
            newp->parent = p->parent;
        if(p->up) {
            p->up->eqkid = newp;

            /* If we deleted the last node in the TST, delete the upper node also */
            if(NULL == newp)
                H5ST_delete_internal(root, p->up);
        } /* end if */
        else /* Deleted last node at top level of tree */
            *root = newp;
    } /* end else */

    p = H5FL_FREE(H5ST_node_t, p);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5ST_delete_internal() */


/*--------------------------------------------------------------------------
 NAME
    H5ST_delete
 PURPOSE
    Delete a node from a TST
 USAGE
    herr_t H5ST_delete(tree,p)
        H5ST_tree_t *tree;  IN/OUT: TST to delete node from
        H5ST_ptr_t p;       IN: Node to delete
 RETURNS
    Success: Non-negative
    Failure: Negative
 DESCRIPTION
    Delete a node from a TST.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    This should be the final node for a string.
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5ST_delete(H5ST_tree_t *tree, H5ST_ptr_t p)
{
    herr_t ret_value = SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    if(H5ST_delete_internal(&tree->root, p) < 0)
        HGOTO_ERROR(H5E_TST, H5E_CANTDELETE, FAIL, "can't delete node from TST")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5ST_delete() */


/*--------------------------------------------------------------------------
 NAME
    H5ST_remove
 PURPOSE
    Remove a node from a TST
 USAGE
    void *H5ST_remove(tree,s)
        H5ST_tree_t *tree;  IN/OUT: TST to remove node from
        const char *s;      IN: String of key for node to remove
 RETURNS
    Success: Non-NULL, pointer to object stored for key
    Failure: Negative
 DESCRIPTION
    Remove a node from a TST, returning the object from the node.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
void *
H5ST_remove(H5ST_tree_t *tree, const char *s)
{
    H5ST_ptr_t node;    /* Pointer to node to remove */
    void *ret_value;    /* Return value */

    FUNC_ENTER_NOAPI(NULL)

    /* Locate the node to remove */
    if(NULL == (node = H5ST_find_internal(tree->root, s)))
        HGOTO_ERROR(H5E_TST, H5E_NOTFOUND, NULL, "key not found in TST")

    /* Get the pointer to the object to return */
    ret_value = node->eqkid;

    /* Remove the node from the TST */
    if(H5ST_delete_internal(&tree->root, node) < 0)
        HGOTO_ERROR(H5E_TST, H5E_CANTDELETE, NULL, "can't delete node from TST")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5ST_remove() */

#ifdef H5ST_DEBUG

/*--------------------------------------------------------------------------
 NAME
    H5ST__dump_internal
 PURPOSE
    Dump all the nodes of a TST
 USAGE
    herr_t H5ST_dump(p)
        H5ST_ptr_t p;   IN: Root of TST to dump
 RETURNS
    Success: Non-negative
    Failure: Negative
 DESCRIPTION
    Dump information for a TST.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5ST__dump_internal(H5ST_ptr_t p)
{
    FUNC_ENTER_STATIC_NOERR

    if(p) {
        printf("p=%p\n", (void *)p);
        printf("\tp->up=%p\n", (void *)p->up);
        printf("\tp->parent=%p\n", (void *)p->parent);
        printf("\tp->lokid=%p\n", (void *)p->lokid);
        printf("\tp->hikid=%p\n", (void *)p->hikid);
        printf("\tp->eqkid=%p\n", (void *)p->eqkid);
        printf("\tp->splitchar=%c\n", p->splitchar);

        H5ST__dump_internal(p->lokid);
        if(p->splitchar)
            H5ST__dump_internal(p->eqkid);
        else
            printf("%s\n", (char *)p->eqkid);
        H5ST__dump_internal(p->hikid);
    } /* end if */

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5ST__dump_internal() */


/*--------------------------------------------------------------------------
 NAME
    H5ST_dump
 PURPOSE
    Dump all the nodes of a TST
 USAGE
    herr_t H5ST_dump(tree)
        H5ST_tree_t *tree;      IN: TST to dump
 RETURNS
    Success: Non-negative
    Failure: Negative
 DESCRIPTION
    Dump information for a TST.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5ST_dump(H5ST_tree_t *tree)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Dump the tree */
    H5ST__dump_internal(tree->root);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5ST_dump() */
#endif /* H5ST_DEBUG */

