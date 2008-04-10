// vim:set noet cinoptions= sw=4 ts=4:
// This file is part of the eix project and distributed under the
// terms of the GNU General Public License v2.
//
// Copyright (c)
//   Wolfgang Frisch <xororand@users.sourceforge.net>
//   Emil Beinroth <emilbeinroth@gmx.net>
//   Martin Väth <vaeth@mathematik.uni-wuerzburg.de>

#ifndef __ANSICOLOR_H__
#define __ANSICOLOR_H__

#include <iostream>
#include <map>

/** A class for using ANSI markers
 * Example:
 * @code
 * AnsiMarker m_inverse(AnsiMarker::amInverse);
 * cout << m_inverse << "inverse text" << m_inverse.end() << "normal text";
 * @endcode
 */
class AnsiMarker {

	public:
		/** The various colors, acNone is the default color */
		enum Marker { amNone=0, amBold=1, amUnderlined=4, amBlink=5, amInverse=7 };
		Marker mk;

		AnsiMarker(const Marker m = amNone) : mk(m) { }

		static const char *reset()
		{ return "\x1B[0m"; }

		std::string asString()
		{
			static const int len = 100;
			char buf[len];
			if( mk!=amNone ) {
				snprintf(buf, len, "\x1B[%dm", int(mk));
			}
			else buf[0]='\0';
			return std::string(buf);
		}
		std::string end()
		{
			if(mk!=amNone)
				return reset();
			return "";
		}

		static std::map<std::string,AnsiMarker::Marker>& init_map_once()
		{
			static std::map<std::string,AnsiMarker::Marker> marker_map;
			marker_map["none"]    = AnsiMarker::amNone;
			marker_map["bold"]    = AnsiMarker::amBold;
			marker_map["underline"] = AnsiMarker::amUnderlined;
			marker_map["underlined"]= AnsiMarker::amUnderlined;
			marker_map["blink"]   = AnsiMarker::amBlink;
			marker_map["inverse"] = AnsiMarker::amInverse;
			marker_map["invert"]  = AnsiMarker::amInverse;
			return marker_map;
		}

		static Marker name_to_marker(std::string name)
		{
			static std::map<std::string,AnsiMarker::Marker> &marker_map = init_map_once();
			return marker_map[name];
		}

		/** Prints the current marker to an ostream */
		friend std::ostream& operator<< (std::ostream& os, AnsiMarker am);

		AnsiMarker(std::string marker_name) throw (ExBasic)
		{
			mk = name_to_marker(marker_name);
		}
};

inline std::ostream&
operator<< (std::ostream& os, AnsiMarker am)
{
	if( am.mk != AnsiMarker::amNone )
		os << "\x1B[" << int(am.mk) << 'm';
	return os;
}

/** A class for using ANSI color codes
 * @note Currently only handles foreground color.
 * Example:
 * @code
 * AnsiColor c_red(AnsiColor::acRed, false);
 * cout << "Normal text, " << c_red << "red text" << AnsiColor(AnsiColor::acDefault) << "normal text" << endl;
 * @endcode
 */
class AnsiColor {

	public:
		/** The various colors, acNone is the default color */
		enum Color { acNone=-1, acDefault=0, acBlack=30, acRed, acGreen, acYellow, acBlue, acPurple, acCyan, acGray };

		/** The current color */
		Color fg;

		/** true -> bright text */
		bool light;

		/** reset - faster than AnsiColor(AnsiColor::acDefault) */
		static const char *reset()
		{ return AnsiMarker::reset(); }

		/** additional marker */
		AnsiMarker::Marker mk;

		/** Constructor
		 * @param fg foreground color
		 * @param light brightness (normal, bright)
		 * defaults to no color output */
		AnsiColor(const Color f = acNone, const bool l = false) : fg(f), light(l), mk(AnsiMarker::amNone) {}

		std::string asString()
		{
			static const int len = 100;
			char buf[len];
			if( fg!=acNone ) {
				if(fg == acDefault) {
					snprintf(buf, len, "\x1B[0m\x1B[%dm", int(light));
				}
				else {
					snprintf(buf, len, "\x1B[%d;%dm", int(light), int(fg));
				}
			}
			else buf[0]='\0';
			if(mk != AnsiMarker::amNone)
				return std::string(buf) + AnsiMarker(mk).asString();
			return std::string(buf);
		}

		static std::map<std::string,AnsiColor::Color>& init_map_once()
		{
			static std::map<std::string,AnsiColor::Color> color_map;
			color_map["default"] = AnsiColor::acDefault;
			color_map["black"]   = AnsiColor::acBlack;
			color_map["red"]     = AnsiColor::acRed;
			color_map["green"]   = AnsiColor::acGreen;
			color_map["yellow"]  = AnsiColor::acYellow;
			color_map["blue"]    = AnsiColor::acBlue;
			color_map["purple"]  = AnsiColor::acPurple;
			color_map["cyan"]    = AnsiColor::acCyan;
			color_map["gray"]    = AnsiColor::acGray;
			return color_map;
		}

		static Color name_to_color(std::string name)
		{
			static std::map<std::string,AnsiColor::Color> &color_map = init_map_once();
			return color_map[name];
		}

		/** Prints the current color to an ostream */
		friend std::ostream& operator<< (std::ostream& os, AnsiColor ac);

		AnsiColor(std::string color_name) throw (ExBasic)
		{
			light = false;

			// look for brightness attribute
			std::string::size_type comma_pos = color_name.find(',');
			std::string::size_type resize = comma_pos;
			if(comma_pos != std::string::npos)
			{
				resize = comma_pos++;
				if(color_name[comma_pos] == '1') {
					light = true;
				}
				else if(color_name[comma_pos] == '0') {
					light = false;
				}
				else {
					throw ExBasic("Invalid brightness value %r")
						% color_name[comma_pos + 1];
				}
			}
			comma_pos = color_name.rfind(';');
			if(comma_pos != std::string::npos)
			{
				if(resize == std::string::npos)
					resize = comma_pos;
				mk = AnsiMarker::name_to_marker(color_name.substr(comma_pos+1));
			}
			else
				mk = AnsiMarker::amNone;
			if(resize != std::string::npos)
				color_name.resize(resize);

			fg = name_to_color(color_name);
		}
};

inline std::ostream&
operator<< (std::ostream& os, AnsiColor ac)
{
	if( ac.fg != AnsiColor::acNone )
	{
		if( ac.fg== AnsiColor::acDefault )
			os << "\x1B[0m";
		os << "\x1B[" << int(ac.light);
		if( ac.fg!= AnsiColor::acDefault )
			os << ";" << int(ac.fg);
		os << 'm';
	}
	os << AnsiMarker(ac.mk);
	return os;
}

#endif /* __ANSICOLOR_H__ */
