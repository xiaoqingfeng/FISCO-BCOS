#pragma once

#include <memory>
#include <string>

class CallVendorHE
{
public:
    struct Impl;

    enum class Err
    {
        OK = 0,
        NOT_INIT,
        SO_LOAD_FAIL,
        SYM_LOAD_FAIL,
        BAD_PARAM,
        CRYPTO_FAIL,
        IO_FAIL,
        INTERNAL
    };

    CallVendorHE();
    ~CallVendorHE();

    Err initSo(const std::string& soPath);
    Err setFilePath(const std::string& path);
    Err loadSK(const std::string& filename);
    Err loadPK(const std::string& filename);
    Err loadDictionary(const std::string& filename);

    Err encryptPubInt(const std::string& in, std::string& out) const;
    Err decryptInt(const std::string& in, std::string& out) const;
    Err encryptPubDouble(const std::string& in, std::string& out) const;
    Err decryptDouble(const std::string& in, std::string& out) const;
    Err encryptPubString(const std::string& in, std::string& out) const;
    Err decryptString(const std::string& in, std::string& out) const;

    Err addInt(const std::string& a, const std::string& b, std::string& out) const;
    Err substractInt(const std::string& a, const std::string& b, std::string& out) const;
    Err addDouble(const std::string& a, const std::string& b, std::string& out) const;
    Err substractDouble(const std::string& a, const std::string& b, std::string& out) const;
    Err multiplyDouble(const std::string& a, const std::string& b, std::string& out) const;
    Err divideDouble(const std::string& a, const std::string& b, std::string& out) const;
    Err compareDouble(const std::string& a, const std::string& b, std::string& out) const;
    Err concatString(const std::string& a, const std::string& b, std::string& out) const;

    Err substring(
        const std::string& data, const std::string& startIn, const std::string& endIn, std::string& out) const;
    Err length(const std::string& data, std::string& out) const;

private:
    std::unique_ptr<Impl> m_impl;
};
