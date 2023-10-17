/*
* cpp-terminal
* C++ library for writing multiplatform terminal applications.
*
* SPDX-FileCopyrightText: 2019-2023 cpp-terminal
*
* SPDX-License-Identifier: MIT
*/

#include "cpp-terminal/color.hpp"
#include "cpp-terminal/terminal.hpp"

#include <iostream>

#ifdef _WIN32
  #include <windows.h>
#endif

#include "monolithic_examples.h"

#if defined(BUILD_MONOLITHIC)
#define main			cppterminal_attach_console_minimal_example_main
#endif

#if defined(_WIN32) && !defined(BUILD_MONOLITHIC)
int __stdcall WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
#else
int main()
#endif
{
  std::cout << Term::color_fg(Term::Color::Name::Red) << "Hello world !" << color_fg(Term::Color::Name::Default) << std::endl;
#if defined(_WIN32)
  MessageBoxA(NULL, "Hello, world", "caption", 0);
#endif
  return 0;
}
