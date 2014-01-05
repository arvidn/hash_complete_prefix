ALL: test count_prefix gen_test_vectors

test : test.cpp trie.hpp
	c++ test.cpp -g -o test

count_prefix : count_prefix.cpp trie.hpp
	c++ count_prefix.cpp MurmurHash3.cpp -g -o count_prefix -lcrypto -L/opt/local/lib -I/opt/local/include -stdlib=libc++ -lz -Wno-deprecated-declarations -O2 -std=c++11

gen_test_vectors : generate_test_vectors.cpp
	c++ generate_test_vectors.cpp -o gen_test_vectors -lz -g -std=c++11 -stdlib=libc++

