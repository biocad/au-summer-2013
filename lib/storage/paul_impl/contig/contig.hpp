#pragma once

#include <deque>
#include <vector>
#include <string>
#include <stack>

#include "trie/trie.hpp"
#include "annotation/annotation.hpp"
#include "kstat/kstat.hpp"
#include "aligner/container.hpp"

#include "contig_const_iterator.hpp"
#include "contig_iterator.hpp"

namespace igc {

template <class T, template <class> class Property, class LabelType=std::string>
class contig
{
public:
    friend class contig_const_iterator<T, Property, LabelType>;
    friend class contig_iterator<T, Property, LabelType>;

    typedef Property<T>                        			  data_type;
    typedef std::vector<byte>				   			  alphabet_type;
    typedef annotation<T, Property, LabelType> 			  anno_type;
    typedef typename anno_type::index          			  link_type;
    typedef typename anno_type::record_type    			  record_type;
    typedef trie<std::vector<link_type>>				  trie_type;
    typedef typename trie_type::index_type				  index_type;
    typedef kstatistics<index_type>		      			  kstat_type;

    typedef typename trie_type::const_iterator      	  const_iterator;
    typedef typename trie_type::iterator       			  iterator;

    typedef contig_const_iterator<T, Property, LabelType> const_aiterator;
    typedef contig_iterator<T, Property, LabelType>       aiterator;

    typedef std::pair<record_type,
                      std::vector<record_type>>           alignment_type;

    contig(std::string const & name, alphabet_type const & alphabet, size_t k = 7)
        : m_name(name), m_stat(alphabet, k)
    {
    }

	void process (std::stringstream& _ss)
	{
		std::string arg;

		std::string file;
		std::string seq;
		std::string matrix;
		int count;

		while(_ss >> arg) {
			if (arg=="add") {
				std::string type;
				_ss >> type;
				if(!type.empty()) {
					if(type=="seq") {
						std::string name;
						std::string seq;
						_ss >> name;
						_ss >> seq;
						if(!name.empty() && !seq.empty()) {
							push(seq.begin(), seq.end(), Lab(name));
						}
						else {
							std::cout << "Empty name or seq argument in add command" << std::endl;
							return;
						}
					}
					else if(type=="file") {
						std::string path;
						_ss >> path;
						if(!path.empty()) {
							FastaReader FR(path);
							if(FR.is_eof()) {
								std::cout << "No file found at path specified: " << path << std::endl;
								return;
							}
							Read tmp;
							std::string::iterator iter;
							std::string::iterator end;
							while(!FR.is_eof()) {
								FR >> tmp;
								iter = tmp.seq.begin();
								end = tmp.seq.end();
								push(iter, end, Lab(tmp.name));
							}
						}
						else {
							std::cout << "Empty path argument in add command" << std::endl;
							return;
						}
					}
					else {
						std::cout << "Unknown argument in add command: " << type << std::endl;
						return;
					}
				}
				else {
					std::cout << "Empty argument in add command" << std::endl;
					return;
				}
			}
			else if (arg=="find") {
				std::string seq;
				_ss >> seq;
				if(!seq.empty()) {
					std::vector<size_t> result = find(seq.begin(), seq.end());
					int size = result.size();
					std::cout << size << std::endl;
					return;
				}
				else {
					std::cout << "Empty argument in find command" << std::endl;
					return;
				}
			}
			else if (arg=="align") {
			}
			else {
				std::cout << "Unknown command: " << arg << std::endl;
				return;
			}
		}
	}

    template <class Iterator1, class Iterator2>
    iterator push(Iterator1 begin, Iterator1 end, Iterator2 anno_begin,
                  LabelType label)
    {
        size_t record_length = std::distance(begin, end);

        record_type & new_record = m_anno.add(label, record_length);
        iterator trie_iter = m_trie.begin();

        link_type link;

        for (Iterator1 iter = begin; iter != end; ++iter, ++anno_begin)
        {
            byte symbol = *iter;
            data_type new_data = *anno_begin;

            // Add trie node
            trie_iter = m_trie.insert(trie_iter, symbol);

            // Add annotation node and link annotation to trie node
            data_type & data = new_record.push(trie_iter.index(), symbol);
            // Add annotation data
            data = new_data;

            // Link trie node and annotation data
            trie_iter->push_back(m_anno.last());

            // Add statistics data and link it with the node
            m_stat.add(iter, end, trie_iter.index());
        }

        return trie_iter - (record_length - 1);
    }

    template <class Iterator>
    iterator push(Iterator begin, Iterator end, LabelType label)
    {
        size_t record_length = std::distance(begin, end);

        record_type & new_record = m_anno.add(label, record_length);
        typename trie_type::iterator trie_iter = m_trie.begin();

        link_type link;
        for (Iterator iter = begin; iter != end; ++iter)
        {
            byte symbol = *iter;
            data_type new_data = data_type();

            // Add trie node
            trie_iter = m_trie.insert(trie_iter, symbol);

            // Add annotation node and link annotation to trie node
            data_type & data = new_record.push(trie_iter.index(), symbol);
            // Add annotation data
            data = new_data;

            // Link trie node and annotation data
            trie_iter->push_back(m_anno.last());

            // Add statistics data and link it with the node
            m_stat.add(iter, end, trie_iter.index());
        }

        return trie_iter - (record_length - 1);
    }

    std::vector<data_type> getAnnotations(const_iterator iter) const
    {
        std::vector<data_type> result;
        for (auto i = iter->begin(); i != iter->end(); ++i)
        {
            result.push_back(m_anno[*i]);
        }
        return result;
    }

    std::vector<LabelType> getLabels(const_iterator iter) const
    {
        std::vector<LabelType> result;
        for (auto i = iter->begin(); i != iter->end(); ++i)
        {
            result.push_back(m_anno.labelOf(*i));
        }
        return result;
    }

    // Trie iterators
    iterator begin()
    {
        return m_trie.begin();
    }

    iterator end()
    {
        return m_trie.end();
    }

    const_iterator begin() const
    {
        return m_trie.begin();
    }

    const_iterator end() const
    {
        return m_trie.end();
    }

    iterator iter(index_type i)
    {
        return iterator(&m_trie, i);
    }

    const_iterator iter(index_type i) const
    {
        return const_iterator(&m_trie, i);
    }

    // Annotation iterators
    aiterator abegin()
    {
        return aiterator(this, begin());
    }

    const_aiterator abegin() const
    {
        return const_aiterator(this, begin());
    }

    aiterator aend()
    {
        return aiterator(this, end());
    }

    const_aiterator aend() const
    {
        return const_aiterator(this, end());
    }

    aiterator aiter(index_type i)
    {
        return aiterator(this, i);
    }

    const_aiterator aiter(index_type i) const
    {
        return const_aiterator(this, i);
    }

    template <class S>
    void copyTrie(trie<S> ** t) const
    {
        *t = new trie<S>(m_trie);
    }

    record_type & getRecord(LabelType const & label)
    {
        return m_anno.getRecordByLabel(label);
    }

    size_t size()
    {
        return m_trie.size();
    }

// API Methods
    template <class Iterator>
    std::vector<index_type> find(Iterator begin, Iterator end)
    {
        typedef typename trie<bool>::iterator tb_iterator;

        std::vector<index_type> result;
        trie<bool>* tmp = nullptr;
        copyTrie(&tmp);

        size_t deep = 0;
        const std::set<index_type>* nodes = nullptr;
        for (Iterator iter = begin; iter != end; ++iter)
        {
            nodes = m_stat.get(iter, end);
            if (nodes == nullptr)
            {
                break;
            }
            for (auto niter = nodes->begin(); niter != nodes->end(); ++niter)
            {
                tb_iterator t(tmp, *niter);
                *t = true;
            }
            deep++;
        }

        if (nodes == nullptr)
        {
            return result;
        }

        for (auto niter = nodes->begin(); niter != nodes->end(); ++niter)
        {
            bool breakflag = false;
            tb_iterator t(tmp, *niter);
            for (size_t i = deep-1; i != 0; --i)
            {
                --t;
                if (t == tmp->end() || !*t)
                {
                    breakflag = true;
                    break;
                }
            }
            if (!breakflag)
            {
                result.push_back(t.index());
            }
        }

        delete tmp;
        return result;
    }

    template <class Iterator>
    std::vector<alignment_result> align(Iterator begin, Iterator end,
                                        int gap, score_matrix const & matrix,
                                        size_t count)
    {
        typedef std::pair<std::string, simple_matrix2i> node_cache_type;
        typedef std::pair<std::string, simple_matrix2i*> stack_cache_type;
        typedef trie<node_cache_type>::iterator cmp_t;

        std::vector<alignment_result> results;
        std::string query(begin, end);
        alicont ali(query, gap, matrix);
        trie<node_cache_type>* tmp;
        copyTrie(&tmp);

        std::string target;
        std::stack<size_t> index_stack;
        std::vector<trie<node_cache_type>::iterator> leafs;
        for (trie<node_cache_type>::iterator i = tmp->begin() + 1;
                                             i != tmp->end();
                                             ++i)
        {
            target.push_back(i.symbol());
            if (!index_stack.empty() && i.prev().fork() &&
                    i.prev().index() != index_stack.top())
            {
                while (!index_stack.empty() && index_stack.top() != i.prev().index())
                {
                    index_stack.pop();
                    ali.pop();
                }
            }
            if (i.leaf())
            {
                leafs.push_back(i);
                *i = std::make_pair(target, ali.score(target));
                ali.push(target, &(i->second));
                index_stack.push(i.index());
                target.clear();
            }
            else if (i.fork())
            {
                *i = std::make_pair(target, ali.score(target));
                ali.push(target, &(i->second));
                index_stack.push(i.index());
                target.clear();
            }
        }

//        for(auto i : leafs)
//        {
//            for (auto j : *i)
//            {
//                for (auto k : j)
//                {
//                    std::cout << std::setw(3) << k << " ";
//                }
//                std::cout << std::endl;
//            }
//            std::cout << std::endl;
//        }
//        std::cout << std::endl;

        std::sort(leafs.begin(), leafs.end(), [](cmp_t a, cmp_t b)
            {return a->second.back().back() > b->second.back().back();});
        size_t current_count = 0;
        for (std::vector<trie<node_cache_type>::iterator>::iterator i = leafs.begin();
                                                                   i != leafs.end();
                                                                   ++i, ++current_count)
        {
            if (current_count == count)
            {
                break;
            }
            ali.clear();

            std::stack<stack_cache_type> st;
            trie<node_cache_type>::iterator iter = *i;

            while (iter != tmp->begin())
            {
                if (iter->second.size() != 0)
                {
                    st.push(std::make_pair(iter->first, &(iter->second)));
                }
                --iter;
            }

            while (st.size() != 0)
            {
                stack_cache_type t = st.top();
                st.pop();
                ali.push(t.first, t.second);
            }

            results.push_back(ali.alignment());
        }

        delete tmp;
        return results;
    }

private:
    std::string m_name;
    trie_type   m_trie;
    anno_type   m_anno;
    kstat_type  m_stat;
};
}