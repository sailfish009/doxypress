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

#include <sortedlist.h>

#include <classdef.h>
#include <config.h>
#include <dirdef.h>
#include <dot.h>
#include <filedef.h>
#include <filename.h>
#include <ftvhelp.h>
#include <groupdef.h>

// BaseClassList --> SortedList<BaseClassDef *>
// BaseClassListIterator --> QListIterator<BaseClassDef *>

int compareListValues(const BaseClassDef *item1, const BaseClassDef *item2) 
{
   QSharedPointer<const ClassDef> c1 = item1->classDef;
   QSharedPointer<const ClassDef> c2 = item2->classDef;

   if (c1 == 0 || c2 == 0) {
      return false;

   } else {
      return c1->name().compare(c2->name(), Qt::CaseInsensitive);
   }
}


// ClassList --> SortedList<ClassDef *>
// ClassListIterator --> QListIterator<ClassDef *>
int compareListValues(const ClassDef *item1, const ClassDef *item2)
{
   static bool b = Config::getBool("sort-by-scope-name");

   if (b) {   
      return item1->name().compare(item2->name(), Qt::CaseInsensitive);

   } else {
      return item1->className().compare(item2->className(), Qt::CaseInsensitive);

   }
}

// DirList ->  SortedList<DirDef *>
int compareListValues(QSharedPointer<DirDef> item1, QSharedPointer<DirDef> item2)
{
    return item1->shortName().compare(item2->shortName(), Qt::CaseInsensitive);
}


// DotNodeList --> SortedList<DotNode *>
int compareListValues(const DotNode *item1, const DotNode *item2) 
{
   return item1->label().compare(item2->label(), Qt::CaseInsensitive);
}


// FileList 
int compareListValues(QSharedPointer<FileDef> item1, QSharedPointer<FileDef> item2) 
{
   return item1->name().compare(item2->name(), Qt::CaseInsensitive);
}

// FileNameList --> SortedList<FileName *>
// FileNameListIterator -->  QListIterator<FileName *>
int compareListValues(QSharedPointer<FileName> item1, QSharedPointer<FileName> item2) 
{
   int retval;

   if (Config::getBool("full-path-names")) {    
      retval = item1->fullName().compare(item2->fullName(), Qt::CaseInsensitive);

   } else {
      retval = item1->fileName().compare(item2->fileName(), Qt::CaseInsensitive);

   }

   return retval;
}


//   GroupList --> SortedList<GroupDef *>
//   GroupListIterator --> QListIterator<GroupDef *>

int compareListValues(QSharedPointer<GroupDef> item1, QSharedPointer<GroupDef> item2) 
{
   return item1->groupTitle().compare(item2->groupTitle());
}


// MemberList

int compareListValues(QSharedPointer<MemberDef> c1, QSharedPointer<MemberDef> c2)
{
   static bool sortConstructorsFirst = Config::getBool("sort-constructors-first");

   if (sortConstructorsFirst) {
      int ord1 = c1->isConstructor() ? 2 : (c1->isDestructor() ? 1 : 0);
      int ord2 = c2->isConstructor() ? 2 : (c2->isDestructor() ? 1 : 0);

      if (ord1 > ord2) {
         return -1;

      } else if (ord2 > ord1) {
         return 1;

      }
   }
   
   int cmp = c1->name().compare(c2->name(), Qt::CaseInsensitive );

   return cmp != 0 ? cmp : c1->getDefLine() - c2->getDefLine();
}


// NavIndexEntryList --> SortedList<NavIndexEntry *>

 
// sort list based on url
int compareListValues(const NavIndexEntry *item1, const NavIndexEntry *item2)
{  
   return item1->m_url.compare(item2->m_url);  
}