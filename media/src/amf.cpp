#include "amf.h"
#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <strings.h>
#include <math.h>
#include "log.h"
#include "tools.h"

/****************************
 * Parser helper for U16
 ***************************/
U16Parser::U16Parser()
{
	//Reset
	Reset();
}

void U16Parser::Reset()
{
	len = 2;
	value = 0;
}

DWORD U16Parser::Parse(BYTE *data,DWORD size)
{
	BYTE i=0;

	while (len && i<size)
		value |= ((DWORD)data[i++]) << (--len)*8;

	return i;
}

bool U16Parser::IsParsed()
{
	return !(len);
}

WORD U16Parser::GetValue()
{
	return value;
}

void U16Parser::SetValue(WORD value)
{
	this->value = value;
}

DWORD U16Parser::Serialize(BYTE* data,DWORD size)
{
	if (size<2)
		return -1;

	//Serialize
	set2(data,0,value);	

	return 2;
}

DWORD U16Parser::GetSize()
{
	return 2;
}

/****************************
 * Parser helper for U32
 ***************************/
U32Parser::U32Parser()
{
	//Reset
	Reset();
}

void U32Parser::Reset()
{
	len = 4;
	value = 0;
}

DWORD U32Parser::Parse(BYTE *data,DWORD size)
{
	BYTE i=0;

	while (len && i<size)
		value |= ((DWORD)data[i++]) << (--len)*8;

	return i;
}

bool U32Parser::IsParsed()
{
	return !(len);
}

DWORD U32Parser::GetValue()
{
	return value;
}

void U32Parser::SetValue(DWORD value)
{
	this->value = value;
}

DWORD U32Parser::Serialize(BYTE* data,DWORD size)
{
	if (size<4)
		return -1;

	//Serialize
	set4(data,0,value);	

	return 4;
}

DWORD U32Parser::GetSize()
{
	return 4;
}

/****************************
 * Parser helper for U29
 ***************************/
U29Parser::U29Parser()
{
	//Reset
	Reset();
}

void U29Parser::Reset()
{
	len = 0;
	value = 0;
}

DWORD U29Parser::Parse(BYTE *data,DWORD size)
{
	BYTE i=0;

	while (!last && i<size)
	{
		//Check if it has overflow
		if (len==4)
			//Error
			return -1;
		//Get byte
		BYTE b = data[0];
		//Check first bit to see if it is the last byte or not
		last = !(b & 0x80);
		//Append value
		value += ((DWORD)(b & 0x7F)) << 7*len;
	}

	return i;
}

bool U29Parser::IsParsed()
{
	return last;
}

DWORD U29Parser::GetValue()
{
	return value;
}
/****************************
 * Parser helper for UTF8
 ****************************/
UTF8Parser::UTF8Parser()
{
	//Reset
	utf8size = -1;
	bytes = 0;
	len = 0;
	w = 0;
}

UTF8Parser::UTF8Parser(const std::wstring& str)
{
	SetWString(str);
}

void UTF8Parser::Reset()
{
	//Remove al string content
	value.clear();
	//Reset number of utf8 bytes stored
	len = 0;
	utf8size = -1;
}

void UTF8Parser::SetSize(DWORD size)
{
	//Store it
	utf8size = size;
	//Allocate memory for speed
	value.reserve(utf8size);
}

DWORD UTF8Parser::SetString(const char* str)
{
	return Parse((BYTE*)str,strlen(str));
}

DWORD UTF8Parser::Parse(const BYTE* buffer,DWORD size)
{
	//Get the data still to be copied in the string
	DWORD copy = utf8size-len;

	//Check the size of the remaining data in the string with the
	if (copy>size)
		//Just copy what it is available
		copy = size;

	//Convert UTF-8 to wide char
	for (DWORD i=0;i<copy;i++)
	{
		//Get char byte
		BYTE c = buffer[i];
		
		if (c <= 0x7f)
		{
			//if it is not first byte
			if (bytes)
			{
				//Append decoding error
				value.push_back(L'.');
				//Start againg
				bytes = 0;
			}
			//Append char to string
			value.push_back((wchar_t)c);
		} else if (c <= 0xbf){
			//second/third/etc byte
			if (bytes)
			{
				//Append to wide char byte
				w = ((w << 6)|(c & 0x3f));
				//Decrease the number of bytes to read
				bytes--;
				//If no mor bytes left to append to wchar
				if (bytes == 0)
					//Append 
					value.push_back(w);
			} else {
				//Error
				value.push_back(L'.');
			}
		} else if (c <= 0xdf){
			//2byte sequence start
			bytes = 1;
			w = c & 0x1f;
		} else if (c <= 0xef){
			//3byte sequence start
			bytes = 2;
			w = c & 0x0f;
		} else if (c <= 0xf7){
			//3byte sequence start
			bytes = 3;
			w = c & 0x07;
		} else{
			value.push_back(L'.');
			bytes = 0;
		}
	}

	//Increase parsed data
	len += copy;

	//Return the number of consumed bytes	
	return copy;
}

bool UTF8Parser::IsParsed()
{
	return (len==utf8size);
}

std::wstring UTF8Parser::GetWString()
{
	return value;
}

const wchar_t* UTF8Parser::GetWChar()
{
	return value.c_str();
}

DWORD UTF8Parser::GetUTF8Size()
{
	return utf8size;
}

DWORD UTF8Parser::GetLength()
{
	return value.length();
}

void UTF8Parser::SetWString(const std::wstring& str)
{
	//Set value
	value = str;
	//Calculate UTF8Size again
	utf8size = 0;
	//Iterate chars
	for (DWORD i=0;i<str.length();i++)
	{
		//Get char
		wchar_t w = str[i];
		//Calculate size
		if (w <= 0x7f)
			utf8size++;
		else if (w <= 0x7ff)
			utf8size += 2;
		else if (w <= 0xffff)
			utf8size += 3;
		else 
			utf8size += 4;
	}
}

void UTF8Parser::SetWChar(const wchar_t *buffer,DWORD bufferLen)
{
	//Set value
	value = std::wstring(buffer,bufferLen);
	//Calculate UTF8Size again
	utf8size = 0;
	//Iterate chars
	for (DWORD i=0;i<bufferLen;i++)
	{
		//Get char
		wchar_t w = buffer[i];
		//Calculate size
		if (w <= 0x7f)
			utf8size++;
		else if (w <= 0x7ff)
			utf8size += 2;
		else if (w <= 0xffff)
			utf8size += 3;
		else
			utf8size += 4;
	}
}

DWORD UTF8Parser::Serialize(BYTE* buffer,DWORD size)
{
	DWORD len=0;

	//check length
	if (size<utf8size)
		//Return error
		return 0;

	//for each wide char
	for (size_t i = 0; i < value.size(); i++)
	{
		//Get wide char
		wchar_t w = value[i];

		if (w <= 0x7f)
		{
			buffer[len++] = (BYTE)w;
		} else if (w <= 0x7ff) {
			buffer[len++] = 0xc0 | ((w >> 6)& 0x1f);
			buffer[len++] = 0x80 | (w & 0x3f);
		} else if (w <= 0xffff) {
			buffer[len++] = 0xe0 | ((w >> 12)& 0x0f);
			buffer[len++] = 0x80 | ((w >> 6) & 0x3f);
			buffer[len++] = 0x80 | (w & 0x3f);
		} else if (w <= 0x10ffff) {
			buffer[len++] = 0xf0 | ((w >> 18)& 0x07);
			buffer[len++] = 0x80 | ((w >> 12) & 0x3f);
			buffer[len++] = 0x80 | ((w >> 6) & 0x3f);
			buffer[len++] = 0x80 | (w & 0x3f);
		} else
			buffer[len++] = '.';
	}

	return len;
}

/*************************
 * AMFParser
 *  
 ************************/
AMFParser::AMFParser()
{
	object = NULL;
}

AMFParser::~AMFParser()
{
	if (object)
		delete object;
}

DWORD AMFParser::Parse(BYTE *data,DWORD size)
{
	//Check if we are already parsing an object
	if (object)
		//Let it parse it
		return object->Parse(data,size);

	//Check marker
	switch((TypeMarker)data[0])
	{
		case NumberMarker:
			//Create the object
			object = (AMFData*) new AMFNumber();
			break; 
		case BooleanMarker:
			//Create the object
			object = (AMFData*) new AMFBoolean();
			break; 
		case StringMarker:
			//Create the object
			object = (AMFData*) new AMFString();
			break; 
		case ObjectMarker:
			//Create the object
			object = (AMFData*) new AMFObject();
			break; 
		case MovieClipMarker:
			//Create the object
			object = (AMFData*) new AMFMovieClip();
			break; 
		case NullMarker:
			//Create the object
			object = (AMFData*) new AMFNull();
			break; 
		case UndefinedMarker:
			//Create the object
			object = (AMFData*) new AMFUndefined();
			break; 
		case ReferenceMarker:
			//Create the object
			object = (AMFData*) new AMFReference();
			break; 
		case EcmaArrayMarker:
			//Create the object
			object = (AMFData*) new AMFEcmaArray();
			break; 
		case StrictArrayMarker:
			//Create the object
			object = (AMFData*) new AMFStrictArray();
			break; 
		case DateMarker:
			//Create the object
			object = (AMFData*) new AMFDate();
			break; 
		case LongStringMarker:
			//Create the object
			object = (AMFData*) new AMFLongString();
			break; 
		case UnsupportedMarker:
			//Create the object
			object = (AMFData*) new AMFUnsupported();
			break; 
		case RecordsetMarker:
			//Create the object
			object = (AMFData*) new AMFRecordSet();
			break; 
		case XmlDocumentMarker:
			//Create the object
			object = (AMFData*) new AMFXmlDocument();
			break; 
		case TypedObjectMarker:
			//Create the object
			object = (AMFData*) new AMFTypedObject();
			break; 
		case AvmplusObjectMarker:
			//Create the object
			throw std::runtime_error(std::string("AVMPlus Not supported"));
			break;
		default:
			//Create the object
			throw std::runtime_error(std::string("Type Not supported"));
	}

	//And let it parse the rest and return the data parsed by the object plus the 1 byte of the marker
	return object->Parse(data+1,size-1)+1;
}

AMFData* AMFParser::GetObject()
{
	//If we have a parsed object
	if (object && object->IsParsed())
	{
		//Get parsed object
		AMFData *parsed = object;
		//Remove object
		object = NULL;
		//Return parsed object
		return parsed;
	}

	//No object
	return NULL;
}

bool AMFParser::IsParsed()
{
	return object?object->IsParsed():false;
}

void AMFParser::Reset()
{
	//Check if it's an object already been parsed
	if (object)
		//Delete it
		delete (object);
	//And set it to null
	object = NULL;
}


/***********************************
 * AMFData object
 *
 **********************************/
AMFData::AMFData()
{
	//No parsed data yet
	len=0;
}

AMFData* AMFData::Clone()
{
	return new AMFNull();
}

AMFData::~AMFData()
{
}

void AMFData::Reset()
{
	len = 0;
}

DWORD AMFData::Serialize(BYTE* buffer,DWORD size)
{
	if (!size)
		return -1;

	//By default unsuported
	buffer[0]=AMFParser::UnsupportedMarker;

	return 1;
}

void AMFData::Dump()
{
	Debug("[%s]\n",GetTypeName());
}

DWORD AMFData::GetSize()
{
	return 1;
}

void AMFData::AssertType(ValueType type)
{ 
	if(GetType()!=type)
		throw std::runtime_error("AMFData cast exception");
}

bool AMFData::CheckType(ValueType type)
{
	return GetType()==type;
}

AMFData::operator double()
{ 
	AssertType(Number);
	return ((AMFNumber*)this)->GetNumber();         
}

AMFData::operator bool()
{
	AssertType(Boolean);
	return ((AMFBoolean*)this)->GetBoolean();
}

AMFData::operator std::wstring()
{
	if (GetType()==String)
		return ((AMFString*)this)->GetWString();
	if (GetType()==LongString)
		return ((AMFLongString*)this)->GetWString();
	if (GetType()==XmlDocument)
		return ((AMFXmlDocument*)this)->GetWString();

	throw std::runtime_error("AMFData cast exception to string"); 
}

const char* AMFData::GetTypeName()
{
	return TypeToString(GetType());
}

const char* AMFData::TypeToString(ValueType type)
{
	switch (type)
	{
		case Number:
			return "Number";
		case Boolean:
			return "Boolean";
		case String:
			return "String";
		case Object:
			return "Object";
		case MovieClip:
			return "MovieClip";
		case Null:
			return "Null";
		case Undefined:
			return "Undefined";
		case Unsupported:
			return "Unsupported";
		case RecordSet:
			return "RecordSet";
		case Reference:
			return "Reference";
		case EcmaArray:
			return "EcmaArray";
		case StrictArray:
			return "StrictArray";
		case Date:
			return "Date";
		case LongString:
			return "LongString";
		case XmlDocument:
			return "XmlDocument";
		case TypedObject:
			return "TypedObject";
		default:
			return "Unknown";
	}
}

/********************
 * AMFNumber
 *
 ********************/
AMFNumber::AMFNumber()
{
	len = 8;
	value = 0;
}

AMFData* AMFNumber::Clone()
{
	AMFNumber *obj = new AMFNumber();
	obj->SetNumber(GetNumber());
	return obj;
}

DWORD AMFNumber::Parse(BYTE *data,DWORD size)
{
	BYTE i=0;

	while (len && i<size)
		value |= (QWORD)data[i++] << (8*(--len));

	return i;
}

bool AMFNumber::IsParsed()
{
	return (!len);
}

double AMFNumber::GetNumber()
{
	if(value+value > 0xFFEULL<<52)
        	return 0.0;
    	return ldexp(((value&((1LL<<52)-1)) + (1LL<<52)) * (value>>63|1), (value>>52&0x7FF)-1075);
}

void AMFNumber::SetNumber(double d)
{
	int e;
	if     ( !d) 
	{ 
		value = 0;
	} else if(d-d) {
		value =  0x7FF0000000000000LL + ((QWORD)(d<0)<<63) + (d!=d);
	} else {
		d = frexp(d, &e);
		value = (QWORD)(d<0)<<63 | (e+1022LL)<<52 | (QWORD)((fabs(d)-0.5)*(1LL<<53));
	}
}

DWORD AMFNumber::GetSize()
{
	return 9;
}

DWORD AMFNumber::Serialize(BYTE* data,DWORD size)
{
	if (size<9)
		return -1;

	//Set marker
	data[0] = AMFParser::NumberMarker;
	//Serialize number
	set8(data,1,value);

	return 9;
}

void AMFNumber::Dump()
{
	Debug("[Number %f]\n",GetNumber());
}
/*************************
 * AMFBoolean
 *
 *************************/
AMFBoolean::AMFBoolean()
{
	SetBoolean(false);
}

AMFBoolean::AMFBoolean(bool val)
{
	SetBoolean(val);
}

DWORD AMFBoolean::Parse(BYTE *data,DWORD size)
{
	//Check we have enought data
	if (!size)	
		return 0;

	//Check if we are reading too much	
	if (len)
		throw std::runtime_error("AMFBoolean parsing too much data");

	//Get value	
	value = (data[0]>0);

	//parsed	
	len = 1;

	return 1;
}

bool AMFBoolean::IsParsed()
{
	//Have we read 1 byte?	
	return (len);
}

bool AMFBoolean::GetBoolean()
{
	return value;
}

void AMFBoolean::SetBoolean(bool value)
{
	this->value = value;
}

AMFData* AMFBoolean::Clone()
{
	AMFBoolean *obj = new AMFBoolean();
	obj->SetBoolean(GetBoolean());
	return obj;
}

void AMFBoolean::Dump()
{
	Debug("[Boolean %s]\n",value?"true":"false");
}

/********************
 * AMFString
 *
 ********************/
AMFString::AMFString()
{

}

AMFString::AMFString(const wchar_t* val)
{
	//Set string value
	SetWString(val);
}
void AMFString::Reset()
{
	u16parser.Reset();
	utf8parser.Reset();
}

AMFData* AMFString::Clone()
{
	AMFString *obj = new AMFString();
	obj->SetWString(GetWString());
	return obj;
}

DWORD AMFString::Parse(BYTE *data,DWORD size)
{
	BYTE *buffer = data;
	DWORD bufferSize = size;

	//Check if we still have not parsed the utf8 size
	if (!u16parser.IsParsed())
	{
		//Parse it
		DWORD copy = u16parser.Parse(buffer,bufferSize);
		//Remove from buffer
		buffer += copy;
		bufferSize -= copy;
		//Check if got the len
		if (u16parser.IsParsed())
			//Set the size to the utf8 parser
		utf8parser.SetSize(u16parser.GetValue());
	}

	//If still got data
	if (bufferSize)
	{
		//Parse utf8 string
		DWORD copy = utf8parser.Parse(buffer,bufferSize);
		//Remove from buffer
		buffer += copy;
		bufferSize -= copy;
	}

	//Return the amount of data consumed
	return (buffer-data);
}

bool AMFString::IsParsed()
{
	return utf8parser.IsParsed();
}

std::wstring AMFString::GetWString()
{
	return utf8parser.GetWString();
}
const wchar_t* AMFString::GetWChar()
{
	return utf8parser.GetWChar();
}

DWORD AMFString::GetUTF8Size()
{
	return u16parser.GetValue();
}

void AMFString::SetWString(const std::wstring& str)
{
	//Set string value
	utf8parser.SetWString(str);
	//Set it
	u16parser.SetValue(utf8parser.GetUTF8Size());
}

DWORD AMFString::GetSize()
{
	return utf8parser.GetUTF8Size()+u16parser.GetSize()+1;
	
}

DWORD AMFString::Serialize(BYTE* data,DWORD size)
{
	DWORD len = GetSize();

	if (size<len)
		return -1;

	//Start coding
	len = 0;
	//Set marker
	data[len++] = AMFParser::StringMarker;
	//Serialize string size
	len += u16parser.Serialize(data+len,size-len);
	//Serialize the
	len += utf8parser.Serialize(data+len,size-len);

	return len;

}

void AMFString::Dump()
{
	Debug("[String \"%ls\"]\n",utf8parser.GetWString().c_str());
}

/********************
 * AMFLongString
 *
 ********************/
void AMFLongString::Reset()
{
	u32parser.Reset();
	utf8parser.Reset();
}

DWORD AMFLongString::Parse(BYTE *data,DWORD size)
{
	BYTE *buffer = data;
	DWORD bufferSize = size;

	//Check if we still have not parsed the utf8 size
	if (!u32parser.IsParsed())
	{
		//Parse it
		DWORD copy = u32parser.Parse(buffer,bufferSize);
		//Remove from buffer
		buffer += copy;
		bufferSize -= copy;
		//Check if got the len
		if (u32parser.IsParsed())
			//Set the size to the utf8 parser
			utf8parser.SetSize(u32parser.GetValue());
	}

	//If still got data
	if (bufferSize)
	{
		//Parse utf8 string
		DWORD copy = utf8parser.Parse(buffer,bufferSize);
		//Remove from buffer
		buffer += copy;
		bufferSize -= copy;
	}

	//Return the amount of data consumed
	return (buffer-data);
}

bool AMFLongString::IsParsed()
{
	return utf8parser.IsParsed();
}

std::wstring AMFLongString::GetWString()
{
	return utf8parser.GetWString();
}

DWORD AMFLongString::GetUTF8Size()
{
	return u32parser.GetValue();
}

/*************************
 * AMFObject
 *
 *************************/	
AMFObject::AMFObject()
{
	marker = false;
}

AMFData* AMFObject::Clone()
{
	AMFObject *obj = new AMFObject();
	for (AMFObjectMap::iterator it=properties.begin(); it!=properties.end(); it++)
		obj->AddProperty(it->first.c_str(),it->second->Clone());
	return obj;
}

AMFObject::~AMFObject()
{
	for (AMFObjectMap::iterator it=properties.begin(); it!=properties.end(); it++)
		if (it->second)
			delete(it->second);
}

DWORD AMFObject::Parse(BYTE *data,DWORD size)
{
	BYTE *buffer = data;
	DWORD bufferSize = size;

	//While input data
	while (bufferSize)
	{
		//Check if we are parsing the name of the property
		if (!key.IsParsed())
		{
			//Parse it
			DWORD copy = key.Parse(buffer,bufferSize);
			//Remove from buffer
			buffer += copy;
			bufferSize -= copy;
		} else {
			//Check if now we have got it and it has a zero size
			if (!key.GetUTF8Size()) 
			{
				//Check if next if the end marker
				if (((AMFParser::TypeMarker)buffer[0])!=AMFParser::ObjectEndMarker)
					//Error should not happend finish parsing 
					throw std::runtime_error("End marker not found");
				//Set marker bit
				marker = true;
				//Remove from buffer
				buffer++;
				bufferSize--;
				//And exit
				break;
			} else {
				//Parse the next object
				DWORD copy = parser.Parse(buffer,bufferSize);
				//Remove from buffer 
				buffer += copy;
				bufferSize -= copy;

				//If we have a value
				if (parser.IsParsed())
				{
					//Append the property to the map
					properties[key.GetWString()] = parser.GetObject();
					//Append to the order
					propertiesOrder.push_back(key.GetWString());
					//Start parsing name again
					key.Reset();
				}
			}
		}
	}

	//Exit and return consumed data
	return (buffer-data);
}

bool AMFObject::IsParsed()
{
	return marker;
}

AMFObjectMap& AMFObject::GetProperties()
{
	return properties;
}

AMFData& AMFObject::GetProperty(const wchar_t* key)
{
	return *(properties[std::wstring(key)]);
}

bool AMFObject::HasProperty(const wchar_t* key)
{
	return properties.find(std::wstring(key)) != properties.end();
}

void AMFObject::AddProperty(const wchar_t* key,const wchar_t* value)
{
	//Create string object
	AMFString* str = new AMFString();
	//Add value
	str->SetWString(std::wstring(value));
	//Set property
	properties[key] = (AMFData*)str;
	//Add to the order
	propertiesOrder.push_back(key);
}

void AMFObject::AddProperty(const wchar_t* key,const wchar_t* value,DWORD length)
{
	//Create string object
	AMFString* str = new AMFString();
	//Add value
	str->SetWString(std::wstring(value,length));
	//Set property
	properties[key] = (AMFData*)str;
	//Add to the order
	propertiesOrder.push_back(key);
}

void AMFObject::AddProperty(const wchar_t* key,const double value)
{
	//Create string object
	AMFNumber* num = new AMFNumber();
	//Add value
	num->SetNumber(value);
	//Set property
	properties[key] = (AMFData*)num;
	//Add to the order
	propertiesOrder.push_back(key);
}

void AMFObject::AddProperty(const wchar_t* key,AMFData *obj)
{
	//Set property
	properties[key] = obj;
	//Add to the order
	propertiesOrder.push_back(key);
}

void AMFObject::AddProperty(const wchar_t* key,AMFData &obj)
{
	//Set property
	properties[key] = obj.Clone();
	//Add to the order
	propertiesOrder.push_back(key);
}

DWORD AMFObject::GetSize()
{
	//Aux string to calc sizes
	UTF8Parser key;
	//Start and end mark
	DWORD len = 4;
	//Loop properties
	for (AMFObjectMap::iterator it=properties.begin();it!=properties.end();it++)
	{
		//Set key value
		key.SetWString(it->first);
		//Calc sizes
		len+=2+key.GetUTF8Size()+it->second->GetSize();
	}

	return len;
}

DWORD AMFObject::Serialize(BYTE* data,DWORD size)
{
	//Aux string to calc sizes
	U16Parser u16key;
	UTF8Parser key;
	//Start 
	DWORD len = 0;
	//Set start mark
	data[len++] = AMFParser::ObjectMarker;
	//Loop properties in insert order
	for (int i=0;i<propertiesOrder.size();i++)
	{
		//Search for the property
		AMFObjectMap::iterator it = properties.find(propertiesOrder[i]);
		//Set key value
		key.SetWString(it->first);
		//Set utf8 size
		u16key.SetValue(key.GetUTF8Size());
		//Serialize utf8 size
		len += u16key.Serialize(data+len,size-len);
		//Serialize key
		len += key.Serialize(data+len,size-len);
		//Serialize value
		len += it->second->Serialize(data+len,size-len);
	}
	//Set end mark
	data[len++] = 0;
	data[len++] = 0;
	data[len++] = AMFParser::ObjectEndMarker;

	//return encoded length
	return len;
}

void AMFObject::Dump()
{
	Debug("[Object]\n");
	for (AMFObjectMap::iterator it=properties.begin();it!=properties.end();it++)
	{
		Debug("  %*ls:\t",20,it->first.c_str());
		it->second->Dump();
	}
	Debug("[/Object]\n");
}

/*************************
 * AMFTypedObject
 *
 *************************/	
DWORD AMFTypedObject::Parse(BYTE *data,DWORD size)
{
	BYTE *buffer = data;
	DWORD bufferSize = size;

	//Iterate data
	while(bufferSize)
	{
		//Check what we have to parse
		if (!u16parser.IsParsed())
		{
			//Parse name length
			DWORD copy = u16parser.Parse(buffer,bufferSize);
			//Remove from buffer
			buffer += copy;
			bufferSize -= copy;
			//Check if got the len
			if (u16parser.IsParsed())
				//Set the size to the utf8 parser
				utf8parser.SetSize(u16parser.GetValue());
		} else if (!utf8parser.IsParsed()) {
			//Parse utf8 name string
			DWORD copy = utf8parser.Parse(buffer,bufferSize);
			//Remove from buffer
			buffer += copy;
			bufferSize -= copy;
		} else {
			//Parse attributes
			DWORD copy = ((AMFObject*)this)->Parse(buffer,bufferSize);
			//Remove from buffer
			buffer += copy;
			bufferSize -= copy;
		}
	}

	//Return the amount of data consumed
	return (buffer-data);
}

bool AMFTypedObject::IsParsed()
{
	return ((AMFObject*)this)->IsParsed();
}

std::wstring AMFTypedObject::GetClassName()
{
	return utf8parser.GetWString();
}

/********************
 * AMFReference
 *
 ********************/
DWORD AMFReference::Parse(BYTE *data,DWORD size)
{
	BYTE i=0;

	if (len>1)
		return -1;
	
	while (len<2)
		((BYTE*)&value)[len++] = data[size+i++];

	return i;
}

bool AMFReference::IsParsed()
{
	return (len==2);
}

WORD AMFReference::GetReference()
{
	return value;
}

/*************************
 * EcmaArray
 *
 *************************/	
AMFEcmaArray::AMFEcmaArray()
{
	marker = 0;
}

AMFEcmaArray::~AMFEcmaArray()
{
	for (AMFObjectMap::iterator it=elements.begin(); it!=elements.end(); it++)
		if (it->second)
			delete(it->second);
}

DWORD AMFEcmaArray::Parse(BYTE *data,DWORD size)
{
	BYTE *buffer = data;
	DWORD bufferSize = size;
	DWORD copy = 0;

	while (bufferSize && !IsParsed())
	{
		//Check if we are processing the size of the array
		if (!num.IsParsed())
		{
			//Append the byte to the string size
			copy = num.Parse(buffer,bufferSize);
			//Remove from buffer
			buffer += copy;
			bufferSize -= copy;
		//Check if we are parsing the name of the property
		} else if (!key.IsParsed()) {
			//Parse it
			DWORD copy = key.Parse(buffer,bufferSize);
			//Remove from buffer
			buffer += copy;
			bufferSize -= copy;
		} else {
			//Check if now we have got it and it has a zero size
			if (!key.GetUTF8Size()) 
			{
				//Check if next if the end marker
				if (((AMFParser::TypeMarker)buffer[0])!=AMFParser::ObjectEndMarker)
					//Error should not happend finish parsing 
					throw std::runtime_error("End marker not found");
				//Set marker bit
				marker = true;
				//Remove from buffer
				buffer++;
				bufferSize--;
				//And exit
				break;
			} else {
				//Parse the next object
				DWORD copy = parser.Parse(buffer,bufferSize);
				//Remove from buffer
				buffer += copy;
				bufferSize -= copy;

				//If we have a value
				if (parser.IsParsed())
				{
					//Append the property to the map
					elements[key.GetWString()] = parser.GetObject();
					//Start parsing name again
					key.Reset();
				}
			}
		}
	}

	//Exit and return consumed data
	return (buffer-data);
}

bool AMFEcmaArray::IsParsed()
{
	return num.IsParsed() && marker;
}

AMFObjectMap& AMFEcmaArray::GetElements()
{
	return elements;
}

AMFData& AMFEcmaArray::GetProperty(const wchar_t* key)
{
	return *(elements[std::wstring(key)]);
}

bool AMFEcmaArray::HasProperty(const wchar_t* key)
{
	return elements.find(std::wstring(key)) != elements.end();
}

void AMFEcmaArray::AddProperty(const wchar_t* key,const wchar_t* value)
{
	//Create string object
	AMFString* str = new AMFString();
	//Add value
	str->SetWString(std::wstring(value));
	//Set elements
	elements[key] = (AMFData*)str;
}

void AMFEcmaArray::AddProperty(const wchar_t* key,const wchar_t* value,DWORD length)
{
	//Create string object
	AMFString* str = new AMFString();
	//Add value
	str->SetWString(std::wstring(value,length));
	//Set elements
	elements[key] = (AMFData*)str;
}

void AMFEcmaArray::AddProperty(const wchar_t* key,const double value)
{
	//Create string object
	AMFNumber* num = new AMFNumber();
	//Add value
	num->SetNumber(value);
	//Set property
	elements[key] = (AMFData*)num;
}

void AMFEcmaArray::AddProperty(const wchar_t* key,AMFData *obj)
{
	//Set property
	elements[key] = obj;
}

void AMFEcmaArray::AddProperty(const wchar_t* key,AMFData &obj)
{
	//Set property
	elements[key] = obj.Clone();
}

void AMFEcmaArray::Dump()
{
	Debug("[EcmaArray]\n");
	for (AMFObjectMap::iterator it=elements.begin();it!=elements.end();it++)
	{
		Debug("  %*ls:\t",20,it->first.c_str());
		it->second->Dump();
	}
	Debug("[/EcmaArray]\n");
}

DWORD AMFEcmaArray::GetSize()
{
	//Aux string to calc sizes
	UTF8Parser key;
	//Start and end mark and number
	DWORD len = 8;
	//Loop properties
	for (AMFObjectMap::iterator it=elements.begin();it!=elements.end();it++)
	{
		//Set key value
		key.SetWString(it->first);
		//Calc sizes
		len+=2+key.GetUTF8Size()+it->second->GetSize();
	}
	return len;	
}

DWORD AMFEcmaArray::Serialize(BYTE* data,DWORD size)
{
	//Check size
	if (size<GetSize())
		return 0;

	//Aux string to calc sizes
	U16Parser u16key;
	UTF8Parser key;
	//Start 
	DWORD len = 0;
	//Set mark
	data[len++] = AMFParser::EcmaArrayMarker;
	//Set number of properties
	num.SetValue(elements.size());
	//Add number of properties
	len += num.Serialize(data+len,size-len);
	//Loop elements
	for (AMFObjectMap::iterator it=elements.begin();it!=elements.end();it++)
	{
		//Set key value
		key.SetWString(it->first);
		//Set utf8 size
		u16key.SetValue(key.GetUTF8Size());
		//Serialize utf8 size
		len += u16key.Serialize(data+len,size-len);
		//Serialize key
		len += key.Serialize(data+len,size-len);
		//Serialize value
		len += it->second->Serialize(data+len,size-len);
	}
	//Set end mark
	data[len++] = 0;
	data[len++] = 0;
	data[len++] = AMFParser::ObjectEndMarker;

	//return encoded length
	return len;
}

AMFData* AMFEcmaArray::Clone()
{
	AMFEcmaArray *obj = new AMFEcmaArray();
	for (AMFObjectMap::iterator it=elements.begin(); it!=elements.end(); it++)
		obj->AddProperty(it->first.c_str(),it->second->Clone());
	return obj;
}

/*************************
 * StrictArray
 *
 *************************/	
AMFStrictArray::AMFStrictArray()
{
	cur = -1;
	elements = NULL;
}

AMFStrictArray::~AMFStrictArray()
{
	if (elements)
	{
		//For each property
		for (DWORD i=0;i<num.GetSize();i++)
			if (elements[i])
				//delete object
				delete(elements[i]);
		//Free array memory
		free(elements);
	}
}

DWORD AMFStrictArray::Parse(BYTE *data,DWORD size)
{
	BYTE *buffer = data;
	DWORD bufferSize = size;
	DWORD copy = 0;

	while (bufferSize)
	{
		//Check if we are processing the size of the array
		if (!num.IsParsed())
		{
			//Append the byte to the string size
			copy = num.Parse(data,size);
			//Increase copied data
			len += copy;
			//Remove from buffer
			buffer += copy;
			bufferSize -= copy;
			//If we have it
			if (num.IsParsed())
			{
				//Allocate memory
				elements = (AMFData**)malloc(num.GetValue()*sizeof(AMFObject*));
				//Set first
				cur = 0;
			}
		} else {
			//Parse the property object
			copy = parser.Parse(buffer,bufferSize);
			//Increase copied data
			len += copy;
			//Remove from buffer
			buffer += copy;
			bufferSize -= copy;

			//If we have the property object parsed
			if (parser.IsParsed())
			{
				//Append the objecct to the array
				elements[cur++] = parser.GetObject();
				//Check number of elements
				if (IsParsed())
					//Finished
					break;

			}
		}
	}

	//Exit and return consumed data
	return (buffer-data);
}

bool AMFStrictArray::IsParsed()
{
	return (cur==num.GetValue());
}

AMFData** AMFStrictArray::GetElements()
{
	return elements;
}

void AMFStrictArray::Dump()
{
	Debug("[StrictArray]\n");
	if (elements)
	{
		//For each property
		for (DWORD i=0;i<num.GetSize();i++)
			if (elements[i])
				elements[i]->Dump();
			else	
				Debug("[NULL/]\n");
	}
	Debug("[/StrictArray]\n");
}


/********************
 * AMFNumber
 *
 ********************/
DWORD AMFDate::Parse(BYTE *data,DWORD size)
{
	BYTE i=0;

	while (len<8 && i<size)
		((BYTE*)&value)[len++] = data[i++];
	while (len<10 && i<size)
		len++;

	return i;
}

bool AMFDate::IsParsed()
{
	return (len==10);
}

time_t& AMFDate::GetDate()
{
	return value;
}


/**************************
 * AMFNull
 * ************************/
DWORD AMFNull::Serialize(BYTE* data,DWORD size)
{
	if (!size)
		return -1;

	data[0] = AMFParser::NullMarker;

	return 1;
}
