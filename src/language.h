/*************************************************************************
 *
 * Copyright (C) 2014-2017 Barbara Geller & Ansel Sermersheim 
 * Copyright (C) 1997-2014 by Dimitri van Heesch.
 * All rights reserved.    
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation under the terms of the GNU General Public License version 2
 * is hereby granted. No representations are made about the suitability of
 * this software for any purpose. It is provided "as is" without express or
 * implied warranty. See the GNU General Public License for more details.
 *
 * Documents produced by DoxyPress are derivative works derived from the
 * input used in their production; they are not affected by this license.
 *
*************************************************************************/

#ifndef LANGUAGE_H
#define LANGUAGE_H

#include <translator.h>
#include <translator_cs.h>

// using the CS i18n system
extern Translator_Cs translator_Cs;
extern Translator *theTranslator;      // can be removed when theTranslator is changed to CsTrans

void loadTranslationFile();
QString getLanguage();

/*! Generate a place holder for a position in a list. Used for
 *  translators to be able to specify different elements orders
 *  depending on whether text flows from left to right or visa versa.
 */
inline QString generateMarker(int id)
{
   QString retval;

   retval.setNum(id);
   retval.prepend('@');

   return retval;
}

#endif
