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

#include <QDir>

#include <htmldocvisitor.h>
#include <docparser.h>
#include <language.h>
#include <doxygen.h>
#include <outputgen.h>
#include <dot.h>
#include <message.h>
#include <config.h>
#include <htmlgen.h>
#include <parserintf.h>
#include <msc.h>
#include <dia.h>
#include <util.h>
#include <filedef.h>
#include <memberdef.h>
#include <htmlentity.h>
#include <plantuml.h>

// must appear after the previous include - resolve soon 
#include <doxy_globals.h>

static const int NUM_HTML_LIST_TYPES = 4;
static const char types[][NUM_HTML_LIST_TYPES] = {"1", "a", "i", "A"};

static QByteArray convertIndexWordToAnchor(const QString &word)
{
   static char hex[] = "0123456789abcdef";

   QByteArray result;

   QByteArray temp = word.toUtf8();
   const char *str = temp.data();

   unsigned char c;

   if (str) {
      while ((c = *str++)) {
         if ((c >= 'a' && c <= 'z') || // ALPHA
               (c >= 'A' && c <= 'A') || // ALPHA
               (c >= '0' && c <= '9') || // DIGIT
               c == '-' ||
               c == '.' ||
               c == '_' ||
               c == '~'
            ) {
            result += c;
         } else {
            char enc[4];
            enc[0] = '%';
            enc[1] = hex[(c & 0xf0) >> 4];
            enc[2] = hex[c & 0xf];
            enc[3] = 0;
            result += enc;
         }
      }
   }

   return result;
}

static bool mustBeOutsideParagraph(DocNode *n)
{
   switch (n->kind()) {
      /* <ul> */
      case DocNode::Kind_HtmlList:
      case DocNode::Kind_SimpleList:
      case DocNode::Kind_AutoList:

      /* <dl> */
      case DocNode::Kind_SimpleSect:
      case DocNode::Kind_ParamSect:
      case DocNode::Kind_HtmlDescList:
      case DocNode::Kind_XRefItem:

      /* <table> */
      case DocNode::Kind_HtmlTable:

      /* <h?> */
      case DocNode::Kind_Section:
      case DocNode::Kind_HtmlHeader:

      /* \internal */
      case DocNode::Kind_Internal:

      /* <div> */
      case DocNode::Kind_Include:
      case DocNode::Kind_Image:
      case DocNode::Kind_SecRefList:

      /* <hr> */
      case DocNode::Kind_HorRuler:
      /* CopyDoc gets paragraph markers from the wrapping DocPara node,
       * but needs to insert them for all documentation being copied to
       * preserve formatting.
       */

      case DocNode::Kind_Copy:
      /* <blockquote> */

      case DocNode::Kind_HtmlBlockQuote:
      /* \parblock */

      case DocNode::Kind_ParBlock:
         return true;

      case DocNode::Kind_Verbatim: {
         DocVerbatim *dv = (DocVerbatim *)n;
         return dv->type() != DocVerbatim::HtmlOnly || dv->isBlock();
      }

      case DocNode::Kind_StyleChange:
         return ((DocStyleChange *)n)->style() == DocStyleChange::Preformatted ||
                ((DocStyleChange *)n)->style() == DocStyleChange::Div ||
                ((DocStyleChange *)n)->style() == DocStyleChange::Center;

      case DocNode::Kind_Formula:
         return !((DocFormula *)n)->isInline();

      default:
         break;
   }

   return false;
}

static QString htmlAttribsToString(const HtmlAttribList &attribs)
{
   QString result;
  
   for (auto att : attribs) { 
      if (! att.value.isEmpty())  {
         // ignore attribute without values as they are not XHTML compliant

         result += " ";
         result += att.name;
         result += "=\"" + convertToXML(att.value) + "\"";
      }
   }
   return result;
}

HtmlDocVisitor::HtmlDocVisitor(QTextStream &t, CodeOutputInterface &ci, Definition *ctx)
   : DocVisitor(DocVisitor_Html), m_t(t), m_ci(ci), m_insidePre(false), m_hide(false), m_ctx(ctx)
{
   if (ctx) {
      m_langExt = ctx->getDefFileExtension();
   }
}

//--------------------------------------
// visitor functions for leaf nodes
//--------------------------------------

void HtmlDocVisitor::visit(DocWord *w)
{
   //printf("word: %s\n",w->word().data());
   if (m_hide) {
      return;
   }
   filter(w->word());
}

void HtmlDocVisitor::visit(DocLinkedWord *w)
{
   if (m_hide) {
      return;
   }
   
   startLink(w->ref(), w->file(), w->relPath(), w->anchor(), w->tooltip());
   filter(w->word());
   endLink();
}

void HtmlDocVisitor::visit(DocWhiteSpace *w)
{
   if (m_hide) {
      return;
   }

   if (m_insidePre) {
      m_t << w->chars();
   } else {
      m_t << " ";
   }
}

void HtmlDocVisitor::visit(DocSymbol *s)
{
   if (m_hide) {
      return;
   }

   const char *res = HtmlEntityMapper::instance()->html(s->symbol());
   if (res) {
      m_t << res;
   } else {
      err("HTML: non supported HTML-entity found: %s\n", HtmlEntityMapper::instance()->html(s->symbol(), true));
   }
}

void HtmlDocVisitor::writeObfuscatedMailAddress(const QByteArray &url)
{
   m_t << "<a href=\"#\" onclick=\"location.href='mai'+'lto:'";
   uint i;
   int size = 3;

   for (i = 0; i < url.length();) {
      m_t << "+'" << url.mid(i, size) << "'";
      i += size;
      if (size == 3) {
         size = 2;
      } else {
         size = 3;
      }
   }
   m_t << "; return false;\">";
}

void HtmlDocVisitor::visit(DocURL *u)
{
   if (m_hide) {
      return;
   }

   if (u->isEmail()) { // mail address
      QByteArray url = u->url();
      writeObfuscatedMailAddress(url);
      uint size = 5, i;

      for (i = 0; i < url.length();) {
         filter(url.mid(i, size));
         if (i < url.length() - size) {
            m_t << "<span style=\"display: none;\">.nosp@m.</span>";
         }
         i += size;
         if (size == 5) {
            size = 4;
         } else {
            size = 5;
         }
      }
      m_t << "</a>";

   } else { // web address
      m_t << "<a href=\"";
      m_t << u->url() << "\">";
      filter(u->url());
      m_t << "</a>";
   }
}

void HtmlDocVisitor::visit(DocLineBreak *)
{
   if (m_hide) {
      return;
   }
   m_t << "<br />\n";
}

void HtmlDocVisitor::visit(DocHorRuler *hr)
{
   if (m_hide) {
      return;
   }

   forceEndParagraph(hr);
   m_t << "<hr/>\n";
   forceStartParagraph(hr);
}

void HtmlDocVisitor::visit(DocStyleChange *s)
{
   if (m_hide) {
      return;
   }

   switch (s->style()) {
      case DocStyleChange::Bold:
         if (s->enable()) {
            m_t << "<b" << htmlAttribsToString(s->attribs()) << ">";
         }      else {
            m_t << "</b>";
         }
         break;

      case DocStyleChange::Italic:
         if (s->enable()) {
            m_t << "<em" << htmlAttribsToString(s->attribs()) << ">";
         }     else {
            m_t << "</em>";
         }
         break;

      case DocStyleChange::Code:
         if (s->enable()) {
            m_t << "<code" << htmlAttribsToString(s->attribs()) << ">";
         }   else {
            m_t << "</code>";
         }
         break;

      case DocStyleChange::Subscript:
         if (s->enable()) {
            m_t << "<sub" << htmlAttribsToString(s->attribs()) << ">";
         }    else {
            m_t << "</sub>";
         }
         break;

      case DocStyleChange::Superscript:
         if (s->enable()) {
            m_t << "<sup" << htmlAttribsToString(s->attribs()) << ">";
         }    else {
            m_t << "</sup>";
         }
         break;

      case DocStyleChange::Center:
         if (s->enable()) {
            forceEndParagraph(s);
            m_t << "<center" << htmlAttribsToString(s->attribs()) << ">";
         } else {
            m_t << "</center>";
            forceStartParagraph(s);
         }
         break;

      case DocStyleChange::Small:
         if (s->enable()) {
            m_t << "<small" << htmlAttribsToString(s->attribs()) << ">";
         }  else {
            m_t << "</small>";
         }
         break;

      case DocStyleChange::Preformatted:
         if (s->enable()) {
            forceEndParagraph(s);
            m_t << "<pre" << htmlAttribsToString(s->attribs()) << ">";
            m_insidePre = true;

         } else {
            m_insidePre = false;
            m_t << "</pre>";
            forceStartParagraph(s);
         }
         break;

      case DocStyleChange::Div:
         if (s->enable()) {
            forceEndParagraph(s);
            m_t << "<div " << htmlAttribsToString(s->attribs()) << ">";

         } else {
            m_t << "</div>";
            forceStartParagraph(s);
         }
         break;

      case DocStyleChange::Span:
         if (s->enable()) {
            m_t << "<span" << htmlAttribsToString(s->attribs()) << ">";
         }  else {
            m_t << "</span>";
         }
         break;

   }
}

void HtmlDocVisitor::visit(DocVerbatim *s)
{
   if (m_hide) {
      return;
   }

   QByteArray lang = m_langExt;
   if (! s->language().isEmpty()) { 
      // explicit language setting
      lang = s->language();
   }

   SrcLangExt langExt = getLanguageFromFileName(lang);

   switch (s->type()) {

      case DocVerbatim::Code:
         forceEndParagraph(s);

         m_t << PREFRAG_START;

         Doxygen::parserManager->getParser(lang)->parseCode(m_ci,
                     s->context(),
                     s->text(),
                     langExt,
                     s->isExample(),
                     s->exampleFile(),
                     0,     // fileDef
                     -1,    // startLine
                     -1,    // endLine
                     false, // inlineFragment
                     0,     // memberDef
                     true,  // show line numbers
                     m_ctx  // search context
                    );

         m_t << PREFRAG_END;

         forceStartParagraph(s);
         break;

      case DocVerbatim::Verbatim:
         forceEndParagraph(s);
         m_t << /*PREFRAG_START <<*/ "<pre class=\"fragment\">";
         filter(s->text());
         m_t << "</pre>" /*<< PREFRAG_END*/;
         forceStartParagraph(s);
         break;

      case DocVerbatim::HtmlOnly:
         if (s->isBlock()) {
            forceEndParagraph(s);
         }
         m_t << s->text();
         if (s->isBlock()) {
            forceStartParagraph(s);
         }
         break;

      case DocVerbatim::ManOnly:
      case DocVerbatim::LatexOnly:
      case DocVerbatim::XmlOnly:
      case DocVerbatim::RtfOnly:
      case DocVerbatim::DocbookOnly:
         /* nothing */
         break;

      case DocVerbatim::Dot: {
         static int dotindex = 1;

         QString fileName;
         fileName = QString("%1%2.dot").arg(QString(Config_getString("HTML_OUTPUT") + "/inline_dotgraph_")).arg(dotindex++);

         QFile file(fileName);

         if (!file.open(QIODevice::WriteOnly)) {
            err("Could not open file %s for writing\n", fileName.data());
         }

         file.write( s->text(), s->text().length() );
         file.close();

         forceEndParagraph(s);
         m_t << "<div align=\"center\">" << endl;

         writeDotFile(fileName.toUtf8(), s->relPath(), s->context());
         m_t << "</div>" << endl;
         forceStartParagraph(s);

         if (Config_getBool("DOT_CLEANUP")) {
            file.remove();
         }
      }
      break;

      case DocVerbatim::Msc: {
         forceEndParagraph(s);

         static int mscindex = 1;

         QString baseName;
         baseName = QString("%1%2").arg(QString(Config_getString("HTML_OUTPUT") + "/inline_mscgraph_")).arg(mscindex++);

         QFile file(baseName + ".msc");

         if (!file.open(QIODevice::WriteOnly)) {
            err("Could not open file %s.msc for writing\n", baseName.data());
         }

         QByteArray text = "msc {";
         text += s->text();
         text += "}";

         file.write( text, text.length() );
         file.close();

         m_t << "<div align=\"center\">" << endl;

         writeMscFile((baseName + ".msc").toUtf8(), s->relPath(), s->context());
         if (Config_getBool("DOT_CLEANUP")) {
            file.remove();
         }

         m_t << "</div>" << endl;
         forceStartParagraph(s);
      }

      break;
      case DocVerbatim::PlantUML: {
         forceEndParagraph(s);

         static QByteArray htmlOutput = Config_getString("HTML_OUTPUT");
         QByteArray baseName = writePlantUMLSource(htmlOutput, s->exampleFile(), s->text());
         m_t << "<div align=\"center\">" << endl;
         writePlantUMLFile(baseName, s->relPath(), s->context());
         m_t << "</div>" << endl;
         forceStartParagraph(s);
      }
      break;
   }
}

void HtmlDocVisitor::visit(DocAnchor *anc)
{
   if (m_hide) {
      return;
   }
   m_t << "<a class=\"anchor\" id=\"" << anc->anchor() << "\"></a>";
}

void HtmlDocVisitor::visit(DocInclude *inc)
{
   if (m_hide) {
      return;
   }

   SrcLangExt langExt = getLanguageFromFileName(inc->extension());

   switch (inc->type()) {

      case DocInclude::Include:
         forceEndParagraph(inc);
         m_t << PREFRAG_START;

         Doxygen::parserManager->getParser(inc->extension())->parseCode(m_ci,
                     inc->context(),
                     inc->text(),
                     langExt,
                     inc->isExample(),
                     inc->exampleFile(),
                     0,     // fileDef
                     -1,    // startLine
                     -1,    // endLine
                     true,  // inlineFragment
                     0,     // memberDef
                     false, // show line numbers
                     m_ctx  // search context
                    );
         m_t << PREFRAG_END;
         forceStartParagraph(inc);
         break;

      case DocInclude::IncWithLines: {
         forceEndParagraph(inc);
         m_t << PREFRAG_START;

         QFileInfo cfi( inc->file() );
         FileDef fd( cfi.path().toUtf8(), cfi.fileName().toUtf8() );

         Doxygen::parserManager->getParser(inc->extension())->parseCode(m_ci,
                     inc->context(),
                     inc->text(),
                     langExt,
                     inc->isExample(),
                     inc->exampleFile(),
                     &fd,   // fileDef,
                     -1,    // start line
                     -1,    // end line
                     false, // inline fragment
                     0,     // memberDef
                     true,  // show line numbers
                     m_ctx  // search context
                    );
         m_t << PREFRAG_END;
         forceStartParagraph(inc);
      }
      break;
      case DocInclude::DontInclude:
         break;
      case DocInclude::HtmlInclude:
         m_t << inc->text();
         break;
      case DocInclude::LatexInclude:
         break;

      case DocInclude::VerbInclude:
         forceEndParagraph(inc);
         m_t << /*PREFRAG_START <<*/ "<pre class=\"fragment\">";
         filter(inc->text());
         m_t << "</pre>" /*<< PREFRAG_END*/;
         forceStartParagraph(inc);
         break;

      case DocInclude::Snippet: {
         forceEndParagraph(inc);
         m_t << PREFRAG_START;

         Doxygen::parserManager->getParser(inc->extension())->parseCode(m_ci,
                     inc->context(),
                     extractBlock(inc->text(), inc->blockId()),
                     langExt,
                     inc->isExample(),
                     inc->exampleFile(),
                     0,
                     -1,    // startLine
                     -1,    // endLine
                     true,  // inlineFragment
                     0,     // memberDef
                     true,  // show line number
                     m_ctx  // search context
                    );
         m_t << PREFRAG_END;
         forceStartParagraph(inc);
      }
      break;
   }
}

void HtmlDocVisitor::visit(DocIncOperator *op)
{  
   if (op->isFirst()) {
      if (! m_hide) {
         m_t << PREFRAG_START;
      }
      pushEnabled();
      m_hide = true;
   }

   SrcLangExt langExt = getLanguageFromFileName(m_langExt);

   if (op->type() != DocIncOperator::Skip) {
      popEnabled();
      if (!m_hide) {
         Doxygen::parserManager->getParser(m_langExt)->parseCode(
            m_ci,
            op->context(),
            op->text(),
            langExt,
            op->isExample(),
            op->exampleFile(),
            0,     // fileDef
            -1,    // startLine
            -1,    // endLine
            false, // inline fragment
            0,     // memberDef
            true,  // show line numbers
            m_ctx  // search context
         );
      }
      pushEnabled();
      m_hide = true;
   }
   if (op->isLast()) {
      popEnabled();
      if (!m_hide) {
         m_t << PREFRAG_END;
      }
   } else {
      if (!m_hide) {
         m_t << endl;
      }
   }
}

void HtmlDocVisitor::visit(DocFormula *f)
{
   if (m_hide) {
      return;
   }

   bool bDisplay = ! f->isInline();

   if (bDisplay) {
      forceEndParagraph(f);
      m_t << "<p class=\"formulaDsp\">" << endl;
   }

   if (Config_getBool("USE_MATHJAX")) {
      QByteArray text = f->text();
      bool closeInline = false;
      if (!bDisplay && !text.isEmpty() && text.at(0) == '$' &&
            text.at(text.length() - 1) == '$') {
         closeInline = true;
         text = text.mid(1, text.length() - 2);
         m_t << "\\(";
      }
      m_t << convertToHtml(text);
      if (closeInline) {
         m_t << "\\)";
      }
   } else {
      m_t << "<img class=\"formula"
          << (bDisplay ? "Dsp" : "Inl");
      m_t << "\" alt=\"";
      filterQuotedCdataAttr(f->text());
      m_t << "\"";
      // TODO: cache image dimensions on formula generation and give height/width
      // for faster preloading and better rendering of the page
      m_t << " src=\"" << f->relPath() << f->name() << ".png\"/>";

   }
   if (bDisplay) {
      m_t << endl << "</p>" << endl;
      forceStartParagraph(f);
   }
}

void HtmlDocVisitor::visit(DocIndexEntry *e)
{
   QByteArray anchor = convertIndexWordToAnchor(e->entry());
   if (e->member()) {
      anchor.prepend(e->member()->anchor() + "_");
   }
   m_t << "<a name=\"" << anchor << "\"></a>";
   //printf("*** DocIndexEntry: word='%s' scope='%s' member='%s'\n",
   //       e->entry().data(),
   //       e->scope()  ? e->scope()->name().data()  : "<null>",
   //       e->member() ? e->member()->name().data() : "<null>"
   //      );
   Doxygen::indexList->addIndexItem(e->scope(), e->member(), anchor, e->entry());
}

void HtmlDocVisitor::visit(DocSimpleSectSep *)
{
   m_t << "</dd>" << endl;
   m_t << "<dd>" << endl;
}

void HtmlDocVisitor::visit(DocCite *cite)
{
   if (m_hide) {
      return;
   }
   if (!cite->file().isEmpty()) {
      startLink(cite->ref(), cite->file(), cite->relPath(), cite->anchor());
   } else {
      m_t << "<b>[";
   }
   filter(cite->text());
   if (!cite->file().isEmpty()) {
      endLink();
   } else {
      m_t << "]</b>";
   }
}


//--------------------------------------
// visitor functions for compound nodes
//--------------------------------------


void HtmlDocVisitor::visitPre(DocAutoList *l)
{
   //printf("DocAutoList::visitPre\n");
   if (m_hide) {
      return;
   }
   forceEndParagraph(l);
   if (l->isEnumList()) {
      //
      // Do list type based on depth:
      // 1.
      //   a.
      //     i.
      //       A.
      //         1. (repeat)...
      //
      m_t << "<ol type=\"" << types[l->depth() % NUM_HTML_LIST_TYPES] << "\">";
   } else {
      m_t << "<ul>";
   }
   if (!l->isPreformatted()) {
      m_t << "\n";
   }
}

void HtmlDocVisitor::visitPost(DocAutoList *l)
{
   //printf("DocAutoList::visitPost\n");
   if (m_hide) {
      return;
   }
   if (l->isEnumList()) {
      m_t << "</ol>";
   } else {
      m_t << "</ul>";
   }
   if (!l->isPreformatted()) {
      m_t << "\n";
   }
   forceStartParagraph(l);
}

void HtmlDocVisitor::visitPre(DocAutoListItem *)
{
   if (m_hide) {
      return;
   }
   m_t << "<li>";
}

void HtmlDocVisitor::visitPost(DocAutoListItem *li)
{
   if (m_hide) {
      return;
   }
   m_t << "</li>";
   if (!li->isPreformatted()) {
      m_t << "\n";
   }
}

template<class T>
bool isFirstChildNode(T *parent, DocNode *node)
{
   return parent->children().first() == node;
}

template<class T>
bool isLastChildNode(T *parent, DocNode *node)
{
   return parent->children().last() == node;
}

bool isSeparatedParagraph(DocSimpleSect *parent, DocPara *par)
{
   QList<DocNode *> nodes = parent->children();

   int i = nodes.indexOf(par);

   if (i == -1) {
      return false;
   }

   int count = parent->children().count();
   if (count > 1 && i == 0) { // first node
      if (nodes.at(i + 1)->kind() == DocNode::Kind_SimpleSectSep) {
         return true;
      }

   } else if (count > 1 && i == count - 1) { // last node
      if (nodes.at(i - 1)->kind() == DocNode::Kind_SimpleSectSep) {
         return true;
      }

   } else if (count > 2 && i > 0 && i < count - 1) { // intermediate node
      if (nodes.at(i - 1)->kind() == DocNode::Kind_SimpleSectSep && nodes.at(i + 1)->kind() == DocNode::Kind_SimpleSectSep) {
         return true;
      }
   }
   return false;
}

static int getParagraphContext(DocPara *p, bool &isFirst, bool &isLast)
{
   int t   = 0;
   isFirst = false;
   isLast   = false;

   if (p && p->parent()) {

      switch (p->parent()->kind()) {
         case DocNode::Kind_ParBlock: {
            // hierarchy: node N -> para -> parblock -> para
            // adapt return value to kind of N

            DocNode::Kind kind = DocNode::Kind_Para;
            if ( p->parent()->parent() && p->parent()->parent()->parent() ) {
               kind = p->parent()->parent()->parent()->kind();
            }

            isFirst = isFirstChildNode((DocParBlock *)p->parent(), p);
            isLast = isLastChildNode ((DocParBlock *)p->parent(), p);
            t = 0;

            if (isFirst) {
               if (kind == DocNode::Kind_HtmlListItem ||
                     kind == DocNode::Kind_SecRefItem) {
                  t = 1;
               } else if (kind == DocNode::Kind_HtmlDescData ||
                          kind == DocNode::Kind_XRefItem ||
                          kind == DocNode::Kind_SimpleSect) {
                  t = 2;
               } else if (kind == DocNode::Kind_HtmlCell ||
                          kind == DocNode::Kind_ParamList) {
                  t = 5;
               }
            }
            if (isLast) {
               if (kind == DocNode::Kind_HtmlListItem ||
                     kind == DocNode::Kind_SecRefItem) {
                  t = 3;
               } else if (kind == DocNode::Kind_HtmlDescData ||
                          kind == DocNode::Kind_XRefItem ||
                          kind == DocNode::Kind_SimpleSect) {
                  t = 4;
               } else if (kind == DocNode::Kind_HtmlCell ||
                          kind == DocNode::Kind_ParamList) {
                  t = 6;
               }
            }
            break;
         }

         case DocNode::Kind_AutoListItem:
            isFirst = isFirstChildNode((DocAutoListItem *)p->parent(), p);
            isLast = isLastChildNode ((DocAutoListItem *)p->parent(), p);
            t = 1; // not used
            break;

         case DocNode::Kind_SimpleListItem:
            isFirst = true;
            isLast = true;
            t = 1; // not used
            break;

         case DocNode::Kind_ParamList:
            isFirst = true;
            isLast = true;
            t = 1; // not used
            break;

         case DocNode::Kind_HtmlListItem:
            isFirst = isFirstChildNode((DocHtmlListItem *)p->parent(), p);
            isLast = isLastChildNode ((DocHtmlListItem *)p->parent(), p);
            if (isFirst) {
               t = 1;
            }
            if (isLast) {
               t = 3;
            }
            break;
         case DocNode::Kind_SecRefItem:
            isFirst = isFirstChildNode((DocSecRefItem *)p->parent(), p);
            isLast = isLastChildNode ((DocSecRefItem *)p->parent(), p);
            if (isFirst) {
               t = 1;
            }
            if (isLast) {
               t = 3;
            }
            break;
         case DocNode::Kind_HtmlDescData:
            isFirst = isFirstChildNode((DocHtmlDescData *)p->parent(), p);
            isLast = isLastChildNode ((DocHtmlDescData *)p->parent(), p);
            if (isFirst) {
               t = 2;
            }
            if (isLast) {
               t = 4;
            }
            break;
         case DocNode::Kind_XRefItem:
            isFirst = isFirstChildNode((DocXRefItem *)p->parent(), p);
            isLast = isLastChildNode ((DocXRefItem *)p->parent(), p);
            if (isFirst) {
               t = 2;
            }
            if (isLast) {
               t = 4;
            }
            break;
         case DocNode::Kind_SimpleSect:
            isFirst = isFirstChildNode((DocSimpleSect *)p->parent(), p);
            isLast = isLastChildNode ((DocSimpleSect *)p->parent(), p);
            if (isFirst) {
               t = 2;
            }
            if (isLast) {
               t = 4;
            }
            if (isSeparatedParagraph((DocSimpleSect *)p->parent(), p))
               // if the paragraph is enclosed with separators it will
               // be included in <dd>..</dd> so avoid addition paragraph
               // markers
            {
               isFirst = isLast = true;
            }
            break;
         case DocNode::Kind_HtmlCell:
            isFirst = isFirstChildNode((DocHtmlCell *)p->parent(), p);
            isLast = isLastChildNode ((DocHtmlCell *)p->parent(), p);
            if (isFirst) {
               t = 5;
            }
            if (isLast) {
               t = 6;
            }
            break;
         default:
            break;
      }
      //printf("para=%p parent()->kind=%d isFirst=%d isLast=%d t=%d\n",
      //    p,p->parent()->kind(),isFirst,isLast,t);
   }
   return t;
}

void HtmlDocVisitor::visitPre(DocPara *p)
{
   if (m_hide) {
      return;
   }

   //printf("DocPara::visitPre: parent of kind %d ",
   //       p->parent() ? p->parent()->kind() : -1);

   bool needsTag = false;
   if (p && p->parent()) {
      switch (p->parent()->kind()) {
         case DocNode::Kind_Section:
         case DocNode::Kind_Internal:
         case DocNode::Kind_HtmlListItem:
         case DocNode::Kind_HtmlDescData:
         case DocNode::Kind_HtmlCell:
         case DocNode::Kind_SimpleListItem:
         case DocNode::Kind_AutoListItem:
         case DocNode::Kind_SimpleSect:
         case DocNode::Kind_XRefItem:
         case DocNode::Kind_Copy:
         case DocNode::Kind_HtmlBlockQuote:
         case DocNode::Kind_ParBlock:
            needsTag = true;
            break;
         case DocNode::Kind_Root:
            needsTag = !((DocRoot *)p->parent())->singleLine();
            break;
         default:
            needsTag = false;
      }
   }

   // if the first element of a paragraph is something that should be outside of
   // the paragraph (<ul>,<dl>,<table>,..) then that will already started the
   // paragraph and we don't need to do it here
   uint nodeIndex = 0;

   if (p && nodeIndex < p->children().count()) {
      while (nodeIndex < p->children().count() &&
             p->children().at(nodeIndex)->kind() == DocNode::Kind_WhiteSpace) {
         nodeIndex++;
      }
      if (nodeIndex < p->children().count()) {
         DocNode *n = p->children().at(nodeIndex);
         if (mustBeOutsideParagraph(n)) {
            needsTag = false;
         }
      }
   }

   // check if this paragraph is the first or last child of a <li> or <dd>.
   // this allows us to mark the tag with a special class so we can
   // fix the otherwise ugly spacing.
   int t;

   static const char *contexts[7] = {
      "",                     // 0
      " class=\"startli\"",   // 1
      " class=\"startdd\"",   // 2
      " class=\"endli\"",     // 3
      " class=\"enddd\"",     // 4
      " class=\"starttd\"",   // 5
      " class=\"endtd\""      // 6
   };

   bool isFirst;
   bool isLast;

   t = getParagraphContext(p, isFirst, isLast);
  
   if (isFirst && isLast) {
      needsTag = false;
   }

   //printf("  needsTag=%d\n",needsTag);
   // write the paragraph tag (if needed)
   if (needsTag) {
      m_t << "<p" << contexts[t] << ">";
   }
}

void HtmlDocVisitor::visitPost(DocPara *p)
{
   bool needsTag = false;
   if (p && p->parent()) {
      switch (p->parent()->kind()) {
         case DocNode::Kind_Section:
         case DocNode::Kind_Internal:
         case DocNode::Kind_HtmlListItem:
         case DocNode::Kind_HtmlDescData:
         case DocNode::Kind_HtmlCell:
         case DocNode::Kind_SimpleListItem:
         case DocNode::Kind_AutoListItem:
         case DocNode::Kind_SimpleSect:
         case DocNode::Kind_XRefItem:
         case DocNode::Kind_Copy:
         case DocNode::Kind_HtmlBlockQuote:
         case DocNode::Kind_ParBlock:
            needsTag = true;
            break;
         case DocNode::Kind_Root:
            needsTag = !((DocRoot *)p->parent())->singleLine();
            break;
         default:
            needsTag = false;
      }
   }

   QByteArray context;
   // if the last element of a paragraph is something that should be outside of
   // the paragraph (<ul>,<dl>,<table>) then that will already have ended the
   // paragraph and we don't need to do it here
   int nodeIndex = p->children().count() - 1;
   if (p && nodeIndex >= 0) {
      while (nodeIndex >= 0 && p->children().at(nodeIndex)->kind() == DocNode::Kind_WhiteSpace) {
         nodeIndex--;
      }
      if (nodeIndex >= 0) {
         DocNode *n = p->children().at(nodeIndex);
         if (mustBeOutsideParagraph(n)) {
            needsTag = false;
         }
      }
   }

   bool isFirst;
   bool isLast;
   getParagraphContext(p, isFirst, isLast);
   //printf("endPara first=%d last=%d\n",isFirst,isLast);
   if (isFirst && isLast) {
      needsTag = false;
   }

   //printf("DocPara::visitPost needsTag=%d\n",needsTag);

   if (needsTag) {
      m_t << "</p>\n";
   }

}

void HtmlDocVisitor::visitPre(DocRoot *)
{
}

void HtmlDocVisitor::visitPost(DocRoot *)
{
}

void HtmlDocVisitor::visitPre(DocSimpleSect *s)
{
   if (m_hide) {
      return;
   }
   forceEndParagraph(s);
   m_t << "<dl class=\"section " << s->typeString() << "\"><dt>";
   switch (s->type()) {
      case DocSimpleSect::See:
         m_t << theTranslator->trSeeAlso();
         break;
      case DocSimpleSect::Return:
         m_t << theTranslator->trReturns();
         break;
      case DocSimpleSect::Author:
         m_t << theTranslator->trAuthor(true, true);
         break;
      case DocSimpleSect::Authors:
         m_t << theTranslator->trAuthor(true, false);
         break;
      case DocSimpleSect::Version:
         m_t << theTranslator->trVersion();
         break;
      case DocSimpleSect::Since:
         m_t << theTranslator->trSince();
         break;
      case DocSimpleSect::Date:
         m_t << theTranslator->trDate();
         break;
      case DocSimpleSect::Note:
         m_t << theTranslator->trNote();
         break;
      case DocSimpleSect::Warning:
         m_t << theTranslator->trWarning();
         break;
      case DocSimpleSect::Pre:
         m_t << theTranslator->trPrecondition();
         break;
      case DocSimpleSect::Post:
         m_t << theTranslator->trPostcondition();
         break;
      case DocSimpleSect::Copyright:
         m_t << theTranslator->trCopyright();
         break;
      case DocSimpleSect::Invar:
         m_t << theTranslator->trInvariant();
         break;
      case DocSimpleSect::Remark:
         m_t << theTranslator->trRemarks();
         break;
      case DocSimpleSect::Attention:
         m_t << theTranslator->trAttention();
         break;
      case DocSimpleSect::User:
         break;
      case DocSimpleSect::Rcs:
         break;
      case DocSimpleSect::Unknown:
         break;
   }

   // special case 1: user defined title
   if (s->type() != DocSimpleSect::User && s->type() != DocSimpleSect::Rcs) {
      m_t << "</dt><dd>";
   }
}

void HtmlDocVisitor::visitPost(DocSimpleSect *s)
{
   if (m_hide) {
      return;
   }
   m_t << "</dd></dl>\n";
   forceStartParagraph(s);
}

void HtmlDocVisitor::visitPre(DocTitle *)
{
}

void HtmlDocVisitor::visitPost(DocTitle *)
{
   if (m_hide) {
      return;
   }
   m_t << "</dt><dd>";
}

void HtmlDocVisitor::visitPre(DocSimpleList *sl)
{
   if (m_hide) {
      return;
   }
   forceEndParagraph(sl);
   m_t << "<ul>";
   if (!sl->isPreformatted()) {
      m_t << "\n";
   }

}

void HtmlDocVisitor::visitPost(DocSimpleList *sl)
{
   if (m_hide) {
      return;
   }
   m_t << "</ul>";
   if (!sl->isPreformatted()) {
      m_t << "\n";
   }
   forceStartParagraph(sl);
}

void HtmlDocVisitor::visitPre(DocSimpleListItem *)
{
   if (m_hide) {
      return;
   }
   m_t << "<li>";
}

void HtmlDocVisitor::visitPost(DocSimpleListItem *li)
{
   if (m_hide) {
      return;
   }
   m_t << "</li>";
   if (!li->isPreformatted()) {
      m_t << "\n";
   }
}

void HtmlDocVisitor::visitPre(DocSection *s)
{
   if (m_hide) {
      return;
   }
   forceEndParagraph(s);
   m_t << "<h" << s->level() << ">";
   m_t << "<a class=\"anchor\" id=\"" << s->anchor();
   m_t << "\"></a>" << endl;
   filter(convertCharEntitiesToUTF8(s->title().data()));
   m_t << "</h" << s->level() << ">\n";
}

void HtmlDocVisitor::visitPost(DocSection *s)
{
   forceStartParagraph(s);
}

void HtmlDocVisitor::visitPre(DocHtmlList *s)
{
   if (m_hide) {
      return;
   }
   forceEndParagraph(s);
   if (s->type() == DocHtmlList::Ordered) {
      m_t << "<ol" << htmlAttribsToString(s->attribs()) << ">\n";
   } else {
      m_t << "<ul" << htmlAttribsToString(s->attribs()) << ">\n";
   }
}

void HtmlDocVisitor::visitPost(DocHtmlList *s)
{
   if (m_hide) {
      return;
   }
   if (s->type() == DocHtmlList::Ordered) {
      m_t << "</ol>";
   } else {
      m_t << "</ul>";
   }
   if (!s->isPreformatted()) {
      m_t << "\n";
   }
   forceStartParagraph(s);
}

void HtmlDocVisitor::visitPre(DocHtmlListItem *i)
{
   if (m_hide) {
      return;
   }
   m_t << "<li" << htmlAttribsToString(i->attribs()) << ">";
   if (!i->isPreformatted()) {
      m_t << "\n";
   }
}

void HtmlDocVisitor::visitPost(DocHtmlListItem *)
{
   if (m_hide) {
      return;
   }
   m_t << "</li>\n";
}

void HtmlDocVisitor::visitPre(DocHtmlDescList *dl)
{
   if (m_hide) {
      return;
   }
   forceEndParagraph(dl);
   m_t << "<dl" << htmlAttribsToString(dl->attribs()) << ">\n";
}

void HtmlDocVisitor::visitPost(DocHtmlDescList *dl)
{
   if (m_hide) {
      return;
   }
   m_t << "</dl>\n";
   forceStartParagraph(dl);
}

void HtmlDocVisitor::visitPre(DocHtmlDescTitle *dt)
{
   if (m_hide) {
      return;
   }
   m_t << "<dt" << htmlAttribsToString(dt->attribs()) << ">";
}

void HtmlDocVisitor::visitPost(DocHtmlDescTitle *)
{
   if (m_hide) {
      return;
   }
   m_t << "</dt>\n";
}

void HtmlDocVisitor::visitPre(DocHtmlDescData *dd)
{
   if (m_hide) {
      return;
   }
   m_t << "<dd" << htmlAttribsToString(dd->attribs()) << ">";
}

void HtmlDocVisitor::visitPost(DocHtmlDescData *)
{
   if (m_hide) {
      return;
   }
   m_t << "</dd>\n";
}

void HtmlDocVisitor::visitPre(DocHtmlTable *t)
{
   if (m_hide) {
      return;
   }

   forceEndParagraph(t);

   QString attrs = htmlAttribsToString(t->attribs());
   if (attrs.isEmpty()) {
      m_t << "<table class=\"doxtable\">\n";
   } else {
      m_t << "<table " << htmlAttribsToString(t->attribs()) << ">\n";
   }
}

void HtmlDocVisitor::visitPost(DocHtmlTable *t)
{
   if (m_hide) {
      return;
   }
   m_t << "</table>\n";
   forceStartParagraph(t);
}

void HtmlDocVisitor::visitPre(DocHtmlRow *tr)
{
   if (m_hide) {
      return;
   }
   m_t << "<tr" << htmlAttribsToString(tr->attribs()) << ">\n";
}

void HtmlDocVisitor::visitPost(DocHtmlRow *)
{
   if (m_hide) {
      return;
   }
   m_t << "</tr>\n";
}

void HtmlDocVisitor::visitPre(DocHtmlCell *c)
{
   if (m_hide) {
      return;
   }
   if (c->isHeading()) {
      m_t << "<th" << htmlAttribsToString(c->attribs()) << ">";
   } else {
      m_t << "<td" << htmlAttribsToString(c->attribs()) << ">";
   }
}

void HtmlDocVisitor::visitPost(DocHtmlCell *c)
{
   if (m_hide) {
      return;
   }
   if (c->isHeading()) {
      m_t << "</th>";
   } else {
      m_t << "</td>";
   }
}

void HtmlDocVisitor::visitPre(DocHtmlCaption *c)
{
   if (m_hide) {
      return;
   }

   bool hasAlign = false;
  
   for (auto att : c->attribs()) {    
      if (att.name == "align") {
         hasAlign = true;
         break;
      }
   }

   m_t << "<caption" << htmlAttribsToString(c->attribs());

   if (! hasAlign) {
      m_t << " align=\"bottom\"";
   }

   m_t << ">";
}

void HtmlDocVisitor::visitPost(DocHtmlCaption *)
{
   if (m_hide) {
      return;
   }
   m_t << "</caption>\n";
}

void HtmlDocVisitor::visitPre(DocInternal *)
{
   if (m_hide) {
      return;
   }
   //forceEndParagraph(i);
   //m_t << "<p><b>" << theTranslator->trForInternalUseOnly() << "</b></p>" << endl;
}

void HtmlDocVisitor::visitPost(DocInternal *)
{
   if (m_hide) {
      return;
   }
   //forceStartParagraph(i);
}

void HtmlDocVisitor::visitPre(DocHRef *href)
{
   if (m_hide) {
      return;
   }
   if (href->url().left(7) == "mailto:") {
      writeObfuscatedMailAddress(href->url().mid(7));
   } else {
      QByteArray url = correctURL(href->url(), href->relPath());
      m_t << "<a href=\"" << convertToXML(url)  << "\""
          << htmlAttribsToString(href->attribs()) << ">";
   }
}

void HtmlDocVisitor::visitPost(DocHRef *)
{
   if (m_hide) {
      return;
   }
   m_t << "</a>";
}

void HtmlDocVisitor::visitPre(DocHtmlHeader *header)
{
   if (m_hide) {
      return;
   }
   forceEndParagraph(header);
   m_t << "<h" << header->level()
       << htmlAttribsToString(header->attribs()) << ">";
}

void HtmlDocVisitor::visitPost(DocHtmlHeader *header)
{
   if (m_hide) {
      return;
   }
   m_t << "</h" << header->level() << ">\n";
   forceStartParagraph(header);
}

void HtmlDocVisitor::visitPre(DocImage *img)
{
   if (img->type() == DocImage::Html) {
      forceEndParagraph(img);
      if (m_hide) {
         return;
      }
      QString baseName = img->name();
      int i;
      if ((i = baseName.lastIndexOf('/')) != -1 || (i = baseName.lastIndexOf('\\')) != -1) {
         baseName = baseName.right(baseName.length() - i - 1);
      }
      m_t << "<div class=\"image\">" << endl;
      QByteArray url = img->url();
      if (url.isEmpty()) {
         m_t << "<img src=\"" << img->relPath() << img->name() << "\" alt=\""
             << baseName << "\"" << htmlAttribsToString(img->attribs())
             << "/>" << endl;
      } else {
         m_t << "<img src=\"" << correctURL(url, img->relPath()) << "\" "
             << htmlAttribsToString(img->attribs())
             << "/>" << endl;
      }
      if (img->hasCaption()) {
         m_t << "<div class=\"caption\">" << endl;
      }
   } else { // other format -> skip
      pushEnabled();
      m_hide = true;
   }
}

void HtmlDocVisitor::visitPost(DocImage *img)
{
   if (img->type() == DocImage::Html) {
      if (m_hide) {
         return;
      }
      if (img->hasCaption()) {
         m_t << "</div>";
      }
      m_t << "</div>" << endl;
      forceStartParagraph(img);
   } else { // other format
      popEnabled();
   }
}

void HtmlDocVisitor::visitPre(DocDotFile *df)
{
   if (m_hide) {
      return;
   }
   m_t << "<div class=\"dotgraph\">" << endl;
   writeDotFile(df->file(), df->relPath(), df->context());
   if (df->hasCaption()) {
      m_t << "<div class=\"caption\">" << endl;
   }
}

void HtmlDocVisitor::visitPost(DocDotFile *df)
{
   if (m_hide) {
      return;
   }
   if (df->hasCaption()) {
      m_t << "</div>" << endl;
   }
   m_t << "</div>" << endl;
}

void HtmlDocVisitor::visitPre(DocMscFile *df)
{
   if (m_hide) {
      return;
   }
   m_t << "<div class=\"mscgraph\">" << endl;
   writeMscFile(df->file(), df->relPath(), df->context());
   if (df->hasCaption()) {
      m_t << "<div class=\"caption\">" << endl;
   }
}
void HtmlDocVisitor::visitPost(DocMscFile *df)
{
   if (m_hide) {
      return;
   }
   if (df->hasCaption()) {
      m_t << "</div>" << endl;
   }
   m_t << "</div>" << endl;
}

void HtmlDocVisitor::visitPre(DocDiaFile *df)
{
   if (m_hide) {
      return;
   }
   m_t << "<div class=\"diagraph\">" << endl;
   writeDiaFile(df->file(), df->relPath(), df->context());
   if (df->hasCaption()) {
      m_t << "<div class=\"caption\">" << endl;
   }
}
void HtmlDocVisitor::visitPost(DocDiaFile *df)
{
   if (m_hide) {
      return;
   }
   if (df->hasCaption()) {
      m_t << "</div>" << endl;
   }
   m_t << "</div>" << endl;
}

void HtmlDocVisitor::visitPre(DocLink *lnk)
{
   if (m_hide) {
      return;
   }
   startLink(lnk->ref(), lnk->file(), lnk->relPath(), lnk->anchor());
}

void HtmlDocVisitor::visitPost(DocLink *)
{
   if (m_hide) {
      return;
   }
   endLink();
}

void HtmlDocVisitor::visitPre(DocRef *ref)
{
   if (m_hide) {
      return;
   }
   if (!ref->file().isEmpty()) {
      // when ref->isSubPage()==true we use ref->file() for HTML and
      // ref->anchor() for LaTeX/RTF
      startLink(ref->ref(), ref->file(), ref->relPath(), ref->isSubPage() ? QByteArray() : ref->anchor());
   }
   if (!ref->hasLinkText()) {
      filter(ref->targetTitle());
   }
}

void HtmlDocVisitor::visitPost(DocRef *ref)
{
   if (m_hide) {
      return;
   }
   if (!ref->file().isEmpty()) {
      endLink();
   }
   //m_t << " ";
}

void HtmlDocVisitor::visitPre(DocSecRefItem *ref)
{
   if (m_hide) {
      return;
   }
   QString refName = ref->file();
   if (refName.right(Doxygen::htmlFileExtension.length()) !=
         QString(Doxygen::htmlFileExtension)) {
      refName += Doxygen::htmlFileExtension;
   }
   m_t << "<li><a href=\"" << refName << "#" << ref->anchor() << "\">";

}

void HtmlDocVisitor::visitPost(DocSecRefItem *)
{
   if (m_hide) {
      return;
   }
   m_t << "</a></li>\n";
}

void HtmlDocVisitor::visitPre(DocSecRefList *s)
{
   if (m_hide) {
      return;
   }
   forceEndParagraph(s);
   m_t << "<div class=\"multicol\">" << endl;
   m_t << "<ul>" << endl;
}

void HtmlDocVisitor::visitPost(DocSecRefList *s)
{
   if (m_hide) {
      return;
   }
   m_t << "</ul>" << endl;
   m_t << "</div>" << endl;
   forceStartParagraph(s);
}

//void HtmlDocVisitor::visitPre(DocLanguage *l)
//{
//  QString langId = Config_getEnum("OUTPUT_LANGUAGE");
//  if (l->id().toLower()!=langId.lower())
//  {
//    pushEnabled();
//    m_hide = true;
//  }
//}
//
//void HtmlDocVisitor::visitPost(DocLanguage *l)
//{
//  QString langId = Config_getEnum("OUTPUT_LANGUAGE");
//  if (l->id().toLower()!=langId.lower())
//  {
//    popEnabled();
//  }
//}

void HtmlDocVisitor::visitPre(DocParamSect *s)
{
   if (m_hide) {
      return;
   }
   forceEndParagraph(s);
   QByteArray className;
   QByteArray heading;
   switch (s->type()) {
      case DocParamSect::Param:
         heading = theTranslator->trParameters();
         className = "params";
         break;
      case DocParamSect::RetVal:
         heading = theTranslator->trReturnValues();
         className = "retval";
         break;
      case DocParamSect::Exception:
         heading = theTranslator->trExceptions();
         className = "exception";
         break;
      case DocParamSect::TemplateParam:
         heading = theTranslator->trTemplateParameters();
         className = "tparams";
         break;
      default:
         assert(0);
   }
   m_t << "<dl class=\"" << className << "\"><dt>";
   m_t << heading;
   m_t << "</dt><dd>" << endl;
   m_t << "  <table class=\"" << className << "\">" << endl;
}

void HtmlDocVisitor::visitPost(DocParamSect *s)
{
   if (m_hide) {
      return;
   }
   m_t << "  </table>" << endl;
   m_t << "  </dd>" << endl;
   m_t << "</dl>" << endl;
   forceStartParagraph(s);
}

void HtmlDocVisitor::visitPre(DocParamList *pl)
{
   if (m_hide) {
      return;
   }

   m_t << "    <tr>";
   DocParamSect *sect = 0;

   if (pl->parent()->kind() == DocNode::Kind_ParamSect) {
      sect = (DocParamSect *)pl->parent();
   }

   if (sect && sect->hasInOutSpecifier()) {
      m_t << "<td class=\"paramdir\">";
      if (pl->direction() != DocParamSect::Unspecified) {
         m_t << "[";
         if (pl->direction() == DocParamSect::In) {
            m_t << "in";
         } else if (pl->direction() == DocParamSect::Out) {
            m_t << "out";
         } else if (pl->direction() == DocParamSect::InOut) {
            m_t << "in,out";
         }
         m_t << "]";
      }
      m_t << "</td>";
   }

   if (sect && sect->hasTypeSpecifier()) {
      m_t << "<td class=\"paramtype\">";
      ;
      bool first = true;
      
      for (auto type : pl->paramTypes()) { 
         if (!first) {
            m_t << "&#160;|&#160;";

         } else {
            first = false;
         }

         if (type->kind() == DocNode::Kind_Word) {
            visit((DocWord *)type);

         } else if (type->kind() == DocNode::Kind_LinkedWord) {
            visit((DocLinkedWord *)type);
         }
      }

      m_t << "</td>";
   }

   m_t << "<td class=\"paramname\">";
   //QStringListIterator li(pl->parameters());
   //const char *s;
  
   bool first = true;
 
   for (auto param : pl->parameters()) {
      if (!first) {
         m_t << ",";
      } else {
         first = false;
      }
      if (param->kind() == DocNode::Kind_Word) {
         visit((DocWord *)param);
      } else if (param->kind() == DocNode::Kind_LinkedWord) {
         visit((DocLinkedWord *)param);
      }
   }
   m_t << "</td><td>";
}

void HtmlDocVisitor::visitPost(DocParamList *)
{
   //printf("DocParamList::visitPost\n");
   if (m_hide) {
      return;
   }
   m_t << "</td></tr>" << endl;
}

void HtmlDocVisitor::visitPre(DocXRefItem *x)
{
   if (m_hide) {
      return;
   }
   if (x->title().isEmpty()) {
      return;
   }

   forceEndParagraph(x);
   bool anonymousEnum = x->file() == "@";
   if (!anonymousEnum) {
      m_t << "<dl class=\"" << x->key() << "\"><dt><b><a class=\"el\" href=\""
          << x->relPath() << x->file() << Doxygen::htmlFileExtension
          << "#" << x->anchor() << "\">";
   } else {
      m_t << "<dl class=\"" << x->key() << "\"><dt><b>";
   }
   filter(x->title());
   m_t << ":";
   if (!anonymousEnum) {
      m_t << "</a>";
   }
   m_t << "</b></dt><dd>";
}

void HtmlDocVisitor::visitPost(DocXRefItem *x)
{
   if (m_hide) {
      return;
   }
   if (x->title().isEmpty()) {
      return;
   }
   m_t << "</dd></dl>" << endl;
   forceStartParagraph(x);
}

void HtmlDocVisitor::visitPre(DocInternalRef *ref)
{
   if (m_hide) {
      return;
   }
   startLink(0, ref->file(), ref->relPath(), ref->anchor());
}

void HtmlDocVisitor::visitPost(DocInternalRef *)
{
   if (m_hide) {
      return;
   }
   endLink();
   m_t << " ";
}

void HtmlDocVisitor::visitPre(DocCopy *)
{
}

void HtmlDocVisitor::visitPost(DocCopy *)
{
}

void HtmlDocVisitor::visitPre(DocText *)
{
}

void HtmlDocVisitor::visitPost(DocText *)
{
}

void HtmlDocVisitor::visitPre(DocHtmlBlockQuote *b)
{
   if (m_hide) {
      return;
   }
   forceEndParagraph(b);

   QString attrs = htmlAttribsToString(b->attribs());
   if (attrs.isEmpty()) {
      m_t << "<blockquote class=\"doxtable\">\n";
   } else {
      m_t << "<blockquote " << htmlAttribsToString(b->attribs()) << ">\n";
   }
}

void HtmlDocVisitor::visitPost(DocHtmlBlockQuote *b)
{
   if (m_hide) {
      return;
   }
   m_t << "</blockquote>" << endl;
   forceStartParagraph(b);
}

void HtmlDocVisitor::visitPre(DocParBlock *)
{
   if (m_hide) {
      return;
   }
}

void HtmlDocVisitor::visitPost(DocParBlock *)
{
   if (m_hide) {
      return;
   }
}

void HtmlDocVisitor::filter(const char *str)
{
   if (str == 0) {
      return;
   }
   const char *p = str;
   char c;
   while (*p) {
      c = *p++;
      switch (c) {
         case '<':
            m_t << "&lt;";
            break;
         case '>':
            m_t << "&gt;";
            break;
         case '&':
            m_t << "&amp;";
            break;
         default:
            m_t << c;
      }
   }
}

/// Escape basic entities to produce a valid CDATA attribute value,
/// assume that the outer quoting will be using the double quote &quot;
void HtmlDocVisitor::filterQuotedCdataAttr(const char *str)
{
   if (str == 0) {
      return;
   }
   const char *p = str;
   char c;
   while (*p) {
      c = *p++;
      switch (c) {
         case '&':
            m_t << "&amp;";
            break;
         case '"':
            m_t << "&quot;";
            break;
         // For SGML compliance, and given the SGML declaration for HTML syntax,
         // it's enough to replace these two, provided that the declaration
         // for the HTML version we generate (and as supported by the browser)
         // specifies that all the other symbols used in rawVal are
         // within the right character class (i.e., they're not
         // some multinational weird characters not in the BASESET).
         // We assume that 1) the browser will support whatever is remaining
         // in the formula and 2) the TeX formulae are generally governed
         // by even stricter character restrictions so it should be enough.
         //
         // On some incompliant browsers, additional translation of
         // '>' and '<' into "&gt;" and "&lt;", respectively, might be needed;
         // but I'm unaware of particular modern (last 4 years) versions
         // with such problems, so let's not do it for performance.
         // Also, some brousers will (wrongly) not process the entity references
         // inside the attribute value and show the &...; form instead,
         // so we won't create entites unless necessary to minimize clutter there.
         // --vassilii
         default:
            m_t << c;
      }
   }
}

void HtmlDocVisitor::startLink(const QByteArray &ref, const QByteArray &file,
                               const QByteArray &relPath, const QByteArray &anchor,
                               const QByteArray &tooltip)
{
   //printf("HtmlDocVisitor: file=%s anchor=%s\n",file.data(),anchor.data());
   if (!ref.isEmpty()) { // link to entity imported via tag file
      m_t << "<a class=\"elRef\" ";
      m_t << externalLinkTarget() << externalRef(relPath, ref, false);
   } else { // local link
      m_t << "<a class=\"el\" ";
   }
   m_t << "href=\"";
   m_t << externalRef(relPath, ref, true);
   if (!file.isEmpty()) {
      m_t << file << Doxygen::htmlFileExtension;
   }
   if (!anchor.isEmpty()) {
      m_t << "#" << anchor;
   }
   m_t << "\"";
   if (!tooltip.isEmpty()) {
      m_t << " title=\"" << substitute(tooltip, "\"", "&quot;") << "\"";
   }
   m_t << ">";
}

void HtmlDocVisitor::endLink()
{
   m_t << "</a>";
}

void HtmlDocVisitor::pushEnabled()
{
   m_enabled.push(new bool(m_hide));
}

void HtmlDocVisitor::popEnabled()
{
   bool v = m_enabled.pop();
   m_hide = v;   
}

void HtmlDocVisitor::writeDotFile(const QByteArray &fn, const QByteArray &relPath, const QByteArray &context)
{
   QByteArray baseName = fn;
   int i;

   if ((i = baseName.lastIndexOf('/')) != -1) {
      baseName = baseName.right(baseName.length() - i - 1);
   }

   if ((i = baseName.indexOf('.')) != -1) { // strip extension
      baseName = baseName.left(i);
   }

   baseName.prepend("dot_");
   QByteArray outDir = Config_getString("HTML_OUTPUT");
   writeDotGraphFromFile(fn, outDir, baseName, GOF_BITMAP);
   writeDotImageMapFromFile(m_t, fn, outDir, relPath, baseName, context);
}

void HtmlDocVisitor::writeMscFile(const QByteArray &fileName, const QByteArray &relPath, const QByteArray &context)
{
   QByteArray baseName = fileName;
   int i;
   if ((i = baseName.lastIndexOf('/')) != -1) { // strip path
      baseName = baseName.right(baseName.length() - i - 1);
   }

   if ((i = baseName.indexOf('.')) != -1) { // strip extension
      baseName = baseName.left(i);
   }
   baseName.prepend("msc_");
   QByteArray outDir = Config_getString("HTML_OUTPUT");
   QByteArray imgExt = Config_getEnum("DOT_IMAGE_FORMAT");

   MscOutputFormat mscFormat = MSC_BITMAP;

   if ("svg" == imgExt) {
      mscFormat = MSC_SVG;
   }

   writeMscGraphFromFile(fileName, outDir, baseName, mscFormat);
   writeMscImageMapFromFile(m_t, fileName, outDir, relPath, baseName, context, mscFormat);
}

void HtmlDocVisitor::writeDiaFile(const QByteArray &fileName, const QByteArray &relPath, const QByteArray &)
{
   QByteArray baseName = fileName;
   int i;

   if ((i = baseName.lastIndexOf('/')) != -1) { // strip path
      baseName = baseName.right(baseName.length() - i - 1);
   }

   if ((i = baseName.indexOf('.')) != -1) { // strip extension
      baseName = baseName.left(i);
   }

   baseName.prepend("dia_");
   QByteArray outDir = Config_getString("HTML_OUTPUT");
   writeDiaGraphFromFile(fileName, outDir, baseName, DIA_BITMAP);

   m_t << "<img src=\"" << relPath << baseName << ".png" << "\" />" << endl;
}

void HtmlDocVisitor::writePlantUMLFile(const QByteArray &fileName, const QByteArray &relPath, const QByteArray &)
{
   QByteArray baseName = fileName;
   int i;

   if ((i = baseName.lastIndexOf('/')) != -1) { // strip path
      baseName = baseName.right(baseName.length() - i - 1);
   }
   if ((i = baseName.lastIndexOf('.')) != -1) { // strip extension
      baseName = baseName.left(i);
   }

   static QByteArray outDir = Config_getString("HTML_OUTPUT");
   static QByteArray imgExt = Config_getEnum("DOT_IMAGE_FORMAT");
   if (imgExt == "svg") {
      generatePlantUMLOutput(fileName, outDir, PUML_SVG);
      //m_t << "<iframe scrolling=\"no\" frameborder=\"0\" src=\"" << relPath << baseName << ".svg" << "\" />" << endl;
      //m_t << "<p><b>This browser is not able to show SVG: try Firefox, Chrome, Safari, or Opera instead.</b></p>";
      //m_t << "</iframe>" << endl;
      m_t << "<object type=\"image/svg+xml\" data=\"" << relPath << baseName << ".svg\"></object>" << endl;
   } else {
      generatePlantUMLOutput(fileName, outDir, PUML_BITMAP);
      m_t << "<img src=\"" << relPath << baseName << ".png" << "\" />" << endl;
   }
}


/** Used for items found inside a paragraph, which due to XHTML restrictions
 *  have to be outside of the paragraph. This method will forcefully end
 *  the current paragraph and forceStartParagraph() will restart it.
 */
void HtmlDocVisitor::forceEndParagraph(DocNode *n)
{
   if (n->parent() && n->parent()->kind() == DocNode::Kind_Para) {

      DocPara *para = (DocPara *)n->parent();
      int nodeIndex = para->children().indexOf(n);

      nodeIndex--;
      if (nodeIndex < 0) {
         // first node
         return;   
      }

      while (nodeIndex >= 0 && para->children().at(nodeIndex)->kind() == DocNode::Kind_WhiteSpace) {
         nodeIndex--;
      }

      if (nodeIndex >= 0) {
         DocNode *n = para->children().at(nodeIndex);
        
         if (mustBeOutsideParagraph(n)) {
            return;
         }
      }

      bool isFirst;
      bool isLast;

      getParagraphContext(para, isFirst, isLast);
      
      if (isFirst && isLast) {
         return;
      }

      m_t << "</p>";
   }
}

/** Used for items found inside a paragraph, which due to XHTML restrictions
 *  have to be outside of the paragraph. This method will forcefully start
 *  the paragraph, that was previously ended by forceEndParagraph().
 */
void HtmlDocVisitor::forceStartParagraph(DocNode *n)
{  
   if (n->parent() && n->parent()->kind() == DocNode::Kind_Para) { 
      // if we are inside a paragraph

      DocPara *para = (DocPara *)n->parent();
      int nodeIndex = para->children().indexOf(n);
      int numNodes  = para->children().count();
      nodeIndex++;

      if (nodeIndex == numNodes) {
         return;   // last node
      }

      while (nodeIndex < numNodes && para->children().at(nodeIndex)->kind() == DocNode::Kind_WhiteSpace) {
         nodeIndex++;
      }

      if (nodeIndex < numNodes) {
         DocNode *n = para->children().at(nodeIndex);

         if (mustBeOutsideParagraph(n)) {
            return;
         }

      } else {
         return; // only whitespace at the end!
      }

      bool isFirst;
      bool isLast;

      getParagraphContext(para, isFirst, isLast);

      if (isFirst && isLast) {
         return;
      }

      m_t << "<p>";
   }
}

