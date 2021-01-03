
         VFleet Volume Renderer Release 1.1 available
         --------------------------------------------

  A new release of the VFleet volume renderer is now available,
by anonymous FTP from ftp://ftp.psc.edu/pub/vfleet or via the WWW at

            http://www.psc.edu/Packages/VFleet_Home

VFleet is a volume renderer for Unix platforms.  It can run either
locally or in distributed mode, using PVM (free from Oak Ridge
National Lab) to distribute the volume rendering task across
multiple platforms.  VFleet is a software-only renderer;  it
makes no use of special hardware for graphics acceleration and
so can run on practically any Unix platform.

  VFleet is designed to be useful for scientific visualization.  It
can handle multiple simultaneous data volumes, producing images which
depend on more than one dataset at a time.  For example, it might be
used to show images of pressure in regions of high temperature.  The
fact that it can run in parallel across multiple machines means that
datasets can be examined which would not fit on a single machine.
VFleet has a Tcl-based scripting facility for generating animations.

  Between release 1.0 and 1.1, VFleet has become a robust and useful
tool.  Many, many new features have been added, including trilinear
interpolation, 3D mipmapping, improved lighting and scripting, and
full camera control.  

  VFleet runs on a variety of Unix platforms, including Linux if
Motif is available.  Source code and compiled binaries for SGI, HP, 
Sun, DEC Alpha, and Linux are available.  VFleet is written in C++.  
See the file COPYRIGHT in the anonymous FTP directory for copyright 
information.

  We will be maintaining a mailing list for VFleet update announcements.
If you would like to be on this mailing list, please send mail to me at
the address below.

  VFleet was written at the Pittsburgh Supercomputing Center, with
major support from the Grand Challenge Computational Cosmology project
funded by the National Science Foundation.

I hope you find it useful,
-Joel Welling
 Pittsburgh Supercomputing Center
 welling@psc.edu