               VFleet 1.1b4p2 Linux Binary Distribution
               ----------------------------------------

The linux binaries for VFleet 1.1b4p2 (almost identical to release 1.1)
were kindly provided by Paolo Zuiani, zuliap@fislab.dsi.unimi.it .
They have been compiled under Linux 2.0.27 (Slackware release) with
g++ 2.7.2 and relative tools.  Motif version 2.0.2 (Moo-tiff) from 
Infomagic (www.infomagic.com) is also needed, and is *not* included.

All libraries are dynamically linked.  The .sl files for libdf, libmfhdf,
libim, libsdsc, libjpeg, and libz are included;  the X and Motif shared
libraries are not included.  

This version of VFleet has been compiled with stub versions of the PVM
library, so it can be run only in local mode.  The bytestohdf and
floatstohdf utilities are also included.

Precompiled linux binaries for various HDF utilities are available
from ftp.ncsa.uiuc.edu .