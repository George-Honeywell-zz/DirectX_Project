#pragma once

#include <string>

class StringConverter
{
public:
	//Class act's as a Singleton.
	//The general purpose of a singleton design pattern is to limit the number of instances of a class to only one.
	static std::wstring StringToWide(std::string str);
};