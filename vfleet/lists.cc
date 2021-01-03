/****************************************************************************
 * lists.cc
 * Author Joel Welling
 * Copyright 1994, Pittsburgh Supercomputing Center, Carnegie Mellon University
 * (derived from an example in Straustrup's The C++ Programming Language,
 *  second edition)
 *
 * Permission use, copy, and modify this software and its documentation
 * without fee for personal use or use within your organization is hereby
 * granted, provided that the above copyright notice is preserved in all
 * copies and that that copyright and this permission notice appear in
 * supporting documentation.  Permission to redistribute this software to
 * other organizations or individuals is not granted;  that must be
 * negotiated with the PSC.  Neither the PSC nor Carnegie Mellon
 * University make any representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *****************************************************************************/

#include <stdio.h>
#include "lists.h"

void sList_base::insert( sLink_base* a)  // add to head of list
{
  if (last) a->next= last->next;
  else last= a;
  last->next= a;
}

void sList_base::append( sLink_base* a )  // add to tail of list
{
  if (last) {
    a->next= last->next;
    last= last->next= a;
  }
  else last= a->next= a;
}

sLink_base* sList_base::pop() // return and remove head of list
{
  if (!last) {
    fprintf(stderr,"sList_base::pop(): Error: empty list!\n");
    return NULL;
  }
  sLink_base* f= last->next;
  if (f==last) last= NULL;
  else last->next= f->next;
  return f;
}

dLink_base* dList_base::pop() // remove and return head of list
{
  if (!last) {
    fprintf(stderr,"dList_base::pop(): Error: empty list!\n");
    return NULL;
  }
  dLink_base* f= last->next;
  if (f==last) last= NULL;
  else { 
    last->next= f->next;
    last->next->prev= last;
  };
  return f;
}

void dList_base::insert( dLink_base* a ) // add at head of list
{
  if (last) { 
    a->next= last->next;
  }
  else {
    last= a;
  }

  last->next= a;
  a->prev= last;
  a->next->prev= a;
  a->prev->next= a;
}

void dList_base::append( dLink_base* a ) // add at tail of list
{
  if (last) {
    a->next= last->next;
    a->prev= last;
    last->next= a;
    last= a;
  }
  else {
    last= a->next= a;
  }

  a->next->prev= a;
  a->prev->next= a;
}

void dList_base_iter::remove() // remove current element
{
  if (last_seen) {
    last_seen->next->prev= last_seen->prev;
    last_seen->prev->next= last_seen->next;
    if (last_seen==cs->last) { // removing last cell
      if (last_seen->next == last_seen) { // and its the only cell
	cs->last= NULL;
	ce= NULL;
      }
      else cs->last= last_seen->prev;
    }
    last_seen= NULL;
  }
  else fprintf(stderr,
	       "dList_base_iter::remove(): Error: no current element!\n");
}

void dList_base_iter::insert_ahead( dLink_base* a )
{
  if (last_seen) {
    a->next= last_seen;
    a->prev= last_seen->prev;
    a->next->prev= a;
    a->prev->next= a;
  }
  else // At the end of list, or it is an empty list
    cs->insert(a);
}

void dList_base_iter::insert_behind( dLink_base* a )
{
  if (last_seen) {
    a->prev= last_seen;
    a->next= last_seen->next;
    a->next->prev= a;
    a->prev->next= a;
  }
  else // At the end of list, or it is an empty list
    cs->append(a);
}

// SGI compiler seems to have trouble with bringing this inline
dLink_base* dList_base_iter::next()
{
  last_seen= ce ? (ce= ce->next) : NULL;
  if (ce == cs->last) ce= NULL;
  else if (!ce) ce= cs->last;
  return last_seen;
}

// SGI compiler seems to have trouble with bringing this inline
dLink_base* dList_base_iter::prev()
{
  last_seen= ce;
  if (ce) ce= ce->prev;
  if (ce == cs->last) ce= NULL;
  else if (!ce) ce= cs->last;
  return last_seen;
}

