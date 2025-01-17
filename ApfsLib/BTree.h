/*
	This file is part of apfs-fuse, a read-only implementation of APFS
	(Apple File System) for FUSE.
	Copyright (C) 2017 Simon Gander

	Apfs-fuse is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	Apfs-fuse is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with apfs-fuse.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <vector>
#include <map>
#include <memory>
#include <mutex>

#include "Global.h"
#include "DiskStruct.h"

#include "ApfsNodeMapper.h"

class BTree;
class BTreeNode;
class BTreeIterator;
class BlockDumper;

class ApfsContainer;
class ApfsVolume;

// This enables a rudimentary disk cache ...
#if !(defined(_LIBCPP_HAS_NO_THREADS) || defined(M1N1) || defined(__UBOOT__) || defined(JEV_BAREMETAL))
#define BTREE_USE_MAP
#endif
// TODO: Think about a better solution.
// 8192 will take max. 32 MB of RAM. Higher may be faster, but use more RAM.
#define BTREE_MAP_MAX_NODES 8192

// ekey < skey: -1, ekey > skey: 1, ekey == skey: 0
typedef int(*BTCompareFunc)(const void *skey, size_t skey_len, const void *ekey, size_t ekey_len, void *context);

int CompareStdKey(const void *skey, size_t skey_len, const void *ekey, size_t ekey_len, void *context);

class BTreeEntry
{
	friend class BTree;
public:
	BTreeEntry();
	~BTreeEntry();

	BTreeEntry(const BTreeEntry &o) = delete;
	BTreeEntry &operator=(const BTreeEntry &o) = delete;

	void clear();

	const void *key;
	const void *val;
	size_t key_len;
	size_t val_len;

private:
	std::shared_ptr<BTreeNode> m_node;
};

class BTreeNode
{
protected:
	BTreeNode(BTree &tree, const uint8_t *block, size_t blocksize, paddr_t paddr, const std::shared_ptr<BTreeNode> &parent, uint32_t parent_index);

public:
	static std::shared_ptr<BTreeNode> CreateNode(BTree &tree, const uint8_t *block, size_t blocksize, paddr_t paddr, const std::shared_ptr<BTreeNode> &parent, uint32_t parent_index);

	virtual ~BTreeNode();

	uint64_t nodeid() const { return m_btn->btn_o.o_oid; }
	uint32_t entries_cnt() const { return m_btn->btn_nkeys; }
	uint16_t level() const { return m_btn->btn_level; }
	uint16_t flags() const { return m_btn->btn_flags; }
	paddr_t paddr() const { return m_paddr; }

	const std::shared_ptr<BTreeNode> &parent() const { return m_parent; }
	uint16_t parent_index() const { return m_parent_index; }

	virtual bool GetEntry(BTreeEntry &result, uint32_t index) const = 0;
	// virtual uint32_t Find(const void *key, size_t key_size, BTCompareFunc func) const = 0;

	const std::vector<uint8_t> &block() const { return m_block; }

protected:
	std::vector<uint8_t> m_block;
	BTree &m_tree;

	uint16_t m_keys_start; // Up
	uint16_t m_vals_start; // Dn

	const uint32_t m_parent_index;
	const std::shared_ptr<BTreeNode> m_parent;

	const paddr_t m_paddr;

	const btree_node_phys_t *m_btn;
};

class BTreeNodeFix : public BTreeNode
{
public:
	BTreeNodeFix(BTree &tree, const uint8_t *block, size_t blocksize, paddr_t paddr, const std::shared_ptr<BTreeNode> &parent, uint32_t parent_index);

	bool GetEntry(BTreeEntry &result, uint32_t index) const override;
	// uint32_t Find(const void *key, size_t key_size, BTCompareFunc func) const override;

private:
	const kvoff_t *m_entries;
};

class BTreeNodeVar : public BTreeNode
{
public:
	BTreeNodeVar(BTree &tree, const uint8_t *block, size_t blocksize, paddr_t paddr, const std::shared_ptr<BTreeNode> &parent, uint32_t parent_index);

	bool GetEntry(BTreeEntry &result, uint32_t index) const override;
	// uint32_t Find(const void *key, size_t key_size, BTCompareFunc func) const override;

private:
	const kvloc_t *m_entries;
};

class BTree
{
	enum class FindMode
	{
		EQ,
		LE,
		LT,
		GE,
		GT
	};

	friend class BTreeIterator;
public:
	BTree(ApfsContainer &container, ApfsVolume *vol = nullptr);
	~BTree();

	bool Init(oid_t oid_root, xid_t xid, ApfsNodeMapper *omap = nullptr);

	bool Lookup(BTreeEntry &result, const void *key, size_t key_size, BTCompareFunc func, void *context, bool exact);
	bool GetIterator(BTreeIterator &it, const void *key, size_t key_size, BTCompareFunc func, void *context);
	bool GetIteratorBegin(BTreeIterator &it);

	uint16_t GetKeyLen() const { return m_treeinfo.bt_fixed.bt_key_size; }
	uint16_t GetValLen() const { return m_treeinfo.bt_fixed.bt_val_size; }

	void dump(BlockDumper &out);

	void EnableDebugOutput() { m_debug = true; }

private:
	void DumpTreeInternal(BlockDumper &out, const std::shared_ptr<BTreeNode> &node);
	uint32_t Find(const std::shared_ptr<BTreeNode> &node, const void *key, size_t key_size, BTCompareFunc func, void *context);
	int FindBin(const std::shared_ptr<BTreeNode> &node, const void *key, size_t key_size, BTCompareFunc func, void *context, FindMode mode);

	std::shared_ptr<BTreeNode> GetNode(oid_t oid, const std::shared_ptr<BTreeNode> &parent, uint32_t parent_index);

	ApfsContainer &m_container;
	ApfsVolume *m_volume;

	std::shared_ptr<BTreeNode> m_root_node;
	ApfsNodeMapper *m_omap;

	btree_info_t m_treeinfo;

	oid_t m_oid;
	xid_t m_xid;
	bool m_debug;

#ifdef BTREE_USE_MAP
	std::map<uint64_t, std::shared_ptr<BTreeNode>> m_nodes;
	std::mutex m_mutex;
#endif
};

class BTreeIterator
{
public:
	BTreeIterator();
	BTreeIterator(BTree *tree, const std::shared_ptr<BTreeNode> &node, uint32_t index);
	~BTreeIterator();

	bool next();
	void reset();

	bool GetEntry(BTreeEntry &res) const;

	void Setup(BTree *tree, const std::shared_ptr<BTreeNode> &node, uint32_t index);

private:
	BTree *m_tree;
	std::shared_ptr<BTreeNode> m_node;
	uint32_t m_index;

	std::shared_ptr<BTreeNode> next_node();
};
