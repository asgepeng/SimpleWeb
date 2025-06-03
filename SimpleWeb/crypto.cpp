#include "crypto.h"
#include <sstream>
#include <iomanip>

unsigned char* CreateKey()
{
    static unsigned char key[16] = { 0 };
    memcpy(key, "Orogin@k-66171", 14);
    return key;
}
unsigned char* CreateIV()
{
    static unsigned char iv[16] = { 0 }; // bisa isi dengan nilai tetap juga
    return iv;
}
std::string Security::Encryption::ToString(std::vector<unsigned char>& cipherText)
{
    std::ostringstream oss;
    for (unsigned char byte : cipherText) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }
    return oss.str();
}
std::vector<unsigned char> Security::Encryption::ToHex(std::string& hexString)
{
    std::vector<unsigned char> bytes;
    for (size_t i = 0; i < hexString.length(); i += 2) {
        std::string byteString = hexString.substr(i, 2);
        unsigned char byte = static_cast<unsigned char>(strtol(byteString.c_str(), nullptr, 16));
        bytes.push_back(byte);
    }
    return bytes;
}
std::string Security::Encryption::AESEncrypt(const std::string& plaintext)
{
    auto key = CreateKey();
    auto iv = CreateIV();

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    std::vector<unsigned char> ciphertext(plaintext.size() + EVP_MAX_BLOCK_LENGTH);
    int len = 0, ciphertext_len = 0;

    EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), nullptr, key, iv);

    EVP_EncryptUpdate(ctx, ciphertext.data(), &len, reinterpret_cast<const unsigned char*>(plaintext.data()), plaintext.length());
    ciphertext_len = len;

    EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len);
    ciphertext_len += len;

    EVP_CIPHER_CTX_free(ctx);
    ciphertext.resize(ciphertext_len);  // Potong hasil akhir

    return ToString(ciphertext);
}
std::string Security::Encryption::AESDecrypt(std::string hexString)
{
    auto key = CreateKey();
    auto iv = CreateIV();
    std::vector<unsigned char> cipherText = ToHex(hexString);

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    std::vector<unsigned char> plainText(cipherText.size() + EVP_MAX_BLOCK_LENGTH);
    int len = 0, plainTextLen = 0;

    EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), nullptr, key, reinterpret_cast<const unsigned char*>(iv));
    EVP_DecryptUpdate(ctx, plainText.data(), &len, cipherText.data(), cipherText.size());
    plainTextLen = len;

    EVP_DecryptFinal_ex(ctx, plainText.data() + len, &len);
    plainTextLen += len;

    EVP_CIPHER_CTX_free(ctx);
    plainText.resize(plainTextLen);

    return std::string(plainText.begin(), plainText.end());
}




