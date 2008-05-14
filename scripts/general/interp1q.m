## Copyright (C) 2008 David Bateman
##
## This file is part of Octave.
##
## Octave is free software; you can redistribute it and/or modify it
## under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 3 of the License, or (at
## your option) any later version.
##
## Octave is distributed in the hope that it will be useful, but
## WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
## General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with Octave; see the file COPYING.  If not, see
## <http://www.gnu.org/licenses/>.

## -*- texinfo -*-
## @deftypefn {Function File} {@var{yi} =} interp1q (@var{x}, @var{y}, @var{xi})
## One-dimensional linear interpolation without error checking.
## Interpolates @var{y}, defined at the points @var{x}, at the points
## @var{xi}. The sample points @var{x} must be a strictly monotonically
## increasing column vector. If @var{y} is an array, treat the columns
## of @var{y} separately. If @var{y} is a vector, it must be a column
## vector of the same length as @var{x}.
##
## Values of @var{xi} beyond the endpoints of the interpolation result
## in NA being returned.
##
## Note that the error checking is only a significant portion of the
## execution time of this @code{interp1} if the size of the input arguments
## is relatively small. Therefore, the benefit of using @code{interp1q}
## is relatively small.
## @seealso{interp1}
## @end deftypefn

function yi = interp1q (x, y, xi)
  x = x(:);
  nx = size (x, 1);
  szy = size (y);
  ny = szy (1);
  nc = prod (szy (2 : end));
  y = reshape (y, ny, nc);
  szx = size (xi);
  xi = xi (:);
  range = find (xi >= x (1) & xi <= x (nx));
  yi = NA (size(xi, 1), size (y, 2));
  xi = xi (range);
  dy = y (2 : ny, :) - y (1 : ny - 1, :);
  dx = x (2 : nx) - x (1 : nx - 1);
  idx = lookup (x, xi, "lr");
  s = (xi - x (idx)) ./ dx (idx);
  yi (range, :) = s (:, ones (1, nc)) .* dy (idx, :) + y (idx, :);
  if (length (szx) == 2 && any (szx == 1))
    yi = reshape (yi, [max (szx), szy (2 : end)]);
  else
    yi = reshape (yi, [szx, szy(2:end)]);
  endif
endfunction

%!shared xp, yp, xi
%! xp=[0:2:10].';      yp = sin(2*pi*xp/5);
%! xi = [-1; 0; 2.2; 4; 6.6; 10; 11];
%!assert (interp1(xp, yp, [min(xp)-1; max(xp)+1]), [NA; NA]);
%!assert (interp1(xp,yp,xp), yp, 100*eps);
%!assert (isempty(interp1(xp,yp,[])));
%!assert (interp1(xp,[yp,yp],xi), [interp1(xp,yp,xi),interp1(xp,yp,xi)]);
%!assert (interp1(xp,yp,[xi,xi]), [interp1(xp,yp,xi),interp1(xp,yp,xi)]);
%!assert (interp1(xp,[yp,yp],[xi,xi]), [interp1(xp,yp,xi),interp1(xp,yp,xi)]);
