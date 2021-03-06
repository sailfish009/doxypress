/*************************************************************************
*
* Copyright (C) 2012-2017 Barbara Geller & Ansel Sermersheim
* All rights reserved.
*
* GNU Free Documentation License
*
*************************************************************************/

/** @defgroup group1 Group A
 *  Entries in group A
 *  @{
 */

/** @brief  Brief description of a function in group 1 */
void func_a1() {}

/** Another function in group 1 */
void func_a2() {}

/** @} */ // end of group1



/**
 *  @defgroup group2 Group B
 *  Entries in group B
 */



/** @defgroup group3 Group C
 *  Entries in group C
 */



/** @defgroup group4 Group D
 *  @ingroup group3
 *  Group D is a child of group C
 */



/**
 *  @ingroup group2
 *  @brief Function in group B
 */
void func_b1() {};

/** @ingroup group2
 *  @brief Function in group B
 */
void func_b2() {};



/** @ingroup group3
 *  @brief Function in @link group3 Group C@endlink.
 */
void func_c1() {};



/** @ingroup group1 group2 group3 group4
 *  Namespace groupNS is in groups A, B, C, D
 *
 */

namespace groupNS{
   void func_all() {};
}




/** @file
 *  @ingroup group3
 *  @brief this file in group C
 */



/** @defgroup group5 Group E
 *  Entries in group E
 *  @{
 */


/** @page mypage1 Section Group E Heading
 *  Additional documentation which will appear in the detailed description.
 */

/** @brief Function in group E
 */
void func_e() {};

/** @} */ // end of group5



/** @addtogroup group1
 *  
 *  More documentation for group A.
 *  @{
 */

/** another function in group A */
void func_a3() {}

/** yet another function in group A */
void func_a4() {}

/** @} */ // end of group1

