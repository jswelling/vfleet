/****************************************************************************
 * jthreads.h
 * Author Joel Welling
 * Copyright 1997, Pittsburgh Supercomputing Center, Carnegie Mellon University
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
/* This code provides a wrapper around system-specific thread libs */

#ifndef JTHREADS_H_INCLUDED
#define JTHREADS_H_INCLUDED

#include <sys/signal.h>
#include <limits.h>
#if ( SGI_MIPS || INTEL_LINUX )
// deal with some apparent errors in some pthread.h
typedef void* spcb_p;
typedef void* ptcb_p;
#define pthread_attr_default NULL
#define pthread_mutexattr_default NULL
#define pthread_condattr_default NULL
#endif
extern "C" {
#include <pthread.h>
};

class JSimpleThread;

// A mutex
class JMutex {
  friend class JSimpleThread;
public:
  JMutex() 
  { 
    if (pthread_mutex_init(&mutex, pthread_mutexattr_default))
      perror("JMutex: error creating mutex");
  }
  virtual ~JMutex()
  {
    if (pthread_mutex_destroy(&mutex))
      perror("JMutex: error destroying mutex");
  }
  void lock() { pthread_mutex_lock(&mutex); }
  void unlock() { pthread_mutex_unlock(&mutex); }
private:
  pthread_mutex_t mutex;
};

// A semaphore
class JSemaphore {
public:
  JSemaphore(const int val)
  {
    v= val;
    if (pthread_mutex_init(&mutex, pthread_mutexattr_default))
      perror("JSemaphore: error creating mutex");
    if (pthread_cond_init(&cond, pthread_condattr_default))
      perror("JSemophore: error creating cond");
  }
  ~JSemaphore()
  {
    if (pthread_cond_destroy(&cond))
      perror("JSemaphore: error destroying cond");
    if (pthread_mutex_destroy(&mutex))
      perror("JSemaphore: error destroying mutex");
  }
  int value() const { return v; }
  int wait()
  {
    if (pthread_mutex_lock(&mutex)) perror("JSemaphore wait: error locking");
    if (pthread_cond_wait(&cond,&mutex)) 
      perror("JSemaphore wait: error waiting");
    int val= v;
    if (pthread_mutex_unlock(&mutex)) 
      perror("JSemaphore wait: error unlocking");
    return val;
  }
  int waitIfNonzero()
  {
    int val= 0;
    if (pthread_mutex_lock(&mutex)) perror("JSemaphore wait: error locking");
    if (v) {
      if (pthread_cond_wait(&cond,&mutex)) 
	perror("JSemaphore wait: error waiting");
      val= v;
    }
    if (pthread_mutex_unlock(&mutex)) 
      perror("JSemaphore wait: error unlocking");
    return val;
  }
  void set( const int val ) // un-sticks waiting threads
  {
    if (pthread_mutex_lock(&mutex)) perror("JSemaphore set: error locking");
    v= val;
    if (pthread_mutex_unlock(&mutex)) 
      perror("JSemaphore set: error unlocking");
    if (pthread_cond_broadcast(&cond)) 
      perror("JSemaphore set: error broadcasting cond");
  }
  void incr() // un-sticks waiting threads, and increments by 1
  {
    if (pthread_mutex_lock(&mutex)) perror("JSemaphore incr: error locking");
    v++;
    if (pthread_mutex_unlock(&mutex)) 
      perror("JSemaphore set: error unlocking");
    if (pthread_cond_broadcast(&cond)) 
      perror("JSemaphore set: error broadcasting cond");    
  }
  void incr(const int n) // un-sticks waiting threads, and increments by n
  {
    if (pthread_mutex_lock(&mutex)) perror("JSemaphore incr: error locking");
    v += n;
    if (pthread_mutex_unlock(&mutex)) 
      perror("JSemaphore set: error unlocking");
    if (pthread_cond_broadcast(&cond)) 
      perror("JSemaphore set: error broadcasting cond");    
  }
  void decr() // un-sticks waiting threads, and decrements by 1
  {
    if (pthread_mutex_lock(&mutex)) perror("JSemaphore incr: error locking");
    v--;
    if (pthread_mutex_unlock(&mutex)) 
      perror("JSemaphore set: error unlocking");
    if (pthread_cond_broadcast(&cond)) 
      perror("JSemaphore set: error broadcasting cond");    
  }
  void decr(const int n) // un-sticks waiting threads, and decrements by n
  {
    if (pthread_mutex_lock(&mutex)) perror("JSemaphore incr: error locking");
    v -= n;
    if (pthread_mutex_unlock(&mutex)) 
      perror("JSemaphore set: error unlocking");
    if (pthread_cond_broadcast(&cond)) 
      perror("JSemaphore set: error broadcasting cond");    
  }
private:
  int v;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
};

// One thread
class JSimpleThread {
public:
  JSimpleThread( void* (*start_addr)(void* arg),
		    void* arg )
  { 
    if (pthread_create(&threadId, pthread_attr_default, start_addr, arg))
      perror("JSimpleThread: error creating Posix thread");
  }
  virtual ~JSimpleThread()
  {
    if (pthread_equal(threadId,pthread_self())) pthread_exit(0);
    else pthread_kill(threadId, SIGKILL);
  }
  void awaitThreadDeath() 
  {
    pthread_join(threadId, NULL);
  }
  static int maxthreads() { return _POSIX_THREAD_THREADS_MAX - 1; }
protected:
  JSimpleThread()
    {
      // Create a simple thread without actually starting it
    }
  pthread_t threadId;
};

// A smarter thread, styled after Java threads
class JThread: public JSimpleThread {
public:
  JThread() : JSimpleThread() {}
  virtual ~JThread() {}; // This will stop and destroy the running thread
  virtual void start() 
    {
      if (pthread_create(&threadId, pthread_attr_default, runFun, this))
	perror("JSimpleThread: error creating Posix thread");
    }
  virtual void run()
    {
      // User subclass provides a method here
    }
protected:
  static void* runFun(void* arg) { ((JThread*)arg)->run(); return NULL; }
};

#endif /* ifndef JTHREADS_H_INCLUDED */
