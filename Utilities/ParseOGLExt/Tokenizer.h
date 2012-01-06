/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Tokenizer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * Copyright 2003 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include <string>

class Tokenizer
{
public:
  Tokenizer(const char *s, const char *delim = " \t\n\r");
  Tokenizer(const std::string &s, const char *delim = " \t\n\r");

  std::string GetNextToken();
  std::string GetRemainingString() const;
  bool HasMoreTokens() const;

  void Reset();

private:
  std::string FullString;
  std::string Delim;
  std::string::size_type Position;
};
