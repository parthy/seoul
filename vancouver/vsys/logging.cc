/**
 * Logging implementation.
 *
 * Copyright (C) 2007-2008, Bernhard Kauer <bk@vmmon.org>
 *
 * This file is part of Vancouver.
 *
 * Vancouver is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Vancouver is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */

#
#include <cstdlib>
#include "driver/logging.h"

void (*Logging::_putcf)(void *, long int value);
void *Logging::_data;

void
Logging::panic(const char *format, ...)
{
  va_list ap;
  Vprintf::printf(_putcf, _data, "\nPANIC: ");
  va_start(ap, format);
  Vprintf::vprintf(_putcf, _data, format, ap);
  va_end(ap);
  Vprintf::printf(_putcf, _data, "\n");
  __exit(0xdeadbeef);
}


void
Logging::printf(const char *format, ...)
{
  va_list ap;
  va_start(ap, format);
  Vprintf::vprintf(_putcf, _data, format, ap);
  va_end(ap);
}