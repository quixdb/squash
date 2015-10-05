/* Copyright (c) 2015 The Squash Authors
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors:
 *   Evan Nemerson <evan@nemerson.com>
 */

#include "internal.h"

#include <strings.h>

/**
 * @def SQUASH_LICENSE_UNKNOWN
 * @brief Unknown license
 */

const struct { SquashLicense value; const char* name; } licenses[] = {
  { SQUASH_LICENSE_PUBLIC_DOMAIN, "Public Domain" },
  { SQUASH_LICENSE_BSD2,          "BSD2" },
  { SQUASH_LICENSE_BSD3,          "BSD3" },
  { SQUASH_LICENSE_BSD4,          "BSD4" },
  { SQUASH_LICENSE_MIT,           "MIT" },
  { SQUASH_LICENSE_ZLIB,          "zlib" },
  { SQUASH_LICENSE_WTFPL,         "WTFPL" },
  { SQUASH_LICENSE_X11,           "X11" },
  { SQUASH_LICENSE_APACHE,        "Apache 2.0" },
  { SQUASH_LICENSE_APACHE2,       "Apache" },
  { SQUASH_LICENSE_CDDL,          "CDDL" },
  { SQUASH_LICENSE_MSPL,          "MsPL" },
  { SQUASH_LICENSE_MPL,           "MPL" },
  { SQUASH_LICENSE_LGPL2P1,       "LGPLv2.1" },
  { SQUASH_LICENSE_LGPL2P1_PLUS,  "LGPLv2.1+" },
  { SQUASH_LICENSE_LGPL3,         "LGPLv3" },
  { SQUASH_LICENSE_LGPL3_PLUS,    "LGPLv3+" },
  { SQUASH_LICENSE_GPL1,          "GPLv1" },
  { SQUASH_LICENSE_GPL1_PLUS,     "GPLv1+" },
  { SQUASH_LICENSE_GPL2,          "GPLv2" },
  { SQUASH_LICENSE_GPL2_PLUS,     "GPLv2+" },
  { SQUASH_LICENSE_GPL3,          "GPLv3" },
  { SQUASH_LICENSE_GPL3_PLUS,     "GPLv3+" },
  { SQUASH_LICENSE_PROPRIETARY,   "Proprietary" },

  { SQUASH_LICENSE_UNKNOWN, NULL }
};

const char*
squash_license_to_string (SquashLicense license) {
  int i;
  for (i = 0 ; licenses[i].value != SQUASH_LICENSE_UNKNOWN ; i++)
    if (license == licenses[i].value)
      return licenses[i].name;
  return NULL;
}

SquashLicense
squash_license_from_string (const char* license) {
  if (license == NULL)
    return SQUASH_LICENSE_UNKNOWN;

  int i;
  for (i = 0 ; licenses[i].value != SQUASH_LICENSE_UNKNOWN ; i++)
    if (strcasecmp (license, licenses[i].name) == 0)
      return licenses[i].value;

  return SQUASH_LICENSE_UNKNOWN;
}
