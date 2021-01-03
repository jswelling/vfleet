/****************************************************************************
 * lists.h 
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

#ifndef INCL_LISTS
#define INCL_LISTS

#ifndef NULL
#define NULL 0
#endif

//
// Singly linked list base structures
//

struct sLink_base {
  sLink_base* next;
  sLink_base() { next= NULL; }
};

class sList_base {
friend class sList_base_iter;
protected:
  sLink_base* last; // last->next is head of list
public:
  void insert(sLink_base* a);  // add at head of list
  void append(sLink_base* a);  // add at tail of list
  sLink_base* pop();           // return and remove head
  sLink_base* head() const { return last ? last->next : NULL; }
  sLink_base* tail() const { return last ? last : NULL; }
  sList_base() { last = NULL; }
  sList_base(sLink_base* a) { last = a->next= a; };
};

class sList_base_iter {
  sLink_base* ce;  // current element
  const sList_base* cs;  // current list
public:
  sList_base_iter(const sList_base& s) { cs= &s; ce= cs->last; }
  sLink_base* next() // returns NULL on completion
    { 
      sLink_base* ret= ce ? (ce= ce->next) : NULL;
      if (ce == cs->last) ce= NULL;
      return ret;
    }
};

//
// Singly linked list templates
//

template<class T>
struct sLink : public sLink_base {
  T info;
  sLink( const T& a ) 
  : info(a) {}
};

template<class T> class isList_iter;

// Intrusive singly linked list
template<class T> 
class isList : private sList_base {
friend class isList_iter<T>;
public:
  void insert( T* a ) { sList_base::insert(a); }
  void append( T* a ) { sList_base::append(a); }
  T* pop() { return (T*)sList_base::pop(); }
  T* head() { return (T*)sList_base::head(); }
  T* tail() { return (T*)sList_base::tail(); }
  isList() 
  : sList_base() {}
  isList( T* a ) 
  : sList_base( a ) {}
};

template<class T> class sList_iter;

// Non-intrusive singly linked list
template<class T>
class sList : private sList_base {
friend class sList_iter<T>;
public:
  void insert( T& a ) { sList_base::insert( new sLink<T>(a) ); }
  void append( T& a ) { sList_base::append( new sLink<T>(a) ); }
  T pop()
  {
    sLink<T>* lnk= (sLink<T>*)sList_base::pop();
    T i= lnk->info;
    delete lnk;
    return i;
  }
  T head() { 
    if (sList_base::head()) return ((sLink<T>*)sList_base::head())->info; 
    else return NULL;
  }
  T tail() { 
    if (sList_base::tail()) return ((sLink<T>*)sList_base::tail())->info; 
    else return NULL;
  }
  sList() 
  : sList_base() {}
  sList( T& a )
  : sList_base( new sLink<T*>(&a) ) {}
  ~sList() { while (last) (void)pop(); }
};

template<class T>
class isList_iter : private sList_base_iter {
public:
  isList_iter(const isList<T>& s) 
  : sList_base_iter(s) {}
  T* next() { return (T*)sList_base_iter::next(); }
};

template<class T>
class sList_iter : private sList_base_iter {
public:
  sList_iter(const sList<T>& s) 
  : sList_base_iter(s) {}
  T* next()
  { 
    sLink<T>* lnk= (sLink<T>*)sList_base_iter::next();
    if (lnk) return &(lnk->info);
    else return NULL;
  }
};

//
// Doubly linked list base structures
//

struct dLink_base {
  dLink_base* next;
  dLink_base* prev;
  dLink_base() { prev= next= NULL; }
};

class dList_base {
friend class dList_base_iter;
protected:
  dLink_base* last; // last->next is head of list
public:
  void insert(dLink_base* a);  // add at head of list
  void append(dLink_base* a);  // add at tail of list
  dLink_base* head() const { return last ? last->next : NULL; }
  dLink_base* tail() const { return last ? last : NULL; }
  dLink_base* pop(); // remove and return head of list
  dList_base() { last = NULL; }
};

class dList_base_iter {
  dLink_base* ce;  // current element
  dList_base* cs;  // current list
  dLink_base* last_seen; // maintained we might be walking either direction
public:
  dList_base_iter(dList_base& s) 
    { 
      cs= &s; 
      ce= cs->last; 
      last_seen= NULL;
    }
  int last_entry() { return( ( (ce == NULL) || (ce->next == NULL) ) ); }
  int first_entry() { return( (ce == NULL) ); }
  dLink_base* next(); // returns NULL on completion, then loops back to begin
  dLink_base* prev(); // returns NULL on completion, then loops back to end
  void remove(); // cuts current from list
  void insert_ahead( dLink_base* a );
  void insert_behind( dLink_base* a );
};

template<class T>
struct dLink : public dLink_base {
  T info;
  dLink( const T& a ) 
  : info(a) {}
};

template<class T> class idList_iter;

// Intrusive doubly linked list
template<class T> 
class idList : private dList_base {
friend class idList_iter<T>;
public:
  void insert( T* a ) { dList_base::insert(a); }
  void append( T* a ) { dList_base::append(a); }
  T* head() { return (T*)dList_base::head(); }
  T* tail() { return (T*)dList_base::tail(); }
  T* pop() { return (T*)dList_base::pop(); }
  idList() 
  : dList_base() {}
};

template<class T> class dList_iter;

// Non-intrusive doubly linked list
template<class T>
class dList : private dList_base {
friend class dList_iter<T>;
public:
  void insert( const T& a ) { dList_base::insert( new dLink<T>(a) ); }
  void append( const T& a ) { dList_base::append( new dLink<T>(a) ); }
  T head() { 
    if (dList_base::head()) return ((dLink<T>*)dList_base::head())->info; 
    else return (T)0;
  }
  T tail() { 
    if (dList_base::tail()) return ((dLink<T>*)dList_base::tail())->info; 
    else return (T)0;
  }
  T pop()
  {
    dLink<T>* lnk= (dLink<T>*)dList_base::pop();
    T i= lnk->info;
    delete lnk;
    return i;
  }
  dList() 
  : dList_base() {}
  ~dList() { while (last) (void)pop(); }
};

template<class T>
class idList_iter : private dList_base_iter {
public:
  idList_iter(idList<T>& s) 
  : dList_base_iter(s) {}
  T* next() { return (T*)dList_base_iter::next(); }
  T* prev() { return (T*)dList_base_iter::prev(); }
  void remove() { dList_base_iter::remove(); }
  void insert_ahead( T* a ) { dList_base_iter::insert_ahead(a); }
  void insert_behind( T* a ) { dList_base_iter::insert_behind(a); }
  int first_entry()
  { return dList_base_iter::first_entry(); }
  int last_entry()
  { return dList_base_iter::last_entry(); }
};

template<class T>
class dList_iter : private dList_base_iter {
dLink<T>* last_seen;
public:
  dList_iter(dList<T>& s) 
  : dList_base_iter(s) {}
  T* next()
  { 
    last_seen= (dLink<T>*)dList_base_iter::next();
    if (last_seen) return &(last_seen->info);
    else return NULL;
  }
  T* prev()
  {
    last_seen= (dLink<T>*)dList_base_iter::prev();
    if (last_seen) return &(last_seen->info);
    else return NULL;
  }
  void remove() 
  { 
    dList_base_iter::remove();
    delete last_seen;
  }
  void insert_ahead( const T& a ) 
  { dList_base_iter::insert_ahead( new dLink<T>(a) ); }
  void insert_behind( const T& a ) 
  { dList_base_iter::insert_behind( new dLink<T>(a) ); }
  int first_entry()
  { return dList_base_iter::first_entry(); }
  int last_entry()
  { return dList_base_iter::last_entry(); }
};

#endif // ifndef INCL_LISTS

