/*************************************************************************
*                                                                         
* Copyright (C) 2012-2016 Barbara Geller & Ansel Sermersheim                                                         
* All rights reserved.                                                    
*                                                                         
*                                                                         
* GNU Free Documentation License                                          
* This file may be used under the terms of the GNU Free Documentation     
* License version 1.3 as published by the Free Software Foundation        
* and appearing in the file included in the packaging of this file.       
*                                                                         
*                                                                         
*************************************************************************/

#ifndef USER_TESTS_H
#define USER_TESTS_H

/// docs for testTabs
class testTabs
{
   public:
      /**
      * 	Retrieves a list of integers.  ( tab parsing test, No issue found. )
      * 	caller is responsible for deleting them
      *
      * @param myvar	some purpose or other
      * @return list of integers
      */
      int* doSomething(long myvar) const;

};

/// docs for testFriend
class testFriend
{
   public:
      /// does this cause a warning about return types? No issue found.
      friend class myFriend;

};

/// docs for testTypeDef
class testTypeDef
{
   public:
      typedef int myTypeDef;      
      myTypeDef someMethod(); 
};


/// base class, test graphViz
class dotFruit
{
   public:
      virtual void chew(void);      ///< describe how to chew a fruit
};

/// child class, test graphViz
class dotApple : public dotFruit
{
   public:
      void chew();

};


#endif      // comment after the end of the header file (does this cause a problem? No isssue found.)
