ALL: test count_prefix

test : test.cpp trie.hpp
	c++ test.cpp -g -o test

count_prefix : count_prefix.cpp trie.hpp
	c++ count_prefix.cpp -g -o count_prefix -lcrypto -stdlib=libc++ -lz -Wno-deprecated-declarations -O2 -std=c++11

