# Copyright (C) 1995 John W. Eaton
# 
# This file is part of Octave.
# 
# Octave is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2, or (at your option) any
# later version.
# 
# Octave is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
# 
# You should have received a copy of the GNU General Public License
# along with Octave; see the file COPYING.  If not, write to the Free
# Software Foundation, 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA. 

function subwindow (xn, yn)

# usage: subwindow (xn, yn)
#
# NOTE: this will work only with gnuplot installed with
#       multiplot patch
#
# Sets subwindow position in multiplot mode for next plot. The
# multiplot mode has to be previously initialized using multiplot()
# command, else this command just becomes an aliad to multiplot()

# Written by Vinayak Dutt, Dutt.Vinayak@mayo.EDU  3 Jul 95 

# global variables to keep track of multiplot options

  global multiplot_mode 
  global multi_xsize multi_ysize 
  global multi_xn multi_yn

# check calling argument count

  if (nargin != 2)
    usage ("subwindow (xn, yn)");
  endif

# check for scalar inputs

  if (! (is_scalar (xn) && is_scalar (yn)))
    error ("subwindow: xn and yn have to be scalars");
  endif

  xn = round (xn);
  yn = round (yn);

# switch to multiplot mode if not already in, and use the args as the
# args to multiplot() 

  if (multiplot_mode != 1)
    multiplot (xn, yn)
    return;
  endif

# get the sub plot location

  if (xn < 1 || xn > multi_xn || yn < 1 || yn > multi_yn)
    error ("subwindow: incorrect xn and yn");
  endif

  xo = (xn - 1.0)*multi_xsize;
  yo = (multi_yn - yn)*multi_ysize;

  eval (sprintf ("set origin %g, %g", xo, yo));

endfunction
