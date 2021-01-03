
               VFleet Distributed Volume Renderer
                          Version 1.1.2
                       Compilation Guide

  This document describes how to build the VFleet distributed volume 
renderer from source code.  You should read the "README" file, which
describes installing precompiled binaries, if you have not done so
already.  If you are compiling for a machine for which we do not have
binaries, we'd be very pleased to include the binaries you build in 
our anonymous FTP distribution.
  VFleet is a volume rendering system for Unix platforms.  It can run
either locally or in distributed mode, using PVM to distribute the
volume rendering task across multiple platforms.  VFleet is a software-
only renderer;  it makes no use of special hardware support on the
platforms on which it runs.
  Up-to-date documentation is available via World Wide Web in
http://www.psc.edu/Packages/VFleet_Home .

-------------

  To build and install VFleet from source code, do the following:

1) Build and install HDF, available by anonymous FTP from ftp.ncsa.uiuc.edu
in the subdirectory HDF.  HDF provides the data file format used by VFleet.

2) If your machine does not already have Tcl, build and install it.  Tcl 
is available by anonymous FTP from ftp.sunlabs.com
in the pub/tcl subdirectory.  Alternately, see the Tcl/Tk web page at 
http://www.sunlabs.com:80/research/tcl .  (Note that VFleet uses only Tcl,
not Tk).  Tcl provides the scripting facilities in VFleet.  If your
machine has the command 'tclsh', it already has tcl.

3) If you want to run VFleet in parallel, build and install PVM
version 3.3 or higher.  This code is available by anonymous FTP from
from ftp.netlib.org in the subdirectory pvm3.  Better still, see the
PVM home page http://www.epm.ornl.gov/pvm/pvm_home.html .  PVM
provides the parallel computing facilities used by VFleet.

4) VFleet also uses the X Window System and Motif, including UIL.
These facilities should already exist on your workstation.  Note that
if your system uses Lesstif rather than Motif you are likely to run
into problems compiling VFleet's 'uil' source code.

5) Get the VFleet source distribution.  Uncompress and untar it.  This
will create a subdirectory called vfleet1.1.2 containing a number of source
files.  CD to that subdirectory.

6) Give the commands:

  ./configure
  make depend
  make

Some warnings may be generated during the 'make depend' stage.  They
typically come from dependencies for components which have been
configured out of your installation and can be safely ignored.

VFleet should now build.  Once compilation is complete, the
executables will be in the subdirectory bin/arch/ , where arch is your
machine architecture (for example Linux).

7) Create a subdirectory called "uid" in your home directory and move
or link vfleet.uid into it.  There are other ways to access UID files,
but this is the easiest to describe.  If you are installing VFleet for
general use, you might put vfleet.uid in /usr/lib/X11/uid or your system's
equivalent.  See the manual page for MrmOpenHierarchy(3X) for system-specific
information.  If your account is set up so that the environment variable
UIDPATH is set, for example by a line in your .cshrc file, you will want
to make sure the location of vfleet.uid is in that path.  For example,
you might add the following line to the bottom of your .cshrc file:

  setenv UIDPATH $HOME/uid/%U:${UIDPATH}

8) Pick a directory for VFleet-specific default files to reside in, for
example the directory into which you have untarred the files.  Modify
your .cshrc file to set the environment variable VFLEET_ROOT to point
to that directory.

9) Add the other executables, bytestohdf, floatstohdf, and vfleet itself,
to your path or move them to some directory that is already in your path.

10) If you are building VFleet for parallel execution, move or link
vrenserver, xlogserver, and servman into the appropriate PVM
directory, for example ${PVM_ROOT}/bin/LINUX or ${PVM_ROOT}/bin/PMAX.
This will make sense once you have installed PVM.  If you are building
for a Cray T3D and have used Makefile_t3d to build vrenserver_t3d,
move that executable to this directory also.  In the PVM directory,
create a symbolic link allowing xlogserver to be run under the name
logserver:

  ln -s xlogserver logserver

That's it.  sample.tfn and vortices.hdf are sample files to let you try
out VFleet.

------------------

Please see the WWW page at http://www.psc.edu/Packages/VFleet_Home for 
additional operating information;  VFleet itself has a lot of help 
information included.  Please send your comments and bug reports to me 
at the address below.


                     MACHINE-SPECIFIC COMMENTS

On Cray machines:

-If you have a Cray T3D, build first on the front end machine, using an
appropriately modified version of Makefile_cray.  Then use Makefile_t3d
to build the T3D version of the rendering server, vrenserver_t3d.  That
file is moved to the ${PVM_ROOT}/bin/${PVM_ARCH} directory along with the
other files described above.

On DEC machines under OSF:

-This is an operating bug, not a build problem.  Under DEC X, the transfer
function editor dialog does not resize correctly.  The work-around is to
use the "Show Debugger Menu" option in the "File" menu, and then use the
"Resize Tfun Dialog" option in the new "Debug".

On DEC PMAX machines:

-We no longer have a PMAX with up-to-date compilers on which to test 
this build process.  Makefile_pmax should be correct, but is untested.
If you build VFleet on this machine and are willing to provide the
binaries, we would like to include them on our FTP site.

On IBM AIX machines:

-We no longer have an AIX machine available on which to test 
this build process.  Makefile_aix should be correct, but is untested.
If you build VFleet on this machine and are willing to provide the
binaries, we would like to include them on our FTP site.

On SGI IRIX machines:

-Despite long debugging, some SGI display configurations still seem
to confuse the X Windows code in VFleet.  If this happens on your system,
consider compiling with the GL image handling option described in
config.mk .

On Sun machines under Solaris:

-The HDF include file vproto.h includes a use of the word "class" as a
dummy parameter name;  the Sun C++ compiler trips on this even though 
the file is included in an "extern C {}" environment.  After installing
HDF, you should edit vproto.h to replace the offending use of the word
"class" with "class_param" or some other string.  The change has no
effect on the meaning of the include file;  it just makes it more C++-
compatible.


Thanks for trying it out,
-Joel Welling
 Pittsburgh Supercomputing Center
 welling@psc.edu
