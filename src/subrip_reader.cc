/*
    Copyright (C) 2014-2015 Carl Hetherington <cth@carlh.net>

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

/** @file  src/subrip_reader.cc
 *  @brief SubripReader class.
 */

#include "subrip_reader.h"
#include "exceptions.h"
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/bind.hpp>
#include <cstdio>
#include <vector>
#include <iostream>

using std::string;
using std::vector;
using std::list;
using std::cout;
using std::hex;
using std::stringstream;
using boost::lexical_cast;
using boost::to_upper;
using boost::optional;
using boost::function;
using namespace sub;

/** @param s Subtitle string encoded in UTF-8 */
SubripReader::SubripReader (string const & s)
{
	stringstream str (s);
	this->read (boost::bind (&SubripReader::get_line_stringstream, this, &str));
}

/** @param f Subtitle file encoded in UTF-8 */
SubripReader::SubripReader (FILE* f)
{
	this->read (boost::bind (&SubripReader::get_line_file, this, f));
}

optional<string>
SubripReader::get_line_stringstream (stringstream* str) const
{
	string s;
	getline (*str, s);
	if (!str->good ()) {
		return optional<string> ();
	}

	return s;
}

optional<string>
SubripReader::get_line_file (FILE* f) const
{
	char buffer[256];
	char* r = fgets (buffer, sizeof (buffer), f);
	if (r == 0 || feof (f)) {
		return optional<string> ();
	}

	return string (buffer);
}

void
SubripReader::read (function<optional<string> ()> get_line)
{
	enum {
		COUNTER,
		METADATA,
		CONTENT
	} state = COUNTER;

	Time from;
	Time to;

	string line;
	int line_number = 0;

	while (true) {
		optional<string> line = get_line ();
		if (!line) {
			break;
		}

		trim_right_if (*line, boost::is_any_of ("\n\r"));

		if (
			line->length() >= 3 &&
			static_cast<unsigned char> (line.get()[0]) == 0xef &&
			static_cast<unsigned char> (line.get()[1]) == 0xbb &&
			static_cast<unsigned char> (line.get()[2]) == 0xbf
			) {

			/* Skip Unicode byte order mark */
			line = line->substr (3);
		}

		switch (state) {
		case COUNTER:
		{
			if (line->empty ()) {
				/* a blank line at the start is ok */
				break;
			}

			state = METADATA;
		}
		break;
		case METADATA:
		{
			vector<string> p;
			boost::algorithm::split (p, *line, boost::algorithm::is_any_of (" "));
			if (p.size() != 3 && p.size() != 7) {
				throw SubripError (*line, "a time/position line");
			}

			from = convert_time (p[0]);
			to = convert_time (p[2]);

			/* XXX: should not ignore coordinate specifications */

			state = CONTENT;
			break;
		}
		case CONTENT:
			if (line->empty ()) {
				state = COUNTER;
				line_number = 0;
			} else {
				convert_line (*line, line_number, from, to);
				line_number++;
			}
			break;
		}
	}
}

Time
SubripReader::convert_time (string t)
{
	vector<string> a;
	boost::algorithm::split (a, t, boost::is_any_of (":"));
	if (a.size() != 3) {
		throw SubripError (t, "time in the format h:m:s,ms");
	}

	vector<string> b;
	boost::algorithm::split (b, a[2], boost::is_any_of (","));

	return Time::from_hms (
		lexical_cast<int> (a[0]),
		lexical_cast<int> (a[1]),
		lexical_cast<int> (b[0]),
		lexical_cast<int> (b[1])
		);
}

void
SubripReader::convert_line (string t, int line_number, Time from, Time to)
{
	enum {
		TEXT,
		TAG
	} state = TEXT;

	string tag;

	RawSubtitle p;
	p.font = "Arial";
	p.font_size.set_points (48);
	p.from = from;
	p.to = to;
	p.vertical_position.line = line_number;
	/* XXX: arbitrary */
	p.vertical_position.lines = 32;
	p.vertical_position.reference = TOP_OF_SUBTITLE;

	list<Colour> colours;
	colours.push_back (Colour (1, 1, 1));

	/* XXX: missing <font> support */
	/* XXX: nesting of tags e.g. <b>foo<i>bar<b>baz</b>fred</i>jim</b> might
	   not work, I think.
	*/

	for (size_t i = 0; i < t.size(); ++i) {
		switch (state) {
		case TEXT:
			if (t[i] == '<' || t[i] == '{') {
				state = TAG;
			} else {
				p.text += t[i];
			}
			break;
		case TAG:
			if (t[i] == '>' || t[i] == '}') {
				if (tag == "b") {
					maybe_content (p);
					p.bold = true;
				} else if (tag == "/b") {
					maybe_content (p);
					p.bold = false;
				} else if (tag == "i") {
					maybe_content (p);
					p.italic = true;
				} else if (tag == "/i") {
					maybe_content (p);
					p.italic = false;
				} else if (tag == "u") {
					maybe_content (p);
					p.underline = true;
				} else if (tag == "/u") {
					maybe_content (p);
					p.underline = false;
				} else if (boost::starts_with (tag, "font")) {
					maybe_content (p);
					boost::regex re (".*color=\"#([0123456789abcdef]+)\"");
					boost::smatch match;
					if (boost::regex_search (tag, match, re) && string (match[1]).size() == 6) {
						p.colour = Colour::from_rgb_hex (match[1]);
						colours.push_back (p.colour);
					}
				} else if (tag == "/font") {
					maybe_content (p);
					colours.pop_back ();
					p.colour = colours.back ();
				}
				tag.clear ();
				state = TEXT;
			} else {
				tag += t[i];
			}
			break;
		}
	}

	maybe_content (p);
}

void
SubripReader::maybe_content (RawSubtitle& p)
{
	if (!p.text.empty ()) {
		_subs.push_back (p);
		p.text.clear ();
	}
}
