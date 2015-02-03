/*************************************************************************
 *
 * Copyright (C) 1997-2014 by Dimitri van Heesch. 
 * Copyright (C) 2014-2015 Barbara Geller & Ansel Sermersheim 
 * All rights reserved.    
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation under the terms of the GNU General Public License version 2
 * is hereby granted. No representations are made about the suitability of
 * this software for any purpose. It is provided "as is" without express or
 * implied warranty. See the GNU General Public License for more details.
 *
 * Documents produced by Doxygen are derivative works derived from the
 * input used in their production; they are not affected by this license.
 *
*************************************************************************/

#include <parser_file.h>
#include <outputgen.h>

void FileParser::parseCode(CodeOutputInterface &codeOutIntf, const char *, const QByteArray &input,
                  SrcLangExt, bool, const char *,  QSharedPointer<FileDef> fd, int startLine, int endLine,
                  bool, QSharedPointer<MemberDef> md, bool showLineNumbers, QSharedPointer<Definition> context, bool)
{
   int lineNr = startLine != -1 ? startLine : 1;
   int length = input.length();
   int i = 0;

   while (i < length && (endLine == -1 || lineNr <= endLine)) {
      int j = i;

      while (j < length && input[j] != '\n') {
         j++;
      }

      QByteArray lineStr = input.mid(i, j - i);
      codeOutIntf.startCodeLine(showLineNumbers);

      if (showLineNumbers) {
         codeOutIntf.writeLineNumber(0, 0, 0, lineNr);
      }

      if (!lineStr.isEmpty()) {
         codeOutIntf.codify(lineStr);
      }

      codeOutIntf.endCodeLine();
      lineNr++;
      i = j + 1;
   }
}
