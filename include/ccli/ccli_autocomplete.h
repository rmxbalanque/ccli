
#ifndef CCLI_CCLI_AUTOCOMPLETE_H
#define CCLI_CCLI_AUTOCOMPLETE_H

#pragma once

#include <vector>
#include <string>
#include "ccli/ccli_api.h"

namespace ccli
{
	// TODO: Check how to add support for UTF Encoding.
	// TODO: Todo add max word suggestion depth.
	// TODO: Only use "const char *" or "std::string" in ccli. (On stl containers use iterators - SLOW). (Need to add std::string version)
	// TODO: Add test cases for all of the functions.
	
	//!< Auto complete ternary tree.
	class CCLI_API acTernarySearchTree
	{
	public:

		// Type definitions.
		using r_sVector = std::vector<std::string> &;
		using sVector = std::vector<std::string>;
		using p_sVector = std::vector<std::string> *;

		//!< Autocomplete node.
		struct acNode
		{
			explicit acNode(const char data, bool isWord = false) : m_Data(data), m_IsWord(isWord), m_Less(nullptr), m_Equal(nullptr), m_Greater(nullptr)
			{};

			explicit acNode(const char &&data, bool isWord = false) : m_Data(data), m_IsWord(isWord), m_Less(nullptr), m_Equal(nullptr), m_Greater(nullptr)
			{};

			~acNode()
			{
				delete m_Less;
				delete m_Equal;
				delete m_Greater;
			};

			char m_Data;		//!< Node data.
			bool m_IsWord;		//!< Flag to determine if node is the end of a word.
			acNode *m_Less;		//!< Left pointer.
			acNode *m_Equal;	//!< Middle pointer.
			acNode *m_Greater; 	//!< Right pointer.
		};

		// Default constructor.
		acTernarySearchTree() = default;

		// No copy allowed
		acTernarySearchTree(const acTernarySearchTree &tree) = default;
		acTernarySearchTree(acTernarySearchTree &&tree) noexcept = default;

		/*!
		 *
		 * \tparam inputType
		 * \param[in] il
		 */
		template<typename inputType>
		acTernarySearchTree(std::initializer_list<inputType> il)
		{
			for (const auto &item : il)
			{
				insert(item);
			}
		}

		/*!
		 *
		 * \tparam T
		 * \param[in] items Container
		 */
		template<typename T>
		explicit acTernarySearchTree(const T &items)
		{
			for (const auto &item : items)
			{
				insert(item);
			}
		}

		/*!
		 * /brief Destructor
		 */
		~acTernarySearchTree();

		/*!
		 * \brief Get tree node count
		 * \return Tree node count
		 */
		[[nodiscard]] size_t size() const;

		/*!
		 * \brief Get tree word count
		 * \return Word count
		 */
		[[nodiscard]] size_t count() const;

		/*!
		 * \brief Search if the given word is in the tree
		 * \param[in] word Word to search
		 * \return Found
		 */
		bool search(const char *word);

		/*!
		 * \brief Insert word into tree
		 * \param[in] word Word to be inserted
		 */
		void insert(const char *word);

		/*!
		 * \brief Insert word into tree
		 * \param[in] word Word to be inserted
		 */
		void insert(const std::string &word);

		/*!
		 * \brief Insert word into tree
		 * \tparam strType String type to be inserted
		 * \param[in] word Word to be inserted
		 */
		template<typename strType>
		void insert(const strType &word)
		{
			acNode **ptr = &m_Root;
			++m_Count;

			while (*word != '\0')
			{
				// Insert char into tree.
				if (*ptr == nullptr)
				{
					*ptr = new acNode(*word);
					++m_Size;
				}

				// Traverse tree.
				if (*word < (*ptr)->m_Data)
				{
					ptr = &(*ptr)->m_Less;
				}
				else if (*word == (*ptr)->m_Data)
				{
					// String is already in tree, therefore only mark as word.
					if (*(word + 1) == '\0')
					{
						if ((*ptr)->m_IsWord)
							--m_Count;

						(*ptr)->m_IsWord = true;
					}

					// Advance.
					ptr = &(*ptr)->m_Equal;
					++word;
				}
				else
				{
					ptr = &(*ptr)->m_Greater;
				}
			}
		}

		/*!
		 * \brief Removes a word from the tree if found
		 * \param[in] word String to be removed
		 */
		void remove(const std::string &word);

		/*!
		 * \brief Retrieve suggestions that match the given prefix
		 * \tparam strType Prefix string type
		 * \param[in] prefix Prefix to use for suggestion lookup
		 * \param[out] ac_options Vector of found suggestions
		 */
		template<typename strType>
		void suggestions(const strType &prefix, r_sVector ac_options)
		{
			acNode *ptr = m_Root;
			auto temp = prefix;

			// Traverse tree and check if prefix exists.
			while (ptr)
			{
				if (*prefix < ptr->m_Data)
				{
					ptr = ptr->m_Less;
				}
				else if (*prefix == ptr->m_Data)
				{
					// Prefix exists in tree.
					if (*(prefix + 1) == '\0')
						break;

					ptr = ptr->m_Equal;
					++prefix;
				}
				else
				{
					ptr = ptr->m_Greater;
				}
			}

			// Already a word. (No need to auto complete).
			if (ptr && ptr->m_IsWord) return;

			// Prefix is not in tree.
			if (!ptr) return;

			// Retrieve auto complete options.
			suggestionsAux(ptr->m_Equal, ac_options, temp);
		}


		/*!
		 * \brief Retrieve suggestions that match the given prefix
		 * \param[in] prefix Prefix to use for suggestion lookup
		 * \param[out] ac_options Vector of found suggestions
		 */
		void suggestions(const char *prefix, r_sVector ac_options);

		/*!
		 * \brief Store suggestions that match prefix in ac_options and return partially completed
		 * 	      prefix if possible.
		 * \param[in] prefix Prefix to use for suggestion lookup
		 * \param[out] ac_options Vector of found suggestions
		 * \return Partially completed prefix
		 */
		std::string suggestions(const std::string &prefix, r_sVector ac_options);

		/*!
		 * \brief Retrieve suggestions that match the given prefix
		 * \param[in/out] prefix Prefix to use for suggestion lookup, will be partially completed if flag partial_complete is on
		 * \param[out] ac_options Vector of found suggestions
		 * \param[in] partial_complete Flag to determine if prefix string will be partially completed
		 */
		void suggestions(std::string & prefix, r_sVector ac_options, bool partial_complete);

		/*!
		 * \brief Retrieve suggestions that match the given prefix
		 * \tparam strType Prefix string type
		 * \param[in] prefix Prefix to use for suggestion lookup
		 * \return Vector of found suggestions
		 */
		template<typename strType>
		p_sVector suggestions(const strType &prefix)
		{
			auto temp = new sVector();
			suggestions(prefix, *temp);
			return temp;
		}
		
		/*!
		 * \brief Retrieve suggestions that match the given prefix
		 * \param[in] prefix Prefix to use for suggestion lookup
		 * \return Vector of found suggestions
		 */
		p_sVector suggestions(const char *prefix);

	private:

		/*!
		 * \param[in] root Permutation root
		 * \param[out] ac_options Vector of found suggestions
		 * \param[in] buffer Prefix buffer
		 */
		void suggestionsAux(acNode *root, r_sVector ac_options, std::string buffer);

		/*!
		 * \brief Remove word auxiliary function
		 * \param[in] root Current node to process
		 * \param[in] word String to look for and remove
		 * \return If node is word
		 */
		bool removeAux(acNode *root, const char * word);

		acNode *m_Root = nullptr;  	//!< Ternary Search Tree Root node
		size_t m_Size = 0;			//!< Node count
		size_t m_Count = 0;			//!< Word count
	};
}

#ifdef CCLI_HEADER_ONLY
#include "ccli/ccli_autocomplete.inl"
#endif

#endif //CCLI_CCLI_AUTOCOMPLETE_H
