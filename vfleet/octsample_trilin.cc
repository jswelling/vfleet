/****************************************************************************
 * octsample_trilin.cc
 * Author Joel Welling
 * Copyright 1993, Pittsburgh Supercomputing Center, Carnegie Mellon University
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
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include "logger.h"
#include "rgbimage.h"
#include "imagehandler.h"
#include "vren.h"
#include "raycastvren.h"

void TrilinWalker::recalc_trilin( const int level_in, const int i_llf_in, 
				  const int j_llf_in, const int k_llf_in, 
				  const int i_trb_in, const int j_trb_in, 
				  const int k_trb_in, LightInfo& lights, 
				  const VRenOptions options,
				  const int debug)
{
  // Check for special case of revisiting old point
  if ((i_llf_in==i_llf_old) && (j_llf_in==j_llf_old) && (k_llf_in==k_llf_old)
      &&(i_trb_in==i_trb_old) && (j_trb_in==j_trb_old) && (k_trb_in==k_trb_old)
      && (level_in==level_old)) {
    gColor clr_tmp;
    
    level_old= level;
    i_llf_old= i_llf;
    j_llf_old= j_llf;
    k_llf_old= k_llf;
    i_trb_old= i_trb;
    j_trb_old= j_trb;
    k_trb_old= k_trb;

    level= level_in;
    i_llf= i_llf_in;
    j_llf= j_llf_in;
    k_llf= k_llf_in;
    i_trb= i_trb_in;
    j_trb= j_trb_in;
    k_trb= k_trb_in;

    clr_tmp= c000_old;
    c000_old= c000;
    c000= clr_tmp;
    clr_tmp= c001_old;
    c001_old= c001;
    c001= clr_tmp;
    clr_tmp= c010_old;
    c010_old= c010;
    c010= clr_tmp;
    clr_tmp= c011_old;
    c011_old= c011;
    c011= clr_tmp;
    clr_tmp= c100_old;
    c100_old= c100;
    c100= clr_tmp;
    clr_tmp= c101_old;
    c101_old= c101;
    c101= clr_tmp;
    clr_tmp= c110_old;
    c110_old= c110;
    c110= clr_tmp;
    clr_tmp= c111_old;
    c111_old= c111;
    c111= clr_tmp;

    return;
  }

  int recalc_000= 0;
  int recalc_100= 0;
  int recalc_010= 0;
  int recalc_110= 0;
  int recalc_001= 0;
  int recalc_101= 0;
  int recalc_011= 0;
  int recalc_111= 0;

  if ((level_in == level) 
      && (i_llf_in == i_llf) && (j_llf_in==j_llf) && (k_llf_in==k_llf)) {
	// frame shift 0 0 0 - all done (most common case)
  }
  else {
    c000_old= c000;
    c001_old= c001;
    c010_old= c010;
    c011_old= c011;
    c100_old= c100;
    c101_old= c101;
    c110_old= c110;
    c111_old= c111;
    level_old= level;
    i_llf_old= i_llf;
    j_llf_old= j_llf;
    k_llf_old= k_llf;
    i_trb_old= i_trb;
    j_trb_old= j_trb;
    k_trb_old= k_trb;

    if (level_in != level) {
      // new level; regen all points
      recalc_000= 1;
      recalc_100= 1;
      recalc_010= 1;
      recalc_110= 1;
      recalc_001= 1;
      recalc_101= 1;
      recalc_011= 1;
      recalc_111= 1;
    }
    else if (i_llf_in == i_llf) {
      // frame shift 0 ? ?
      if (j_llf_in == j_llf) {
	// frame shift 0 0 ?
	if (k_llf_in == k_llf) {
	  // frame shift 0 0 0 - all done (most common case)
	  // but since we caught this case above we should never hit this
	}
	else if (k_llf_in == k_trb) {
	  // frame shift 0 0 +
	  c000= c001;
	  c100= c101;
	  c010= c011;
	  c110= c111;
	  recalc_001= 1;
	  recalc_101= 1;
	  recalc_011= 1;
	  recalc_111= 1;
	}
	else if (k_trb_in == k_llf) {
	  // frame shift 0 0 -
	  c001= c000;
	  c101= c100;
	  c011= c010;
	  c111= c110;
	  recalc_000= 1;
	  recalc_100= 1;
	  recalc_010= 1;
	  recalc_110= 1;
	}
	else {
	  // shift out of frame; regen all points
	  recalc_000= 1;
	  recalc_100= 1;
	  recalc_010= 1;
	  recalc_110= 1;
	  recalc_001= 1;
	  recalc_101= 1;
	  recalc_011= 1;
	  recalc_111= 1;
	}
      }
      else if (j_llf_in == j_trb) {
	// frame shift 0 + ?
	if (k_llf_in == k_llf) {
	  // frame shift 0 + 0
	  c000= c010;
	  c100= c110;
	  c001= c011;
	  c101= c111;
	  recalc_010= 1;
	  recalc_110= 1;
	  recalc_011= 1;
	  recalc_111= 1;
	}
	else if (k_llf_in == k_trb) {
	  // frame shift 0 + +
	  c000= c011;
	  c100= c111;
	  recalc_010= 1;
	  recalc_110= 1;
	  recalc_001= 1;
	  recalc_101= 1;
	  recalc_011= 1;
	  recalc_111= 1;
	}
	else if (k_trb_in == k_llf) {
	  // frame shift 0 + -
	  c001= c010;
	  c101= c110;
	  recalc_000= 1;
	  recalc_100= 1;
	  recalc_010= 1;
	  recalc_110= 1;
	  recalc_011= 1;
	  recalc_111= 1;
	}
	else {
	  // shift out of frame; regen all points
	  recalc_000= 1;
	  recalc_100= 1;
	  recalc_010= 1;
	  recalc_110= 1;
	  recalc_001= 1;
	  recalc_101= 1;
	  recalc_011= 1;
	  recalc_111= 1;
	}
      }
      else if (j_trb_in == j_llf) {
	// frame shift 0 - ?
	if (k_llf_in == k_llf) {
	  // frame shift 0 - 0
	  c010= c000;
	  c110= c100;
	  c011= c001;
	  c111= c101;
	  recalc_000= 1;
	  recalc_100= 1;
	  recalc_001= 1;
	  recalc_101= 1;
	}
	else if (k_llf_in == k_trb) {
	  // frame shift 0 - +
	  c010= c001;
	  c110= c101;
	  recalc_000= 1;
	  recalc_100= 1;
	  recalc_001= 1;
	  recalc_101= 1;
	  recalc_011= 1;
	  recalc_111= 1;
	}
	else if (k_trb_in == k_llf) {
	  // frame shift 0 - -
	  c011= c000;
	  c111= c100;
	  recalc_000= 1;
	  recalc_100= 1;
	  recalc_010= 1;
	  recalc_110= 1;
	  recalc_001= 1;
	  recalc_101= 1;
	}
	else {
	  // shift out of frame; regen all points
	  recalc_000= 1;
	  recalc_100= 1;
	  recalc_010= 1;
	  recalc_110= 1;
	  recalc_001= 1;
	  recalc_101= 1;
	  recalc_011= 1;
	  recalc_111= 1;
	}
      }
      else {
	// shift out of frame; regen all points
	recalc_000= 1;
	recalc_100= 1;
	recalc_010= 1;
	recalc_110= 1;
	recalc_001= 1;
	recalc_101= 1;
	recalc_011= 1;
	recalc_111= 1;
      }
    }
    else if (i_llf_in == i_trb) {
      // frame shift + ? ?
      if (j_llf_in == j_llf) {
	// frame shift + 0 ?
	if (k_llf_in == k_llf) {
	  // frame shift + 0 0
	  c000= c100;
	  c010= c110;
	  c001= c101;
	  c011= c111;
	  recalc_100= 1;
	  recalc_110= 1;
	  recalc_101= 1;
	  recalc_111= 1;
	}
	else if (k_llf_in == k_trb) {
	  // frame shift + 0 +
	  c000= c101;
	  c010= c111;
	  recalc_100= 1;
	  recalc_110= 1;
	  recalc_001= 1;
	  recalc_101= 1;
	  recalc_011= 1;
	  recalc_111= 1;
	}
	else if (k_trb_in == k_llf) {
	  // frame shift + 0 -
	  c001= c100;
	  c011= c110;
	  recalc_000= 1;
	  recalc_100= 1;
	  recalc_010= 1;
	  recalc_110= 1;
	  recalc_101= 1;
	  recalc_111= 1;
	}
	else {
	  // shift out of frame; regen all points
	  recalc_000= 1;
	  recalc_100= 1;
	  recalc_010= 1;
	  recalc_110= 1;
	  recalc_001= 1;
	  recalc_101= 1;
	  recalc_011= 1;
	  recalc_111= 1;
	}
      }
      else if (j_llf_in == j_trb) {
	// frame shift + + ?
	if (k_llf_in == k_llf) {
	  // frame shift + + 0
	  c000= c110;
	  c001= c111;
	  recalc_100= 1;
	  recalc_010= 1;
	  recalc_110= 1;
	  recalc_101= 1;
	  recalc_011= 1;
	  recalc_111= 1;
	}
	else if (k_llf_in == k_trb) {
	  // frame shift + + +
	  c000= c111;
	  recalc_100= 1;
	  recalc_010= 1;
	  recalc_110= 1;
	  recalc_001= 1;
	  recalc_101= 1;
	  recalc_011= 1;
	  recalc_111= 1;
	}
	else if (k_trb_in == k_llf) {
	  // frame shift + + -
	  c001= c110;
	  recalc_000= 1;
	  recalc_100= 1;
	  recalc_010= 1;
	  recalc_110= 1;
	  recalc_101= 1;
	  recalc_011= 1;
	  recalc_111= 1;
	}
	else {
	  // shift out of frame; regen all points
	  recalc_000= 1;
	  recalc_100= 1;
	  recalc_010= 1;
	  recalc_110= 1;
	  recalc_001= 1;
	  recalc_101= 1;
	  recalc_011= 1;
	  recalc_111= 1;
	}
      }
      else if (j_trb_in == j_llf) {
	// frame shift + - ?
	if (k_llf_in == k_llf) {
	  // frame shift + - 0
	  c010= c100;
	  c011= c101;
	  recalc_000= 1;
	  recalc_100= 1;
	  recalc_110= 1;
	  recalc_001= 1;
	  recalc_101= 1;
	  recalc_111= 1;
	}
	else if (k_llf_in == k_trb) {
	  // frame shift + - +
	  c010= c101;
	  recalc_000= 1;
	  recalc_100= 1;
	  recalc_110= 1;
	  recalc_001= 1;
	  recalc_101= 1;
	  recalc_011= 1;
	  recalc_111= 1;
	}
	else if (k_trb_in == k_llf) {
	  // frame shift + - -
	  c011= c100;
	  recalc_000= 1;
	  recalc_100= 1;
	  recalc_010= 1;
	  recalc_110= 1;
	  recalc_001= 1;
	  recalc_101= 1;
	  recalc_111= 1;
	}
	else {
	  // shift out of frame; regen all points
	  recalc_000= 1;
	  recalc_100= 1;
	  recalc_010= 1;
	  recalc_110= 1;
	  recalc_001= 1;
	  recalc_101= 1;
	  recalc_011= 1;
	  recalc_111= 1;
	}
      }
      else {
	// shift out of frame; regen all points
	recalc_000= 1;
	recalc_100= 1;
	recalc_010= 1;
	recalc_110= 1;
	recalc_001= 1;
	recalc_101= 1;
	recalc_011= 1;
	recalc_111= 1;
      }
    }
    else if (i_trb_in == i_llf) {
      // frame shift - ? ?
      if (j_llf_in == j_llf) {
	// frame shift - 0 ?
	if (k_llf_in == k_llf) {
	  // frame shift - 0 0
	  c100= c000;
	  c110= c010;
	  c101= c001;
	  c111= c011;
	  recalc_000= 1;
	  recalc_010= 1;
	  recalc_001= 1;
	  recalc_011= 1;
	}
	else if (k_llf_in == k_trb) {
	  // frame shift - 0 +
	  c100= c001;
	  c110= c011;
	  recalc_000= 1;
	  recalc_010= 1;
	  recalc_001= 1;
	  recalc_101= 1;
	  recalc_011= 1;
	  recalc_111= 1;
	}
	else if (k_trb_in == k_llf) {
	  // frame shift - 0 -
	  c101= c000;
	  c111= c010;
	  recalc_000= 1;
	  recalc_100= 1;
	  recalc_010= 1;
	  recalc_110= 1;
	  recalc_001= 1;
	  recalc_011= 1;
	}
	else {
	  // shift out of frame; regen all points
	  recalc_000= 1;
	  recalc_100= 1;
	  recalc_010= 1;
	  recalc_110= 1;
	  recalc_001= 1;
	  recalc_101= 1;
	  recalc_011= 1;
	  recalc_111= 1;
	}
      }
      else if (j_llf_in == j_trb) {
	// frame shift - + ?
	if (k_llf_in == k_llf) {
	  // frame shift - + 0
	  c100= c010;
	  c101= c011;
	  recalc_000= 1;
	  recalc_010= 1;
	  recalc_110= 1;
	  recalc_001= 1;
	  recalc_011= 1;
	  recalc_111= 1;
	}
	else if (k_llf_in == k_trb) {
	  // frame shift - + +
	  c100= c011;
	  recalc_000= 1;
	  recalc_010= 1;
	  recalc_110= 1;
	  recalc_001= 1;
	  recalc_101= 1;
	  recalc_011= 1;
	  recalc_111= 1;
	}
	else if (k_trb_in == k_llf) {
	  // frame shift - + -
	  c101= c010;
	  recalc_000= 1;
	  recalc_100= 1;
	  recalc_010= 1;
	  recalc_110= 1;
	  recalc_001= 1;
	  recalc_011= 1;
	  recalc_111= 1;
	}
	else {
	  // shift out of frame; regen all points
	  recalc_000= 1;
	  recalc_100= 1;
	  recalc_010= 1;
	  recalc_110= 1;
	  recalc_001= 1;
	  recalc_101= 1;
	  recalc_011= 1;
	  recalc_111= 1;
	}
      }
      else if (j_trb_in == j_llf) {
	// frame shift - - ?
	if (k_llf_in == k_llf) {
	  // frame shift - - 0
	  c110= c000;
	  c111= c001;
	  recalc_000= 1;
	  recalc_100= 1;
	  recalc_010= 1;
	  recalc_001= 1;
	  recalc_101= 1;
	  recalc_011= 1;
	}
	else if (k_llf_in == k_trb) {
	  // frame shift - - +
	  c110= c001;
	  recalc_000= 1;
	  recalc_100= 1;
	  recalc_010= 1;
	  recalc_001= 1;
	  recalc_101= 1;
	  recalc_011= 1;
	  recalc_111= 1;
	}
	else if (k_trb_in == k_llf) {
	  // frame shift - - -
	  c111= c000;
	  recalc_000= 1;
	  recalc_100= 1;
	  recalc_010= 1;
	  recalc_110= 1;
	  recalc_001= 1;
	  recalc_101= 1;
	  recalc_011= 1;
	}
	else {
	  // shift out of frame; regen all points
	  recalc_000= 1;
	  recalc_100= 1;
	  recalc_010= 1;
	  recalc_110= 1;
	  recalc_001= 1;
	  recalc_101= 1;
	  recalc_011= 1;
	  recalc_111= 1;
	}
      }
      else {
	// shift out of frame; regen all points
	recalc_000= 1;
	recalc_100= 1;
	recalc_010= 1;
	recalc_110= 1;
	recalc_001= 1;
	recalc_101= 1;
	recalc_011= 1;
	recalc_111= 1;
      }
    }
    else {
      // shift out of frame; regen all points
      recalc_000= 1;    
      recalc_100= 1;
      recalc_010= 1;
      recalc_110= 1;
      recalc_001= 1;
      recalc_101= 1;
      recalc_011= 1;
      recalc_111= 1;
    }
  }

  if (recalc_000) {
    c000= sample_tree->get_cell(level_in, i_llf_in, j_llf_in, k_llf_in)
      ->calc_color_o0(lights, options, debug);
    c000.scale_by_alpha();
  }

  if (recalc_100) {
    c100= sample_tree->get_cell(level_in, i_trb_in, j_llf_in, k_llf_in)
      ->calc_color_o0(lights, options, debug);
    c100.scale_by_alpha();
  }

  if (recalc_010) {
    c010= sample_tree->get_cell(level_in, i_llf_in, j_trb_in, k_llf_in)
	->calc_color_o0(lights, options, debug);
    c010.scale_by_alpha();
  }

  if (recalc_110) {
    c110= sample_tree->get_cell(level_in, i_trb_in, j_trb_in, k_llf_in)
      ->calc_color_o0(lights, options, debug);
    c110.scale_by_alpha();
  }

  if (recalc_001) {
    c001= sample_tree->get_cell(level_in, i_llf_in, j_llf_in, k_trb_in)
      ->calc_color_o0(lights, options, debug);
    c001.scale_by_alpha();
  }

  if (recalc_101) {
    c101= sample_tree->get_cell(level_in, i_trb_in, j_llf_in, k_trb_in)
      ->calc_color_o0(lights, options, debug);
    c101.scale_by_alpha();
  }

  if (recalc_011) {
    c011= sample_tree->get_cell(level_in, i_llf_in, j_trb_in, k_trb_in)
	->calc_color_o0(lights, options, debug);
    c011.scale_by_alpha();
  }

  if (recalc_111) {
    c111= sample_tree->get_cell(level_in, i_trb_in, j_trb_in, k_trb_in)
	->calc_color_o0(lights, options, debug);
    c111.scale_by_alpha();
  }

  level= level_in;
  i_llf= i_llf_in;
  j_llf= j_llf_in;
  k_llf= k_llf_in;
  i_trb= i_trb_in;
  j_trb= j_trb_in;
  k_trb= k_trb_in;
}

gColor TrilinWalker::trilinear_color( const int level_new, const int i, 
				       const int j, const int k, 
				       const gVector& offset, 
				       LightInfo& lights,
				       const VRenOptions options,
				       const int debug)
{
  // This method returns color components scaled by alpha!
  gColor result;

  if (!level_new) { // Too shallow!
    result= sample_tree->get_cell(level_new,i,j,k)->clr;
    return result;
  }

  int imax, jmax, kmax;
  sample_tree->get_level_dims( level_new, &imax, &jmax, &kmax );
  imax--; // valid indices 0 to dim-1
  jmax--;
  kmax--;

  float offset_pos_x, offset_pos_y, offset_pos_z;
  int i_llf_new, i_trb_new, j_llf_new, j_trb_new, k_llf_new, k_trb_new;

  if (offset.x() < 0.0) {
    offset_pos_x= 1.0 + offset.x();
    if (i>0) {
      i_llf_new= i-1;
      i_trb_new= i;
    }
    else {
      i_llf_new= i_trb_new= i;
    }
  }
  else {
    offset_pos_x= offset.x();
    if (i<imax) {
      i_llf_new= i;
      i_trb_new= i+1;
    }
    else {
      i_llf_new= i_trb_new= i;
    }
  }

  if (offset.y() < 0.0) {
    offset_pos_y= 1.0 + offset.y();
    if (j>0) {
      j_llf_new= j-1;
      j_trb_new= j;
    }
    else {
      j_llf_new= j_trb_new= j;
    }
  }
  else {
    offset_pos_y= offset.y();
    if (j<jmax) {
      j_llf_new= j;
      j_trb_new= j+1;
    }
    else {
      j_llf_new= j_trb_new= j;
    }
  }

  if (offset.z() < 0.0) {
    offset_pos_z= 1.0 + offset.z();
    if (k>0) {
      k_llf_new= k-1;
      k_trb_new= k;
    }
    else {
      k_llf_new= k_trb_new= k;
    }
  }
  else {
    offset_pos_z= offset.z();
    if (k<kmax) {
      k_llf_new= k;
      k_trb_new= k+1;
    }
    else {
      k_llf_new= k_trb_new= k;
    }
  }

  if (debug) {
    fprintf( stderr,
	    "trilin color: cell (%d %d %d)(%d %d %d), offsets %f %f %f\n",
	    i_llf_new, j_llf_new, k_llf_new, i_trb_new, j_trb_new, k_trb_new,
	    offset_pos_x,offset_pos_y,offset_pos_z);
  }

  if ((i_llf_new != i_llf) || (j_llf_new != j_llf) || (k_llf_new != k_llf)
      || (i_trb_new != i_trb) || (j_trb_new != j_trb) || (k_trb_new != k_trb)
      || level_new != level)
    recalc_trilin( level_new, i_llf_new, j_llf_new, k_llf_new, 
		   i_trb_new, j_trb_new, k_trb_new,
		   lights, options, debug );

  gColor cx00 = c100;
  cx00.subtract_noclamp( c000 );
  cx00.mult_noclamp( offset_pos_x );
  cx00.add_noclamp( c000 );

  gColor cx01 = c101;
  cx01.subtract_noclamp( c001 );
  cx01.mult_noclamp( offset_pos_x );
  cx01.add_noclamp( c001 );

  gColor cx10 = c110;
  cx10.subtract_noclamp( c010 );
  cx10.mult_noclamp( offset_pos_x );
  cx10.add_noclamp( c010 );

  gColor cx11 = c111;
  cx11.subtract_noclamp( c011 );
  cx11.mult_noclamp( offset_pos_x );
  cx11.add_noclamp( c011 );

  gColor cxy0= cx10;
  cxy0.subtract_noclamp( cx00 );
  cxy0.mult_noclamp( offset_pos_y );
  cxy0.add_noclamp( cx00 );

  gColor cxy1= cx11;
  cxy1.subtract_noclamp( cx01 );
  cxy1.mult_noclamp( offset_pos_y );
  cxy1.add_noclamp( cx01 );

  result= cxy1;
  result.subtract_noclamp( cxy0 );
  result.mult_noclamp( offset_pos_z );
  result.add_noclamp( cxy0 );
  result.clamp();

  if (debug) {
#ifdef never
    fprintf(stderr,"trilin color c000: ( %f %f %f %f )\n",
	    c000.r(),c000.g(),c000.b(),c000.a());
    fprintf(stderr,"             c001: ( %f %f %f %f )\n",
	    c001.r(),c001.g(),c001.b(),c001.a());
    fprintf(stderr,"             c010: ( %f %f %f %f )\n",
	    c010.r(),c010.g(),c010.b(),c010.a());
    fprintf(stderr,"             c011: ( %f %f %f %f )\n",
	    c011.r(),c011.g(),c011.b(),c011.a());
    fprintf(stderr,"             c100: ( %f %f %f %f )\n",
	    c100.r(),c100.g(),c100.b(),c100.a());
    fprintf(stderr,"             c101: ( %f %f %f %f )\n",
	    c101.r(),c101.g(),c101.b(),c101.a());
    fprintf(stderr,"             c110: ( %f %f %f %f )\n",
	    c110.r(),c110.g(),c110.b(),c110.a());
    fprintf(stderr,"             c111: ( %f %f %f %f )\n",
	    c111.r(),c111.g(),c111.b(),c111.a());
#endif
    fprintf(stderr,"trilin color returning (%f %f %f %f)\n",
	    result.r(),result.g(),result.b(),result.a());
  }
  return result;
}

void octreeSampleVolume::step_trilin( gColor* clr, const int level,
				     const int i, const int j, const int k,
				     LightInfo& lights, 
				     const VRenOptions options,
				     const gVector& start, const gVector& end,
				     const float step_dist, 
				     TrilinWalker& twalker,
				     const int debug )
{
  gVector midway= (start + end) * 0.5;

  gColor clr_midway= twalker.trilinear_color( level, i, j, k, midway, 
					      lights, options, debug );
  
  float alpha_midway= (( clr->a() + 0.5*(clr_midway.a()*step_dist) )
		       /(1.0 + 0.5*(clr_midway.a()*step_dist)));
  float alpha_dif= 1.0 - alpha_midway;

  // clr_midway is already scaled by alpha
  clr_midway.mult_noclamp( alpha_dif*step_dist );
  clr->add_noclamp( clr_midway );
}

void octreeSampleVolume::step_trilin_subdiv( gColor* clr, const int level,
					    const int i, const int j,
					    const int k, LightInfo& lights,
					    const VRenOptions options,
					    const gVector& start,
					    const gVector& end, 
					    const float step_dist,
					    const gColor& prev_guess,
					    const int recur_depth,
					    const float tol,
					    TrilinWalker& twalker,
					    const int debug )
{
  const int MAX_RECUR= 5;

  gVector midway= (start + end) * 0.5;

  gColor better_clr= *clr;
  step_trilin( &better_clr, level, i, j, k, lights, options,
	      start, midway, 0.5*step_dist, twalker, debug );
  gColor next_guess= better_clr;
  step_trilin( &better_clr, level, i, j, k, lights, options,
	      midway, end, 0.5*step_dist, twalker, debug );
  
  float accuracy_test= ((better_clr.a() - clr->a()) == 0.0) ?
    0.0 : ( fabs(better_clr.a() - prev_guess.a()) 
	   / fabs( better_clr.a() - clr->a() ) );
  if (debug) fprintf(stderr,
		     "trilin integration: test %f < %f, depth %d of %d\n",
		     accuracy_test, tol, recur_depth, MAX_RECUR);
  if ( (accuracy_test < tol) || (recur_depth >= MAX_RECUR) ) {
    *clr= better_clr;
    return;
  }
  else {
    step_trilin_subdiv( clr, level, i, j, k, lights, options,
		       start, midway, 0.5*step_dist, 
		       next_guess, recur_depth+1, tol, twalker, debug );
    next_guess= *clr;
    step_trilin( &next_guess, level, i, j, k, lights, options,
		midway, end, 0.5*step_dist, twalker, debug );
    
    // Was all the action in the first half step?
    accuracy_test= ((next_guess.a() - clr->a()) == 0.0) ?
      0.0 : ( fabs(next_guess.a() - better_clr.a()) 
	     / fabs( next_guess.a() - clr->a() ) );
    if (debug) fprintf(stderr,
		       "                  test 2 %f < %f, depth %d of %d\n",
		       accuracy_test, tol, recur_depth, MAX_RECUR);
    if (accuracy_test < tol) {
      *clr= next_guess;
      return;
    }
    else step_trilin_subdiv( clr, level, i, j, k, lights, options,
			    midway, end, 0.5*step_dist,
			    next_guess, recur_depth+1, tol, twalker, debug );
  }
}

void octreeSampleVolume::integrate_cell_trilin( gColor& clr, 
				  Octcell_intersect_iter<octSample>& iter,
						LightInfo& lights,
						const VRenOptions  options,
						const float dist, 
						const float tol,
						TrilinWalker& twalker,
						const int debug)
{
  gVector scaled_entry= iter.get_cell_entry_scaled();
  gVector scaled_exit= iter.get_cell_exit_scaled();

  // Sometimes the ray just touches the cell...
  if (scaled_entry == scaled_exit) {
    return;
  }

  int i, j, k;
  iter.get_indices( &i, &j, &k );
  int level= iter.get_level();

  gColor guess_color= clr;
  step_trilin( &guess_color, level, i, j, k, lights, options, 
	      scaled_entry, scaled_exit, dist, twalker, debug );
  step_trilin_subdiv( &clr, level, i, j, k, lights, options,
		     scaled_entry, scaled_exit, dist, 
		     guess_color, 0, tol, twalker, debug );
  clr.clamp(); // watch out for integration errors
}

void octreeSampleVolume::walk_one_ray_trilin(Ray& ray, LightInfo& lights,
				      const QualityMeasure& required_qual,
				      const VRenOptions options,
				      Octcell_intersect_iter<octSample>& iter)
{
  TrilinWalker twalker(sample_tree);
  octSample *next_cell;
  const float MIN_TOL= 0.001;
  float tol= required_qual.get_color_comp_error() + MIN_TOL;
  if (ray.debug_me) {
    while (next_cell= iter.next(ray.debug_me)) {
      fprintf(stderr,
	      "walk_one_ray_trilin level %d length= %f, clr= %f %f %f %f\n",
	      iter.get_level(),
	      ray.length,
	      next_cell->clr.r(),next_cell->clr.g(),next_cell->clr.b(),
	      next_cell->clr.a());
      
      // Escape if opaque enough, or if we have gone far enough.
      if ( !(required_qual.opacity_test(ray.clr.a())) ) {
	ray.rescale_me= 1;
	break;
      }
      if ( ray.length >= ray.termination_length )
	break;
      
      // Use order zero shading if accurate enough, or even skip the
      // volume if it is effectively transparent
      if (next_cell->clr_error()
	   <= (1.0-ray.clr.a())*required_qual.get_color_comp_ierr()) {
	// This level is precise enough
	float dist= iter.dist_in_cell();
	if (next_cell->clr.ia() 
	    && (next_cell->clr.ia() >= required_qual.get_opacity_min())) {
	  integrate_cell_o0( ray, next_cell->calc_color_o0(lights, options,
							   ray.debug_me),
			     size_scale*dist );
	  //      ray.qual.update_with_greater(QualityMeasure(0,next_cell->clr_error()));
	ray.length += dist;
	fprintf(stderr, 
		"Ray color becomes (%f %f %f %f) over dist %f (o0 step)\n",
		ray.clr.r(),ray.clr.g(),
		ray.clr.b(),ray.clr.a(), dist);
	}
	else {
	ray.length += dist;
	fprintf(stderr, 
		"Ray color becomes (%f %f %f %f) over dist %f (skip cell)\n",
		ray.clr.r(),ray.clr.g(),
		ray.clr.b(),ray.clr.a(), dist);
	}
      }

      // If we have hit max refinement and error criterion still isn't
      // satisfied, use trilinear interpolation
      else if ( iter.get_level() == sample_tree->get_nlevels() ) {
	float dist= iter.dist_in_cell();
	integrate_cell_trilin( ray.clr, iter, lights, options, 
			      size_scale*dist, tol, twalker, ray.debug_me );
//      ray.qual.update_with_greater(required_qual);
	ray.length += dist;

	fprintf(stderr, 
		"Ray color becomes (%f %f %f %f) over dist %f (trilin)\n",
		ray.clr.r(),ray.clr.g(),
		ray.clr.b(),ray.clr.a(), dist);
      }

      else { // Need to subdivide.
	iter.push();
      }

    }
  }
  else { // Faster version of same thing, without passing debug param
    while (next_cell= iter.next()) {
      // Escape if opaque enough, or if we have gone far enough.
      if ( !(required_qual.opacity_test(ray.clr.a())) ) {
	ray.rescale_me= 1;
	break;
      }
      if ( ray.length >= ray.termination_length )
	break;
      
      // Use order zero shading if accurate enough, or even skip the
      // volume if it is effectively transparent
      if (next_cell->clr_error()
	   <= (1.0-ray.clr.a())*required_qual.get_color_comp_ierr()) {
	// This level is precise enough
	float dist= iter.dist_in_cell();
	if (next_cell->clr.ia() 
	    && (next_cell->clr.ia() >= required_qual.get_opacity_min())) {
	  integrate_cell_o0( ray, next_cell->calc_color_o0(lights, options),
			     size_scale*dist );
	  //      ray.qual.update_with_greater(QualityMeasure(0,next_cell->clr_error()));
	}
	ray.length += dist;
      }

      // If we have hit max refinement and error criterion still isn't
      // satisfied, use trilinear interpolation
      else if ( iter.get_level() == sample_tree->get_nlevels() ) {
	float dist= iter.dist_in_cell();
	integrate_cell_trilin( ray.clr, iter, lights, options, 
			      size_scale*dist, tol, twalker );
//      ray.qual.update_with_greater(required_qual);
	ray.length += dist;
      }

      else { // Need to subdivide.
	iter.push();
      }
    }
  }
}

void octreeSampleVolume::walk_one_ray_trilin_mm(Ray& ray, LightInfo& lights,
				      const QualityMeasure& required_qual,
				      const VRenOptions options,
				      Octcell_intersect_iter<octSample>& iter)
{
  TrilinWalker twalker_lower(sample_tree);
  TrilinWalker twalker_upper(sample_tree);
  octSample *next_cell;
  const float MIN_TOL= 0.001;
  float tol= required_qual.get_color_comp_error() + MIN_TOL;

  int mipmap_min_level= sample_tree->get_nlevels();
  int mipmap_max_level;
  float mipmap_frac;
  float mipmap_dist_rescale;
  int mipmap_flag;
  gColor mipmap_upper_color;
  float mipmap_upper_dist_inv= 1.0;

  float mipmap_level_sep= voxel_mean_size;
  float ray_sep= ray.initial_separation + ray.length*ray.divergence;
  if (ray_sep <= mipmap_level_sep) {
    // voxels too big;  turn mipmapping off
    mipmap_max_level= mipmap_min_level;
    mipmap_dist_rescale= 0.0;
    mipmap_frac= 0.0;
    mipmap_flag= 0;
  }
  else {
    while ((mipmap_level_sep < ray_sep) && (mipmap_min_level > 0)) {
      mipmap_min_level -= 1;
      mipmap_level_sep *= 2;
    }
    mipmap_max_level= mipmap_min_level + 1;
    mipmap_frac= 2.0*( 1.0 - (ray_sep/mipmap_level_sep));
    mipmap_dist_rescale= 2.0*ray.divergence/mipmap_level_sep;
    mipmap_flag= 1;
  }

  if (!mipmap_flag) {
    // no need for this
    walk_one_ray_trilin(ray, lights, required_qual, options, iter);
  }
  else {
    if (ray.debug_me) {
      fprintf(stderr,
	   "walk_one_ray_trilin_mm mipmap setup:  min and max levels %d %d\n",
	      mipmap_min_level, mipmap_max_level);
      fprintf(stderr,
	      "                              frac= %f, dist_rescale= %f\n",
	      mipmap_frac,mipmap_dist_rescale);
      
      while (next_cell= iter.next(ray.debug_me)) {
	fprintf(stderr,
	     "walk_one_ray_trilin_mm level %d length= %f, clr= %f %f %f %f\n",
		iter.get_level(),
		ray.length,
		next_cell->clr.r(),next_cell->clr.g(),next_cell->clr.b(),
		next_cell->clr.a());
	fprintf(stderr,"                       mipmap_frac= %f\n",mipmap_frac);
      
	// Escape if opaque enough, or if we have gone far enough.
	if ( !(required_qual.opacity_test(ray.clr.a())) ) {
	  ray.rescale_me= 1;
	  break;
	}
	if ( ray.length >= ray.termination_length )
	  break;
	
	// Use order zero shading if accurate enough, or even skip the
	// volume (remembering upper mipmap contrib) if it is effectively 
	// transparent
	if (next_cell->clr_error()
	    <= (1.0-ray.clr.a())*required_qual.get_color_comp_ierr()) {
	  // This level is precise enough
	  
	  float dist= iter.dist_in_cell();
	  mipmap_frac += dist * mipmap_dist_rescale;
	  if (iter.get_level() == mipmap_max_level) {
	    gColor mipmap_color= mipmap_upper_color;
	    mipmap_color.mult_noclamp( mipmap_frac );
	    if (next_cell->clr.ia() 
		&& (next_cell->clr.ia() >= required_qual.get_opacity_min())) {
	      gColor mipmap_color_lower= 
		next_cell->calc_color_o0( lights, options, ray.debug_me );
	      mipmap_color_lower.mult_noclamp( 1.0 - mipmap_frac );
	      mipmap_color.add_noclamp( mipmap_color_lower );
	    }
	    mipmap_color.clamp();
	    integrate_cell_o0( ray, mipmap_color, size_scale*dist );
	    fprintf(stderr, 
		"Ray color becomes (%f %f %f %f) over dist %f (mm o0 step)\n",
		    ray.clr.r(),ray.clr.g(),
		    ray.clr.b(),ray.clr.a(), dist);
	  }
	  else if (next_cell->clr.ia() 
		   && (next_cell->clr.ia() 
		       >= required_qual.get_opacity_min())) {
	    integrate_cell_o0( ray, 
			       next_cell->calc_color_o0(lights, options,
							ray.debug_me),
			       size_scale*dist );
	    fprintf(stderr, 
		    "Ray color becomes (%f %f %f %f) over dist %f (o0 step)\n",
		    ray.clr.r(),ray.clr.g(),
		    ray.clr.b(),ray.clr.a(), dist);
	  }
	  else {
	    fprintf(stderr, 
		    "Ray color becomes (%f %f %f %f) over dist %f (skip)\n",
		    ray.clr.r(),ray.clr.g(),
		    ray.clr.b(),ray.clr.a(), dist);
	  }
//    ray.qual.update_with_greater(QualityMeasure(0,next_cell->clr_error()));
	  ray.length += dist;
	}
	
	// If we have hit max refinement without satisfying error criterion,
	// trilinear interpolation is used.
	else if ( iter.get_level() == mipmap_max_level ) {
	  float dist= iter.dist_in_cell();
	  mipmap_frac += dist * mipmap_dist_rescale;
	  gColor mipmap_lower_color= ray.clr;
	  integrate_cell_trilin( mipmap_lower_color, iter, lights, options,
				 size_scale*dist, tol, twalker_lower,
				 ray.debug_me );
	  mipmap_lower_color.subtract_noclamp( ray.clr );
	  mipmap_lower_color.mult_noclamp( 1.0 - mipmap_frac );
	  gColor mipmap_color= mipmap_upper_color;
	  mipmap_color.mult_noclamp( mipmap_frac
				     * dist * mipmap_upper_dist_inv );
	  mipmap_color.add_noclamp( mipmap_lower_color );
	  ray.clr += mipmap_color;
//        ray.qual.update_with_greater(required_qual);
	  ray.length += dist;
	  
	  fprintf(stderr, 
		  "Ray color becomes (%f %f %f %f) over dist %f (mm trilin)\n",
		  ray.clr.r(),ray.clr.g(),
		  ray.clr.b(),ray.clr.a(), dist);
	}
	
	else { // Need to subdivide.
	  if (iter.get_level() == mipmap_min_level) {
	    // Save color contrib at this level for later mipmapping
	    float dist= iter.dist_in_cell();
	    mipmap_upper_dist_inv= (1.0/dist);
	    mipmap_upper_color= ray.clr;
	    integrate_cell_trilin( mipmap_upper_color, iter, lights, options,
				   size_scale*dist, tol, twalker_upper, 
				   ray.debug_me );
	    mipmap_upper_color.subtract_noclamp( ray.clr );
	  }
	  iter.push();
	}
	if ((mipmap_frac > 1.0) && (iter.get_level() != mipmap_max_level)) {
	  // Rays have diverged; move up mipmap level
	  mipmap_frac= 0.5*(mipmap_frac - 1.0);
	  mipmap_dist_rescale *= 0.5;
	  mipmap_upper_color= gColor(); // OK since mipmap_frac approx. 0.0
	  mipmap_upper_dist_inv= 1.0; 
	  if (mipmap_min_level) {
	    mipmap_min_level--;
	    mipmap_max_level--;
	  }
	  fprintf(stderr,
     "walk_one_ray_trilin_mm step up: levels %d %d, frac %f, rescale %f\n",
		  mipmap_min_level, mipmap_max_level, mipmap_frac, 
		  mipmap_dist_rescale);
	}
      }
    }
    else { // Faster version of same thing, without passing debug param

      while (next_cell= iter.next()) {
      
	// Escape if opaque enough, or if we have gone far enough.
	if ( !(required_qual.opacity_test(ray.clr.a())) ) {
	  ray.rescale_me= 1;
	  break;
	}
	if ( ray.length >= ray.termination_length )
	  break;
	
	// Use order zero shading if accurate enough, or even skip the
	// volume (remembering upper mipmap contrib) if it is effectively 
	// transparent
	if (next_cell->clr_error()
	    <= (1.0-ray.clr.a())*required_qual.get_color_comp_ierr()) {
	  // This level is precise enough
	  
	  float dist= iter.dist_in_cell();
	  mipmap_frac += dist * mipmap_dist_rescale;
	  if (iter.get_level() == mipmap_max_level) {
	    gColor mipmap_color= mipmap_upper_color;
	    mipmap_color.mult_noclamp( mipmap_frac );
	    if (next_cell->clr.ia() 
		&& (next_cell->clr.ia() >= required_qual.get_opacity_min())) {
	      gColor mipmap_color_lower= 
		next_cell->calc_color_o0( lights, options );
	      mipmap_color_lower.mult_noclamp( 1.0 - mipmap_frac );
	      mipmap_color.add_noclamp( mipmap_color_lower );
	    }
	    mipmap_color.clamp();
	    integrate_cell_o0( ray, mipmap_color, size_scale*dist );
	  }
	  else if (next_cell->clr.ia() 
		   && (next_cell->clr.ia() 
		       >= required_qual.get_opacity_min())) {
	    integrate_cell_o0( ray, 
			       next_cell->calc_color_o0(lights, options),
			       size_scale*dist );
	  }
//    ray.qual.update_with_greater(QualityMeasure(0,next_cell->clr_error()));
	  ray.length += dist;
	}
	
	// If we have hit max refinement without satisfying error criterion,
	// trilinear interpolation is used.
	else if ( iter.get_level() == mipmap_max_level ) {
	  float dist= iter.dist_in_cell();
	  mipmap_frac += dist * mipmap_dist_rescale;
	  gColor mipmap_lower_color= ray.clr;
	  integrate_cell_trilin( mipmap_lower_color, iter, lights, options,
				 size_scale*dist, tol, twalker_lower );
	  mipmap_lower_color.subtract_noclamp( ray.clr );
	  mipmap_lower_color.mult_noclamp( 1.0 - mipmap_frac );
	  gColor mipmap_color= mipmap_upper_color;
	  mipmap_color.mult_noclamp( mipmap_frac
				     * dist * mipmap_upper_dist_inv );
	  mipmap_color.add_noclamp( mipmap_lower_color );
	  ray.clr += mipmap_color;
//        ray.qual.update_with_greater(required_qual);
	  ray.length += dist;
	}
	
	else { // Need to subdivide.
	  if (iter.get_level() == mipmap_min_level) {
	    // Save color contrib at this level for later mipmapping
	    float dist= iter.dist_in_cell();
	    mipmap_upper_dist_inv= (1.0/dist);
	    mipmap_upper_color= ray.clr;
	    integrate_cell_trilin( mipmap_upper_color, iter, lights, options,
				   size_scale*dist, tol, twalker_upper );
	    mipmap_upper_color.subtract_noclamp( ray.clr );
	  }
	  iter.push();
	}
	if ((mipmap_frac > 1.0) && (iter.get_level() != mipmap_max_level)) {
	  // Rays have diverged; move up mipmap level
	  mipmap_frac= 0.5*(mipmap_frac - 1.0);
	  mipmap_dist_rescale *= 0.5;
	  mipmap_upper_color= gColor(); // OK since mipmap_frac approx. 0.0
	  mipmap_upper_dist_inv= 1.0; 
	  if (mipmap_min_level) {
	    mipmap_min_level--;
	    mipmap_max_level--;
	  }
	}
      }
    }
  }
}

