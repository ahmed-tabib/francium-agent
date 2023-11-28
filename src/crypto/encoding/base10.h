#pragma once

#include <string>

namespace crypto {


template<typename T>
class base10 {

public:

	static T decode(std::string str)
	{
		T res = 0;
		for (int i = 0; i < (int)str.size(); i++)
		{
			res *= 10;
			res += str[i] - (T)'0';
		}
		return res;
	}
	static std::string encode(T num)
	{
		std::string res;
		std::string tmp;

		while (num != 0)
		{
			tmp.push_back((char)('0' + (num % 10)));
			num /= 10;
		}

		res.reserve(tmp.size());
		for (int i = (int)tmp.size(); i >= 0; i--)
		{
			res.push_back(tmp[i]);
		}

		return res;
	}

};


}