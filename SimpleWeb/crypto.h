#pragma once
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <iostream>
#include <vector>
#include <string>
#include <cstring>

namespace Security::Encryption
{
    std::string AESEncrypt(const std::string& plaintext);
    std::string AESDecrypt(std::string hexString);
    std::string ToString(std::vector<unsigned char>& cipherText);
    std::vector<unsigned char> ToHex(std::string& hexString);
}