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

#include <boost/test/unit_test.hpp>
#include "stl_binary_reader.h"
#include "subtitle.h"
#include "test.h"
#include <fstream>

using std::list;
using std::ifstream;

/* Test reading of a binary STL file */
BOOST_AUTO_TEST_CASE (stl_binary_reader_test)
{
	if (private_test.empty ()) {
		return;
	}

	boost::filesystem::path p = private_test / "Vampire_Academy_24fps_Reel_6_DE_FR.stl";
	ifstream f (p.string().c_str ());
	sub::STLBinaryReader r (f);
}
