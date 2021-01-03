/****************************************************************************
 * lists_tester.cc
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

class mysclass : public sLink_base {
public:
  int i;
  mysclass(int i_in) { i= i_in; }
};

class mydclass : public dLink_base {
public:
  int i;
  mydclass(int i_in) { i= i_in; }
};

void test_single()
{
  int i;
  int *ip;
  sList<int> slist;

  fprintf(stderr,"slist insert:\n");
  for (i=0; i<10; i++) slist.insert(i);
  fprintf(stderr,"slist head and tail: %d %d\n",slist.head(),slist.tail());
  for (i=0; i<10; i++) fprintf(stderr,"slist pop -> %d\n",slist.pop());

  fprintf(stderr,"slist append:\n");
  for (i=0; i<10; i++) slist.append(i);
  for (i=0; i<10; i++) fprintf(stderr,"slist pop -> %d\n",slist.pop());

  fprintf(stderr,"slist siterator:\n");
  for (i=0; i<10; i++) slist.append(i);
  sList_iter<int> siter(slist);
  while (ip= siter.next()) fprintf(stderr,"slist iter -> %d\n",*ip);

  isList<mysclass> islist;
  mysclass *tmp;

  fprintf(stderr,"islist insert:\n");
  for (i=0; i<10; i++) islist.insert( new mysclass(i) );
  fprintf(stderr,"islist head and tail: %d %d\n",
	  islist.head()->i,islist.tail()->i);
  for (i=0; i<10; i++) {
    tmp= islist.pop();
    fprintf(stderr,"islist pop -> %d\n",tmp->i);
    delete tmp;
  }

  fprintf(stderr,"islist append:\n");
  for (i=0; i<10; i++) islist.append( new mysclass(i) );
  for (i=0; i<10; i++) {
    tmp= islist.pop();
    fprintf(stderr,"islist pop -> %d\n",tmp->i);
    delete tmp;
  }

  fprintf(stderr,"islist iterator:\n");
  for (i=0; i<10; i++) islist.append( new mysclass(i) );
  isList_iter<mysclass> isiter(islist);
  while (tmp= isiter.next()) {
    fprintf(stderr,"islist iter -> %d\n",tmp->i);
    delete tmp;
  }
}

void test_intrusive_double()
{
  int i;
  mydclass *tmp;

  idList<mydclass> idlist;

  fprintf(stderr,"\nidlist iterator:\n");
  for (i=0; i<10; i++) idlist.append(new mydclass(i));
  fprintf(stderr,"idlist head and tail: %d %d\n",
	  idlist.head()->i,idlist.tail()->i);
  idList_iter<mydclass> *iditer= new idList_iter<mydclass>(idlist);
  while (tmp= iditer->next()) fprintf(stderr,"idlist iter -> %d\n",tmp->i);
  while (tmp= iditer->next()) fprintf(stderr,"idlist iter (pass 2)-> %d\n",
				      tmp->i);
  delete iditer;

  iditer= new idList_iter<mydclass>(idlist);
  fprintf(stderr,"idlist iterator backwards:\n");
  while (tmp= iditer->prev()) fprintf(stderr,"idlist prev -> %d\n",tmp->i);  
  while (tmp= iditer->prev()) fprintf(stderr,"idlist prev (pass 2) -> %d\n",
				      tmp->i);  
  while (tmp= iditer->next()) fprintf(stderr,"idlist iter (pass 3)-> %d\n",
				      tmp->i);
  delete iditer;

  iditer= new idList_iter<mydclass>(idlist);
  while (tmp= iditer->next()) {
    if ((tmp->i)%2) {
      fprintf(stderr,"removing %d\n",tmp->i);
      iditer->remove();
      delete tmp;
    }
  }
  delete iditer;
  iditer= new idList_iter<mydclass>(idlist); // start from head
  while (tmp= iditer->prev()) fprintf(stderr,"idlist iter -> %d\n",tmp->i);  
  delete iditer;

  // Delete head of list
  fprintf(stderr,"Removing head of list\n");
  iditer= new idList_iter<mydclass>(idlist); // start from head
  (void)iditer->next(); 
  iditer->remove();
  delete iditer;
  iditer= new idList_iter<mydclass>(idlist); // start from head
  while (tmp= iditer->prev()) fprintf(stderr,"idlist iter -> %d\n",tmp->i);  
  delete iditer;

  // Delete all that remain
  fprintf(stderr,"Deleting the whole list\n");
  iditer= new idList_iter<mydclass>(idlist);
  while (tmp= iditer->next()) {
    fprintf(stderr,"removing %d\n",tmp->i);
    iditer->remove();
    delete tmp;
  }
  delete iditer;
  fprintf(stderr,"printing remaining list:\n");
  iditer= new idList_iter<mydclass>(idlist);
  while (tmp= iditer->prev()) fprintf(stderr,"idlist iter -> %d\n",tmp->i);  
  delete iditer;
  
  // Wander around inserting things
  fprintf(stderr,"Wander around inserting things\n");

  iditer= new idList_iter<mydclass>(idlist);
  iditer->insert_ahead( new mydclass(0) );
  iditer->insert_ahead( new mydclass(1) );
  iditer->insert_behind( new mydclass(2) );
  iditer->next();
  iditer->insert_ahead( new mydclass(3) );
  iditer->next();
  iditer->insert_behind( new mydclass(4) );
  iditer->prev();
  iditer->insert_ahead( new mydclass(5) );
  iditer->prev();
  iditer->insert_behind( new mydclass(6) );
  while (tmp= iditer->prev()) fprintf(stderr,"idlist iter -> %d\n",tmp->i);  
  delete iditer;

  iditer= new idList_iter<mydclass>(idlist);
  while (tmp= iditer->prev()) fprintf(stderr,"idlist iter -> %d\n",tmp->i);  
  delete iditer;
}

void test_nonintrusive_double()
{
  int i;
  int *ip;
  dList<int> dlist;

  fprintf(stderr,"\ndlist iterator:\n");
  for (i=0; i<10; i++) dlist.append(i);
  fprintf(stderr,"dlist head and tail: %d %d\n",dlist.head(),dlist.tail());
  dList_iter<int> *iditer= new dList_iter<int>(dlist);
  while (ip= iditer->next()) fprintf(stderr,"dlist iter -> %d\n",*ip);
  while (ip= iditer->next()) fprintf(stderr,"dlist iter (pass 2)-> %d\n",*ip);
  delete iditer;

  iditer= new dList_iter<int>(dlist);
  fprintf(stderr,"dlist iterator backwards:\n");
  while (ip= iditer->prev()) fprintf(stderr,"dlist prev -> %d\n",*ip);  
  while (ip= iditer->prev()) fprintf(stderr,"dlist prev (pass 2)-> %d\n",*ip);
  while (ip= iditer->next()) fprintf(stderr,"dlist iter (pass 3)-> %d\n",*ip);
  delete iditer;

  iditer= new dList_iter<int>(dlist);
  while (ip= iditer->next()) {
    if ((*ip)%2) {
      fprintf(stderr,"removing %d\n",*ip);
      iditer->remove();
      delete ip;
    }
  }
  delete iditer;
  iditer= new dList_iter<int>(dlist); // start from head
  while (ip= iditer->prev()) fprintf(stderr,"dlist iter -> %d\n",*ip);  
  delete iditer;

  // Delete head of list
  fprintf(stderr,"Removing head of list\n");
  iditer= new dList_iter<int>(dlist); // start from head
  (void)iditer->next(); 
  iditer->remove();
  delete iditer;
  iditer= new dList_iter<int>(dlist); // start from head
  while (ip= iditer->prev()) fprintf(stderr,"dlist iter -> %d\n",*ip);  
  delete iditer;

  // Delete all that remain
  fprintf(stderr,"Deleting the whole list\n");
  iditer= new dList_iter<int>(dlist);
  while (ip= iditer->next()) {
    fprintf(stderr,"removing %d\n",*ip);
    iditer->remove();
    delete ip;
  }
  delete iditer;
  fprintf(stderr,"printing remaining list:\n");
  iditer= new dList_iter<int>(dlist);
  while (ip= iditer->prev()) fprintf(stderr,"dlist iter -> %d\n",*ip);  
  delete iditer;
  
  // Wander around inserting things
  fprintf(stderr,"Wander around inserting things\n");

  iditer= new dList_iter<int>(dlist);
  iditer->insert_ahead( 0 );
  iditer->insert_ahead( 1 );
  iditer->insert_behind( 2 );
  iditer->next();
  iditer->insert_ahead( 3 );
  iditer->next();
  iditer->insert_behind( 4 );
  iditer->prev();
  iditer->insert_ahead( 5 );
  iditer->prev();
  iditer->insert_behind( 6 );
  while (ip= iditer->prev()) fprintf(stderr,"dlist iter -> %d\n",*ip);  
  delete iditer;

  iditer= new dList_iter<int>(dlist);
  while (ip= iditer->prev()) fprintf(stderr,"dlist iter -> %d\n",*ip);  
  delete iditer;
}

main()
{
  test_single();

  test_intrusive_double();

  test_nonintrusive_double();
}

