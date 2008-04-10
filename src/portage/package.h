// vim:set noet cinoptions= sw=4 ts=4:
// This file is part of the eix project and distributed under the
// terms of the GNU General Public License v2.
//
// Copyright (c)
//   Wolfgang Frisch <xororand@users.sourceforge.net>
//   Emil Beinroth <emilbeinroth@gmx.net>
//   Martin Väth <vaeth@mathematik.uni-wuerzburg.de>

#ifndef __PACKAGE_H__
#define __PACKAGE_H__

#include <eixTk/ptr_list.h>
#include <eixTk/inttypes.h>

#include <list>
#include <string>
#include <vector>
#include <map>
#include <cstdlib>
#include <sstream>

#include <portage/version.h>
#include <portage/instversion.h>

class VarDbPkg;

/** A sorted list of pointer to Versions */

class VersionList : public eix::ptr_list<Version>
{
	public:
		VersionList(Version *v) : std::list<Version*>(1, v)
		{ }

		Version* best(bool allow_unstable = false) const;
};

class SlotVersions
{
	private:
		const char  *m_slotname;
		VersionList  m_version_list;

	public:
		const char *slotname() const
		{ return m_slotname; }

		const VersionList &const_version_list() const
		{ return m_version_list; }

		VersionList &version_list()
		{ return m_version_list; }

		SlotVersions(const char *s, Version *v) :
			m_slotname(s), m_version_list(v)
		{ }
};

/** This list is always sorted with respect to the first version for each slot */
class SlotList : public std::vector<SlotVersions>
{
	public:
		void push_back_largest(Version *version);
		const VersionList *operator [] (const char *s) const;
};

/** A class to represent a package in portage It contains various information
 * about a package, including a sorted(!) list of versions. */
class Package
	: public eix::ptr_list<Version>
{
	public:
		friend class PackageReader;

		/** True if duplicated versions are found in for this package.
		 * That means e.g. that version 0.2 is found in two overlays. */
		typedef uint8_t Duplicates;
		static const Duplicates
			DUP_NONE,
			DUP_SOME,    /* Duplicate versions are somewhere */
			DUP_OVERLAYS;/* Duplicate versions are both in overlays */

		Duplicates have_duplicate_versions;


		const SlotList& slotlist() const
		{
			if (! m_has_cached_slotlist) {
				build_slotslit();
				m_has_cached_slotlist = true;
			}
			return m_slotlist;
		}

		/** True if at least one slot is nonempty (different from "0") */
		bool have_nontrivial_slots;

		/** True if all versions come from one overlay. */
		bool have_same_overlay_key;

		/** True if all versions come from at least two overlays. */
		bool at_least_two_overlays;

		/** The largest overlay from which one of the version comes. */
		Version::Overlay largest_overlay;

		/** True if every version is in the system-profile. */
		bool is_system_package;

		/** Package properties (stored in db) */
		std::string category, name, desc, homepage, licenses, provide;

		/** Collected IUSE for all versions of that package */
		const std::string& coll_iuse() const
		{
			if (m_collected_iuse_cache.empty())
				m_collected_iuse_cache = join_set(m_collected_iuse);
			return m_collected_iuse_cache;
		}

		const std::set<std::string>& coll_iuse_vector() const
		{ return m_collected_iuse; }

		template<typename Iter>
		void add_coll_iuse(const Iter& a, const Iter& b)
		{ 
			m_collected_iuse.insert(a, b); 
			m_collected_iuse_cache.clear();
		}

		void add_coll_iuse(const std::string &s)
		{
			m_collected_iuse_cache.clear();
			m_collected_iuse.insert(s);
		}

#if !defined(NOT_FULL_USE)
		/** Does at least one version have individual iuse data? */
		bool versions_have_full_use;
#endif

		/** How upgrades for new better slots are treated in tests */
		static bool upgrade_to_best;

		/// Preset with defaults
		Package()
		{ defaults(); }

		/// Fill in name and category and preset with defaults
		Package(const std::string& c, const std::string& n)
			: category(c), name(n)
		{ defaults(); }

		/** De-constructor, delete content of Version-list. */
		~Package();

		/** Adds a version to "the versions" list.
		    Only BasicVersion needs to be filled here.
		    You must call addVersionFinalize() after filling
		    the remaining data */
		void addVersionStart(Version *version)
		{ checkDuplicates(version); sortedPushBack(version); }

		/** Finishes addVersion() after the remaining data
		    have been filled */
		void addVersionFinalize(Version *version);

		/** Adds a version to "the versions" list. */
		void addVersion(Version *version)
		{ addVersionStart(version); addVersionFinalize(version); }

		void save_keyflags(Version::SavedKeyIndex i)
		{
			for(iterator it = begin(); it != end(); ++it)
				it->save_keyflags(i);
		}

		void save_maskflags(Version::SavedMaskIndex i)
		{
			for(iterator it = begin(); it != end(); ++it)
				it->save_maskflags(i);
		}

		bool restore_keyflags(Version::SavedKeyIndex i)
		{
			for(iterator it = begin(); it != end(); ++it) {
				if(!(it->restore_keyflags(i)))
					return false;
			}
			return true;
		}

		bool restore_maskflags(Version::SavedMaskIndex i)
		{
			for(iterator it = begin(); it != end(); ++it) {
				if(!(it->restore_maskflags(i)))
					return false;
			}
			return true;
		}

		Version *best(bool allow_unstable = false) const;

		Version *best_slot(const char *slot_name) const;

		void best_slots(std::vector<Version*> &l, bool allow_unstable = false) const;

		/** Test whether p has a worse best_slot().
		    @return
			-  1: p has  a worse/missing best_slot
			-  3: p has no worse/missing best_slot, but an
			      identical from a different overlay
			-  0: else */
		int worse_best_slots(const Package &p) const;

		/** Compare best_slots() versions with that of p.
		    @return
			-  0: Everything matches
			-  1: p has a worse/missing best_slot, and *this has not
			- -1: *this has a worse/missing best_slot, and p has not
			-  2: p and *this both have a worse/missing best_slot
			-  3: all matches, but at least one overlay differs */
		int compare_best_slots(const Package &p) const;

		/** Compare best() version with that of p.
		    @return
			-  0: same
			-  1: p is smaller
			- -1: p is larger
			-  3: same, but overlays (or slots if test_slot)
			      are different */
		int compare_best(const Package &p, bool test_slot) const;

		/** has p a worse/missing best/best_slot/different overlay? */
		bool have_worse(const Package &p, bool test_slots) const
		{
			if(test_slots)
				return (worse_best_slots(p) > 0);
			return (compare_best(p, false) > 0);
		}

		/** differs p in at least one best/best_slot? */
		bool differ(const Package &p, bool test_slots) const
		{
			if(test_slots)
				return compare_best_slots(p);
			return compare_best(p, false);
		}

		/** Compare best_slots() versions with that installed in v.
		    if v is NULL, it is assumed that none is installed.
		    @return
			-  0: All installed versions are best and
			      (unless only_installed) one is installed
			      or nothing is installed and nothing can be
			      installed
			-  1: upgrade   necessary but no downgrade
			- -1: downgrade necessary but no upgrade
			-  2: upgrade and downgrade necessary
			-  4: (if only_installed) nothing is installed,
			      but one can be installed */
		int check_best_slots(VarDbPkg *v, bool only_installed) const;

		/** Compare best() version with that installed in v.
		    if v is NULL, it is assumed that none is installed.
		    @return
			-  0: All installed versions are best and
			      (unless only_installed) one is installed
			      or nothing is installed and nothing can be
			      installed
			-  1: upgrade necessary
			- -1: downgrade necessary
			-  3: (if test_slot) everything matches,
			      but slots are different.
			-  4: (if only_installed) nothing is installed,
			      but one can be installed */
		int check_best(VarDbPkg *v, bool only_installed, bool test_slot) const;

		/** can we upgrade v or has v different slots? */
		bool can_upgrade(VarDbPkg *v, bool only_installed, bool test_slots) const
		{
			if(!test_slots)
				return (check_best(v, only_installed, false) > 0);
			if(upgrade_to_best)
			{
				if(check_best(v, only_installed, true) > 0)
					return true;
			}
			return (check_best_slots(v, only_installed) > 0);
		}

		/** must we downgrade v or has v different categories/slots? */
		bool must_downgrade(VarDbPkg *v, bool test_slots) const
		{
			int c = check_best(v, true, test_slots);
			if((c < 0) || (c == 3))
				return true;
			if(!test_slots)
				return false;
			c = check_best_slots(v, true);
			return ((c < 0) || (c == 2));
		}

		/** do we have an upgrade/downgrade recommendation? */
		bool recommend(VarDbPkg *v, bool only_installed, bool test_slots) const
		{
			return can_upgrade(v, only_installed, test_slots) ||
				must_downgrade(v, test_slots);
		}

		bool differ(const Package &p, VarDbPkg *v, bool only_installed, bool testvardb, bool test_slots) const
		{
			if(testvardb)
				return recommend(v, only_installed, test_slots);
			return differ(p, test_slots);
		}

		/** Get the name of a slot of a version.
		    returns NULL if not found. */
		const char *slotname(const ExtendedVersion &v) const;

		/** Get the name of a slot of an installed version,
		    possibly reading it from disk.
		    Returns true if a reasonable choice seems to be found
		    (v.know_slot determines whether we had full success). */
		bool guess_slotname(InstVersion &v, const VarDbPkg *vardbpkg) const;

		Version *latest() const
		{ return *rbegin(); }

	protected:
		/** \c slotlist is always sorted with respect to the
		    first version. Moreover, this list is complete, i.e. it
		    contains also the trivial slot (-> each version is
		    contained exactly in one slot) */
		mutable SlotList m_slotlist;
		mutable bool m_has_cached_slotlist;

		/// Create new slotlist. Const because we operate on mutable cache
		/// types.
		void build_slotslit() const;

		std::set<std::string> m_collected_iuse;
		mutable std::string m_collected_iuse_cache;

		/** This is called by addVersionFinalize() to calculate
		    coll_iuse and to save memory by freeing original iuse */
		void collect_iuse(const Version *version);

		/** Check if a package has duplicated vsions. */
		void checkDuplicates(const Version *version);

		void sortedPushBack(Version *version);

		void defaults()
		{
			m_has_cached_slotlist = false;
			is_system_package = false;
			have_same_overlay_key = true;
			at_least_two_overlays = false;
			have_duplicate_versions = DUP_NONE;
			have_nontrivial_slots = false;
#if !defined(NOT_FULL_USE)
			versions_have_full_use = false;
#endif
		}
};

class PackageSave {
		typedef std::map<const Version*, KeywordSave> DataType;
		DataType data;
	public:
		PackageSave(const Package *p = NULL)
		{ store(p); }

		void store(const Package *p = NULL);

		void restore(Package *p) const;
};


#endif /* __PACKAGE_H__ */
