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