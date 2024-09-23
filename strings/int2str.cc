/* Copyright (c) 2000, 2023, Oracle and/or its affiliates.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License, version 2.0,
   as published by the Free Software Foundation.

   This program is also distributed with certain software (including
   but not limited to OpenSSL) that is licensed under separate terms,
   as designated in a particular file or component or in included license
   documentation.  The authors of MySQL hereby grant you an additional
   permission to link the program and your derivative works with the
   separately licensed software that they have included with MySQL.

   Without limiting anything contained in the foregoing, this file,
   which is part of C Driver for MySQL (Connector/C), is also subject to the
   Universal FOSS Exception, version 1.0, a copy of which can be found at
   http://oss.oracle.com/licenses/universal-foss-exception.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License, version 2.0, for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA */

#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <iterator>

#include "integer_digits.h"
#include "m_string.h"  // IWYU pragma: keep

/*
  _dig_vec arrays are public because they are used in several outer places.
*/
const char _dig_vec_upper[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
const char _dig_vec_lower[] = "0123456789abcdefghijklmnopqrstuvwxyz";

#ifdef WESQL
/*
  Convert integer to its string representation in given scale of notation.

  SYNOPSIS
    int2str()
      val     - value to convert
      dst     - points to buffer where string representation should be stored
      radix   - radix of scale of notation
      upcase  - set to 1 if we should use upper-case digits

  DESCRIPTION
    Converts the (long) integer value to its character form and moves it to
    the destination buffer followed by a terminating NUL.
    If radix is -2..-36, val is taken to be SIGNED, if radix is  2..36, val is
    taken to be UNSIGNED. That is, val is signed if and only if radix is.
    All other radixes treated as bad and nothing will be changed in this case.

    For conversion to decimal representation (radix is -10 or 10) one can use
    optimized int10_to_str() function.

  RETURN VALUE
    Pointer to ending NUL character or NullS if radix is bad.
*/

char *int2str(long int val, char *dst, int radix, int upcase) {
  char buffer[65];
  char *p;
  long int new_val;
  const char *dig_vec = upcase ? _dig_vec_upper : _dig_vec_lower;
  ulong uval = (ulong)val;

  if (radix < 0) {
    if (radix < -36 || radix > -2) return NullS;
    if (val < 0) {
      *dst++ = '-';
      /* Avoid integer overflow in (-val) for LLONG_MIN (BUG#31799). */
      uval = (ulong)0 - uval;
    }
    radix = -radix;
  } else if (radix > 36 || radix < 2)
    return NullS;

  /*
    The slightly contorted code which follows is due to the fact that
    few machines directly support unsigned long / and %.  Certainly
    the VAX C compiler generates a subroutine call.  In the interests
    of efficiency (hollow laugh) I let this happen for the first digit
    only; after that "val" will be in range so that signed integer
    division will do.  Sorry 'bout that.  CHECK THE CODE PRODUCED BY
    YOUR C COMPILER.  The first % and / should be unsigned, the second
    % and / signed, but C compilers tend to be extraordinarily
    sensitive to minor details of style.  This works on a VAX, that's
    all I claim for it.
  */
  p = &buffer[sizeof(buffer) - 1];
  *p = '\0';
  new_val = uval / (ulong)radix;
  *--p = dig_vec[(uchar)(uval - (ulong)new_val * (ulong)radix)];
  val = new_val;
  while (val != 0) {
    ldiv_t res;
    res = ldiv(val, radix);
    *--p = dig_vec[res.rem];
    val = res.quot;
  }
  while ((*dst++ = *p++) != 0)
    ;
  return dst - 1;
}
#endif

/**
  Converts a 64-bit integer value to its character form and moves it to the
  destination buffer followed by a terminating NUL. If radix is -2..-36, val is
  taken to be SIGNED, if radix is 2..36, val is taken to be UNSIGNED. That is,
  val is signed if and only if radix is. All other radixes are treated as bad
  and nothing will be changed in this case.

  For conversion to decimal representation (radix is -10 or 10) one should use
  the optimized #longlong10_to_str() function instead.

  @param val the value to convert
  @param dst the buffer where the string representation should be stored
  @param radix radix of scale of notation
  @param upcase true if we should use upper-case digits

  @return pointer to the ending NUL character, or nullptr if radix is bad
*/
char *ll2str(int64_t val, char *dst, int radix, bool upcase) {
  char buffer[65];
  const char *const dig_vec = upcase ? _dig_vec_upper : _dig_vec_lower;
  auto uval = static_cast<uint64_t>(val);

  if (radix < 0) {
    if (radix < -36 || radix > -2) return nullptr;
    if (val < 0) {
      *dst++ = '-';
      /* Avoid integer overflow in (-val) for LLONG_MIN (BUG#31799). */
      uval = 0ULL - uval;
    }
    radix = -radix;
  } else if (radix > 36 || radix < 2) {
    return nullptr;
  }

  char *p = std::end(buffer);
  do {
    *--p = dig_vec[uval % radix];
    uval /= radix;
  } while (uval != 0);

  const size_t length = std::end(buffer) - p;
  memcpy(dst, p, length);
  dst[length] = '\0';
  return dst + length;
}

/**
  Converts a 64-bit integer to its string representation in decimal notation.

  It is optimized for the normal case of radix 10/-10. It takes only the sign of
  radix parameter into account and not its absolute value.

  @param val the value to convert
  @param dst the buffer where the string representation should be stored
  @param radix 10 if val is unsigned, -10 if val is signed

  @return pointer to the ending NUL character
*/
char *longlong10_to_str(int64_t val, char *dst, int radix) {
  assert(radix == 10 || radix == -10);

  uint64_t uval = static_cast<uint64_t>(val);

  if (radix < 0) /* -10 */
  {
    if (val < 0) {
      *dst++ = '-';
      /* Avoid integer overflow in (-val) for LLONG_MIN (BUG#31799). */
      uval = uint64_t{0} - uval;
    }
  }

  char *end = write_digits(uval, count_digits(uval), dst);
  *end = '\0';
  return end;
}
