// include/Parser.h
/**
 * Parser.h - Command parsing for DNS C2 Agent
 */

#ifndef PARSER_H
#define PARSER_H

#include <windows.h>
#include "mythic.h"  // para la definición de MythicTask

// Parse a command+args from a space-separated string
BOOL parse_command(const char* command_str, char** command, char** args);

// Parse an args string en argc/argv
BOOL parse_args(const char* args, int* argc, char*** argv);

// Free the argv array creado por parse_args()
void free_args(int argc, char** argv);

// Parsea un JSON de tasking de Mythic a MythicTask
BOOL parse_tasking_json(const char* buf, size_t len, MythicTask* out);

#endif // PARSER_H
