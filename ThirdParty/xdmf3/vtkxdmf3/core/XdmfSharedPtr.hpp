/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfSharedPtr.hpp                                                   */
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

#ifndef XDMFSHAREDPTR_HPP_
#define XDMFSHAREDPTR_HPP_

#ifdef __cplusplus

#include "XdmfCoreConfig.hpp"
#include <boost/shared_ptr.hpp>

using boost::shared_ptr;

#ifdef HAVE_BOOST_SHARED_DYNAMIC_CAST

using boost::shared_dynamic_cast;

#else

template <typename T, typename U>
shared_ptr<T> shared_dynamic_cast(shared_ptr<U> const & r) 
{
  typedef typename shared_ptr<T>::element_type E;
  E * p = dynamic_cast< E* >( r.get() );
  return p? shared_ptr<T>( r, p ): shared_ptr<T>();
}

#endif /* HAVE_BOOST_SHARED_DYNAMIC_CAST */

// Used by C wrappers to prevent shared pointers from prematurely deleting objects
// Normally this would be completely against the point of shared pointers,
// but the  C wrapping requires that objects be seperated from the shared pointers.
struct XdmfNullDeleter
{
template<typename T>
void operator()(T*) {}
};

#endif

#endif /* XDMFSHAREDPTR_HPP_ */
