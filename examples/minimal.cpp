/*
* cpp-terminal
* C++ library for writing multi-platform terminal applications.
*
* SPDX-FileCopyrightText: 2019-2023 cpp-terminal
*
* SPDX-License-Identifier: MIT
*/

#include "cpp-terminal/terminal.hpp"

#include <iostream>

#include "monolithic_examples.h"

#if defined(BUILD_MONOLITHIC)
#define main			cppterminal_minimal_example_main
#endif

int main(void)
{
  std::cout << "Just including terminal.hpp activate \033[31mcolor\033[0m !" << std::endl;
  int val{0};
  std::cin >> val;
  return 0;
}
