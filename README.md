
               VFleet Distributed Volume Renderer
                          Version 1.1
              Release Notes and Installation Guide


  VFleet is a volume rendering system for Unix platforms.  It can run
either locally or in distributed mode, using PVM to distribute the
volume rendering task across multiple platforms.  VFleet is a software-
only renderer;  it makes no use of special hardware support on the
platforms on which it runs.
  Up-to-date documentation is available via World Wide Web in
http://www.psc.edu/Packages/VFleet_Home .
  Compiled binaries are available by anonymous FTP from ftp.psc.edu,
in the directory pub/vfleet.  The correspondence between machine
architectures and tar file names is as follows:

vfleet1.1_sgi.tar.gz	SGI MIPS hardware under IRIX 6.2 or higher,
                        compiled with the -mips2 flag (32 bit mode)
vfleet1.1_alpha.tar.gz	DEC ALPHA architecture under Digital Ultrix 3.2
vfleet1.1_sun.tar.gz    Sun SPARC architecture under SunOS 5.4
vfleet1.1_hp.tar.gz     HP PA Risc architecture under HP-UX
vfleet1.1_linux.tar.gz  Intel Linux 2.0.27 distribution- note that this
                        requires Infomagic Motif; see README_LINUX

We would be happy to provide binaries for other architectures, if someone
can offer machines to do the compilations.  
  For source code contact Joel Welling at welling@psc.edu .  VFleet is written
in C++.

  To install the code, do the following:

1) Install PVM version 3.3 if you plan to use the code in distributed mode.

2) Get and untar the appropriate tar file.  This will produce a directory with
   the following files in it:

vfleet1.1/bytestohdf
vfleet1.1/default_bbox_tfun.tfn
vfleet1.1/default_block_tfun.tfn
vfleet1.1/default_grad_tfun.tfn
vfleet1.1/default_mask_tfun.tfn
vfleet1.1/default_ssum_tfun.tfn
vfleet1.1/default_sum_tfun.tfn
vfleet1.1/default_table_tfun.tfn
vfleet1.1/default_tfun.tfn
vfleet1.1/floatstohdf
vfleet1.1/logserver
vfleet1.1/sample.tfn
vfleet1.1/servman
vfleet1.1/vfleet
vfleet1.1/vfleet.uid
vfleet1.1/vortices.hdf
vfleet1.1/vrenserver

The Silicon Graphics distribution also contains the file
vfleet1.1/vfleet_gl. This is an alternate version of the vfleet
executable which uses Silicon Graphics GL for display.  It should be
treated as described below for the file vfleet.

3) Move or link vrenserver, logserver, and servman into the appropriate
PVM directory, for example ${PVM_ROOT}/bin/SGI5 or ${PVM_ROOT}/bin/PMAX.
This will make sense once you have installed PVM.

4) Create a subdirectory called "uid" in your home directory and move
or link vfleet.uid into it.  There are other ways to access UID files,
but this is the easiest to describe.  If you are installing VFleet for
general use, you might put vfleet.uid in /usr/lib/X11/uid or your system's
equivalent.  See the manual page for MrmOpenHierarchy(3X) for system-specific
information.  If your account is set up so that the environment variable
UIDPATH is set, for example by a line in your .cshrc file, you will want
to make sure the location of vfleet.uid is in that path.  For example,
you might add the following line to the bottom of your .cshrc file:

  setenv UIDPATH $HOME/uid/%U:${UIDPATH}

5) Pick a directory for VFleet-specific default files to reside in, for
example the directory into which you have untarred the files.  Modify
your .cshrc file to set the environment variable VFLEET_ROOT to point
to that directory.

6) Add the other executables, bytestohdf, floatstohdf, and vfleet itself,
to your path or move them to some directory that is already in your path.

That's it.  sample.tfn and vortices.hdf are sample files to let you try
out VFleet.  They are small, so they can be used locally, with no need to
fire things up in distributed mode.  You don't need to install PVM to do
this.

Please see the WWW page at http://www.psc.edu/Packages/VFleet_Home for 
additional operating information;  VFleet itself has a lot of help 
information included.  Please send your comments and bug reports to me 
at the address below.

Thanks for trying it out,
-Joel Welling
 Pittsburgh Supercomputing Center
 welling@psc.edu



