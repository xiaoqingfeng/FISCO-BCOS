#include "CallVendorHE.h"
#include <dlfcn.h>
#include <mutex>

namespace
{
template <typename T>
T loadSym(void* handle, const char* name)
{
    dlerror();
    void* sym = dlsym(handle, name);
    const char* e = dlerror();
    if (e || !sym)
    {
        return nullptr;
    }
    return reinterpret_cast<T>(sym);
}
}  // namespace

struct CallVendorHE::Impl
{
    typedef int (*set_path_t)(const char*);
    typedef int (*load_key_t)(const char*);
    typedef int (*one_in_one_out_t)(const char*, char*, int);
    typedef int (*two_in_one_out_t)(const char*, const char*, char*, int);
    typedef int (*three_in_one_out_t)(const char*, const char*, const char*, char*, int);

    void* soHandle = nullptr;
    std::string basePath;
    bool ready = false;
    mutable std::mutex mutex;

    set_path_t setPath = nullptr;
    load_key_t loadSK = nullptr;
    load_key_t loadPK = nullptr;
    load_key_t loadDict = nullptr;

    one_in_one_out_t encInt = nullptr;
    one_in_one_out_t decInt = nullptr;
    one_in_one_out_t encDouble = nullptr;
    one_in_one_out_t decDouble = nullptr;
    one_in_one_out_t encString = nullptr;
    one_in_one_out_t decString = nullptr;

    two_in_one_out_t addInt = nullptr;
    two_in_one_out_t subInt = nullptr;
    two_in_one_out_t addDouble = nullptr;
    two_in_one_out_t subDouble = nullptr;
    two_in_one_out_t mulDouble = nullptr;
    two_in_one_out_t divDouble = nullptr;
    two_in_one_out_t cmpDouble = nullptr;
    two_in_one_out_t concatString = nullptr;

    three_in_one_out_t substring = nullptr;
    one_in_one_out_t length = nullptr;
};

namespace
{
inline CallVendorHE::Err mapRet(int ret)
{
    return (ret == 0) ? CallVendorHE::Err::OK : CallVendorHE::Err::CRYPTO_FAIL;
}
}  // namespace

CallVendorHE::CallVendorHE() : m_impl(new Impl()) {}

CallVendorHE::~CallVendorHE()
{
    if (m_impl && m_impl->soHandle)
    {
        dlclose(m_impl->soHandle);
        m_impl->soHandle = nullptr;
    }
}

CallVendorHE::Err CallVendorHE::initSo(const std::string& soPath)
{
    std::lock_guard<std::mutex> lock(m_impl->mutex);

    if (m_impl->soHandle)
    {
        dlclose(m_impl->soHandle);
        m_impl->soHandle = nullptr;
        m_impl->ready = false;
    }

    m_impl->soHandle = dlopen(soPath.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!m_impl->soHandle)
    {
        return Err::SO_LOAD_FAIL;
    }

    // TODO: replace symbols with real vendor api names
    m_impl->setPath = loadSym<Impl::set_path_t>(m_impl->soHandle, "VHE_SetFilePath");
    m_impl->loadSK = loadSym<Impl::load_key_t>(m_impl->soHandle, "VHE_LoadSK");
    m_impl->loadPK = loadSym<Impl::load_key_t>(m_impl->soHandle, "VHE_LoadPK");
    m_impl->loadDict = loadSym<Impl::load_key_t>(m_impl->soHandle, "VHE_LoadDictionary");

    m_impl->encInt = loadSym<Impl::one_in_one_out_t>(m_impl->soHandle, "VHE_EncryptPubInt");
    m_impl->decInt = loadSym<Impl::one_in_one_out_t>(m_impl->soHandle, "VHE_DecryptInt");
    m_impl->encDouble = loadSym<Impl::one_in_one_out_t>(m_impl->soHandle, "VHE_EncryptPubDouble");
    m_impl->decDouble = loadSym<Impl::one_in_one_out_t>(m_impl->soHandle, "VHE_DecryptDouble");
    m_impl->encString = loadSym<Impl::one_in_one_out_t>(m_impl->soHandle, "VHE_EncryptPubString");
    m_impl->decString = loadSym<Impl::one_in_one_out_t>(m_impl->soHandle, "VHE_DecryptString");

    m_impl->addInt = loadSym<Impl::two_in_one_out_t>(m_impl->soHandle, "VHE_AddInt");
    m_impl->subInt = loadSym<Impl::two_in_one_out_t>(m_impl->soHandle, "VHE_SubstractInt");
    m_impl->addDouble = loadSym<Impl::two_in_one_out_t>(m_impl->soHandle, "VHE_AddDouble");
    m_impl->subDouble = loadSym<Impl::two_in_one_out_t>(m_impl->soHandle, "VHE_SubstractDouble");
    m_impl->mulDouble = loadSym<Impl::two_in_one_out_t>(m_impl->soHandle, "VHE_MultiplyDouble");
    m_impl->divDouble = loadSym<Impl::two_in_one_out_t>(m_impl->soHandle, "VHE_DivideDouble");
    m_impl->cmpDouble = loadSym<Impl::two_in_one_out_t>(m_impl->soHandle, "VHE_CompareDouble");
    m_impl->concatString = loadSym<Impl::two_in_one_out_t>(m_impl->soHandle, "VHE_ConcatString");

    m_impl->substring = loadSym<Impl::three_in_one_out_t>(m_impl->soHandle, "VHE_Substring");
    m_impl->length = loadSym<Impl::one_in_one_out_t>(m_impl->soHandle, "VHE_Length");

    if (!m_impl->setPath || !m_impl->loadSK || !m_impl->loadPK || !m_impl->loadDict ||
        !m_impl->encInt || !m_impl->decInt || !m_impl->encDouble || !m_impl->decDouble ||
        !m_impl->encString || !m_impl->decString || !m_impl->addInt || !m_impl->subInt ||
        !m_impl->addDouble || !m_impl->subDouble || !m_impl->mulDouble || !m_impl->divDouble ||
        !m_impl->cmpDouble || !m_impl->concatString || !m_impl->substring || !m_impl->length)
    {
        return Err::SYM_LOAD_FAIL;
    }

    return Err::OK;
}

CallVendorHE::Err CallVendorHE::setFilePath(const std::string& path)
{
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    if (!m_impl->soHandle || !m_impl->setPath)
    {
        return Err::NOT_INIT;
    }
    if (path.empty())
    {
        return Err::BAD_PARAM;
    }
    if (m_impl->setPath(path.c_str()) != 0)
    {
        return Err::IO_FAIL;
    }
    m_impl->basePath = path;
    return Err::OK;
}

CallVendorHE::Err CallVendorHE::loadSK(const std::string& filename)
{
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    if (!m_impl->soHandle || !m_impl->loadSK)
    {
        return Err::NOT_INIT;
    }
    if (filename.empty())
    {
        return Err::BAD_PARAM;
    }
    return mapRet(m_impl->loadSK((m_impl->basePath + filename).c_str()));
}

CallVendorHE::Err CallVendorHE::loadPK(const std::string& filename)
{
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    if (!m_impl->soHandle || !m_impl->loadPK)
    {
        return Err::NOT_INIT;
    }
    if (filename.empty())
    {
        return Err::BAD_PARAM;
    }
    return mapRet(m_impl->loadPK((m_impl->basePath + filename).c_str()));
}

CallVendorHE::Err CallVendorHE::loadDictionary(const std::string& filename)
{
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    if (!m_impl->soHandle || !m_impl->loadDict)
    {
        return Err::NOT_INIT;
    }
    if (filename.empty())
    {
        return Err::BAD_PARAM;
    }
    auto ret = mapRet(m_impl->loadDict((m_impl->basePath + filename).c_str()));
    m_impl->ready = (ret == Err::OK);
    return ret;
}

static CallVendorHE::Err call1(const CallVendorHE::Impl* impl,
    CallVendorHE::Impl::one_in_one_out_t fn, const std::string& input, std::string& out)
{
    if (!impl->soHandle || !impl->ready || !fn)
    {
        return CallVendorHE::Err::NOT_INIT;
    }
    if (input.empty())
    {
        return CallVendorHE::Err::BAD_PARAM;
    }
    char buf[65536] = {0};
    auto ret = mapRet(fn(input.c_str(), buf, sizeof(buf)));
    if (ret != CallVendorHE::Err::OK)
    {
        return ret;
    }
    out = std::string(buf);
    return CallVendorHE::Err::OK;
}

static CallVendorHE::Err call2(const CallVendorHE::Impl* impl,
    CallVendorHE::Impl::two_in_one_out_t fn, const std::string& a, const std::string& b,
    std::string& out)
{
    if (!impl->soHandle || !impl->ready || !fn)
    {
        return CallVendorHE::Err::NOT_INIT;
    }
    if (a.empty() || b.empty())
    {
        return CallVendorHE::Err::BAD_PARAM;
    }
    char buf[65536] = {0};
    auto ret = mapRet(fn(a.c_str(), b.c_str(), buf, sizeof(buf)));
    if (ret != CallVendorHE::Err::OK)
    {
        return ret;
    }
    out = std::string(buf);
    return CallVendorHE::Err::OK;
}

CallVendorHE::Err CallVendorHE::encryptPubInt(const std::string& in, std::string& out) const
{
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    return call1(m_impl.get(), m_impl->encInt, in, out);
}
CallVendorHE::Err CallVendorHE::decryptInt(const std::string& in, std::string& out) const
{
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    return call1(m_impl.get(), m_impl->decInt, in, out);
}
CallVendorHE::Err CallVendorHE::encryptPubDouble(const std::string& in, std::string& out) const
{
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    return call1(m_impl.get(), m_impl->encDouble, in, out);
}
CallVendorHE::Err CallVendorHE::decryptDouble(const std::string& in, std::string& out) const
{
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    return call1(m_impl.get(), m_impl->decDouble, in, out);
}
CallVendorHE::Err CallVendorHE::encryptPubString(const std::string& in, std::string& out) const
{
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    return call1(m_impl.get(), m_impl->encString, in, out);
}
CallVendorHE::Err CallVendorHE::decryptString(const std::string& in, std::string& out) const
{
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    return call1(m_impl.get(), m_impl->decString, in, out);
}
CallVendorHE::Err CallVendorHE::length(const std::string& data, std::string& out) const
{
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    return call1(m_impl.get(), m_impl->length, data, out);
}

CallVendorHE::Err CallVendorHE::addInt(const std::string& a, const std::string& b, std::string& out) const
{
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    return call2(m_impl.get(), m_impl->addInt, a, b, out);
}
CallVendorHE::Err CallVendorHE::substractInt(
    const std::string& a, const std::string& b, std::string& out) const
{
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    return call2(m_impl.get(), m_impl->subInt, a, b, out);
}
CallVendorHE::Err CallVendorHE::addDouble(
    const std::string& a, const std::string& b, std::string& out) const
{
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    return call2(m_impl.get(), m_impl->addDouble, a, b, out);
}
CallVendorHE::Err CallVendorHE::substractDouble(
    const std::string& a, const std::string& b, std::string& out) const
{
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    return call2(m_impl.get(), m_impl->subDouble, a, b, out);
}
CallVendorHE::Err CallVendorHE::multiplyDouble(
    const std::string& a, const std::string& b, std::string& out) const
{
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    return call2(m_impl.get(), m_impl->mulDouble, a, b, out);
}
CallVendorHE::Err CallVendorHE::divideDouble(
    const std::string& a, const std::string& b, std::string& out) const
{
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    return call2(m_impl.get(), m_impl->divDouble, a, b, out);
}
CallVendorHE::Err CallVendorHE::compareDouble(
    const std::string& a, const std::string& b, std::string& out) const
{
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    return call2(m_impl.get(), m_impl->cmpDouble, a, b, out);
}
CallVendorHE::Err CallVendorHE::concatString(
    const std::string& a, const std::string& b, std::string& out) const
{
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    return call2(m_impl.get(), m_impl->concatString, a, b, out);
}

CallVendorHE::Err CallVendorHE::substring(
    const std::string& data, const std::string& startIn, const std::string& endIn, std::string& out) const
{
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    if (!m_impl->soHandle || !m_impl->ready || !m_impl->substring)
    {
        return Err::NOT_INIT;
    }
    if (data.empty())
    {
        return Err::BAD_PARAM;
    }
    char buf[65536] = {0};
    auto ret = mapRet(m_impl->substring(data.c_str(), startIn.c_str(), endIn.c_str(), buf, sizeof(buf)));
    if (ret != Err::OK)
    {
        return ret;
    }
    out = std::string(buf);
    return Err::OK;
}
