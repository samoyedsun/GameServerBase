#ifndef wMD5_H
#define wMD5_H

#include <string>
#include <fstream>

/* wMD5 declaration. */
class wMD5 {
public:
	/* Type define */
	typedef unsigned char byte;
	typedef unsigned int ulong;

public:
	wMD5();
	wMD5(const void *input, size_t length);
	wMD5(const std::string &str);
	void update(const void *input, size_t length);
	void update(const std::string &str);
	const byte* digest();
	std::string toString();
	void reset();
private:
	void update(const byte *input, size_t length);
	void final();
	void transform(const byte block[64]);
	void encode(const ulong *input, byte *output, size_t length);
	void decode(const byte *input, ulong *output, size_t length);
	std::string bytesToHexString(const byte *input, size_t length);

	/* class uncopyable */
	wMD5(const wMD5&);
	wMD5& operator=(const wMD5&);
private:
	ulong _state[4];	/* state (ABCD) */
	ulong _count[2];	/* number of bits, modulo 2^64 (low-order word first) */
	byte _buffer[64];	/* input buffer */
	byte _digest[16];	/* message digest */
	bool _finished;		/* calculate finished ? */

	static const byte PADDING[64];	/* padding for calculate */
	static const char HEX[16];
	static const size_t BUFFER_SIZE = 1024;
};

#endif/*wMD5_H*/