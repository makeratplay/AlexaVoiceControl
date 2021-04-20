#pragma once

#include "WString.h"
#include <map>
#include <vector>

class JsonValue
{
    public:
        JsonValue(){ type = UNKNOWN; };
        JsonValue(String val){ strValue = val; type = STRING; }
        JsonValue(int val){ iValue = val; type = NUMBER_INT; }
        JsonValue(float val){ fValue = val; type = NUMBER_FLOAT; }
        JsonValue(bool bValue, bool isNull = false){  if ( isNull ) type = NULL_TYPE; else  type = bValue ? TRUE_TYPE : FALSE_TYPE; }

        void setValue(String val){ strValue = val; type = STRING; }
        void setValue(int val){ iValue = val; type = NUMBER_INT; }
        void setValue(float val){ fValue = val; type = NUMBER_FLOAT; }
        void setValue(bool bValue, bool isNull = false){  if ( isNull ) type = NULL_TYPE; else  type = bValue ? TRUE_TYPE : FALSE_TYPE; }
        void setValue(std::map<String, JsonValue> val){ oValue = val; type = OBJECT; }
        void setValue(std::vector<JsonValue> val){ arrayValue = val; type = ARRAY; }

        bool hasPropery( String name){ return type == OBJECT ? oValue.count(name) : false; }
        int getInt() { return iValue; }
        float getFloat() { return fValue; }
        bool getBool() { return type == TRUE_TYPE ? true : false; }
        bool isNull() { return type == NULL_TYPE; }
        String getString() { return strValue; }
        JsonValue operator[](String name){return oValue[name];}
        JsonValue operator[](int index){return arrayValue[index];}

        typedef enum {
            UNKNOWN,
            STRING,
            NUMBER_INT,
            NUMBER_FLOAT,
            OBJECT,
            ARRAY,
            TRUE_TYPE,
            FALSE_TYPE,
            NULL_TYPE,
        } ValueType;        

    private:
        ValueType type;
        String strValue;
        int iValue;
        float fValue;
        // object
        std::map<String, JsonValue> oValue;
        std::vector<JsonValue> arrayValue;
};

class SimpleJson
{
    public:
        void parse(String data);
        bool hasPropery( String name){ return rootValue.hasPropery(name); }
        JsonValue operator[]( String name);
        JsonValue operator[]( int index);

    private:
        JsonValue rootValue;

        std::vector<JsonValue> getArray( int* index, const char* ptr );
        std::map<String, JsonValue> getObject( int* index, const char* ptr );
        JsonValue getValue( int* index, const char* ptr );
        String getString( int* index, const char* ptr );
        int getNumber( int* index, const char* ptr );
        void skipWhitespace( int* index, const char* ptr );

        bool isArray( int* index, const char* ptr );
        bool isObject( int* index, const char* ptr );
        bool isString( int* index, const char* ptr );

        bool isTrue( int* index, const char* ptr );
        bool isFalse( int* index, const char* ptr );
        bool isNull( int* index, const char* ptr );
        bool isNumber( int* index, const char* ptr );
};