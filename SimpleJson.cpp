
#include <Arduino.h>
#include "WString.h"
#include <map>
#include "SimpleJson.h"

// https://www.json.org/json-en.html

void SimpleJson::parse(String data){
    int index = 0;

    const char* ptr = data.c_str();
    rootValue = getValue( &index, ptr );
}

JsonValue SimpleJson::getValue( int* index, const char* ptr ){
    JsonValue value;

    skipWhitespace(index, ptr);

    if ( isString(index, ptr) )
    {
        value.setValue( getString( index, ptr ) );
    }
    else if ( isNumber(index, ptr) )
    {
        value.setValue( getNumber( index, ptr ) );
    }
    else if (isObject(index, ptr) )
    {
        value.setValue( getObject( index, ptr ) );
    }
    else if ( isArray(index, ptr) )
    {
        value.setValue( getArray( index, ptr ) );
    }
    else if ( isTrue(index, ptr) )
    {
        (*index) += 4; 
        value.setValue( true );
    }
    else if ( isFalse(index, ptr) )
    {
        (*index) += 5;
        value.setValue( false );
    }
    else if ( isNull(index, ptr) )
    {
        (*index) += 4;
        value.setValue( false, true );
    }
    return value;
}
 
std::map<String, JsonValue> SimpleJson::getObject( int* index, const char* ptr ){
    std::map<String, JsonValue> oValue;
    skipWhitespace(index, ptr);
    if ( ptr[*index] == '{' )
    {
        (*index)++;   // skip the opening curly bracket 
        while ( ptr[*index] != 0 && ptr[*index] != '}' )
        {
            String name = getString( index, ptr );

            skipWhitespace(index, ptr);
            if( ptr[*index] == ':' )
            {
                (*index)++; 
                JsonValue value = getValue( index, ptr );
                oValue.insert(std::make_pair(name, value));
            }
            skipWhitespace(index, ptr);
            if( ptr[*index] == ',' )
            {
                (*index)++; 
            }
        }
    }

    if ( ptr[*index] == '}' )
    {
        (*index)++; // skip the closing curly bracket 
    }       
    return oValue;
}

std::vector<JsonValue> SimpleJson::getArray( int* index, const char* ptr ){
    std::vector<JsonValue> oValue;
    skipWhitespace(index, ptr);
    if ( ptr[*index] == '[' )
    {
        (*index)++;   // skip the opening square bracket 
        while ( ptr[*index] != 0 && ptr[*index] != ']' )
        {
            oValue.push_back(getValue( index, ptr ));
            skipWhitespace(index, ptr);
            if( ptr[*index] == ',' )
            {
                (*index)++; // skip ','
            }
        }
    }

    if ( ptr[*index] == ']' )
    {
        (*index)++; // skip the closing square bracket 
    }    
    return oValue;
}

String SimpleJson::getString( int* index, const char* ptr )
{
    String retVal = "";
    skipWhitespace(index, ptr);

    if ( ptr[*index] == '"' )
    {
        (*index)++;   // skip the openning quote
        while ( ptr[*index] != 0 && ptr[*index] != '"' )
        {
            if ( ptr[*index] == '\\' )
            {
                (*index)++;    
                switch( ptr[*index] )
                {
                    case '"':
                        retVal += '"';
                        break;
                    case '\\':
                        retVal += '\\';
                        break;
                    case '/':
                        retVal += '/';
                        break;            
                    case 'b':
                        retVal += '\b';
                        break;   
                    case 'f':
                        retVal += '\f';
                        break;   
                    case 'n':
                        retVal += '\n';
                        break;   
                    case 'r':
                        retVal += '\r';
                        break;   
                    case 't':
                        retVal += '\t';
                        break;   
                    case 'u':
                        char digits[5];
                        digits[0] = ptr[(*index) + 1];
                        digits[1] = ptr[(*index) + 2];
                        digits[2] = ptr[(*index) + 3];
                        digits[3] = ptr[(*index) + 4];
                        digits[4] = 0;
                        (*index) += 4;
                        retVal += atoi(digits);
                        break;                                                                                                                                                               
                }
            }    
            else
            {
                retVal += ptr[(*index)];
            }
            (*index)++;
        }
    }

    if ( ptr[*index] == '"' )
    {
        (*index)++; // skip the closing quote
    }
    return retVal;
}

int SimpleJson::getNumber( int* index, const char* ptr )
{
    int retVal = 0;
    String num = "";
    bool negative = false;
    skipWhitespace(index, ptr);

    if ( ptr[*index] == '-' )
    {
        negative = true;
        (*index)++;
    }

    while ( ptr[*index] >= '0' && ptr[*index] <= '9' )
    {
        num += ptr[*index];
        (*index)++;
    }

    retVal = atoi(num.c_str());
    if ( negative ){
        retVal = retVal * -1;
    }
    return retVal;
}

bool SimpleJson::isArray( int* index, const char* ptr ){
    bool retVal = false;
    skipWhitespace(index, ptr);
    if ( ptr[*index] == '['){
        retVal = true;
    }
    return retVal;
}

bool SimpleJson::isObject( int* index, const char* ptr ){
    bool retVal = false;
    skipWhitespace(index, ptr);
    if ( ptr[*index] == '{'){
        retVal = true;
    }
    return retVal;
}

bool SimpleJson::isString( int* index, const char* ptr ){
    bool retVal = false;
    skipWhitespace(index, ptr);
    if ( ptr[*index] == '"'){
        retVal = true;
    }
    return retVal;
}

bool SimpleJson::isTrue( int* index, const char* ptr ){
    bool retVal = false;
    skipWhitespace(index, ptr);
    if ( ptr[*index] != 0 && ptr[*index] == 't' ){
        if ( ptr[(*index) + 1] != 0 &&  ptr[(*index) + 1] == 'r' ){
            if ( ptr[(*index) + 2] != 0 &&  ptr[(*index) + 2] == 'u' ){
                if ( ptr[(*index) + 3] != 0 &&  ptr[(*index) + 3] == 'e' ){
                    retVal = true;
                }
            }
        }        
    }
    return retVal;
}

bool SimpleJson::isFalse( int* index, const char* ptr ){
    bool retVal = false;
    skipWhitespace(index, ptr);
    if ( ptr[*index] != 0 && ptr[*index] == 'f' ){
        if ( ptr[(*index) + 1] != 0 &&  ptr[(*index) + 1] == 'a' ){
            if ( ptr[(*index) + 2] != 0 &&  ptr[(*index) + 2] == 'l' ){
                if ( ptr[(*index) + 3] != 0 &&  ptr[(*index) + 3] == 's' ){
                    if ( ptr[(*index) + 4] != 0 &&  ptr[(*index) + 4] == 'e' ){
                        retVal = true;
                    }
                }
            }
        }        
    }
    return retVal;
}

bool SimpleJson::isNull( int* index, const char* ptr ){
    bool retVal = false;
    skipWhitespace(index, ptr);
    if ( ptr[*index] != 0 && ptr[*index] == 'n' ){
        if ( ptr[(*index) + 1] != 0 &&  ptr[(*index) + 1] == 'u' ){
            if ( ptr[(*index) + 2] != 0 &&  ptr[(*index) + 2] == 'l' ){
                if ( ptr[(*index) + 3] != 0 &&  ptr[(*index) + 3] == 'l' ){
                    retVal = true;
                }
            }
        }        
    }
    return retVal;
}

void SimpleJson::skipWhitespace( int* index, const char* ptr )
{
    while ( ptr[*index] != 0 && (ptr[*index] == ' ' || ptr[*index] == '\n' || ptr[*index] == '\r' || ptr[*index] == '\t' ) )
    {
        (*index)++;
    }
}

// cheating and just checking of the first character is a digit or minus sign
bool SimpleJson::isNumber( int* index, const char* ptr ){
    bool retVal = false;
    skipWhitespace(index, ptr);

    if ( ptr[*index] == '-' || ( ptr[*index] >= '0' && ptr[*index] <= '9' )){
        retVal = true;
    }
    return retVal;
}

JsonValue SimpleJson::operator[]( String name)
{
    return rootValue[name];
}

JsonValue SimpleJson::operator[]( int index )
{
    return rootValue[index];
}