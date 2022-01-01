#include <string>

#include "PList.h"

PLArray::PLArray()
{
}

PLArray::~PLArray()
{
	for (std::vector<PLObject *>::iterator it = m_array.begin(); it != m_array.end(); it++)
		delete *it;
	m_array.clear();
}

PLObject * PLArray::get(size_t idx) const
{
	if (idx < m_array.size())
		return m_array[idx];
	else
		return nullptr;
}

PLDict::PLDict()
{
}

PLDict::~PLDict()
{
	for (std::map<std::string, PLObject*>::iterator it = m_dict.begin(); it != m_dict.end(); it++)
		delete it->second;
	m_dict.clear();
}

PLObject * PLDict::get(const char * name) const
{
	std::map<std::string, PLObject *>::const_iterator it = m_dict.find(name);

	if (it != m_dict.cend())
		return it->second;
	else
		return nullptr;
}

PListXmlParser::PListXmlParser(const char * data, size_t size)
	:
	m_data(data),
	m_size(size)
{
	m_idx = 0;
}

PListXmlParser::~PListXmlParser()
{

}

ExpectedPLObject PListXmlParser::Parse()
{
	std::string name;
	std::string content;
	TagType type;

	while (FindTag(name, type))
	{
		if (type == TagType::Start && name == "plist")
		{
			ExpectedPLObject root_obj = ParseObject();
			if (!root_obj) {
				fprintf(stderr, "XML PList parse error: %s\n", root_obj.error());
				return kz::unexpected{root_obj.error()};
			}
			if (std::holds_alternative<PLDict>(**root_obj) || std::holds_alternative<PLArray>(**root_obj)) {
				return *root_obj;
			} else {
				return kz::unexpected{"PList root wasn't dict or array."};
			}

			break;
		}
	}

	return kz::unexpected{"PList root fail."};;
}

ExpectedPLArray PListXmlParser::ParseArray()
{
	PLArray *arr = new PLArray();

	for (;;)
	{
		ExpectedPLObject obj = ParseObject();

		if (obj)
			arr->m_array.push_back(*obj);
		else
			return kz::unexpected{obj.error()};
	}

	return arr;
}

ExpectedPLDict PListXmlParser::ParseDict()
{
	PLDict *dict = new PLDict();

	TagType tagtype;
	std::string tagname;
	std::string key;

	for (;;)
	{
		FindTag(tagname, tagtype);

		if (tagname == "dict" && tagtype == TagType::End)
			break;

		if (tagname != "key" || tagtype != TagType::Start)
			return kz::unexpected{"Invalid tag in dict"};

		GetContent(key);

		if (key.empty())
			return kz::unexpected{"Empty key in dict"};

		FindTag(tagname, tagtype);

		if (tagname != "key" || tagtype != TagType::End)
			return kz::unexpected{"Invalid tag type, expected </key>"};

		ExpectedPLObject obj = ParseObject();

		if (obj)
			dict->m_dict[key] = *obj;
		else
			return kz::unexpected{obj.error()};
	}

	return dict;
}

ExpectedPLObject PListXmlParser::ParseObject()
{
	std::string name;
	TagType type;
	std::string content_str;
	const char *content_start;
	size_t content_size;

	FindTag(name, type);

	if (type == TagType::Start)
	{
		if (name == "integer")
		{
			GetContent(content_str);
			FindTag(name, type);
			if (name != "integer" || type != TagType::End)
				return kz::unexpected{"Invalid end tag, expected </integer>."};

			return new PLObject{PLInteger{strtoll(content_str.c_str(), nullptr, 0)}};
		}
		else if (name == "string")
		{
			GetContent(content_str);
			FindTag(name, type);
			if (name != "string" || type != TagType::End)
				return kz::unexpected{"Invalid end tag, expected </string>."};

			return new PLObject{PLString{content_str}};
		}
		else if (name == "data")
		{
			content_start = m_data + m_idx;
			content_size = GetContentSize();

			FindTag(name, type);
			if (name != "data" || type != TagType::End)
				return kz::unexpected{"Invalid end tag, expected </data>."};

			PLObject *obj = new PLObject{PLData{}};
			Base64Decode(std::get_if<PLData>(obj)->m_data, content_start, content_size);
			return obj;
		}
		else if (name == "array")
		{
			ExpectedPLArray obj = ParseArray();
			if (!obj)
				return kz::unexpected{obj.error()};
			return obj;
		}
		else if (name == "dict")
		{
			PLDict *obj = ParseDict();
			if (!obj)
				return kz::unexpected{"Missing dict."};
			*robj = *obj;
		}
		else
		{
			return kz::unexpected{"Unexpected start tag."};
		}
	}
	else if (type == TagType::Empty)
	{
		if (name == "true")
		{
			PLInteger *obj = new PLInteger();
			obj->m_value = 1;
			*robj = *obj;
		}
		else if (name == "false")
		{
			PLInteger *obj = new PLInteger();
			obj->m_value = 0;
			*robj = *obj;
		}
		else
		{
			kz::unexpected{"Unexpected empty tag."};
		}
	}
	else if (type == TagType::End)
	{
		return kz::unexpected{"PList end."};
	}

	return robj;
}

void PListXmlParser::Base64Decode(std::vector<uint8_t>& bin, const char * str, size_t size)
{
	int chcnt;
	uint32_t buf;
	size_t ip;
	char ch;
	uint32_t dec;

	bin.clear();
	bin.reserve(size * 4 / 3);

	chcnt = 0;
	buf = 0;

	for (ip = 0; ip < size; ip++)
	{
		ch = str[ip];

		if (ch >= 'A' && ch <= 'Z')
			dec = ch - 'A';
		else if (ch >= 'a' && ch <= 'z')
			dec = ch - 'a' + 0x1A;
		else if (ch >= '0' && ch <= '9')
			dec = ch - '0' + 0x34;
		else if (ch == '+')
			dec = 0x3E;
		else if (ch == '/')
			dec = 0x3F;
		else if (ch != '=')
			continue;
		else
			break;

		buf = (buf << 6) | dec;
		chcnt++;

		if (chcnt == 2)
			bin.push_back((buf >> 4) & 0xFF);
		else if (chcnt == 3)
			bin.push_back((buf >> 2) & 0xFF);
		else if (chcnt == 4)
		{
			bin.push_back((buf >> 0) & 0xFF);
			chcnt = 0;
		}
	}
}

bool PListXmlParser::FindTag(std::string & name, TagType & type)
{
	char ch;
	bool in_name = false;

	name.clear();
	type = TagType::None;

	do {
		ch = GetChar();
	} while (ch != '<' && ch != 0);

	if (ch == 0)
		return false;

	ch = GetChar();

	switch (ch)
	{
	case '?':
		type = TagType::ProcInstr;
		break;
	case '!':
		type = TagType::Doctype;
		break;
	case '/':
		type = TagType::End;
		in_name = true;
		break;
	default:
		type = TagType::Start;
		name.push_back(ch);
		in_name = true;
		break;
	}

	do
	{
		ch = GetChar();

		switch (ch)
		{
		case 0x09:
		case 0x0A:
		case 0x0D:
		case ' ':
		case '>':
			in_name = false;
			break;
		case '/':
			if (m_data[m_idx] == '>')
				type = TagType::Empty;
			in_name = false;
			break;
		}

		if (in_name)
			name.push_back(ch);
	} while (ch != '>' && ch != 0);

	return true;
}

bool PListXmlParser::GetContent(std::string & content)
{
	size_t start = m_idx;

	content.clear();

	while (m_idx < m_size && m_data[m_idx] != '<')
		m_idx++;

	if (m_idx == m_size)
		return false;

	content.assign(m_data + start, m_idx - start);

	return true;
}

size_t PListXmlParser::GetContentSize()
{
	size_t start = m_idx;

	while (m_idx < m_size && m_data[m_idx] != '<')
		m_idx++;

	if (m_idx == m_size)
		return m_idx - start;

	return m_idx - start;
}
