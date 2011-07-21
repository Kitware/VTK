/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAMRBox.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef VTKASSERTUTILS_HPP_
#define VTKASSERTUTILS_HPP_

#include <cassert>
#include "vtkSetGet.h"

class vtkAssertUtils
{

  public:

    /**
     * @brief Checks if the predicate is false.
     * @param predicate the predicate to check.
     * @param file the file name from where this function is called.
     * @param line the line number where this function is called.
     */
    inline static void assertFalse(
        const bool predicate,
        const char* file,
        int line )
    {
      #ifdef ASSERT_ON
        if( predicate != false )
        {
          std::cerr << "===========================================\n";
          std::cerr <<  __DATE__ << " " << __TIME__ << std::endl;
          std::cerr << "ERROR: AssertUtils::assertFalse() failed from:\n";
          std::cerr << "FILE: " << file << std::endl;
          std::cerr << "LINE: " << line << std::endl;
          assert( predicate == false );
        }
      #endif
      return;

    }


    /**
     * @brief Checks if the predicate is true.
     * @param predicate the predicate to check.
     * @param file the file name from where this function is called.
     * @param line the line number where this function is called.
     */
    inline static void assertTrue(
        const bool predicate,
        const char* file,
        int line )
    {
      #ifdef ASSERT_ON
        if( predicate != true )
        {
          std::cerr << "===========================================\n";
          std::cerr <<  __DATE__ << " " << __TIME__ << std::endl;
          std::cerr << "ERROR: AssertUtils::assertTrue() failed from:\n";
          std::cerr << "FILE: " << file << std::endl;
          std::cerr << "LINE: " << line << std::endl;
          assert( predicate == true );
        }
      #endif
      return;

    }


    /**
     * @brief Checks if the supplied pointer is null.
     * @param ptr the pointer to check.
     * @param file the file name from where this function is called.
     * @param line the line number where this function is called.
     */
    inline static void assertNull(
        const void* ptr,
        const char* file,
        int line )
    {

      #ifdef ASSERT_ON
        if( ptr != NULL )
        {
          std::cerr << "===========================================\n";
          std::cerr <<  __DATE__ << " " << __TIME__ << std::endl;
          std::cerr << "ERROR: AssertUtils::assertNull() failed from:\n";
          std::cerr << "FILE: " << file << std::endl;
          std::cerr << "LINE: " << line << std::endl;
          assert( ptr == NULL );
        }
      #endif
      return;

    }

    /**
     * @brief Checks if the supplied pointer is NOT null.
     * @param ptr the pointer to check.
     * @param file the file name from where this function is called.
     * @param line the line number where this function is called.
     */
    inline static void assertNotNull(
        const void* ptr,
        const char* file,
        int line )
    {

      #ifdef ASSERT_ON
        if( ptr == NULL )
        {
          std::cerr << "===========================================\n";
          std::cerr <<  __DATE__ << " " << __TIME__ << std::endl;
          std::cerr << "ERROR: AssertUtils::assertNull() failed from:\n";
          std::cerr << "FILE: " << file << std::endl;
          std::cerr << "LINE: " << line << std::endl;
          assert( ptr != NULL );
        }
      #endif
      return;

    }


    inline static void assertNotEquals(
        const int rhs,
        const int lhs,
        const char* file,
        int line )
    {
      #ifdef ASSERT_ON
       if( rhs == lhs )
         {
         std::cerr << "===========================================\n";
         std::cerr <<  __DATE__ << " " << __TIME__ << std::endl;
         std::cerr << "ERROR: AssertUtils::assertNotEquals() failed from:\n";
         std::cerr << "FILE: " << file << std::endl;
         std::cerr << "LINE: " << line << std::endl;
         std::cerr << "num1: " << rhs  << std::endl;
         std::cerr << "num2: " << lhs  << std::endl;
         assert( rhs == lhs );
         }
      #endif
      return;
    }

    /**
     * @brief Checks if the two numbers are equal.
     * @param rhs the number on the right-hand-side.
     * @param lhs the number on the left-hand-side.
     */
    inline static void assertEquals(
        const int rhs,
        const int lhs,
        const char* file,
        int line )
    {
      #ifdef ASSERT_ON
        if( rhs != lhs )
        {
          std::cerr << "===========================================\n";
          std::cerr <<  __DATE__ << " " << __TIME__ << std::endl;
          std::cerr << "ERROR: AssertUtils::assertEquals() failed from:\n";
          std::cerr << "FILE: " << file << std::endl;
          std::cerr << "LINE: " << line << std::endl;
          std::cerr << "num1: " << rhs  << std::endl;
          std::cerr << "num2: " << lhs  << std::endl;
          assert( rhs == lhs );
        }
      #endif
      return;
    }

    /**
     * @brief Checks if a number is within the given range.
     * @param num the number to check.
     * @param lb lower-bound.
     * @param ub upper-bound.
     * @note Checks if  lb <= num <= up, i.e. the check is inclusive
     * of the lower and upper bounds.
     */
    inline static void assertInRange(
        const int num,
        const int lb,
        const int ub,
        const char* file,
        int line )
    {

      #ifdef ASSERT_ON
        if( !( num >= lb ) || !( num <= ub ) )
        {
          std::cerr << "===========================================\n";
          std::cerr <<  __DATE__ << " " << __TIME__ << std::endl;
          std::cerr << "ERROR: AssertUtils::assertInRange() failed from:\n";
          std::cerr << "FILE: "        << file << std::endl;
          std::cerr << "LINE: "        << line << std::endl;
          std::cerr << "NUMBER: "      << num  << std::endl;
          std::cerr << "LOWER BOUND: " << lb   << std::endl;
          std::cerr << "UPPER BOUND: " << ub   << std::endl;
          assert( num >= lb && num <= ub );
        }
      #endif
      return;

    }

};

#endif /* VTKASSERTUTILS_HPP_ */
