/*
 * Copyright 1998-2018 University Corporation for Atmospheric Research/Unidata
 *  See the LICENSE file for more information.
 */

/**
Extra ezxml functionality
*/

#include "ezxml.h"

/**
Get list of all the xml attributes.
Returns NULL, if none
WARNING: returns actual list, so do not free
*/
const char**
ezxml_all_attr(ezxml_t xml, int* countp)
{
    if(xml && xml->attr) {
        char** p;
        int count = 0;
        for(p=xml->attr;*p;p+=2) count += 2; /* get number of attributes */
	return (const char**)xml->attr;
    }
    return NULL;
}
