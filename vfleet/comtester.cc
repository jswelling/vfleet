/****************************************************************************
 * comtester.cc
 * Author Joel Welling
 * Copyright 1994, Pittsburgh Supercomputing Center, Carnegie Mellon University
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
#include <stdlib.h>
#include "logger.h"
#include "basenet.h"
#include "netcomtest.h"
#include "servman.h"

main( int argc, char *argv[] )
{
  fprintf(stderr,"comtester begins\n");

  netComTest::initialize( argv[0] );

  netComTest** test_table= new netComTest*[10];
  int i;

  for (i=0; i<10; i++) test_table[i]= new netComTest();

  for (i=0; i<100; i++) test_table[i%10]->doit(i);

  for (i=0; i<10; i++) delete test_table[i];

  delete [] test_table;

  fprintf(stderr,"comtester ends\n");
}
