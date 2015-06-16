#include "coro.h"

bool memcmp(char* s1, char* s2, int len) {
	for (int i = 0; i < len; i++) {
		if (s1[i] != s2[i])
			return true;
		if (s1[i] == 0 && s2[i] == 0)
			return false;
	}
	return false;
}

int hexdigit(char ch) {
	if ('a' <= ch && ch <= 'f')
		return 0xA + (ch - 'a');
	if ('A' <= ch && ch <= 'F')
		return 0xA + (ch - 'A');
	if ('0' <= ch && ch <= '9')
		return ch - '0';

	die();
	return 0;
}