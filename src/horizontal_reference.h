/*
    Copyright (C) 2017 Carl Hetherington <cth@carlh.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#ifndef LIBSUB_HORIZONTAL_REFERENCE_H
#define LIBSUB_HORIZONTAL_REFERENCE_H

#include <string>

namespace sub {

enum HorizontalReference
{
	/** distance is from the left of the screen, positive moves right */
	LEFT_OF_SCREEN,
	/** distance is from the centre of the screen, positive moves right */
	HORIZONTAL_CENTRE_OF_SCREEN,
	/** distance is from the right of the screen, positive moves left */
	RIGHT_OF_SCREEN
};

}

#endif
