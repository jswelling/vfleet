
-I think I should be using rgbimage.cc rather than rgbimage_dumb.cc
-Why is xlogserver failing?

Beta testers:
Bob Kozdema      koz@sgi.com                   SGI machines
John Dubinski    dubindki@lick.ucsc.edu      DECstation 5000's and alphas
Dennis Gannon    gannon@cs.indiana.edu         SGI machines
Frank Summers    summers@astro.princeton.edu   SGI machines, Suns
Greg Byran       gbyran@ncsa.uiuc.edu          HP cluster
Mike Norman      norman@ncsa.uiuc.edu          SGI Power Challenge, CM5
Randy Heiland    heiland@bigsky.pnl.gov        ?
Polly Baker      baker@ncsa.uiuc.edu           SGI
Paul 'Shag' Walmsley ccshag@cclabs.missouri.edu	SGI
Hans-Christian Hege hege@zib-berlin.de         T3D
Peggy Li peggy@sun11.jpl.nasa.gov developing   T3D volume
	renderer at NASA JPL.

Interested parties:
Thomas Kelleher  kelleher@phoenix.ocf.llnl.gov MIEKO CS-2?
Rick Avila       volvis@cs.sunysb.edu          ?
Andrews Chan     CHAN@civil.gla.ac.uk          ?

Mailing list applicants:
Bernard Cena     bernard@cs.uwa.edu.au         DEC Alpha
	medical vis. from ultrasound, U. of West Australia
Chris Eddington  chris@einstein.jpl.nasa.gov   HP715, SUN Sparc
	Jet Propulsion Laboratory
Vinod Nair       vtn@hpcf.cc.utexas.edu        Cray Y-MP, Intel iPSC
	High Performance Computing Facility, U. Texas at Austin Comp. Facility
C. Robert Appledorn appldorn@foyt.indyrad.iupui.edu HP machines
	Dept of Radiology / Imaging Sciences
	Indiana University School of Medicine
	Indianapolis, Indiana, USA
John Holliman	holliman@astron.berkeley.edu
Alexander-James Annala merlin@neuro.usc.edu
	Principal Investigator
	Neuroscience Image Analysis Network
	HEDCO Neuroscience Building, Fifth Floor
	University of Southern California
	University Park
	Los Angeles, CA 90089-2520
	Telephone:  (213)740-3406
	FAX:        (213)740-5687
Stephen Aylward aylward@cs.unc.edu
Fernando Martins martins@robin.ece.cmu.edu
Hans-Christian Hege hege@zib-berlin.de 
	Konrad-Zuse-Zentrum fuer Informationstechnik (ZIB) 
	Visualization and Parallel Computing 
	Heilbronner St. 10 
	D-10711 Berlin, Germany
Brad Pillow bpillow@pillowsoft.com
	Pillowsoft Inc.
	11715 Fox Rd., Suite 400-120
	Indianapolis, IN 46236
	Phone: 317-823-8756, Fax: 317-823-5988
	(commercial developer of Mac applications)
Ilya A Erofeev ilya@bug.pccentre.msk.su
	DOS or NT version of VFleet?
Jacob Gotwals jgotwals@cs.indiana.edu
	Dennis Gannon's group;  working with VFleet and pC++ .
Roberto Gomez (roberto@artemis.phyast.pitt.edu)
	Pitt relativity and Black Hole Coalescence Grand Challenge
Dieter Hartmann (dieter@informatik.uni-siegen.de)
	"parallel systems" group at the University of Siegen, Germany
Peggy Li peggy@sun11.jpl.nasa.gov developing T3D volume
	renderer at NASA JPL.
Tim Raymund traymund@arlut.utexas.edu Ionospheric electron density
	on a cluster of HPs.
Juhana Kallio, Postintie 1 19770 Valittula, jkallio@sci.fi Finland 918-177384

Document notes:
-Add documentation of camera control dialog.
-Add documentation of mask and block tfuns.
-Add documentation of new script elements.
-Add documentation of new command line options.
-Update to bytestohdf doc to describe -s option
-Update to floatstohdf doc to describe reading Fortran files.  Note that
 axes get flipped!
-Write the tutorial!
-More text is needed describing the settings and meanings of rendering
 options.

Feature requests:
-Add a facility for generating a histogram of datasets, to aid in tfun design.
 (Greg Foss)
-"It'd be a nice thing to be able to enter colors by number/window box/slider;
  the mouse accessed color map is somewhat frustrating".
-The documentation on the tcl function replace_data is ambiguous about the
 meaning of "index" and what it refers to.  More script examples would clear
 this up. (Greg Foss).
-Include some transfer functions to start with. (Greg Foss)
-Be able to run in batch mode and from a windowless machine. (Greg Foss)
-Be able to map one of the walls of the bounding box with a 2d picture.
 (Greg Foss).

Known bugs:
-Under Solaris, HDF include file vproto.h uses the word "class" on line
 931 or somewhere, which causes CC to choke even if it is within
 extern "C" {}.
-For some reason, DEC Make can't follow the rule for rgbimage_dumb.o .
 Work-around for this is to use "make -c".
-Should be able to set quality dialog values from scripts
-Rotation should be about lookat point, even if that point has moved.
-At high magnifications, trilin interp shows octree atrifacts.
-Hither and yon distances should be planes, not spheres.
-Be able to read HDF datasets after the first
-Be able to reset volume dimensions
-When you render a block, the top surface is not uniformly lighted 
 (goes away in trilin mode; block has to be large enough).  Due to failing
 to check normal vector accuracy criterion?
-maybe use Blinn's methods in geometry.h and rgbimage.cc?  (byte math)
-update copyright notices
-If the highest resolution grid is initially too fine for mipmapping, 
 mipmapping turns off for the ray and doesn't turn back on even if the
 rays diverge enough to warrant it.  Unlikely, but it could happen.
-The mipmap algorithm doesn't take into account non-unity aspect ratio for
 voxels.
-Should we be dithering on averaging, as per Danskin and Hanrahan 1992,
 section 6, paragraph 2?
-The averaging and error pyramid calcs could be accelerated by reusing
 partial results from adjacent cells.
-The "rough_dist_in_cell()" algorithm is wrong, because V_compare is not
 consistently proportional to unity_scale.
-The compositing facilities in rgbImage could use some optimization for
 cache management.
-crashes when running from scripts on T3D?
-Does the floatstohdf mod for reading binary files produced by Fortran
 work on all systems?  Document the new feature, including the fact that
 the coordinate system gets flipped when reading Fortran files.
-When a tcl script that does not actually save a file is run (it must
 contain a loop), a crash results.
-num_text_check_cb in vfleet.cc can be fooled if you try hard enough.
-T3D restrictions: service manager must be on C90, logging server 
 somewhere other than T3D, all renderers in same process block on T3D.

raycastVRen design:

-Note that if you use rgbImage::clear() on a compressed image, only
 the non-black pixels get cleared to the given color.
-Should use exceptions instead of exit(-1).

Functionality tricks:
-Sample::calc_values is relying on knowledge of the meaning of the
bytes in gBVector and gBColor.

Timing tests 6/8/95, on Supercluster (alpha hosts) (running timing2.tcl)

Set 1:
Dataset: c60_homo_256.hdf
tfun: bucky_iso.tfn

Set 2:
Dataset: engine_128.hdf
tfun: sample.tfn
opacity scaled to 10

Set 3:
Dataset: mdm02rho.hdf
tfun:	mdm02rho_lighted.tfn

NPROCS/NPES	WC_set_1	WC_set_2	WC_set_3

1/1(cpu time)	********	27.8,27.8,27.8

2/2		18,18,18	18,18,18

4/4		13,13,13	12,12,13

8/8		17,16,15	8,9,9

16/8		15,14,14	9,9,9

32/8		15,15,15	10,10,10


Similar tests on T3D 6/26/95 (running timing2.tcl)

NPROCS		WC_set_1	WC_set_2	WC_set_3

1/1(cpu time)	******		34.4,34.3,34.3	64.7,64.7,64.7

2/2		******		21.6,21.6,21.5	39.2,39.2,39.2

4/4		******		14.7,14.6,14.6	33.8,33.8,33.8

8/8		11.2,11.1,11.1	7.9,7.8,7.8	17.9,17.9,18.0

16/16		7.3,7.3,7.3	5.1,5.1,5.1	10.7,10.9,10.8

32/32		6.0,6.0,6.0	4.6,4.6,4.6	7.2,7.2,7.2

64/64		4.1,4.0,4.0	3.3,3.3,3.3	5.0,4.9,4.9

Timing note on the T3D: at 64 PEs for the c60_homo_256 dataset and
bucky_iso.tfn (w/ timing.tcl script), compositing time seems to be
about 0.5 sec. on PE 0, 0.01 second of which is data communication time.
(There may be a lot more data communication time when the clock is not
running).

all_white.tfn at opacity scale 10 seems to take about 2.6 sec. to render
at 64 PEs (dataset is engine_128)

Re-timings 6/30/95, 7/3/95 f/WC_set_2, instrumented

NPROCS	CPU	WC	ave. ray	min, max ray 	PE 0	PE 0
			cast time	cast time	wait	comp

1	30.14	***	30.14		30.14,30.14	0.0	0.0
2	19.46	20.49	16.48		13.89,19.07	0.0	0.39
4	12.49	13.20	10.03		8.26,12.09	0.0	0.40
8	8.04	8.55	5.67		4.19,7.67	1.24	0.42
16	4.83	5.15	3.08		2.26,4.28	0.36	0.45
32	3.90	4.24	2.08		1.50,3.26	1.42	0.45
64	2.51	2.83	1.15		0.73,1.81	1.05	0.50

CPU= ray cast + comp + wait

on PE 0 at 64 PEs, for a separate run on 7/5/95,

2.37 CPU 2.71 WC (average render 1.15 sec; min 0.73, max 1.85)

0.98 sec. ray cast
0.46 sec. comp
0.93 sec. wait
----
2.37 sec. total CPU

Load balanced, it would be:

1.15 sec. ray cast (average value)
0.46 sec. comp
0.00 sec. wait
----
1.61 sec. total CPU (32% speedup, 0.67 times orig. run time)

Parallelize compositing, it would be:

0.98 sec. ray cast
0.00 sec. comp
0.93 sec. wait
----
1.91 sec. total CPU (19% speedup, 0.81 times orig. run time)

Load balance and parallelize compositing, it would be:

1.15 sec. ray cast 
0.00 sec. comp
0.00 sec. wait
----
1.15 sec. total CPU (51% speedup, 0.49 times orig. run time)



