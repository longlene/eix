/***************************************************************************
 *   eix is a small utility for searching ebuilds in the                   *
 *   Gentoo Linux portage system. It uses indexing to allow quick searches *
 *   in package descriptions with regular expressions.                     *
 *                                                                         *
 *   https://sourceforge.net/projects/eix                                  *
 *                                                                         *
 *   Copyright (c)                                                         *
 *     Wolfgang Frisch <xororand@users.sourceforge.net>                    *
 *     Emil Beinroth <emilbeinroth@gmx.net>                                *
 *     Martin V�th <vaeth@mathematik.uni-wuerzburg.de>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef __NOCACHE_H__
#define __NOCACHE_H__

#include <portage/cache/base.h>

class NoneCache : public BasicCache {

	private:
		bool nosubst;

		void readPackage(Category &vec, const char *pkg_name, std::string *directory_path, struct dirent **list, int numfiles) throw(ExBasic);
	public:
		NoneCache(bool no_substitute = false) : BasicCache(), nosubst(no_substitute)
		{ }

		bool readCategory(Category &vec) throw(ExBasic);

		bool use_prefixport() const
		{ return true; }

		const char *getType() const
		{
			if(nosubst)
				return "none*";
			return "none";
		}
};

#endif /* __NOCACHE_H__ */
