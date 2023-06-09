#include "definitions.h"

struct String
{
	char* text;
	u32 length;
};

internal String 
string(char* text)
{
	String result = {0};
	result.text = text;
	while(*text)
	{
		result.length++;
		text++;
	}
	return result;
}

internal bool
compare_strings(String s1, String s2)
{
	for(u32 i=0 ; s1.text[i] && s2.text[i]; i++)
	{
		if(s1.text[i] != s2.text[i])
			return false;
	}
	return true;
}

internal bool
compare_strings(String s1, char* s2)
{
	for(u32 i=0 ; s1.text[i] && s2[i]; i++)
	{
		if(s1.text[i] != s2[i])
			return false;
	}
	return true;
}

internal char
char_to_number(char c)
{
    return c - 48;
}

internal s32
string_to_int(String s)
{
	ASSERT(s.length);
	s32 sign = 1;
	s32 bigger_digit_pos = 0;
	if(*s.text == '-')
	{
		bigger_digit_pos = 1;
		sign = -1;
	}
	s32 result = 0;
	s32 power_of_10 = 1;
	for(s32 i = s.length-1; bigger_digit_pos <= i ; i--)
	{
		char digit = char_to_number(s.text[i]);
		result += digit*power_of_10;
		power_of_10 *= 10;
	}
	return result * sign;
}

internal bool
string_to_bool(String s)
{
	if(!s.text || compare_strings(s, "false") || compare_strings(s, "False"))
		return false;
	else
	{
		if( compare_strings(s, "true") ||
			compare_strings(s, "True") )
			return true;
		else
			return false;
	}
}
