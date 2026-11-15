#ifndef SS_NAMES_H
#define SS_NAMES_H
/*
 * ShakilScript word table.
 * Programs are written with the ShakilScript names (show, ask, when, ...).
 * ss_translate_source() converts them to the internal keyword names that the
 * Flex/Bison front end understands, and rejects the old Python names.
 * ss_localize_message() turns internal names in error messages back into
 * ShakilScript names.
 */
#include <stddef.h>

/* Returns a malloc'd translated copy of src, or NULL after printing an error
   (error text goes to stderr; line numbers are never changed). */
char *ss_translate_source(const char *src);

/* Returns a malloc'd copy of msg with builtin names shown the ShakilScript way. */
char *ss_localize_message(const char *msg);
#endif
