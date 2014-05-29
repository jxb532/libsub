/*
    Copyright (C) 2014 Carl Hetherington <cth@carlh.net>

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

#include "subtitle.h"

using namespace sub;

bool
sub::operator< (Subtitle const & a, Subtitle const & b)
{
	if (a.from.frame() && b.from.frame()) {
		return a.from.frame().get() < b.from.frame().get();
	}

	if (a.from.metric() && b.from.metric()) {
		return a.from.metric().get() < b.from.metric().get();
	}

	assert (false);
}


float
Block::FontSize::proportional (int screen_height_in_points) const
{
	if (_proportional) {
		return _proportional.get ();
	}

	return float (_points.get ()) / screen_height_in_points;
}

int
Block::FontSize::points (int screen_height_in_points) const
{
	if (_points) {
		return _points.get ();
	}

	return _proportional.get() * screen_height_in_points;
}

bool
Subtitle::same_metadata (Subtitle const & other) const
{
	return (
		vertical_position == other.vertical_position &&
		from == other.from &&
		to == other.to &&
		fade_up == other.fade_up &&
		fade_down == other.fade_down
		);
}

bool
Subtitle::VerticalPosition::operator== (Subtitle::VerticalPosition const & other) const
{
	if (proportional && reference && other.proportional && other.reference) {
		return proportional.get() == other.proportional.get() && reference.get() == other.reference.get();
	} else if (line && other.line) {
		return line.get() == other.line.get();
	}

	return false;
}

