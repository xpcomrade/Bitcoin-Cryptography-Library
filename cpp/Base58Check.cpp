/* 
 * Bitcoin cryptography library
 * Copyright (c) Project Nayuki
 * 
 * http://www.nayuki.io/page/bitcoin-cryptography-library
 * https://github.com/nayuki/Bitcoin-Cryptography-Library
 */

#include <cassert>
#include <cstring>
#include "Base58Check.hpp"
#include "Sha256.hpp"
#include "Sha256Hash.hpp"


void Base58Check::pubkeyHashToBase58Check(const uint8_t pubkeyHash[RIPEMD160_HASH_LEN], char outStr[35]) {
	assert(pubkeyHash != nullptr && outStr != nullptr);
	uint8_t toEncode[1 + RIPEMD160_HASH_LEN + 4] = {};
	toEncode[0] = 0x00;  // Version byte
	memcpy(&toEncode[1], pubkeyHash, RIPEMD160_HASH_LEN);
	bytesToBase58Check(toEncode, static_cast<int>(sizeof(toEncode) - 4), outStr);
}


void Base58Check::privateKeyToBase58Check(const Uint256 &privKey, char outStr[53]) {
	assert(outStr != nullptr);
	uint8_t toEncode[1 + 32 + 1 + 4] = {};
	toEncode[0] = 0x80;  // Version byte
	privKey.getBigEndianBytes(&toEncode[1]);
	toEncode[33] = 0x01;  // Compressed marker
	bytesToBase58Check(toEncode, static_cast<int>(sizeof(toEncode) - 4), outStr);
}


void Base58Check::bytesToBase58Check(uint8_t *data, int len, char *outStr) {
	// Append 4-byte hash
	#define MAX_TOTAL_BYTES 38  // Including the 4-byte hash
	assert(0 <= len && len <= MAX_TOTAL_BYTES - 4);
	Sha256Hash sha256Hash = Sha256::getDoubleHash(data, len);
	for (int i = 0; i < 4; i++, len++)
		data[len] = sha256Hash.getByte(i);
	
	// Count leading zero bytes
	int leadingZeros = 0;
	for (int i = 0; i < len && data[i] == 0; i++)
		leadingZeros++;
	
	// Encode to Base 58
	int outLen = 0;
	while (!isZero(data, len)) {  // Extract digits in little-endian
		outStr[outLen] = ALPHABET[mod58(data, len)];
		outLen++;
		uint8_t quotient[MAX_TOTAL_BYTES] = {};
		divide58(data, quotient, len);  // quotient = floor(data / 58)
		memcpy(data, quotient, len);  // data = quotient
	}
	for (int i = 0; i < leadingZeros; i++) {  // Append leading zeros
		outStr[outLen] = ALPHABET[0];
		outLen++;
	}
	outStr[outLen] = '\0';
	
	// Reverse the string
	for (int i = 0, j = outLen - 1; i < j; i++, j--) {
		char temp = outStr[i];
		outStr[i] = outStr[j];
		outStr[j] = temp;
	}
	#undef MAX_TOTAL_BYTES
}


bool Base58Check::isZero(const uint8_t *x, int len) {
	for (int i = 0; i < len; i++) {
		if (x[i] != 0)
			return false;
	}
	return true;
}


uint8_t Base58Check::mod58(const uint8_t *x, int len) {
	uint_fast16_t sum = 0;
	for (int i = 0; i < len; i++)
		sum = ((sum * 24) + x[i]) % 58;  // Note: 256 % 58 = 24
	return static_cast<uint8_t>(sum);
}


void Base58Check::divide58(const uint8_t *x, uint8_t *y, int len) {
	memset(y, 0, len);
	uint_fast16_t dividend = 0;
	for (int i = 0; i < len * 8; i++) {  // For each output bit
		dividend = (dividend << 1) | ((x[i >> 3] >> (7 - (i & 7))) & 1);  // Shift next input bit into right side
		if (dividend >= 58) {
			dividend -= 58;
			y[i >> 3] |= 1 << (7 - (i & 7));
		}
	}
}


Base58Check::Base58Check() {}


// Static initializers
const char *Base58Check::ALPHABET = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";