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

#include <QHash>
#include <QFileInfo>
#include <QList>
#include <QStack>
#include <QString>
#include <QStringList>
#include <QXmlAttributes>
#include <QXmlDefaultHandler>
#include <QXmlErrorHandler>
#include <QXmlInputSource>
#include <QXmlLocator>
#include <QXmlParseException>
#include <QXmlSimpleReader>

#include <stdio.h>
#include <stdarg.h>

#include <tagreader.h>

#include <arguments.h>
#include <default_args.h>
#include <doxy_globals.h>
#include <entry.h>
#include <message.h>
#include <util.h>

/** Information about an linkable anchor */
class TagAnchorInfo
{
 public:
   TagAnchorInfo(const QString &f, const QString &l, const QString &t = QString())
      : label(l), fileName(f), title(t)
   {}

   QString label;
   QString fileName;
   QString title;
};

/** List of TagAnchorInfo objects. */
class TagAnchorInfoList : public QList<TagAnchorInfo>
{
 public:
   TagAnchorInfoList() : QList<TagAnchorInfo>() {}
   virtual ~TagAnchorInfoList() {}
};

/** Container for enum values that are scoped within an enum */
class TagEnumValueInfo
{
 public:
   QString name;
   QString file;
   QString anchor;
   QString clangid;
};

/** Container for member specific info that can be read from a tagfile */
class TagMemberInfo
{
 public:
   TagMemberInfo() : prot(Public), virt(Normal), isStatic(false)
   {
   }

   QString type;
   QString name;
   QString anchorFile;
   QString anchor;
   QString arglist;
   QString kind;
   QString clangId;

   TagAnchorInfoList docAnchors;
   Protection prot;
   Specifier virt;

   bool isStatic;

   QList<TagEnumValueInfo> enumValues;
};

/** Container for class specific info that can be read from a tagfile */
class TagClassInfo
{
 public:
   enum Kind { Class, Struct, Union, Interface, Exception, Protocol, Category, Enum, Service, Singleton };

   TagClassInfo() {
      bases = 0;
      templateArguments = 0;
      isObjC = false;
   }

   ~TagClassInfo() {
      delete bases;
      delete templateArguments;
   }

   QString name;
   QString filename;
   QString clangId;

   TagAnchorInfoList      docAnchors;
   QVector<BaseInfo>     *bases;
   QList<TagMemberInfo>   members;
   QList<QString>        *templateArguments;

   QStringList classList;
   Kind kind;
   bool isObjC;
};

/** Container for namespace specific info that can be read from a tagfile */
class TagNamespaceInfo
{
 public:
   TagNamespaceInfo() { }

   QString name;
   QString filename;
   QString clangId;
   QStringList classList;
   QStringList namespaceList;

   TagAnchorInfoList docAnchors;

   QList<TagMemberInfo> members;
};

/** Container for package specific info that can be read from a tagfile */
class TagPackageInfo
{
 public:
   TagPackageInfo()
   {
   }

   QString name;
   QString filename;

   TagAnchorInfoList docAnchors;
   QList<TagMemberInfo> members;

   QStringList classList;
};

/** Container for include info that can be read from a tagfile */
class TagIncludeInfo
{
 public:
   QString id;
   QString name;
   QString text;

   bool isLocal;
   bool isImported;
};

/** Container for file specific info that can be read from a tagfile */
class TagFileInfo
{
 public:
   TagFileInfo()
   {
   }

   QString name;
   QString path;
   QString filename;

   TagAnchorInfoList docAnchors;

   QList<TagMemberInfo> members;

   QStringList classList;
   QStringList namespaceList;

   QList<TagIncludeInfo> includes;
};

/** Container for group specific info that can be read from a tagfile */
class TagGroupInfo
{
 public:
   TagGroupInfo()
   {
   }

   QString name;
   QString title;
   QString filename;

   TagAnchorInfoList docAnchors;

   QList<TagMemberInfo> members;

   QStringList subgroupList;
   QStringList classList;
   QStringList namespaceList;
   QStringList fileList;
   QStringList pageList;
   QStringList dirList;
};

/** Container for page specific info that can be read from a tagfile */
class TagPageInfo
{
 public:
   QString name;
   QString title;
   QString filename;

   TagAnchorInfoList docAnchors;
};

/** Container for directory specific info that can be read from a tagfile */
class TagDirInfo
{
 public:
   QString name;
   QString filename;
   QString path;
   QStringList subdirList;
   QStringList fileList;
   TagAnchorInfoList docAnchors;
};

/** Tag file parser.
 *
 *  Reads an XML-structured tagfile and builds up the structure in
 *  memory. The method buildLists() is used to transfer/translate
 *  the structures to the DoxyPress engine.
 */
class TagFileParser : public QXmlDefaultHandler
{
   enum State { Invalid,
                InClass,
                InFile,
                InNamespace,
                InGroup,
                InPage,
                InMember,
                InEnumValue,
                InPackage,
                InDir,
                InTempArgList
              };

   class StartElementHandler
   {
      typedef void (TagFileParser::*Handler)   (const QXmlAttributes &attrib);

      public:
         StartElementHandler(TagFileParser *parent, Handler h) : m_parent(parent), m_handler(h)
         {
         }

         void operator()(const QXmlAttributes &attrib) {
            (m_parent->*m_handler)(attrib);
         }

       private:
         TagFileParser *m_parent;
         Handler m_handler;
   };

   class EndElementHandler
   {
      typedef void (TagFileParser::*Handler)();

    public:
      EndElementHandler(TagFileParser *parent, Handler h) : m_parent(parent), m_handler(h) {}
      void operator()() {
         (m_parent->*m_handler)();
      }

    private:
      TagFileParser *m_parent;
      Handler m_handler;
   };

 public:
   TagFileParser(const QString &tagName) : m_startElementHandlers(), m_endElementHandlers(), m_tagName(tagName) {
      m_curClass = 0;
      m_curFile = 0;
      m_curNamespace = 0;
      m_curPackage = 0;
      m_curGroup = 0;
      m_curPage = 0;
      m_curDir = 0;
      m_curMember = 0;
      m_curEnumValue = 0;
      m_curIncludes = 0;
      m_state = Invalid;
      m_locator = 0;
   }

   void setDocumentLocator ( QXmlLocator *locator ) override {
      m_locator = locator;
   }

   void setFileName( const QString &fileName ) {
      m_inputFileName = fileName;
   }

   void warn(const char *fmt) {
      ::warn(m_inputFileName, m_locator->lineNumber(), fmt);
   }

   void warn(const char *fmt, const char *s) {
      ::warn(m_inputFileName, m_locator->lineNumber(), fmt, s);
   }

   void startCompound( const QXmlAttributes &attrib ) {
      m_curString = "";

      QString kind   = attrib.value("kind");
      QString isObjC = attrib.value("objc");

      if (kind == "class") {
         m_curClass = new TagClassInfo;
         m_curClass->kind = TagClassInfo::Class;
         m_state = InClass;
      } else if (kind == "struct") {
         m_curClass = new TagClassInfo;
         m_curClass->kind = TagClassInfo::Struct;
         m_state = InClass;
      } else if (kind == "union") {
         m_curClass = new TagClassInfo;
         m_curClass->kind = TagClassInfo::Union;
         m_state = InClass;
      } else if (kind == "interface") {
         m_curClass = new TagClassInfo;
         m_curClass->kind = TagClassInfo::Interface;
         m_state = InClass;
      } else if (kind == "enum") {
         m_curClass = new TagClassInfo;
         m_curClass->kind = TagClassInfo::Enum;
         m_state = InClass;
      } else if (kind == "exception") {
         m_curClass = new TagClassInfo;
         m_curClass->kind = TagClassInfo::Exception;
         m_state = InClass;
      } else if (kind == "protocol") {
         m_curClass = new TagClassInfo;
         m_curClass->kind = TagClassInfo::Protocol;
         m_state = InClass;
      } else if (kind == "category") {
         m_curClass = new TagClassInfo;
         m_curClass->kind = TagClassInfo::Category;
         m_state = InClass;
      } else if (kind == "service") {
         m_curClass = new TagClassInfo;
         m_curClass->kind = TagClassInfo::Service;
         m_state = InClass;

      } else if (kind == "singleton") {
         m_curClass = new TagClassInfo;
         m_curClass->kind = TagClassInfo::Singleton;
         m_state = InClass;

      } else if (kind == "file") {
         m_curFile = new TagFileInfo;
         m_state = InFile;

      } else if (kind == "namespace") {
         m_curNamespace = new TagNamespaceInfo;
         m_state = InNamespace;

      } else if (kind == "group") {
         m_curGroup = new TagGroupInfo;
         m_state = InGroup;

      } else if (kind == "page") {
         m_curPage = new TagPageInfo;
         m_state = InPage;

      } else if (kind == "package") {
         m_curPackage = new TagPackageInfo;
         m_state = InPackage;

      } else if (kind == "dir") {
         m_curDir = new TagDirInfo;
         m_state = InDir;

      } else {
         warn("Unknown compound attribute `%s' found\n", qPrintable(kind) );
         m_state = Invalid;
      }

      if (isObjC == "yes" && m_curClass) {
         m_curClass->isObjC = true;
      }
   }

   void endCompound() {
      switch (m_state) {
         case InClass:
            m_tagFileClasses.append(*m_curClass);
            m_curClass = 0;
            break;

         case InFile:
            m_tagFileFiles.append(*m_curFile);
            m_curFile = 0;
            break;

         case InNamespace:
            m_tagFileNamespaces.append(*m_curNamespace);
            m_curNamespace = 0;
            break;

         case InGroup:
            m_tagFileGroups.append(*m_curGroup);
            m_curGroup = 0;
            break;

         case InPage:
            m_tagFilePages.append(*m_curPage);
            m_curPage = 0;
            break;

         case InDir:
            m_tagFileDirs.append(*m_curDir);
            m_curDir = 0;
            break;

         case InPackage:
            m_tagFilePackages.append(*m_curPackage);
            m_curPackage = 0;
            break;

         default:
            warn("Tag `compound' was not expected\n");
      }
   }

   void startMember( const QXmlAttributes &attrib) {
      m_curMember = new TagMemberInfo;
      m_curMember->kind = attrib.value("kind");

      QString protStr   = attrib.value("protection");
      QString virtStr   = attrib.value("virtualness");
      QString staticStr = attrib.value("static");

      if (protStr == "protected") {
         m_curMember->prot = Protected;

      } else if (protStr == "private") {
         m_curMember->prot = Private;

      }

      if (virtStr == "virtual") {
         m_curMember->virt = Virtual;

      } else if (virtStr == "pure") {
         m_curMember->virt = Pure;

      }

      if (staticStr == "yes") {
         m_curMember->isStatic = true;
      }

      m_stateStack.push(m_state);
      m_state = InMember;
   }

   void endMember() {
      m_state = m_stateStack.pop();

      switch (m_state) {
         case InClass:
            m_curClass->members.append(*m_curMember);
            break;

         case InFile:
            m_curFile->members.append(*m_curMember);
            break;

         case InNamespace:
            m_curNamespace->members.append(*m_curMember);
            break;

         case InGroup:
            m_curGroup->members.append(*m_curMember);
            break;

         case InPackage:
            m_curPackage->members.append(*m_curMember);
            break;

         default:
            warn("Unexpected tag `member' found\n");
            break;
      }
   }

   void startEnumValue( const QXmlAttributes &attrib) {
      if (m_state == InMember) {
         m_curString = "";
         m_curEnumValue = new TagEnumValueInfo;
         m_curEnumValue->file    = attrib.value("file");
         m_curEnumValue->anchor  = attrib.value("anchor");
         m_curEnumValue->clangid = attrib.value("clangid");

         m_stateStack.push(m_state);
         m_state = InEnumValue;

      } else {
         warn("Found enumvalue tag outside of member tag\n");
      }
   }

   void endEnumValue() {
      m_curEnumValue->name = m_curString.trimmed();

      m_state = m_stateStack.pop();

      if (m_state == InMember) {
         m_curMember->enumValues.append(*m_curEnumValue);
         m_curEnumValue = 0;
      }
   }

   void endDocAnchor() {
      switch (m_state) {
         case InClass:
            m_curClass->docAnchors.append(TagAnchorInfo(m_fileName, m_curString, m_title));
            break;

         case InFile:
            m_curFile->docAnchors.append(TagAnchorInfo(m_fileName, m_curString, m_title));
            break;

         case InNamespace:
            m_curNamespace->docAnchors.append(TagAnchorInfo(m_fileName, m_curString, m_title));
            break;

         case InGroup:
            m_curGroup->docAnchors.append(TagAnchorInfo(m_fileName, m_curString, m_title));
            break;

         case InPage:
            m_curPage->docAnchors.append(TagAnchorInfo(m_fileName, m_curString, m_title));
            break;

         case InMember:
            m_curMember->docAnchors.append(TagAnchorInfo(m_fileName, m_curString, m_title));
            break;

         case InPackage:
            m_curPackage->docAnchors.append(TagAnchorInfo(m_fileName, m_curString, m_title));
            break;

         case InDir:
            m_curDir->docAnchors.append(TagAnchorInfo(m_fileName, m_curString, m_title));
            break;

         default:
            warn("Unexpected tag member found\n");
            break;
      }
   }

   void endClass() {
      switch (m_state) {
         case InClass:
            m_curClass->classList.append(m_curString);
            break;
         case InFile:
            m_curFile->classList.append(m_curString);
            break;
         case InNamespace:
            m_curNamespace->classList.append(m_curString);
            break;
         case InGroup:
            m_curGroup->classList.append(m_curString);
            break;
         case InPackage:
            m_curPackage->classList.append(m_curString);
            break;
         default:
            warn("Unexpected tag `class' found\n");
            break;
      }
   }

   void endNamespace() {
      switch (m_state) {
         case InNamespace:
            m_curNamespace->classList.append(m_curString);
            break;
         case InFile:
            m_curFile->namespaceList.append(m_curString);
            break;
         case InGroup:
            m_curGroup->namespaceList.append(m_curString);
            break;
         default:
            warn("Unexpected tag `namespace' found\n");
            break;
      }
   }

   void endFile() {
      switch (m_state) {
         case InGroup:
            m_curGroup->fileList.append(m_curString);
            break;
         case InDir:
            m_curDir->fileList.append(m_curString);
            break;
         default:
            warn("Unexpected tag `file' found\n");
            break;
      }
   }

   void endPage() {
      switch (m_state) {
         case InGroup:
            m_curGroup->fileList.append(m_curString);
            break;
         default:
            warn("Unexpected tag `page' found\n");
            break;
      }
   }

   void endDir() {
      switch (m_state) {
         case InDir:
            m_curDir->subdirList.append(m_curString);
            break;
         default:
            warn("Unexpected tag `page' found\n");
            break;
      }
   }

   void startStringValue(const QXmlAttributes &) {
      m_curString = "";
   }

   void startDocAnchor(const QXmlAttributes &attrib ) {
      m_fileName = attrib.value("file");
      m_title = attrib.value("title");
      m_curString = "";
   }

   void endType() {
      if (m_state == InMember) {
         m_curMember->type = m_curString;
      } else {
         warn("Unexpected tag `type' found\n");
      }
   }

   void endName() {
      switch (m_state) {
         case InClass:
            m_curClass->name     = m_curString;
            break;
         case InFile:
            m_curFile->name      = m_curString;
            break;
         case InNamespace:
            m_curNamespace->name = m_curString;
            break;
         case InGroup:
            m_curGroup->name     = m_curString;
            break;
         case InPage:
            m_curPage->name      = m_curString;
            break;
         case InDir:
            m_curDir->name       = m_curString;
            break;
         case InMember:
            m_curMember->name    = m_curString;
            break;
         case InPackage:
            m_curPackage->name   = m_curString;
            break;
         default:
            warn("Unexpected tag `name' found\n");
            break;
      }
   }

   void startBase(const QXmlAttributes &attrib ) {
      m_curString = "";

      if (m_state == InClass && m_curClass) {
         QString protStr = attrib.value("protection");
         QString virtStr = attrib.value("virtualness");

         Protection prot = Public;
         Specifier  virt = Normal;

         if (protStr == "protected") {
            prot = Protected;

         } else if (protStr == "private") {
            prot = Private;

         }

         if (virtStr == "virtual") {
            virt = Virtual;
         }

         if (m_curClass->bases == 0) {
            m_curClass->bases = new QVector<BaseInfo>;
         }

         m_curClass->bases->append(BaseInfo(m_curString, prot, virt));

      } else {
         warn("Unexpected tag `base' found\n");
      }
   }

   void endBase() {
      if (m_state == InClass && m_curClass) {
         m_curClass->bases->last().name = m_curString;

      } else {
         warn("Unexpected tag `base' found\n");
      }
   }

   void startIncludes(const QXmlAttributes &attrib ) {
      if (m_state == InFile && m_curFile) {

         m_curIncludes = new TagIncludeInfo;
         m_curIncludes->id = attrib.value("id");
         m_curIncludes->name = attrib.value("name");
         m_curIncludes->isLocal = attrib.value("local") == "yes" ? true : false;
         m_curIncludes->isImported = attrib.value("imported") == "yes" ? true : false;

         m_curFile->includes.append(* m_curIncludes);

      } else {
         warn("Unexpected tag `includes' found\n");
      }

      m_curString = "";
   }

   void endIncludes() {
      m_curIncludes->text = m_curString;
   }

   void endTemplateArg() {
      if (m_state == InClass && m_curClass) {
         if (m_curClass->templateArguments == 0) {
            m_curClass->templateArguments = new QList<QString>;
         }

         m_curClass->templateArguments->append(m_curString);

      } else {
         warn("Unexpected tag templarg found\n");
      }
   }

   void endFilename() {
      switch (m_state) {
         case InClass:
            m_curClass->filename = m_curString;
            break;
         case InNamespace:
            m_curNamespace->filename = m_curString;
            break;
         case InFile:
            m_curFile->filename      = m_curString;
            break;
         case InGroup:
            m_curGroup->filename     = m_curString;
            break;
         case InPage:
            m_curPage->filename      = m_curString;
            break;
         case InPackage:
            m_curPackage->filename   = m_curString;
            break;
         case InDir:
            m_curDir->filename       = m_curString;
            break;
         default:
            warn("Unexpected tag `filename' found\n");
            break;
      }
   }

   void endPath() {
      switch (m_state) {
         case InFile:
            m_curFile->path          = m_curString;
            break;
         case InDir:
            m_curDir->path           = m_curString;
            break;
         default:
            warn("Unexpected tag `path' found\n");
            break;
      }
   }

   void endAnchor() {
      if (m_state == InMember) {
         m_curMember->anchor = m_curString;
      } else {
         warn("Unexpected tag `anchor' found\n");
      }
   }

   void endClangId() {
      if (m_state == InMember) {
         m_curMember->clangId = m_curString;
      } else if (m_state == InClass) {
         m_curClass->clangId =  m_curString;
      } else if (m_state == InNamespace) {
         m_curNamespace->clangId = m_curString;
      } else {
         warn("warning: Unexpected tag `anchor' found\n");
      }
   }



   void endAnchorFile() {
      if (m_state == InMember) {
         m_curMember->anchorFile = m_curString;
      } else {
         warn("Unexpected tag `anchorfile' found\n");
      }
   }

   void endArglist() {
      if (m_state == InMember) {
         m_curMember->arglist = m_curString;
      } else {
         warn("Unexpected tag `arglist' found\n");
      }
   }
   void endTitle() {
      switch (m_state) {
         case InGroup:
            m_curGroup->title     = m_curString;
            break;
         case InPage:
            m_curPage->title      = m_curString;
            break;
         default:
            warn("Unexpected tag `title' found\n");
            break;
      }
   }

   void endSubgroup() {
      if (m_state == InGroup) {
         m_curGroup->subgroupList.append(m_curString);
      } else {
         warn("Unexpected tag `subgroup' found\n");
      }
   }

   void startIgnoreElement(const QXmlAttributes &) {
   }

   void endIgnoreElement() {
   }

   bool startDocument() override {
      m_state = Invalid;

      m_curClass = 0;
      m_curNamespace = 0;
      m_curFile = 0;
      m_curGroup = 0;
      m_curPage = 0;
      m_curPackage = 0;
      m_curDir = 0;

      m_startElementHandlers.insert("compound",    StartElementHandler(this, &TagFileParser::startCompound));
      m_startElementHandlers.insert("member",      StartElementHandler(this, &TagFileParser::startMember));
      m_startElementHandlers.insert("enumvalue",   StartElementHandler(this, &TagFileParser::startEnumValue));
      m_startElementHandlers.insert("name",        StartElementHandler(this, &TagFileParser::startStringValue));
      m_startElementHandlers.insert("base",        StartElementHandler(this, &TagFileParser::startBase));
      m_startElementHandlers.insert("filename",    StartElementHandler(this, &TagFileParser::startStringValue));
      m_startElementHandlers.insert("includes",    StartElementHandler(this, &TagFileParser::startIncludes));
      m_startElementHandlers.insert("path",        StartElementHandler(this, &TagFileParser::startStringValue));
      m_startElementHandlers.insert("anchorfile",  StartElementHandler(this, &TagFileParser::startStringValue));
      m_startElementHandlers.insert("anchor",      StartElementHandler(this, &TagFileParser::startStringValue));
      m_startElementHandlers.insert("clangid",     StartElementHandler(this, &TagFileParser::startStringValue));
      m_startElementHandlers.insert("arglist",     StartElementHandler(this, &TagFileParser::startStringValue));
      m_startElementHandlers.insert("title",       StartElementHandler(this, &TagFileParser::startStringValue));
      m_startElementHandlers.insert("subgroup",    StartElementHandler(this, &TagFileParser::startStringValue));
      m_startElementHandlers.insert("class",       StartElementHandler(this, &TagFileParser::startStringValue));
      m_startElementHandlers.insert("namespace",   StartElementHandler(this, &TagFileParser::startStringValue));
      m_startElementHandlers.insert("file",        StartElementHandler(this, &TagFileParser::startStringValue));
      m_startElementHandlers.insert("dir",         StartElementHandler(this, &TagFileParser::startStringValue));
      m_startElementHandlers.insert("page",        StartElementHandler(this, &TagFileParser::startStringValue));
      m_startElementHandlers.insert("docanchor",   StartElementHandler(this, &TagFileParser::startDocAnchor));
      m_startElementHandlers.insert("tagfile",     StartElementHandler(this, &TagFileParser::startIgnoreElement));
      m_startElementHandlers.insert("templarg",    StartElementHandler(this, &TagFileParser::startStringValue));
      m_startElementHandlers.insert("type",        StartElementHandler(this, &TagFileParser::startStringValue));

      m_endElementHandlers.insert("compound",      EndElementHandler(this, &TagFileParser::endCompound));
      m_endElementHandlers.insert("member",        EndElementHandler(this, &TagFileParser::endMember));
      m_endElementHandlers.insert("enumvalue",     EndElementHandler(this, &TagFileParser::endEnumValue));
      m_endElementHandlers.insert("name",          EndElementHandler(this, &TagFileParser::endName));
      m_endElementHandlers.insert("base",          EndElementHandler(this, &TagFileParser::endBase));
      m_endElementHandlers.insert("filename",      EndElementHandler(this, &TagFileParser::endFilename));
      m_endElementHandlers.insert("includes",      EndElementHandler(this, &TagFileParser::endIncludes));
      m_endElementHandlers.insert("path",          EndElementHandler(this, &TagFileParser::endPath));
      m_endElementHandlers.insert("anchorfile",    EndElementHandler(this, &TagFileParser::endAnchorFile));
      m_endElementHandlers.insert("anchor",        EndElementHandler(this, &TagFileParser::endAnchor));
      m_endElementHandlers.insert("clangid",       EndElementHandler(this, &TagFileParser::endClangId));
      m_endElementHandlers.insert("arglist",       EndElementHandler(this, &TagFileParser::endArglist));
      m_endElementHandlers.insert("title",         EndElementHandler(this, &TagFileParser::endTitle));
      m_endElementHandlers.insert("subgroup",      EndElementHandler(this, &TagFileParser::endSubgroup));
      m_endElementHandlers.insert("class"   ,      EndElementHandler(this, &TagFileParser::endClass));
      m_endElementHandlers.insert("namespace",     EndElementHandler(this, &TagFileParser::endNamespace));
      m_endElementHandlers.insert("file",          EndElementHandler(this, &TagFileParser::endFile));
      m_endElementHandlers.insert("dir",           EndElementHandler(this, &TagFileParser::endDir));
      m_endElementHandlers.insert("page",          EndElementHandler(this, &TagFileParser::endPage));
      m_endElementHandlers.insert("docanchor",     EndElementHandler(this, &TagFileParser::endDocAnchor));
      m_endElementHandlers.insert("tagfile",       EndElementHandler(this, &TagFileParser::endIgnoreElement));
      m_endElementHandlers.insert("templarg",      EndElementHandler(this, &TagFileParser::endTemplateArg));
      m_endElementHandlers.insert("type",          EndElementHandler(this, &TagFileParser::endType));

      return true;
   }

   bool startElement(const QString &, const QString &, const QString &name, const QXmlAttributes &attrib) override {

      auto iter = m_startElementHandlers.find(name);

      if (iter != m_startElementHandlers.end())  {

         StartElementHandler &handler = *iter;
         handler(attrib);

      } else {
         warn("Unknown tag `%s' found\n", qPrintable(name));

      }

      return true;
   }

   bool endElement( const QString &, const QString &, const QString &name ) override {

      auto iter = m_endElementHandlers.find(name);

      if (iter != m_endElementHandlers.end()) {

         EndElementHandler &handler = *iter;
         handler();

      } else {
         warn("Unknown tag `%s' found\n", qPrintable(name));

      }

      return true;
   }

   bool characters (const QString &ch) override {
      m_curString += ch;
      return true;
   }

   void dump();
   void buildLists(QSharedPointer<Entry> root);
   void addIncludes();

 private:
   void buildMemberList(QSharedPointer<Entry> ce, QList<TagMemberInfo> &members);
   void addDocAnchors(QSharedPointer<Entry> e, const TagAnchorInfoList &l);

   QList<TagClassInfo>        m_tagFileClasses;
   QList<TagFileInfo>         m_tagFileFiles;
   QList<TagNamespaceInfo>    m_tagFileNamespaces;
   QList<TagGroupInfo>        m_tagFileGroups;
   QList<TagPageInfo>         m_tagFilePages;
   QList<TagPackageInfo>      m_tagFilePackages;
   QList<TagDirInfo>          m_tagFileDirs;

   QHash<QString, StartElementHandler>  m_startElementHandlers;
   QHash<QString, EndElementHandler>    m_endElementHandlers;

   TagClassInfo          *m_curClass;
   TagFileInfo           *m_curFile;
   TagNamespaceInfo      *m_curNamespace;
   TagPackageInfo        *m_curPackage;
   TagGroupInfo          *m_curGroup;
   TagPageInfo           *m_curPage;
   TagDirInfo            *m_curDir;
   TagMemberInfo         *m_curMember;
   TagEnumValueInfo      *m_curEnumValue;
   TagIncludeInfo        *m_curIncludes;

   QString               m_curString;
   QString               m_tagName;
   QString               m_fileName;
   QString               m_title;
   State                 m_state;

   QStack<State>         m_stateStack;

   QXmlLocator           *m_locator;
   QString               m_inputFileName;
};

/** Error handler for the XML tag file parser.
 *
 *  Basically dumps all fatal error to stderr using err().
 */
class TagFileErrorHandler : public QXmlErrorHandler
{
 public:
   virtual ~TagFileErrorHandler() {}

   bool warning( const QXmlParseException &) override {
      return false;
   }

   bool error( const QXmlParseException &) override {
      return false;
   }

   bool fatalError( const QXmlParseException &exception ) override {
      err("Error at line %d column %d: %s\n",
          exception.lineNumber(), exception.columnNumber(), csPrintable(exception.message()) );
      return false;
   }

   QString errorString() const override {
      return "";
   }

 private:
   QString errorMsg;
};

/*! Dumps the internal structures. For debugging only */
void TagFileParser::dump()
{
   msg("Result:\n");

   //  classes
   for (auto cd : m_tagFileClasses) {
      msg("class `%s'\n",      csPrintable(cd.name));
      msg("  filename `%s'\n", csPrintable(cd.filename));

      if (cd.bases) {
         for (auto bi : *cd.bases) {
            msg( "  base: %s \n", csPrintable(bi.name) );
         }
      }

      for (auto md : cd.members) {
         msg("  member:\n");
         msg("    kind: `%s'\n",    csPrintable(md.kind));
         msg("    name: `%s'\n",    csPrintable(md.name));
         msg("    anchor: `%s'\n",  csPrintable(md.anchor));
         msg("    arglist: `%s'\n", csPrintable(md.arglist));
      }
   }

   //  NAMESPACES
   for (auto nd : m_tagFileNamespaces) {
      msg("namespace `%s'\n",  csPrintable(nd.name));
      msg("  filename `%s'\n", csPrintable(nd.filename));

      QStringList::Iterator it;
      for ( it = nd.classList.begin(); it != nd.classList.end(); ++it ) {
         msg( "  class: %s \n", qPrintable((*it)) );
      }

      for (auto md : nd.members) {
         msg("  member:\n");
         msg("    kind: `%s'\n",    csPrintable(md.kind));
         msg("    name: `%s'\n",    csPrintable(md.name));
         msg("    anchor: `%s'\n",  csPrintable(md.anchor));
         msg("    arglist: `%s'\n", csPrintable(md.arglist));
      }
   }

   //  FILES
   for (auto fd : m_tagFileFiles) {
      msg("file `%s'\n",       csPrintable(fd.name));
      msg("  filename `%s'\n", csPrintable(fd.filename));

      QStringList::Iterator it;
      for ( it = fd.namespaceList.begin(); it != fd.namespaceList.end(); ++it ) {
         msg( "  namespace: %s \n", qPrintable((*it)) );
      }

      for ( it = fd.classList.begin(); it != fd.classList.end(); ++it ) {
         msg( "  class: %s \n", qPrintable((*it)) );
      }

      for (auto md : fd.members) {
         msg("  member:\n");
         msg("    kind: `%s'\n",    csPrintable(md.kind));
         msg("    name: `%s'\n",    csPrintable(md.name));
         msg("    anchor: `%s'\n",  csPrintable( md.anchor));
         msg("    arglist: `%s'\n", csPrintable(md.arglist));
      }

      for (auto item : fd.includes) {
         msg("  includes id: %s name: %s\n", csPrintable(item.id), csPrintable(item.name));
      }
   }

   //  GROUPS
   for (auto gd : m_tagFileGroups) {
      msg("group `%s'\n",      csPrintable(gd.name));
      msg("  filename `%s'\n", csPrintable(gd.filename));

      QStringList::Iterator it;
      for ( it = gd.namespaceList.begin(); it != gd.namespaceList.end(); ++it ) {
         msg( "  namespace: %s \n", qPrintable((*it)) );
      }

      for ( it = gd.classList.begin(); it != gd.classList.end(); ++it ) {
         msg( "  class: %s \n", qPrintable((*it)) );
      }

      for ( it = gd.fileList.begin(); it != gd.fileList.end(); ++it ) {
         msg( "  file: %s \n", qPrintable((*it)) );
      }

      for ( it = gd.subgroupList.begin(); it != gd.subgroupList.end(); ++it ) {
         msg( "  subgroup: %s \n",qPrintable((*it)) );
      }
      for ( it = gd.pageList.begin(); it != gd.pageList.end(); ++it ) {
         msg( "  page: %s \n", qPrintable((*it)) );
      }

      for (auto md : gd.members) {
         msg("  member:\n");
         msg("    kind: `%s'\n",    csPrintable(md.kind));
         msg("    name: `%s'\n",    csPrintable(md.name));
         msg("    anchor: `%s'\n",  csPrintable(md.anchor));
         msg("    arglist: `%s'\n", csPrintable(md.arglist));
      }
   }

   //  PAGES
   for (auto pd : m_tagFilePages) {
      msg("page `%s'\n",       csPrintable(pd.name));
      msg("  title `%s'\n",    csPrintable(pd.title));
      msg("  filename `%s'\n", csPrintable(pd.filename));
   }

   //  DIRS
   for (auto dd : m_tagFileDirs) {
      msg("dir `%s'\n",    csPrintable(dd.name));
      msg("  path `%s'\n", csPrintable(dd.path));

      QStringList::Iterator it;
      for ( it = dd.fileList.begin(); it != dd.fileList.end(); ++it ) {
         msg( "  file: %s \n", qPrintable((*it)) );
      }

      for ( it = dd.subdirList.begin(); it != dd.subdirList.end(); ++it ) {
         msg( "  subdir: %s \n", qPrintable((*it)) );
      }
   }
}

void TagFileParser::addDocAnchors(QSharedPointer<Entry> e, const TagAnchorInfoList &list)
{
   for (auto ta : list) {
      if (Doxy_Globals::sectionDict.find(ta.label) == 0) {

         QSharedPointer<SectionInfo> si (new SectionInfo(ta.fileName, -1, ta.label, ta.title, SectionInfo::Anchor, 0, m_tagName));
         Doxy_Globals::sectionDict.insert(ta.label, si);

         e->m_anchors.append(*si);

      } else {
         warn("Duplicate anchor %s found\n", qPrintable(ta.label));

      }
   }
}

void TagFileParser::buildMemberList(QSharedPointer<Entry> ce, QList<TagMemberInfo> &members)
{
   for (auto tmi : members) {
      QSharedPointer<Entry> me = QMakeShared<Entry>();

      me->type  = tmi.type;
      me->name  = tmi.name;
      me->args  = tmi.arglist;

      if (! me->args.isEmpty()) {        
         me->argList = stringToArgumentList(me->args);
      }

      if (tmi.enumValues.count() > 0) {
         me->m_traits.setTrait(Entry::Virtue::Strong);

         for (auto evi : tmi.enumValues) {
            QSharedPointer<Entry> ev = QMakeShared<Entry>();

            ev->type       = "@";
            ev->name       = evi.name;
            ev->id         = evi.clangid;
            ev->section    = Entry::VARIABLE_SEC;

            TagInfo ti;
            ti.tagName     = m_tagName;
            ti.anchor      = evi.anchor;
            ti.fileName    = evi.file;

            ev->m_tagInfoEntry = ti;
            me->addSubEntry(ev, me);
         }
      }

      me->protection = tmi.prot;
      me->virt       = tmi.virt;
      me->stat       = tmi.isStatic;
      me->fileName   = ce->fileName;
      me->id         = tmi.clangId;

      if (ce->section == Entry::GROUPDOC_SEC) {
         me->m_groups.append(Grouping(ce->name, Grouping::GROUPING_INGROUP));
      }

      addDocAnchors(me, tmi.docAnchors);

      TagInfo ti;
      ti.tagName    = m_tagName;
      ti.anchor     = tmi.anchor;
      ti.fileName   = tmi.anchorFile;

      me->m_tagInfoEntry = ti;

      if (tmi.kind == "define") {
         me->type = "#define";
         me->section = Entry::DEFINE_SEC;

      } else if (tmi.kind == "enumvalue") {
         me->section = Entry::VARIABLE_SEC;
         me->mtype = Method;

      } else if (tmi.kind == "property") {
         me->section = Entry::VARIABLE_SEC;
         me->mtype = Property;

      } else if (tmi.kind == "event") {
         me->section = Entry::VARIABLE_SEC;
         me->mtype = Event;

      } else if (tmi.kind == "variable") {
         me->section = Entry::VARIABLE_SEC;
         me->mtype = Method;

      } else if (tmi.kind == "typedef") {
         me->section = Entry::VARIABLE_SEC; //Entry::TYPEDEF_SEC;
         me->type.prepend("typedef ");
         me->mtype = Method;

      } else if (tmi.kind == "enumeration") {
         me->section = Entry::ENUM_SEC;
         me->mtype = Method;

      } else if (tmi.kind == "function") {
         me->section = Entry::FUNCTION_SEC;
         me->mtype = Method;

      } else if (tmi.kind == "signal") {
         me->section = Entry::FUNCTION_SEC;
         me->mtype = Signal;

      } else if (tmi.kind == "prototype") {
         me->section = Entry::FUNCTION_SEC;
         me->mtype = Method;

      } else if (tmi.kind == "friend") {
         me->section = Entry::FUNCTION_SEC;
         me->type.prepend("friend ");
         me->mtype = Method;

      } else if (tmi.kind == "dcop") {
         me->section = Entry::FUNCTION_SEC;
         me->mtype = DCOP;

      } else if (tmi.kind == "slot") {
         me->section = Entry::FUNCTION_SEC;
         me->mtype = Slot;
      }

      ce->addSubEntry(me, ce);
   }
}

static QString stripPath_tag(const QString &s)
{
   int i = s.lastIndexOf('/');

   if (i != -1) {
      return s.right(s.length() - i - 1);

   } else {
      return s;

   }
}

/*! Injects the info gathered by the XML parser into the Entry tree.
 *  This tree contains the information extracted from the input in a
 *  "unrelated" form.
 */
void TagFileParser::buildLists(QSharedPointer<Entry> root)
{
   // build class list
   for (auto tci : m_tagFileClasses) {

      QSharedPointer<Entry> ce = QMakeShared<Entry>();
      ce->section = Entry::CLASS_SEC;

      switch (tci.kind) {
         case TagClassInfo::Class:
            break;

         case TagClassInfo::Struct:
            ce->m_traits.clear();
            ce->m_traits.setTrait(Entry::Virtue::Struct);
            break;

         case TagClassInfo::Union:
            ce->m_traits.clear();
            ce->m_traits.setTrait(Entry::Virtue::Union);
            break;

         case TagClassInfo::Interface:
            ce->m_traits.clear();
            ce->m_traits.setTrait(Entry::Virtue::Interface);
            break;

         case TagClassInfo::Enum:
            ce->m_traits.clear();
            ce->m_traits.setTrait(Entry::Virtue::Enum);
            break;

         case TagClassInfo::Exception:
            ce->m_traits.clear();
            ce->m_traits.setTrait(Entry::Virtue::Exception);
            break;

         case TagClassInfo::Protocol:
            ce->m_traits.clear();
            ce->m_traits.setTrait(Entry::Virtue::Protocol);
            break;

         case TagClassInfo::Category:
            ce->m_traits.clear();
            ce->m_traits.setTrait(Entry::Virtue::Category);
            break;

         case TagClassInfo::Service:
            ce->m_traits.clear();
            ce->m_traits.setTrait(Entry::Virtue::Service);
            break;

         case TagClassInfo::Singleton:
            ce->m_traits.clear();
            ce->m_traits.setTrait(Entry::Virtue::Singleton);
            break;
      }

      ce->name = tci.name;

      if (tci.kind == TagClassInfo::Protocol) {
         ce->name += "-p";
      }

      addDocAnchors(ce, tci.docAnchors);

      TagInfo ti;
      ti.tagName   = m_tagName;
      ti.fileName  = tci.filename;

      ce->id             = tci.clangId;
      ce->m_tagInfoEntry = ti;
      ce->lang           = tci.isObjC ? SrcLangExt_ObjC : SrcLangExt_Unknown;

      // transfer base class list
      if (tci.bases) {
         ce->extends = *tci.bases;

         delete tci.bases;
         tci.bases = 0;
      }

      if (tci.templateArguments) {
         ArgumentList al;
         ce->m_templateArgLists.append(al);

         for (auto argName : *tci.templateArguments) {
            Argument arg;

            arg.type = "class";
            arg.name = argName;
            al.append(arg);
         }
      }

      buildMemberList(ce, tci.members);
      root->addSubEntry(ce, root);
   }

   // build file list
   for (auto tfi : m_tagFileFiles) {
      QSharedPointer<Entry> fe = QMakeShared<Entry>();

      fe->section = determineSection(tfi.name);
      fe->name    = tfi.name;

      addDocAnchors(fe, tfi.docAnchors);

      TagInfo ti;
      ti.tagName  = m_tagName;
      ti.fileName = tfi.filename;

      fe->m_tagInfoEntry = ti;

      QString fullName = m_tagName + ":" + tfi.path + stripPath_tag(tfi.name);
      fe->fileName = fullName;

      QSharedPointer<FileDef> fd = QMakeShared<FileDef>(m_tagName + ":" + tfi.path, tfi.name, m_tagName, tfi.filename);

      QSharedPointer<FileNameList> mn (Doxy_Globals::inputNameDict.find(tfi.name));

      if (mn) {
         mn->append(fd);

      } else {
         mn = QMakeShared<FileNameList>(fullName, tfi.name);
         mn->append(fd);

         Doxy_Globals::inputNameList.inSort(mn);
         Doxy_Globals::inputNameDict.insert(tfi.name, mn);
      }

      buildMemberList(fe, tfi.members);
      root->addSubEntry(fe, root);
   }

   // build namespace list
   for (auto tni : m_tagFileNamespaces) {
      QSharedPointer<Entry> ne = QMakeShared<Entry>();

      ne->section  = Entry::NAMESPACE_SEC;
      ne->name     = tni.name;
      addDocAnchors(ne, tni.docAnchors);

      TagInfo ti;
      ti.tagName   = m_tagName;
      ti.fileName  = tni.filename;

      ne->id              = tni.clangId;
      ne->m_tagInfoEntry  = ti;

      buildMemberList(ne, tni.members);
      root->addSubEntry(ne, root);
   }

   // build package list
   for (auto tpgi : m_tagFilePackages) {
      QSharedPointer<Entry> pe = QMakeShared<Entry>();

      pe->section  = Entry::PACKAGE_SEC;
      pe->name     = tpgi.name;
      addDocAnchors(pe, tpgi.docAnchors);

      TagInfo ti;
      ti.tagName  = m_tagName;
      ti.fileName = tpgi.filename;

      pe->m_tagInfoEntry  = ti;

      buildMemberList(pe, tpgi.members);
      root->addSubEntry(pe, root);
   }

   // build group list
   for (auto tgi : m_tagFileGroups) {
      QSharedPointer<Entry> ge = QMakeShared<Entry>();

      ge->section  = Entry::GROUPDOC_SEC;
      ge->name     = tgi.name;
      ge->type     = tgi.title;

      addDocAnchors(ge, tgi.docAnchors);

      TagInfo ti;
      ti.tagName  = m_tagName;
      ti.fileName = tgi.filename;

      ge->m_tagInfoEntry  = ti;

      buildMemberList(ge, tgi.members);
      root->addSubEntry(ge, root);
   }

   // build page list
   for (auto tpi : m_tagFilePages) {
      QSharedPointer<Entry> pe = QMakeShared<Entry>();

      // hard sets the section
      if (tpi.filename == "index") {
         pe->section = Entry::MAINPAGEDOC_SEC;

      } else {
         pe->section = Entry::PAGEDOC_SEC;
      }

      pe->name     = tpi.name;
      pe->args     = tpi.title;

      addDocAnchors(pe, tpi.docAnchors);

      TagInfo ti;
      ti.tagName  = m_tagName;
      ti.fileName = tpi.filename;

      pe->m_tagInfoEntry = ti;
      root->addSubEntry(pe, root);
   }
}

void TagFileParser::addIncludes()
{
   for (auto tfi : m_tagFileFiles) {
      QSharedPointer<FileNameList> fn (Doxy_Globals::inputNameDict.find(tfi.name));

      if (fn) {

         for (auto fd : *fn) {
            if (fd->getPath() == (m_tagName + ":" + tfi.path)) {

               for (auto item : tfi.includes) {

                  QSharedPointer<FileNameList> ifn (Doxy_Globals::inputNameDict.find(item.name));
                  assert(ifn != 0);

                  if (ifn) {
                     for (auto ifd : *ifn) {
                        if (ifd->getOutputFileBase() == item.id) {
                           fd->addIncludeDependency(ifd, item.text, item.isLocal, item.isImported, false);
                        }
                     }
                  }

               }
            }
         }
      }
   }
}

void parseTagFile(QSharedPointer<Entry> root, const QString &fullName)
{
   QFileInfo fi(fullName);

   if (! fi.exists()) {
      return;
   }

   TagFileErrorHandler errorHandler;

   // tagName
   TagFileParser handler(fullName);
   handler.setFileName(fullName);

   //
   QFile xmlFile(fullName);
   QByteArray data = xmlFile.readAll();

   QXmlInputSource source;
   source.setData(data);

   QXmlSimpleReader reader;
   reader.setContentHandler(&handler);
   reader.setErrorHandler(&errorHandler);
   reader.parse(source);

   handler.buildLists(root);
   handler.addIncludes();
}
