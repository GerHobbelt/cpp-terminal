/*
* cpp-terminal
* C++ library for writing multiplatform terminal applications.
*
* SPDX-FileCopyrightText: 2019-2023 cpp-terminal
*
* SPDX-License-Identifier: MIT
*/

#include "cpp-terminal/exception.hpp"
#include "cpp-terminal/iostream.hpp"
#include "cpp-terminal/prompt.hpp"
#include "cpp-terminal/version.hpp"

#include "monolithic_examples.h"

#if defined(BUILD_MONOLITHIC)
#define main			cppterminal_prompt_simple_example_main
#endif

int main(void)
{
  Term::cout << "Running cpp-terminal version: " << Term::Version << std::endl;
  try
  {
    Term::cout << "CPP-Terminal basic prompt example: \n\n";
    switch(Term::prompt_simple("Proceed?"))
    {
      case Term::Result_simple::YES: Term::cout << "Proceeding...\n"; break;
      case Term::Result_simple::NO: Term::cout << "Stopping...\n"; break;
      case Term::Result_simple::ABORT: Term::cout << "Exit signal received, exiting now...\n"; break;
    }
  }
  catch(const Term::Exception& re)
  {
    Term::cerr << "cpp-terminal error: " << re.what() << std::endl;
    return 2;
  }
  catch(...)
  {
    Term::cerr << "Unknown error." << std::endl;
    return 1;
  }
  return 0;
}
