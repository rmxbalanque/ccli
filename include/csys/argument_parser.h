// Copyright (c) 2020-present, Roland Munguia & Tristan Florian Bouchard.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#ifndef CSYS_ARGUMENT_PARSER_H
#define CSYS_ARGUMENT_PARSER_H
#pragma once

#include "csys/api.h"
#include "csys/string.h"
#include "csys/exceptions.h"
#include <vector>
#include <stdexcept>

// TODO: Make argument list null terminating to test if the function has been given more arguments than needed
namespace csys
{
	namespace
	{
		inline constexpr std::string_view s_Reserved("\\[]\"");
		inline constexpr char s_ErrMsgReserved[] = "Reserved chars '\\, [, ], \"' must be escaped with \\";
	}

	// unary argument
	template<typename T>
	struct CSYS_API ArgumentParser
	{
		inline ArgumentParser(String &input, size_t &start);
		static inline bool IsEscapeChar(char c) { return c == '\\'; }
		static inline bool IsReservedChar(char c)
		{
			for (auto rc : s_Reserved) if (c == rc) return true;
			return false;
		}
		static inline bool IsEscaping(std::string &input, size_t pos)
		{
			return pos < input.size() - 1 && IsEscapeChar(input[pos]) && IsReservedChar(input[pos + 1]);
		}
		static inline bool IsEscaped(std::string &input, size_t pos)
		{
			bool result = false;
			for (size_t i = pos; i > 0; --i)
				if (IsReservedChar(input[i]) && IsEscapeChar(input[i - 1]))
					result = !result;
				else
					break;
			return result;
		}
		T m_Value;
	};

	template<typename T>
	inline ArgumentParser<T>::ArgumentParser(String &input, size_t &start)
	{
		// getting rid of warnings
		if (input.End() == start) {}

		// TYPE T NOT SUPPORTED
		throw Exception("Unsupported type: " + std::string(typeid(T).name()));
	}

#define ARG_PARSE_BASE_SPEC(TYPE) \
  template<> \
  struct CSYS_API ArgumentParser<TYPE> \
  { \
    inline ArgumentParser(String &input, size_t &start); \
		static inline bool IsEscapeChar(char c) { return c == '\\'; } \
	  static inline bool IsReservedChar(char c) \
	  { \
	  	for (auto rc : s_Reserved) if (c == rc) return true; \
			return false; \
		} \
		static inline bool IsEscaping(std::string &input, size_t pos) \
		{ \
			return pos < input.size() - 1 && IsEscapeChar(input[pos]) && IsReservedChar(input[pos + 1]); \
		} \
		static inline bool IsEscaped(std::string &input, size_t pos) \
		{ \
			bool result = false; \
			for (size_t i = pos; i > 0; --i) \
				if (IsReservedChar(input[i]) && IsEscapeChar(input[i - 1])) \
					result = !result; \
				else \
					break; \
			return result; \
		} \
    TYPE m_Value = 0; \
  }; \
  inline ArgumentParser<TYPE>::ArgumentParser(String &input, size_t &start)

#define ARG_PARSE_SUBSTR(range) input.m_String.substr(range.first, range.second - range.first)

#define ARG_PARSE_GENERAL_SPEC(TYPE, TYPE_NAME, FUNCTION) \
  ARG_PARSE_BASE_SPEC(TYPE) \
  { \
    auto range = input.NextPoi(start); \
    try \
    { \
      m_Value = (TYPE)FUNCTION(input.m_String, &range.first); \
    } \
    catch (const std::out_of_range&) \
    { \
			throw Exception(std::string("Argument too large for ") + TYPE_NAME, \
    													input.m_String.substr(range.first, range.second));  \
		} \
    catch (const std::invalid_argument&) \
    { \
    	throw Exception(std::string("Missing or invalid ") + TYPE_NAME + " argument", \
    													input.m_String.substr(range.first, range.second)); } \
  }

	ARG_PARSE_BASE_SPEC(csys::String)
	{
		static auto GetWord = [](std::string &str, size_t start, size_t end) {
				// For issues with reserved chars
				static std::string invalid_chars;
				invalid_chars.clear();

			  std::string result;

				for (size_t i = start; i < end; ++i)
					// general case, not reserved char
					if (!IsReservedChar(str[i]))
						result.push_back(str[i]);
					// is a reserved char
					else
					{
						// check for \ char and if its escaping
						if (IsEscapeChar(str[i]) && IsEscaping(str, i))
							result.push_back(str[++i]);
						// reserved char but not being escaped
						else
							throw Exception(s_ErrMsgReserved, str.substr(start, end - start));
					}

				return result;
		};
		auto range = input.NextPoi(start);

		// If its a single string
		if (input.m_String[range.first] != '"')
			m_Value = GetWord(input.m_String, range.first, range.second);
		// Multi word string
		else
		{
			++range.first;
			while (true)
			{
				// Get a non-escaped "
				range.second = input.m_String.find('"', range.first);
				while (range.second != std::string::npos && IsEscaped(input.m_String, range.second))
					range.second = input.m_String.find('"', range.second + 1);

				// Check for closing "
				if (range.second == std::string::npos)
				{
					range.second = input.m_String.size();
					throw Exception("Could not find closing '\"'", ARG_PARSE_SUBSTR(range));
				}

				// Add word to already existing string
				m_Value.m_String += GetWord(input.m_String, range.first, range.second);

				// Go to next word
				range.first = range.second + 1;

				// End of string check
				if (range.first < input.m_String.size() && !std::isspace(input.m_String[range.first]))
				{
					// joining two strings together
					if (input.m_String[range.first] == '"')
						++range.first;
				}
				else
					// End of input
					break;
			}
		}

		// Finished parsing
		start = range.second + 1;
	}

	ARG_PARSE_BASE_SPEC(bool)
	{
		static const char *s_err_msg = "Missing or invalid boolean argument";
		static const char *s_false = "false";
		static const char *s_true = "true";

		// TODO: TEST THIS OUT AND THE NULL_ARGUMENT STUFF 
		// Get argument
		auto range = input.NextPoi(start);

		// check if the length is between the len of "true" and "false"
		input.m_String[range.first] = char(std::tolower(input.m_String[range.first]));

		// true branch
		if (range.second - range.first == 4 && input.m_String[range.first] == 't')
		{
			for (size_t i = range.first + 1; i < range.second; ++i)
				if ((input.m_String[i] = char(std::tolower(input.m_String[i]))) != s_true[i - range.first])
					throw Exception(s_err_msg + std::string(", expected true"), ARG_PARSE_SUBSTR(range));
			m_Value = true;
		}
		// false branch
		else if (range.second - range.first == 5 && input.m_String[range.first] == 'f')
		{
			for (size_t i = range.first + 1; i < range.second; ++i)
				if ((input.m_String[i] = char(std::tolower(input.m_String[i]))) != s_false[i - range.first])
					throw Exception(s_err_msg + std::string(", expected false"), ARG_PARSE_SUBSTR(range));
			m_Value = false;
		}
		// anything else
		else
			throw Exception(s_err_msg, ARG_PARSE_SUBSTR(range));
	}

	ARG_PARSE_BASE_SPEC(char)
	{
		auto range = input.NextPoi(start);
		size_t len = range.second - range.first;

		if (len > 2 || len <= 0)
			throw Exception("Too many or no chars were given", ARG_PARSE_SUBSTR(range));
			// potential reserved char
		else if (len == 2)
		{
			if (!IsEscaping(input.m_String, range.first))
				throw Exception("Too many chars were given", ARG_PARSE_SUBSTR(range));
			m_Value = input.m_String[range.first + 1];
		}
			// if its one char and reserved
		else if (IsReservedChar(input.m_String[range.first]))
			throw Exception(s_ErrMsgReserved, ARG_PARSE_SUBSTR(range));
			// one char, not reserved
		else
			m_Value = input.m_String[range.first];
	}

	ARG_PARSE_BASE_SPEC(unsigned char)
	{
		auto range = input.NextPoi(start);
		size_t len = range.second - range.first;

		if (len > 2 || len <= 0)
			throw Exception("Too many or no chars were given", ARG_PARSE_SUBSTR(range));
			// potential reserved char
		else if (len == 2)
		{
			if (!IsEscaping(input.m_String, range.first))
				throw Exception("Too many chars were given", ARG_PARSE_SUBSTR(range));
			m_Value = static_cast<unsigned char>(input.m_String[range.first + 1]);
		}
			// if its one char and reserved
		else if (IsReservedChar(input.m_String[range.first]))
			throw Exception(s_ErrMsgReserved, ARG_PARSE_SUBSTR(range));
			// one char, not reserved
		else
			m_Value = static_cast<unsigned char>(input.m_String[range.first]);
	}

	// add the type name to the error message
	ARG_PARSE_GENERAL_SPEC(short, "signed short", std::stoi)

	ARG_PARSE_GENERAL_SPEC(unsigned short, "unsigned short", std::stoul)

	ARG_PARSE_GENERAL_SPEC(int, "signed int", std::stoi)

	ARG_PARSE_GENERAL_SPEC(unsigned int, "unsigned int", std::stoul)

	ARG_PARSE_GENERAL_SPEC(long, "long", std::stol)

	ARG_PARSE_GENERAL_SPEC(unsigned long, "unsigned long", std::stoul)

	ARG_PARSE_GENERAL_SPEC(long long, "long long", std::stoll)

	ARG_PARSE_GENERAL_SPEC(unsigned long long, "unsigned long long", std::stoull)

	ARG_PARSE_GENERAL_SPEC(float, "float", std::stof)

	ARG_PARSE_GENERAL_SPEC(double, "double", std::stod)

	ARG_PARSE_GENERAL_SPEC(long double, "long double", std::stold)

	template<typename T>
	struct CSYS_API ArgumentParser<std::vector<T>>
	{
		ArgumentParser(String &input, size_t &start);
		static inline bool IsEscapeChar(char c) { return c == '\\'; }
		static inline bool IsReservedChar(char c)
		{
			for (auto rc : s_Reserved) if (c == rc) return true;
			return false;
		}
		static inline bool IsEscaping(std::string &input, size_t pos)
		{
			return pos < input.size() - 1 && IsEscapeChar(input[pos]) && IsReservedChar(input[pos + 1]);
		}
		static inline bool IsEscaped(std::string &input, size_t pos)
		{
			bool result = false;
			for (size_t i = pos; i > 0; --i)
				if (IsReservedChar(input[i]) && IsEscapeChar(input[i - 1]))
					result = !result;
				else
					break;
			return result;
		}
		std::vector<T> m_Value;
	};

	template<typename T>
	ArgumentParser<std::vector<T>>::ArgumentParser(String &input, size_t &start)
	{
		auto range = input.NextPoi(start);
		if (range.first == input.End()) return;
		if (input.m_String[range.first] != '[')
			throw Exception("Invalid vector argument missing opening [", ARG_PARSE_SUBSTR(range));

		input.m_String[range.first] = ' ';
		while (true)
		{
			range = input.NextPoi(range.first);
			if (range.first == input.End()) return;
			else if (input.m_String[range.first] == '[')
			{
				// shits fucked
				m_Value.push_back(ArgumentParser<T>(input, range.first).m_Value);
			}
			else
			{
				range.second = input.m_String.find(']', range.first);
				while (range.second != std::string::npos && IsEscaped(input.m_String, range.second))
					range.second = input.m_String.find(']', range.second + 1);

				if (range.second == std::string::npos)
				{
					range.second = input.m_String.size();
					throw Exception("Invalid vector argument missing closing ]", ARG_PARSE_SUBSTR(range));
				}

				input.m_String[range.second] = ' ';
				start = range.first;
				while (true)
				{
					if ((range.first = input.NextPoi(range.first).first) >= range.second)
					{
						start = range.first;
						return;
					}
					m_Value.push_back(ArgumentParser<T>(input, start).m_Value);
					range.first = start;
				}
			}
		}
	}
}

#endif //CSYS_ARGUMENT_PARSER_H