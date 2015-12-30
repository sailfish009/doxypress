/*************************************************************************
 *
 * Copyright (C) 2014-2015 Barbara Geller & Ansel Sermersheim 
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

#ifndef LATEXDOCVISITOR_H
#define LATEXDOCVISITOR_H

#include <QList>
#include <QStack>
#include <QTextStream>

#include <docvisitor.h>
class CodeOutputInterface;

/*! @brief Concrete visitor implementation for LaTeX output. */
class LatexDocVisitor : public DocVisitor
{
 public:
   LatexDocVisitor(QTextStream &t, CodeOutputInterface &ci, const QString &langExt, bool insideTabbing);

   // visitor functions for leaf nodes

   void visit(DocWord *);
   void visit(DocLinkedWord *);
   void visit(DocWhiteSpace *);
   void visit(DocSymbol *);
   void visit(DocURL *);
   void visit(DocLineBreak *);
   void visit(DocHorRuler *);
   void visit(DocStyleChange *);
   void visit(DocVerbatim *);
   void visit(DocAnchor *);
   void visit(DocInclude *);
   void visit(DocIncOperator *);
   void visit(DocFormula *);
   void visit(DocIndexEntry *);
   void visit(DocSimpleSectSep *);
   void visit(DocCite *);

   //--------------------------------------
   // visitor functions for compound nodes
   //--------------------------------------

   void visitPre(DocAutoList *);
   void visitPost(DocAutoList *);
   void visitPre(DocAutoListItem *);
   void visitPost(DocAutoListItem *);
   void visitPre(DocPara *);
   void visitPost(DocPara *);
   void visitPre(DocRoot *);
   void visitPost(DocRoot *);
   void visitPre(DocSimpleSect *);
   void visitPost(DocSimpleSect *);
   void visitPre(DocTitle *);
   void visitPost(DocTitle *);
   void visitPre(DocSimpleList *);
   void visitPost(DocSimpleList *);
   void visitPre(DocSimpleListItem *);
   void visitPost(DocSimpleListItem *);
   void visitPre(DocSection *s);
   void visitPost(DocSection *);
   void visitPre(DocHtmlList *s);
   void visitPost(DocHtmlList *s);
   void visitPre(DocHtmlListItem *);
   void visitPost(DocHtmlListItem *);
   //void visitPre(DocHtmlPre *);
   //void visitPost(DocHtmlPre *);
   void visitPre(DocHtmlDescList *);
   void visitPost(DocHtmlDescList *);
   void visitPre(DocHtmlDescTitle *);
   void visitPost(DocHtmlDescTitle *);
   void visitPre(DocHtmlDescData *);
   void visitPost(DocHtmlDescData *);
   void visitPre(DocHtmlTable *t);
   void visitPost(DocHtmlTable *t);
   void visitPre(DocHtmlCaption *);
   void visitPost(DocHtmlCaption *);
   void visitPre(DocHtmlRow *);
   void visitPost(DocHtmlRow *) ;
   void visitPre(DocHtmlCell *);
   void visitPost(DocHtmlCell *);
   void visitPre(DocInternal *);
   void visitPost(DocInternal *);
   void visitPre(DocHRef *);
   void visitPost(DocHRef *);
   void visitPre(DocHtmlHeader *);
   void visitPost(DocHtmlHeader *) ;
   void visitPre(DocImage *);
   void visitPost(DocImage *);
   void visitPre(DocDotFile *);
   void visitPost(DocDotFile *);
   void visitPre(DocMscFile *);
   void visitPost(DocMscFile *);
   void visitPre(DocDiaFile *);
   void visitPost(DocDiaFile *);
   void visitPre(DocLink *lnk);
   void visitPost(DocLink *);
   void visitPre(DocRef *ref);
   void visitPost(DocRef *);
   void visitPre(DocSecRefItem *);
   void visitPost(DocSecRefItem *);
   void visitPre(DocSecRefList *);
   void visitPost(DocSecRefList *);
   void visitPre(DocParamSect *);
   void visitPost(DocParamSect *);
   void visitPre(DocParamList *);
   void visitPost(DocParamList *);
   void visitPre(DocXRefItem *);
   void visitPost(DocXRefItem *);
   void visitPre(DocInternalRef *);
   void visitPost(DocInternalRef *);
   void visitPre(DocCopy *);
   void visitPost(DocCopy *);
   void visitPre(DocText *);
   void visitPost(DocText *);
   void visitPre(DocHtmlBlockQuote *);
   void visitPost(DocHtmlBlockQuote *);  
   void visitPre(DocParBlock *);
   void visitPost(DocParBlock *);

 private:

   struct ActiveRowSpan {
      ActiveRowSpan(DocHtmlCell *c, int rs, int cs, int col)
         : cell(c), rowSpan(rs), colSpan(cs), column(col) {}

      DocHtmlCell *cell;
      int rowSpan;
      int colSpan;
      int column;
   };

   typedef QList<ActiveRowSpan> RowSpanList;

   void filter(const QString &str);
   void startLink(const QString &ref, const QString &file, const QString &anchor);
   void endLink(const QString &ref, const QString &file, const QString &anchor);
   QString escapeMakeIndexChars(const QString &s);
   void startDotFile(const QString &fileName, const QString &width, const QString &height, bool hasCaption);
   void endDotFile(bool hasCaption);

   void startMscFile(const QString &fileName, const QString &width, const QString &height, bool hasCaption);
   void endMscFile(bool hasCaption);
   void writeMscFile(const QString &fileName, DocVerbatim *s);

   void startDiaFile(const QString &fileName, const QString &width, const QString &height, bool hasCaption);
   void endDiaFile(bool hasCaption);
   void writeDiaFile(const QString &fileName, DocVerbatim *s);
   void writePlantUMLFile(const QString &fileName, DocVerbatim *s);

   void pushEnabled();
   void popEnabled();

   QTextStream &m_t;
   CodeOutputInterface &m_ci;
   bool m_insidePre;
   bool m_insideItem;
   bool m_hide;
   bool m_insideTabbing;
   bool m_insideTable;
   int  m_numCols;

   QStack<bool> m_enabled;
   QString m_langExt;
   RowSpanList m_rowSpans;

   int m_currentColumn;
   bool m_inRowspan;
   bool m_inColspan;
   bool m_firstRow;
};

#endif
