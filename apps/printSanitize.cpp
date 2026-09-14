// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#include <iostream>
#include <cstring>
#include "vulkanite/vkn-common.h"

#ifdef WIN32
# define OWL_TERMINAL_RED ""
# define OWL_TERMINAL_GREEN ""
# define OWL_TERMINAL_LIGHT_GREEN ""
# define OWL_TERMINAL_YELLOW ""
# define OWL_TERMINAL_BLUE ""
# define OWL_TERMINAL_LIGHT_BLUE ""
# define OWL_TERMINAL_RESET ""
# define OWL_TERMINAL_DEFAULT OWL_TERMINAL_RESET
# define OWL_TERMINAL_BOLD ""

# define OWL_TERMINAL_MAGENTA ""
# define OWL_TERMINAL_LIGHT_MAGENTA ""
# define OWL_TERMINAL_CYAN ""
# define OWL_TERMINAL_LIGHT_RED ""
#else
# define OWL_TERMINAL_RED "\033[0;31m"
# define OWL_TERMINAL_GREEN "\033[0;32m"
# define OWL_TERMINAL_LIGHT_GREEN "\033[1;32m"
# define OWL_TERMINAL_YELLOW "\033[1;33m"
# define OWL_TERMINAL_BLUE "\033[0;34m"
# define OWL_TERMINAL_LIGHT_BLUE "\033[1;34m"
# define OWL_TERMINAL_RESET "\033[0m"
# define OWL_TERMINAL_DEFAULT OWL_TERMINAL_RESET
# define OWL_TERMINAL_BOLD "\033[1;1m"

# define OWL_TERMINAL_MAGENTA "\e[35m"
# define OWL_TERMINAL_LIGHT_MAGENTA "\e[95m"
# define OWL_TERMINAL_CYAN "\e[36m"
# define OWL_TERMINAL_LIGHT_RED "\033[1;31m"
#endif


int main(int, char **)
{
  enum { LINE_SZ = 10000 };
  char line[LINE_SZ];
  while (fgets(line,LINE_SZ,stdin)) {
    // std::cout << "GOT LINE " << line << std::endl;
    if (strstr(line,"VVL-DEBUG-PRINTF(INFO / SPEC)") == line) {
      // nvidia and intel: VVL-DEBUG... at beginning, message after second pipe symbol
      // fgets(line,1000,stdin);
      char *where = strstr(line," | ");
      if (where) where = strstr(where+3," | ");
      char *eol = strrchr(where,'\n');
      if (eol) *eol = 0;
      std::cout << OWL_TERMINAL_LIGHT_BLUE << (where?where+3:line) << OWL_TERMINAL_RESET;
    } else if (strstr(line,"VVL-DEBUG-PRINTF")) {
      // amd: more verbose stuff, and extra outputs

      // std::cout << "SKIPPING1 " << line << std::endl;
      // vkQueueSubmit(): ...
      fgets(line,LINE_SZ,stdin);
      // std::cout << "SKIPPING2 " << line << std::endl;

      while (true) {
        fgets(line,LINE_SZ,stdin);
        if (strstr(line,"Objects: 1")) {
          //   [0] VkDevice ...
          fgets(line,LINE_SZ,stdin);
          break;
        }
        if (strstr(line,"Objects: 2")) {
          //   [0] VkDevice ...
          fgets(line,LINE_SZ,stdin);
          //   [1] VkPipeline ...
          fgets(line,LINE_SZ,stdin);
          break;
        }

        if (strchr(line,10)) *strchr(line,10) = 0;
        // std::cout << "FOUND: " << line  << std::endl;
        std::cout << OWL_TERMINAL_LIGHT_BLUE << line  << OWL_TERMINAL_RESET;
        
      }
    } else
      std::cout << line;
  }
  return 0;

}
