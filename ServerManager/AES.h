#pragma once
//AES.h

#include <windows.h>
#include <bcrypt.h>
#include <vector>
#include <string>
#include <stdexcept>
#include<iostream>

#pragma comment(lib, "bcrypt.lib")

class Encrypt {
private:
    static std::vector<BYTE> deriveKey(const std::string& keyStr) {
        BCRYPT_ALG_HANDLE hHashAlg = nullptr;
        std::vector<BYTE> derivedKey(32);  // AES-256��Ҫ32�ֽ�
        DWORD derivedBytes = 0;

        // ʹ��PBKDF2��Կ�����㷨
        NTSTATUS status = BCryptOpenAlgorithmProvider(
            &hHashAlg,
            BCRYPT_SHA256_ALGORITHM,
            nullptr,
            BCRYPT_ALG_HANDLE_HMAC_FLAG
        );
        if (!BCRYPT_SUCCESS(status)) {
            throw std::runtime_error("Open PBKDF2 failed");
        }

        // ִ����Կ�������򻯲�����ʵ��Ӧʹ������Σ�
        status = BCryptDeriveKeyPBKDF2(
            hHashAlg,
            reinterpret_cast<PUCHAR>(const_cast<char*>(keyStr.data())),
            static_cast<ULONG>(keyStr.size()),
            nullptr,  // ʵ��Ӧ��Ӧʹ�������
            0,
            10000,    // ��������
            derivedKey.data(),
            static_cast<ULONG>(derivedKey.size()),
            0
        );

        BCryptCloseAlgorithmProvider(hHashAlg, 0);

        if (!BCRYPT_SUCCESS(status)) {
            throw std::runtime_error("Key derivation failed");
        }
        return derivedKey;
    }

public:
    static std::string encodeByAES(const std::string& content, const std::string& keyStr) {
        BCRYPT_ALG_HANDLE hAlg = nullptr;
        BCRYPT_KEY_HANDLE hKey = nullptr;
        NTSTATUS status;
        DWORD cipherLen = 0;
        std::vector<BYTE> cipherText;

        try {
            // ������Կ
            auto aesKey = deriveKey(keyStr);

            // ��ʼ���㷨
            status = BCryptOpenAlgorithmProvider(
                &hAlg,
                BCRYPT_AES_ALGORITHM,
                nullptr,
                0
            );
            if (!BCRYPT_SUCCESS(status)) {
                throw std::runtime_error("Open algorithm failed");
            }

            // ����ECBģʽ
            status = BCryptSetProperty(
                hAlg,
                BCRYPT_CHAINING_MODE,
                (PBYTE)BCRYPT_CHAIN_MODE_ECB,
                sizeof(BCRYPT_CHAIN_MODE_ECB),
                0
            );
            if (!BCRYPT_SUCCESS(status)) {
                throw std::runtime_error("Set ECB mode failed");
            }

            // ������Կ
            status = BCryptGenerateSymmetricKey(
                hAlg,
                &hKey,
                nullptr,
                0,
                aesKey.data(),
                static_cast<ULONG>(aesKey.size()),
                0
            );
            if (!BCRYPT_SUCCESS(status)) {
                throw std::runtime_error("Create key failed");
            }

            // ������ܳ���
            DWORD plainLen = static_cast<DWORD>(content.size());
            status = BCryptEncrypt(
                hKey,
                reinterpret_cast<PUCHAR>(const_cast<char*>(content.data())),
                plainLen,
                nullptr,
                nullptr,
                0,
                nullptr,
                0,
                &cipherLen,
                BCRYPT_BLOCK_PADDING
            );
            if (!BCRYPT_SUCCESS(status)) {
                throw std::runtime_error("Get size failed");
            }

            // ִ�м���
            cipherText.resize(cipherLen);
            status = BCryptEncrypt(
                hKey,
                reinterpret_cast<PUCHAR>(const_cast<char*>(content.data())),
                plainLen,
                nullptr,
                nullptr,
                0,
                cipherText.data(),
                cipherLen,
                &cipherLen,
                BCRYPT_BLOCK_PADDING
            );
            if (!BCRYPT_SUCCESS(status)) {
                throw std::runtime_error("Encryption failed");
            }

            BCryptDestroyKey(hKey);
            BCryptCloseAlgorithmProvider(hAlg, 0);

            return std::string(cipherText.begin(), cipherText.end());

        }
        catch (...) {
            if (hKey) BCryptDestroyKey(hKey);
            if (hAlg) BCryptCloseAlgorithmProvider(hAlg, 0);
            throw;
        }
    }

    static std::string decodeByAES(const std::string& cipherText, const std::string& keyStr) {
        BCRYPT_ALG_HANDLE hAlg = nullptr;
        BCRYPT_KEY_HANDLE hKey = nullptr;
        NTSTATUS status;
        DWORD plainLen = 0;
        std::vector<BYTE> plainText;

        try {
            auto aesKey = deriveKey(keyStr);

            status = BCryptOpenAlgorithmProvider(
                &hAlg,
                BCRYPT_AES_ALGORITHM,
                nullptr,
                0
            );
            if (!BCRYPT_SUCCESS(status)) {
                throw std::runtime_error("Open algorithm failed");
            }

            status = BCryptSetProperty(
                hAlg,
                BCRYPT_CHAINING_MODE,
                (PBYTE)BCRYPT_CHAIN_MODE_ECB,
                sizeof(BCRYPT_CHAIN_MODE_ECB),
                0
            );
            if (!BCRYPT_SUCCESS(status)) {
                throw std::runtime_error("Set ECB mode failed");
            }

            status = BCryptGenerateSymmetricKey(
                hAlg,
                &hKey,
                nullptr,
                0,
                aesKey.data(),
                static_cast<ULONG>(aesKey.size()),
                0
            );
            if (!BCRYPT_SUCCESS(status)) {
                throw std::runtime_error("Create key failed");
            }

            DWORD cipherLen = static_cast<DWORD>(cipherText.size());
            status = BCryptDecrypt(
                hKey,
                reinterpret_cast<PUCHAR>(const_cast<char*>(cipherText.data())),
                cipherLen,
                nullptr,
                nullptr,
                0,
                nullptr,
                0,
                &plainLen,
                BCRYPT_BLOCK_PADDING
            );
            if (!BCRYPT_SUCCESS(status)) {
                throw std::runtime_error("Get size failed");
            }

            plainText.resize(plainLen);
            status = BCryptDecrypt(
                hKey,
                reinterpret_cast<PUCHAR>(const_cast<char*>(cipherText.data())),
                cipherLen,
                nullptr,
                nullptr,
                0,
                plainText.data(),
                plainLen,
                &plainLen,
                BCRYPT_BLOCK_PADDING
            );
            if (!BCRYPT_SUCCESS(status)) {
                throw std::runtime_error("Decryption failed");
            }

            BCryptDestroyKey(hKey);
            BCryptCloseAlgorithmProvider(hAlg, 0);

            return std::string(plainText.begin(), plainText.end());

        }
        catch (...) {
            if (hKey) BCryptDestroyKey(hKey);
            if (hAlg) BCryptCloseAlgorithmProvider(hAlg, 0);
            throw;
        }
    }
};
