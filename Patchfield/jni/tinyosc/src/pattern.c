/*
Copyright © 1998. The Regents of the University of California(Regents). 
All Rights Reserved.

Written by Matt Wright, The Center for New Music and Audio Technologies,
University of California, Berkeley.

Revisions by Peter Brinkmann <peter.brinkmann@gmail.com>.

Permission to use, copy, modify, distribute, and distribute modified versions
of this software and its documentation without fee and without a signed
licensing agreement, is hereby granted, provided that the above copyright
notice, this paragraph and the following two paragraphs appear in all copies,
modifications, and distributions.

IN NO EVENT SHALL REGENTS BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT,
SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS, ARISING
OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF REGENTS HAS
BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

REGENTS SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE. THE SOFTWARE AND ACCOMPANYING DOCUMENTATION, IF ANY, PROVIDED
HEREUNDER IS PROVIDED "AS IS". REGENTS HAS NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

The OpenSound Control WWW page is 
    http://www.cnmat.berkeley.edu/OpenSoundControl
*/

#include "pattern.h"

static const char *theWholePattern;	/* Just for warning messages */

static int MatchBrackets(const char *pattern, const char *test);
static int MatchList(const char *pattern, const char *test);

int pattern_matches(const char *  pattern, const char * test) {
  theWholePattern = pattern;
  
  if (pattern == 0 || pattern[0] == 0) {
    return test[0] == 0;
  } 
  
  if (test[0] == 0) {
    if (pattern[0] == '*')
      return pattern_matches(pattern+1,test);
    else
      return 0;
  }

  switch (pattern[0]) {
    case 0      : return test[0] == 0;
    case '?'    : return pattern_matches(pattern + 1, test + 1);
    case '*'    : 
      if (pattern_matches(pattern+1, test)) {
        return 1;
      } else {
	return pattern_matches(pattern, test+1);
      }
    case ']'    :
    case '}'    :
      // OSCWarning("Spurious %c in pattern \".../%s/...\"",pattern[0], theWholePattern);
      return 0;
    case '['    :
      return MatchBrackets(pattern,test);
    case '{'    :
      return MatchList(pattern,test);
    case '\\'   :  
      if (pattern[1] == 0) {
	return test[0] == 0;
      } else if (pattern[1] == test[0]) {
	return pattern_matches(pattern+2,test+1);
      } else {
	return 0;
      }
    default     :
      if (pattern[0] == test[0]) {
	return pattern_matches(pattern+1,test+1);
      } else {
	return 0;
      }
  }
}


/* we know that pattern[0] == '[' and test[0] != 0 */

static int MatchBrackets(const char *pattern, const char *test) {
  int result;
  int negated = 0;
  const char *p = pattern;

  if (pattern[1] == 0) {
    // OSCWarning("Unterminated [ in pattern \".../%s/...\"", theWholePattern);
    return 0;
  }

  if (pattern[1] == '!') {
    negated = 1;
    p++;
  }

  while (*p != ']') {
    if (*p == 0) {
      // OSCWarning("Unterminated [ in pattern \".../%s/...\"", theWholePattern);
      return 0;
    }
    if (p[1] == '-' && p[2] != 0) {
      if (test[0] >= p[0] && test[0] <= p[2]) {
	result = !negated;
	goto advance;
      }
    }
    if (p[0] == test[0]) {
      result = !negated;
      goto advance;
    }
    p++;
  }

  result = negated;

advance:

  if (!result)
    return 0;

  while (*p != ']') {
    if (*p == 0) {
      // OSCWarning("Unterminated [ in pattern \".../%s/...\"", theWholePattern);
      return 0;
    }
    p++;
  }

  return pattern_matches(p+1,test+1);
}

static int MatchList(const char *pattern, const char *test) {

 const char *restOfPattern, *tp = test;


 for (restOfPattern = pattern; *restOfPattern != '}'; restOfPattern++) {
   if (*restOfPattern == 0) {
     // OSCWarning("Unterminated { in pattern \".../%s/...\"", theWholePattern);
     return 0;
   }
 }

 restOfPattern++; /* skip close curly brace */


 pattern++; /* skip open curly brace */

 while (1) {
   
   if (*pattern == ',') {
     if (pattern_matches(restOfPattern, tp)) {
       return 1;
     } else {
       tp = test;
       ++pattern;
     }
   } else if (*pattern == '}') {
     return pattern_matches(restOfPattern, tp);
   } else if (*pattern == *tp) {
     ++pattern;
     ++tp;
   } else {
     tp = test;
     while (*pattern != ',' && *pattern != '}') {
       pattern++;
     }
     if (*pattern == ',') {
       pattern++;
     }
   }
 }
}
