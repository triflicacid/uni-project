#pragma once

/** @file
 *  @brief ANSI/VT100 escape codes for terminal text color, used to format CLI output. */

#define ANSI_YELLOW  "\x1b[33m" ///< Yellow foreground.
#define ANSI_BRIGHT_YELLOW  "\x1b[93m" ///< Bright yellow foreground.
#define ANSI_RED     "\x1b[31m" ///< Red foreground.
#define ANSI_RED_BG  "\x1b[101m" ///< Bright red background.
#define ANSI_GREEN   "\x1b[32m" ///< Green foreground.
#define ANSI_BLUE    "\x1b[34m" ///< Blue foreground.
#define ANSI_BLACK    "\x1b[30m" ///< Black foreground.
#define ANSI_VIOLET  "\x1b[1;35m" ///< Bold violet foreground.
#define ANSI_CYAN    "\x1b[1;36m" ///< Bold cyan foreground.
#define ANSI_RESET   "\x1b[0m" ///< Reset all formatting.
#define ERROR_STR    ANSI_RESET "[" ANSI_RED "ERROR" ANSI_RESET "]" ///< Pre-formatted "[ERROR]" label for CLI output.

#define SHELL_RED "\e[0;31m" ///< Red foreground.
#define SHELL_LIGHT_RED "\e[1;31m" ///< Bold light red foreground.
#define SHELL_GREEN "\e[0;32m" ///< Green foreground.
#define SHELL_BLUE "\e[0;34m" ///< Blue foreground.
#define SHELL_LIGHT_BLUE "\e[1;34m" ///< Bold light blue foreground.
#define SHELL_YELLOW "\e[1;33m" ///< Bold yellow foreground.
#define SHELL_LIGHT_PURPLE "\e[1;35m" ///< Bold light purple foreground.
#define SHELL_RESET "\e[0m" ///< Reset all formatting.
#define SHELL_LIGHT_GREY "\e[0;37m" ///< Light grey foreground.
#define SHELL_CYAN "\e[0;36m" ///< Cyan foreground.
#define SHELL_LIGHT_CYAN "\e[1;36m" ///< Bold light cyan foreground.
#define SHELL_BROWN "\e[0;33m" ///< Brown foreground.
