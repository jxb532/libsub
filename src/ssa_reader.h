/*
    Copyright (C) 2016 Carl Hetherington <cth@carlh.net>

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

/** @file  src/ssa_reader.h
 *  @brief SSAReader class.
 */

#ifndef LIBSUB_SSA_READER_H
#define LIBSUB_SSA_READER_H

#include "reader.h"
#include <boost/function.hpp>

namespace sub {

class SSAReader : public Reader
{
public:
	SSAReader (FILE* f);
	SSAReader (std::string const & subs);

private:
	void read (boost::function<boost::optional<std::string> ()> get_line);
	Time parse_time (std::string t) const;
};

}

#endif
