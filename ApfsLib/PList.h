#pragma once

#include <vector>
#include <map>
#include <memory>
#include <variant>
#include <cassert>
#include <tl/expected.hpp>

enum class PLType
{
	PLType_Integer,
	PLType_String,
	PLType_Data,
	PLType_Array,
	PLType_Dict,
	PLType_Base,
};

class PLObjectBase;
class PLInteger;
class PLString;
class PLData;
class PLArray;
class PLDict;

using PLObject = std::variant<PLInteger, PLString, PLData, PLArray, PLDict>;
using ExpectedPLObject = tl::expected<PLObject *, const char *>;

class PLObjectBase
{
	friend class PListXmlParser;
public:

	PLType type() const {
		assert(!"type() called on PLObjectBase");
	}

	const PLInteger * toInt() const {
		return nullptr;
	}
	const PLString * toString() const {
		return nullptr;
	}
	const PLData * toData() const {
		return nullptr;
	}
	const PLArray * toArray() const {
		return nullptr;
	}
	const PLDict * toDict() const {
		return nullptr;
	}
};

class PLInteger : public PLObjectBase
{
	friend class PListXmlParser;
public:

	PLType type() const { return PLType::PLType_Integer; }

	int64_t value() const { return m_value; }

private:
	int64_t m_value;
};

class PLString
{
	friend class PListXmlParser;
public:
	PLString();
	virtual ~PLString();

	PLType type() const { return PLType::PLType_String; }

	const std::string &string() const { return m_string; }

private:
	std::string m_string;
};

class PLData
{
	friend class PListXmlParser;
public:
	PLData();
	virtual ~PLData();

	PLType type() const { return PLType::PLType_Data; }

	const uint8_t *data() const { return m_data.data(); }
	size_t size() const { return m_data.size(); }

private:
	std::vector<uint8_t> m_data;
};

class PLArray
{
	friend class PListXmlParser;
public:
	PLArray();
	virtual ~PLArray();

	PLType type() const { return PLType::PLType_Array; }

	PLObject *get(size_t idx) const;
	size_t size() const { return m_array.size(); }

	const std::vector<PLObject *> &array() const { return m_array; }

private:
	std::vector<PLObject *> m_array;
};

class PLDict
{
	friend class PListXmlParser;
public:
	PLDict();
	virtual ~PLDict();

	PLType type() const { return PLType::PLType_Dict; }

	PLObject *get(const char *name) const;

	const std::map<std::string, PLObject *> &dict() const { return m_dict; }

private:
	std::map<std::string, PLObject *> m_dict;
};

class PListXmlParser
{
	enum class TagType
	{
		None,
		Empty,
		Start,
		End,
		ProcInstr,
		Doctype
	};

public:
	PListXmlParser(const char *data, size_t size);
	~PListXmlParser();

	tl::expected<PLDict *, const char *> Parse();

private:
	PLArray * ParseArray();
	PLDict * ParseDict();
	ExpectedPLObject ParseObject();
	void Base64Decode(std::vector<uint8_t> &bin, const char *str, size_t size);

	bool FindTag(std::string &name, TagType &type);
	bool GetContent(std::string &content);
	size_t GetContentSize();

	char GetChar()
	{
		if (m_idx < m_size)
			return m_data[m_idx++];
		else
			return 0;
	}

	const char * const m_data;
	const size_t m_size;
	size_t m_idx;
};
