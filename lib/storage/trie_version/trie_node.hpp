#pragma once

#include <map>

#include "debug.h"

typedef unsigned char byte;

class trie_node
{
public:
	typedef std::map<byte, trie_node*> children_t;

	trie_node(trie_node* parent_, byte symbol_, size_t id_)
		: m_parent(parent_), m_symbol(symbol_), m_id(id_)
	{

	}

	~trie_node()
	{
		for (children_t::iterator i = m_children.begin(); i != m_children.end(); ++i)
		{
			delete i->second;
		}
	}

	trie_node* push(byte next_symbol, size_t next_id)
	{
		children_t::iterator i = m_children.find(next_symbol);
		if (i != m_children.end())
		{
			return i->second;
		}
		trie_node* new_node = new trie_node(this, next_symbol, next_id);
		m_children.insert(std::make_pair(next_symbol, new_node));
		return new_node;
	}

	trie_node* next(byte next_symbol)
	{
		children_t::iterator i = m_children.find(next_symbol);
		if (i != m_children.end())
		{
			return i->second;
		}
		return nullptr;
	}

	trie_node* prev()
	{
		return m_parent;
	}

	byte symbol()
	{
		return m_symbol;
	}

	size_t id()
	{
		return m_id;
	}

	children_t* getChildren()
	{
		return &m_children;
	}

private:
	children_t m_children;
	trie_node* m_parent;
	byte       m_symbol;
	size_t     m_id;
};