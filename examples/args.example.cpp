/*
* cpp-terminal
* C++ library for writing multiplatform terminal applications.
*
* SPDX-FileCopyrightText: 2019-2023 cpp-terminal
*
* SPDX-License-Identifier: MIT
*/

#include "cpp-terminal/args.hpp"

#include <iostream>

#include "monolithic_examples.h"

#if defined(BUILD_MONOLITHIC)
#define main			cppterminal_args_example_main
#endif

int main(void)
{
  std::cout << "argc : " << Term::argc << std::endl;
  for(std::size_t i = 0; i != Term::argc; ++i) { std::cout << "argv[" << i << "] : *" << Term::argv[i] << "*" << std::endl; }
  return 0;
}
