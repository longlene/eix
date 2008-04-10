// vim:set noet cinoptions= sw=4 ts=4:
// This file is part of the eix project and distributed under the
// terms of the GNU General Public License v2.
//
// Copyright (c)
//   Wolfgang Frisch <xororand@users.sourceforge.net>
//   Emil Beinroth <emilbeinroth@gmx.net>
//   Martin Väth <vaeth@mathematik.uni-wuerzburg.de>

#include "cascadingprofile.h"

#include <eixTk/utils.h>
#include <varsreader.h>
#include <portage/package.h>
#include <portage/conf/portagesettings.h>

#include <fstream>

#include <unistd.h>

/* Path to symlink to profile */
#define PROFILE_LINK "/etc/make.profile"
#define PROFILE_LINK_DIRECTORY "/etc/"

/* Buffer size for the readlink-call */
#define READLINK_BUFFER 128

using namespace std;

/** Exclude this files from listing of files in profile. */
static const char *profile_exclude[] = { "parent", "..", "." , NULL };

/** Add all files from profile ans its parents to m_profile_files. */
void CascadingProfile::addProfile(const string &profile)
{
	/* Open stream and check if it's open */
	ifstream ifstr((profile + "parent").c_str());
	if(ifstr.is_open()) {
		/* while there are lines in the file */
		for(string buf; getline(ifstr, buf); ) {
			trim(&buf);
			/* If it's a comment or a empty line continue with the next line */
			if(buf.empty() || buf[0] == '#')
				continue;
			addProfile(profile + buf + "/");
		}
	}
	pushback_files(profile, m_profile_files, profile_exclude, 3);
}

bool
CascadingProfile::readremoveFiles()
{
	bool ret = false;
	for(vector<string>::iterator file = m_profile_files.begin();
		file != m_profile_files.end();
		++file)
	{
		bool (CascadingProfile::*handler)(const string &line);

		if(strcmp(strrchr(file->c_str(), '/'), "/packages") == 0)
		{
			handler = &CascadingProfile::readPackages;
		}
		else if(strcmp(strrchr(file->c_str(), '/'), "/package.mask") == 0)
		{
			handler = &CascadingProfile::readPackageMasks;
		}
		else
			continue;

		vector<string> lines;
		pushback_lines(file->c_str(), &lines, true, true, true);

		for(vector<string>::iterator i(lines.begin()), i_end(lines.end());
			i != i_end;
			++i)
		{
			try {
				if((this->*handler) (*i))
					ret = true;
			}
			catch(const ExBasic &e)
			{
				portage_parse_error(*file, lines.begin(), i, e);
			}
		}
	}
	m_profile_files.clear();
	return ret;
}

bool
CascadingProfile::readPackages(const string &line)
{
	/* Cycle through and get rid of comments ..
	 * lines beginning with '*' are m_system-packages
	 * all others are masked by profile .. if they don't match :) */
	const char *p = line.c_str();
	bool remove = (*p == '-');

	if(remove)
	{
		++p;
	}

	Mask *m = NULL;
	MaskList<Mask> *ml = NULL;
	switch(*p)
	{
		case '*':
			++p;
			m = new Mask(p, Mask::maskInSystem) ;
			ml = &m_system;
			break;
		default:
			m = new Mask(p, Mask::maskAllowedByProfile);
			ml = &m_system_allowed;
			break;
	}

	if(remove)
	{
		bool ret = (ml->remove(m));
		delete m;
		return ret;
	}
	else
	{
		return (ml->add(m));
	}
}

/** Read all "make.defaults" files found in profile. */
void
CascadingProfile::readMakeDefaults()
{
	for(vector<string>::size_type i = 0; i < m_profile_files.size(); ++i) {
		if( strcmp(strrchr(m_profile_files[i].c_str(), '/'), "/make.defaults") == 0) {
			m_portagesettings->read_config(m_profile_files[i], "");
		}
	}
}

bool
CascadingProfile::readPackageMasks(const string &line)
{
	if(line[0] == '-')
	{
		Mask *m = new Mask(line.substr(1).c_str(), Mask::maskMask);
		bool ret = m_package_masks.remove(m);
		delete m;
		return ret;
	}
	else
	{
		return m_package_masks.add(new Mask(line.c_str(), Mask::maskMask));
	}
}

void
CascadingProfile::ReadLink(string &path) const
{
	char readlink_buffer[READLINK_BUFFER];
	string profile_linkname = (m_portagesettings->m_eprefixconf) + PROFILE_LINK;
	int len = readlink(profile_linkname.c_str() , readlink_buffer, READLINK_BUFFER - 1);
	if(len <= 0) {
		throw ExBasic("readlink(%r) failed: %s") % profile_linkname % strerror(errno);
	}
	readlink_buffer[len] = '\0';

	path = readlink_buffer;
	/* If it's a relative path prepend the dirname of ${PORTAGE_CONFIGROOT}/PROFILE_LINK_DIRECTORY */
	if( path[0] != '/' ) {
		path.insert(0, (m_portagesettings->m_eprefixconf) + PROFILE_LINK_DIRECTORY);
	}
}

/** Cycle through profile and put path to files into this->m_profile_files. */
void
CascadingProfile::listaddProfile(const char *profile_dir) throw(ExBasic)
{
	string path_buffer;
	if(profile_dir)
		path_buffer = profile_dir;
	else
	{
		path_buffer = (*m_portagesettings)["PORTAGE_PROFILE"];
		if(path_buffer.empty()) {
			ReadLink(path_buffer);
			if(path_buffer.empty())
				return;
		}
		else {
			path_buffer.insert(0, m_portagesettings->m_eprefixprofile);
		}
	}

	if(path_buffer[path_buffer.size() - 1] != '/')
		path_buffer.append("/");

	addProfile(path_buffer);
}

void
CascadingProfile::applyMasks(Package *p) const
{
	for(Package::iterator it = p->begin(); it != p->end(); ++it) {
		(*it)->maskflags.set(MaskFlags::MASK_NONE);
	}
	getAllowedPackages()->applyMasks(p);
	getSystemPackages()->applyMasks(p);
	getPackageMasks()->applyMasks(p);
}

