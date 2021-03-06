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

#include <QFile>
#include <QTextStream>

#include <cassert>
#include <stdlib.h>

#include <rtfstyle.h>

#include <doxy_globals.h>
#include <message.h>

RTFListItemInfo rtf_listItemInfo[rtf_maxIndentLevels];

QString rtf_title;
QString rtf_subject;
QString rtf_comments;
QString rtf_company;
QString rtf_logoFilename;
QString rtf_author;
QString rtf_manager;
QString rtf_documentType;
QString rtf_documentId;
QString rtf_keywords;

char rtf_Style_Reset[] = "\\pard\\plain ";

Rtf_Style_Default rtf_Style_Default[] = {
   {
      "Heading1",
      "\\s1\\sb240\\sa60\\keepn\\widctlpar\\adjustright \\b\\f1\\fs36\\kerning36\\cgrid ",
      "\\sbasedon0 \\snext0 heading 1"
   },
   {
      "Heading2",
      "\\s2\\sb240\\sa60\\keepn\\widctlpar\\adjustright \\b\\f1\\fs28\\kerning28\\cgrid ",
      "\\sbasedon0 \\snext0 heading 2"
   },
   {
      "Heading3",
      "\\s3\\sb240\\sa60\\keepn\\widctlpar\\adjustright \\b\\f1\\cgrid ",
      "\\sbasedon0 \\snext0 heading 3"
   },
   {
      "Heading4",
      "\\s4\\sb240\\sa60\\keepn\\widctlpar\\adjustright \\b\\f1\\fs20\\cgrid ",
      "\\sbasedon0 \\snext0 heading 4;}{\\*\\cs10 \\additive Default Paragraph Font"
   },
   {
      "Heading5",
      "\\s5\\sb90\\sa30\\keepn\\widctlpar\\adjustright \\b\\f1\\fs20\\cgrid ",
      "\\sbasedon0 \\snext0 heading 5;}{\\*\\cs10 \\additive Default Paragraph Font"
   },
   {
      "Title",
      "\\s15\\qc\\sb240\\sa60\\widctlpar\\outlinelevel0\\adjustright \\b\\f1\\fs32\\kerning28\\cgrid ",
      "\\sbasedon0 \\snext15 Title"
   },
   {
      "SubTitle",
      "\\s16\\qc\\sa60\\widctlpar\\outlinelevel1\\adjustright \\f1\\cgrid ",
      "\\sbasedon0 \\snext16 Subtitle"
   },
   {
      "BodyText",
      "\\s17\\sa60\\sb30\\widctlpar\\qj \\fs22\\cgrid ",
      "\\sbasedon0 \\snext17 BodyText"
   },
   {
      "DenseText",
      "\\s18\\widctlpar\\fs22\\cgrid ",
      "\\sbasedon0 \\snext18 DenseText"
   },
   {
      "Header",
      "\\s28\\widctlpar\\tqc\\tx4320\\tqr\\tx8640\\adjustright \\fs20\\cgrid ",
      "\\sbasedon0 \\snext28 header"
   },
   {
      "Footer",
      "\\s29\\widctlpar\\tqc\\tx4320\\tqr\\tx8640\\qr\\adjustright \\fs20\\cgrid ",
      "\\sbasedon0 \\snext29 footer"
   },
   {
      "GroupHeader",
      "\\s30\\li360\\sa60\\sb120\\keepn\\widctlpar\\adjustright \\b\\f1\\fs20\\cgrid ",
      "\\sbasedon0 \\snext30 GroupHeader"
   },
   {
      "CodeExample0",
      "\\s40\\li0\\widctlpar\\adjustright \\shading1000\\cbpat8 \\f2\\fs16\\cgrid ",
      "\\sbasedon0 \\snext41 Code Example 0"
   },
   {
      "CodeExample1",
      "\\s41\\li360\\widctlpar\\adjustright \\shading1000\\cbpat8 \\f2\\fs16\\cgrid ",
      "\\sbasedon0 \\snext42 Code Example 1"
   },
   {
      "CodeExample2",
      "\\s42\\li720\\widctlpar\\adjustright \\shading1000\\cbpat8 \\f2\\fs16\\cgrid ",
      "\\sbasedon0 \\snext43 Code Example 2"
   },
   {
      "CodeExample3",
      "\\s43\\li1080\\widctlpar\\adjustright \\shading1000\\cbpat8 \\f2\\fs16\\cgrid ",
      "\\sbasedon0 \\snext44 Code Example 3"
   },
   {
      "CodeExample4",
      "\\s44\\li1440\\widctlpar\\adjustright \\shading1000\\cbpat8 \\f2\\fs16\\cgrid ",
      "\\sbasedon0 \\snext45 Code Example 4"
   },
   {
      "CodeExample5",
      "\\s45\\li1800\\widctlpar\\adjustright \\shading1000\\cbpat8 \\f2\\fs16\\cgrid ",
      "\\sbasedon0 \\snext46 Code Example 5"
   },
   {
      "CodeExample6",
      "\\s46\\li2160\\widctlpar\\adjustright \\shading1000\\cbpat8 \\f2\\fs16\\cgrid ",
      "\\sbasedon0 \\snext47 Code Example 6"
   },
   {
      "CodeExample7",
      "\\s47\\li2520\\widctlpar\\adjustright \\shading1000\\cbpat8 \\f2\\fs16\\cgrid ",
      "\\sbasedon0 \\snext48 Code Example 7"
   },
   {
      "CodeExample8",
      "\\s48\\li2880\\widctlpar\\adjustright \\shading1000\\cbpat8 \\f2\\fs16\\cgrid ",
      "\\sbasedon0 \\snext49 Code Example 8"
   },
   {
      "CodeExample9",
      "\\s49\\li3240\\widctlpar\\adjustright \\shading1000\\cbpat8 \\f2\\fs16\\cgrid ",
      "\\sbasedon0 \\snext49 Code Example 9"
   },
   {
      "ListContinue0",
      "\\s50\\li0\\sa60\\sb30\\qj\\widctlpar\\qj\\adjustright \\fs20\\cgrid ",
      "\\sbasedon0 \\snext51 List Continue 0"
   },
   {
      "ListContinue1",
      "\\s51\\li360\\sa60\\sb30\\qj\\widctlpar\\qj\\adjustright \\fs20\\cgrid ",
      "\\sbasedon0 \\snext52 List Continue 1"
   },
   {
      "ListContinue2",
      "\\s52\\li720\\sa60\\sb30\\qj\\widctlpar\\qj\\adjustright \\fs20\\cgrid ",
      "\\sbasedon0 \\snext53 List Continue 2"
   },
   {
      "ListContinue3",
      "\\s53\\li1080\\sa60\\sb30\\qj\\widctlpar\\qj\\adjustright \\fs20\\cgrid ",
      "\\sbasedon0 \\snext54 List Continue 3"
   },
   {
      "ListContinue4",
      "\\s54\\li1440\\sa60\\sb30\\qj\\widctlpar\\qj\\adjustright \\fs20\\cgrid ",
      "\\sbasedon0 \\snext55 List Continue 4"
   },
   {
      "ListContinue5",
      "\\s55\\li1800\\sa60\\sb30\\qj\\widctlpar\\qj\\adjustright \\fs20\\cgrid ",
      "\\sbasedon0 \\snext56 List Continue 5"
   },
   {
      "ListContinue6",
      "\\s56\\li2160\\sa60\\sb30\\qj\\widctlpar\\qj\\adjustright \\fs20\\cgrid ",
      "\\sbasedon0 \\snext57 List Continue 6"
   },
   {
      "ListContinue7",
      "\\s57\\li2520\\sa60\\sb30\\qj\\widctlpar\\qj\\adjustright \\fs20\\cgrid ",
      "\\sbasedon0 \\snext58 List Continue 7"
   },
   {
      "ListContinue8",
      "\\s58\\li2880\\sa60\\sb30\\qj\\widctlpar\\qj\\adjustright \\fs20\\cgrid ",
      "\\sbasedon0 \\snext59 List Continue 8"
   },
   {
      "ListContinue9",
      "\\s59\\li3240\\sa60\\sb30\\qj\\widctlpar\\qj\\adjustright \\fs20\\cgrid ",
      "\\sbasedon0 \\snext59 List Continue 9"
   },
   {
      "DescContinue0",
      "\\s60\\li0\\widctlpar\\ql\\adjustright \\fs20\\cgrid ",
      "\\sbasedon0 \\snext61 DescContinue 0"
   },
   {
      "DescContinue1",
      "\\s61\\li360\\widctlpar\\ql\\adjustright \\fs20\\cgrid ",
      "\\sbasedon0 \\snext62 DescContinue 1"
   },
   {
      "DescContinue2",
      "\\s62\\li720\\widctlpar\\ql\\adjustright \\fs20\\cgrid ",
      "\\sbasedon0 \\snext63 DescContinue 2"
   },
   {
      "DescContinue3",
      "\\s63\\li1080\\widctlpar\\ql\\adjustright \\fs20\\cgrid ",
      "\\sbasedon0 \\snext64 DescContinue 3"
   },
   {
      "DescContinue4",
      "\\s64\\li1440\\widctlpar\\ql\\adjustright \\fs20\\cgrid ",
      "\\sbasedon0 \\snext65 DescContinue 4"
   },
   {
      "DescContinue5",
      "\\s65\\li1800\\widctlpar\\ql\\adjustright \\fs20\\cgrid ",
      "\\sbasedon0 \\snext66 DescContinue 5"
   },
   {
      "DescContinue6",
      "\\s66\\li2160\\widctlpar\\ql\\adjustright \\fs20\\cgrid ",
      "\\sbasedon0 \\snext67 DescContinue 6"
   },
   {
      "DescContinue7",
      "\\s67\\li2520\\widctlpar\\ql\\adjustright \\fs20\\cgrid ",
      "\\sbasedon0 \\snext68 DescContinue 7"
   },
   {
      "DescContinue8",
      "\\s68\\li2880\\widctlpar\\ql\\adjustright \\fs20\\cgrid ",
      "\\sbasedon0 \\snext69 DescContinue 8"
   },
   {
      "DescContinue9",
      "\\s69\\li3240\\widctlpar\\ql\\adjustright \\fs20\\cgrid ",
      "\\sbasedon0 \\snext69 DescContinue 9"
   },
   {
      "LatexTOC0",
      "\\s70\\li0\\sa30\\sb30\\widctlpar\\tqr\\tldot\\tx8640\\adjustright \\fs20\\cgrid ",
      "\\sbasedon0 \\snext81 LatexTOC 0"
   },
   {
      "LatexTOC1",
      "\\s71\\li360\\sa27\\sb27\\widctlpar\\tqr\\tldot\\tx8640\\adjustright \\fs20\\cgrid ",
      "\\sbasedon0 \\snext82 LatexTOC 1"
   },
   {
      "LatexTOC2",
      "\\s72\\li720\\sa24\\sb24\\widctlpar\\tqr\\tldot\\tx8640\\adjustright \\fs20\\cgrid ",
      "\\sbasedon0 \\snext83 LatexTOC 2"
   },
   {
      "LatexTOC3",
      "\\s73\\li1080\\sa21\\sb21\\widctlpar\\tqr\\tldot\\tx8640\\adjustright \\fs20\\cgrid ",
      "\\sbasedon0 \\snext84 LatexTOC 3"
   },
   {
      "LatexTOC4",
      "\\s74\\li1440\\sa18\\sb18\\widctlpar\\tqr\\tldot\\tx8640\\adjustright \\fs20\\cgrid ",
      "\\sbasedon0 \\snext85 LatexTOC 4"
   },
   {
      "LatexTOC5",
      "\\s75\\li1800\\sa15\\sb15\\widctlpar\\tqr\\tldot\\tx8640\\adjustright \\fs20\\cgrid ",
      "\\sbasedon0 \\snext86 LatexTOC 5"
   },
   {
      "LatexTOC6",
      "\\s76\\li2160\\sa12\\sb12\\widctlpar\\tqr\\tldot\\tx8640\\adjustright \\fs20\\cgrid ",
      "\\sbasedon0 \\snext87 LatexTOC 6"
   },
   {
      "LatexTOC7",
      "\\s77\\li2520\\sa9\\sb9\\widctlpar\\tqr\\tldot\\tx8640\\adjustright \\fs20\\cgrid ",
      "\\sbasedon0 \\snext88 LatexTOC 7"
   },
   {
      "LatexTOC8",
      "\\s78\\li2880\\sa6\\sb6\\widctlpar\\tqr\\tldot\\tx8640\\adjustright \\fs20\\cgrid ",
      "\\sbasedon0 \\snext89 LatexTOC 8"
   },
   {
      "LatexTOC9",
      "\\s79\\li3240\\sa3\\sb3\\widctlpar\\tqr\\tldot\\tx8640\\adjustright \\fs20\\cgrid ",
      "\\sbasedon0 \\snext89 LatexTOC 9"
   },
   {
      "ListBullet0",
      "\\s80\\fi-360\\li360\\widctlpar\\jclisttab\\tx360{\\*\\pn \\pnlvlbody\\ilvl0\\ls1\\pnrnot0\\pndec }\\ls1\\adjustright \\fs20\\cgrid ",
      "\\sbasedon0 \\snext81 \\sautoupd List Bullet 0"
   },
   {
      "ListBullet1",
      "\\s81\\fi-360\\li720\\widctlpar\\jclisttab\\tx720{\\*\\pn \\pnlvlbody\\ilvl0\\ls2\\pnrnot0\\pndec }\\ls2\\adjustright \\fs20\\cgrid ",
      "\\sbasedon0 \\snext82 \\sautoupd List Bullet 1"
   },
   {
      "ListBullet2",
      "\\s82\\fi-360\\li1080\\widctlpar\\jclisttab\\tx1080{\\*\\pn \\pnlvlbody\\ilvl0\\ls3\\pnrnot0\\pndec }\\ls3\\adjustright \\fs20\\cgrid ",
      "\\sbasedon0 \\snext83 \\sautoupd List Bullet 2"
   },
   {
      "ListBullet3",
      "\\s83\\fi-360\\li1440\\widctlpar\\jclisttab\\tx1440{\\*\\pn \\pnlvlbody\\ilvl0\\ls4\\pnrnot0\\pndec }\\ls4\\adjustright \\fs20\\cgrid ",
      "\\sbasedon0 \\snext84 \\sautoupd List Bullet 3"
   },
   {
      "ListBullet4",
      "\\s84\\fi-360\\li1800\\widctlpar\\jclisttab\\tx1800{\\*\\pn \\pnlvlbody\\ilvl0\\ls5\\pnrnot0\\pndec }\\ls5\\adjustright \\fs20\\cgrid ",
      "\\sbasedon0 \\snext85 \\sautoupd List Bullet 4"
   },
   {
      "ListBullet5",
      "\\s85\\fi-360\\li2160\\widctlpar\\jclisttab\\tx2160{\\*\\pn \\pnlvlbody\\ilvl0\\ls6\\pnrnot0\\pndec }\\ls6\\adjustright \\fs20\\cgrid ",
      "\\sbasedon0 \\snext86 \\sautoupd List Bullet 5"
   },
   {
      "ListBullet6",
      "\\s86\\fi-360\\li2520\\widctlpar\\jclisttab\\tx2520{\\*\\pn \\pnlvlbody\\ilvl0\\ls7\\pnrnot0\\pndec }\\ls7\\adjustright \\fs20\\cgrid ",
      "\\sbasedon0 \\snext87 \\sautoupd List Bullet 6"
   },
   {
      "ListBullet7",
      "\\s87\\fi-360\\li2880\\widctlpar\\jclisttab\\tx2880{\\*\\pn \\pnlvlbody\\ilvl0\\ls8\\pnrnot0\\pndec }\\ls8\\adjustright \\fs20\\cgrid ",
      "\\sbasedon0 \\snext88 \\sautoupd List Bullet 7"
   },
   {
      "ListBullet8",
      "\\s88\\fi-360\\li3240\\widctlpar\\jclisttab\\tx3240{\\*\\pn \\pnlvlbody\\ilvl0\\ls9\\pnrnot0\\pndec }\\ls9\\adjustright \\fs20\\cgrid ",
      "\\sbasedon0 \\snext89 \\sautoupd List Bullet 8"
   },
   {
      "ListBullet9",
      "\\s89\\fi-360\\li3600\\widctlpar\\jclisttab\\tx3600{\\*\\pn \\pnlvlbody\\ilvl0\\ls10\\pnrnot0\\pndec }\\ls10\\adjustright \\fs20\\cgrid ",
      "\\sbasedon0 \\snext89 \\sautoupd List Bullet 9"
   },
   {
      "ListEnum0",
      "\\s90\\fi-360\\li360\\widctlpar\\fs20\\cgrid ",
      "\\sbasedon0 \\snext91 \\sautoupd List Enum 0"
   },
   {
      "ListEnum1",
      "\\s91\\fi-360\\li720\\widctlpar\\fs20\\cgrid ",
      "\\sbasedon0 \\snext92 \\sautoupd List Enum 1"
   },
   {
      "ListEnum2",
      "\\s92\\fi-360\\li1080\\widctlpar\\fs20\\cgrid ",
      "\\sbasedon0 \\snext93 \\sautoupd List Enum 2"
   },
   {
      "ListEnum3",
      "\\s93\\fi-360\\li1440\\widctlpar\\fs20\\cgrid ",
      "\\sbasedon0 \\snext94 \\sautoupd List Enum 3"
   },
   {
      "ListEnum4",
      "\\s94\\fi-360\\li1800\\widctlpar\\fs20\\cgrid ",
      "\\sbasedon0 \\snext95 \\sautoupd List Enum 4"
   },
   {
      "ListEnum5",
      "\\s95\\fi-360\\li2160\\widctlpar\\fs20\\cgrid ",
      "\\sbasedon0 \\snext96 \\sautoupd List Enum 5"
   },
   {
      "ListEnum6",
      "\\s96\\fi-360\\li2520\\widctlpar\\fs20\\cgrid ",
      "\\sbasedon0 \\snext96 \\sautoupd List Enum 5"
   },
   {
      "ListEnum7",
      "\\s97\\fi-360\\li2880\\widctlpar\\fs20\\cgrid ",
      "\\sbasedon0 \\snext98 \\sautoupd List Enum 7"
   },
   {
      "ListEnum8",
      "\\s98\\fi-360\\li3240\\widctlpar\\fs20\\cgrid ",
      "\\sbasedon0 \\snext99 \\sautoupd List Enum 8"
   },
   {
      "ListEnum9",
      "\\s99\\fi-360\\li3600\\widctlpar\\fs20\\cgrid ",
      "\\sbasedon0 \\snext99 \\sautoupd List Enum 9"
   },
   {
      0,
      0,
      0
   }
};

const QRegExp StyleData::s_clause("\\\\s([0-9]+)\\s*");

StyleData::StyleData(const QString &reference, const QString &definition)
{  
   int start = s_clause.indexIn(reference);
   assert(start >= 0);

   QString tmp = reference.mid(start);   
   index = s_clause.cap(1).toLong();

   assert(index > 0);

   this->reference  = tmp;
   this->definition = definition;
}

StyleData::StyleData()
{
   assert(false);  
} 

StyleData::~StyleData()
{  
}

bool StyleData::setStyle(const QString &str, const QString &styleName)
{
   static const QRegExp subgroup("^\\{[^}]*\\}\\s*");
   static const QRegExp any_clause("^\\\\[a-z][a-z0-9-]*\\s*");

   int len = 0;     // length of a particular RTF formatting control
   int ref_len = 0; // length of the whole formatting section of a style

   QString s = str;

   int start = s_clause.indexIn(s);
   len = s_clause.matchedLength();   

   if (start < 0) {
      err("Style sheet '%s' contains no '\\s' clause.\n{%s}\n", qPrintable(styleName), qPrintable(s));
      return false;
   }

   s = s.mid(start);  
   index = s.mid(2).toLong();

   assert(index > 0);

   // search for the end of pure formatting codes
   QString end = s.mid(len);
   ref_len = len;

   bool haveNewDefinition = true;

   while (true) {

      if (end.startsWith('{')) {
         // subgroups are used for \\additive

         int startX = subgroup.indexIn(end);
         len = subgroup.matchedLength();  

         if (startX != 0) {
            break;

         } else {
            end = end.mid(len);
            ref_len += len;
         }

      } else if (end.startsWith('\\')) {
         if (end.startsWith("\\snext")) {
            break;
         }

         if (end.startsWith("\\sbasedon")) {
            break;
         }

         int startX = any_clause.indexIn(end);
         len = any_clause.matchedLength();  

         if (startX != 0) {
            break;
         }

         end = end.mid(len);
         ref_len += len;

      } else if (end.isEmpty()) {
         // no style-definition part, keep default value
         haveNewDefinition = false;
         break;

      } else { 
         // plain name without leading \\snext
         break;

      }
   } 
   
   reference = s.mid(ref_len);

   if (haveNewDefinition) {
      definition = end;
   }

   return true;
}

void loadStylesheet(const QString &name, QHash<QString, StyleData> &dict)
{
   QFile file(name);

   if (! file.open(QIODevice::ReadOnly)) {
      err("Unable to open RTF style sheet file %s, using default file\n", qPrintable(name));
      return;
   }
   msg("Loading RTF style sheet %s\n", qPrintable(name));

   static const QRegExp seperator("[ \t]*=[ \t]*");
   uint lineNr = 1;

   QTextStream t(&file);
   t.setCodec("UTF-8");

   while (! t.atEnd()) {
      QString s = t.readLine().trimmed();

      if (s.isEmpty() || s.at(0) == '#') {
         continue;   // skip blanks & comments
      }

      int sepStart;
      int sepLength;
      
      sepStart  = seperator.indexIn(s);
      sepLength = seperator.matchedLength();  

      if (sepStart <= 0) {
         // no valid assignment statement
         warn(qPrintable(name), lineNr, "Assignment of style sheet name expected\n");
         continue;
      }

      QString key = s.left(sepStart);

      if (! dict.contains(key)) { 
         // not a valid style sheet name
         warn(qPrintable(name), lineNr, "Invalid style sheet name %s ignored.\n", qPrintable(key));
         continue;
      }

      // add command separator   
      StyleData &styleData = dict.find(key).value();

      s += " "; 
      styleData.setStyle(s.mid(sepStart + sepLength), key);

      lineNr++;
   }
}

QHash<QString, StyleData> rtf_Style;

void loadExtensions(const QString &name)
{
   QFile file(name);

   if (! file.open(QIODevice::ReadOnly)) {
      err("Unable to open RTF extensions file %s, error: %d, Using defaut values\n", qPrintable(name), file.error());      
      return;
   }
   msg("Loading RTF extensions %s\n", qPrintable(name));

   static const QRegExp separator("[ \t]*=[ \t]*");
   uint lineNr = 1;

   QTextStream t(&file);
   t.setCodec("UTF-8");

   while (! t.atEnd()) {
      // string buffer of max line length

      QString s = t.readLine().trimmed();

      if (s.length() == 0 || s.at(0) == '#') {
         continue;   // skip blanks & comments
      }

      int sepStart  = separator.indexIn(s);
      int sepLength = separator.matchedLength();  

      if (sepStart <= 0) { 
         // no valid assignment statement
         warn(qPrintable(name), lineNr, "Assignment of extension field expected\n");
         continue;
      }

      QString key  = s.left(sepStart);
      QString data = s.mid(sepStart + sepLength);

      if (key == "Title") {
         rtf_title            = data;
      }
      if (key == "Subject") {
         rtf_subject          = data;
      }
      if (key == "Comments") {
         rtf_comments         = data;
      }
      if (key == "Company") {
         rtf_company          = data;
      }
      if (key == "LogoFilename") {
         rtf_logoFilename     = data;
      }
      if (key == "Author") {
         rtf_author           = data;
      }
      if (key == "Manager") {
         rtf_manager          = data;
      }
      if (key == "DocumentType") {
         rtf_documentType     = data;
      }
      if (key == "DocumentId") {
         rtf_documentId       = data;
      }
      if (key == "Keywords") {
         rtf_keywords         = data;
      }

      lineNr++;
   }
}

