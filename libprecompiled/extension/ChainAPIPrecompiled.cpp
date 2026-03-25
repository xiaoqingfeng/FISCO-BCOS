#include "ChainAPIPrecompiled.h"
#include <boost/algorithm/string.hpp>
#include <libdevcore/Common.h>
#include <libethcore/ABI.h>
#include <cstdlib>

using namespace dev;
using namespace dev::blockverifier;
using namespace dev::precompiled;

const char* const CHAIN_API_GET = "get()";
const char* const CHAIN_API_SET = "set(string)";
const char* const CHAIN_API_V_SET_FILE_PATH = "VSetFilePath(string)";
const char* const CHAIN_API_V_LOAD_SK = "VLoadSK(string)";
const char* const CHAIN_API_V_LOAD_PK = "VLoadPK(string)";
const char* const CHAIN_API_V_LOAD_DICTIONARY = "VLoadDictionary(string)";
const char* const CHAIN_API_V_ENCRYPT_PUB_INT = "VEncryptPubInt(string)";
const char* const CHAIN_API_V_DECRYPT_INT = "VDecryptInt(string)";
const char* const CHAIN_API_V_ENCRYPT_PUB_DOUBLE = "VEncryptPubDouble(string)";
const char* const CHAIN_API_V_DECRYPT_DOUBLE = "VDecryptDouble(string)";
const char* const CHAIN_API_V_ENCRYPT_PUB_STRING = "VEncryptPubString(string)";
const char* const CHAIN_API_V_DECRYPT_STRING = "VDecryptString(string)";
const char* const CHAIN_API_V_ADD_INT = "VAddInt(string,string)";
const char* const CHAIN_API_V_SUBSTRACT_INT = "VSubstractInt(string,string)";
const char* const CHAIN_API_V_ADD_DOUBLE = "VAddDouble(string,string)";
const char* const CHAIN_API_V_SUBSTRACT_DOUBLE = "VSubstractDouble(string,string)";
const char* const CHAIN_API_V_MULTIPLY_DOUBLE = "VMultiplyDouble(string,string)";
const char* const CHAIN_API_V_DIVIDE_DOUBLE = "VDivideDouble(string,string)";
const char* const CHAIN_API_V_COMPARE_DOUBLE = "VCompareDouble(string,string)";
const char* const CHAIN_API_V_CONCAT_STRING = "VConcatString(string,string)";
const char* const CHAIN_API_V_SUBSTRING = "VSubstring(string,string,string)";
const char* const CHAIN_API_V_LENGTH = "VLength(string)";

namespace
{
enum ChainAPILogLevel
{
    LOG_LEVEL_TRACE = 0,
    LOG_LEVEL_DEBUG = 1,
    LOG_LEVEL_INFO = 2,
    LOG_LEVEL_WARNING = 3,
    LOG_LEVEL_ERROR = 4
};

ChainAPILogLevel chainAPILogLevel()
{
    static ChainAPILogLevel s_level = []() {
        const char* configured = std::getenv("CHAIN_API_PRECOMPILED_LOG_LEVEL");
        if (!configured)
        {
            return LOG_LEVEL_DEBUG;
        }
        std::string level = configured;
        boost::to_upper(level);
        if (level == "TRACE")
        {
            return LOG_LEVEL_TRACE;
        }
        if (level == "DEBUG")
        {
            return LOG_LEVEL_DEBUG;
        }
        if (level == "INFO")
        {
            return LOG_LEVEL_INFO;
        }
        if (level == "WARNING" || level == "WARN")
        {
            return LOG_LEVEL_WARNING;
        }
        if (level == "ERROR")
        {
            return LOG_LEVEL_ERROR;
        }
        return LOG_LEVEL_DEBUG;
    }();
    return s_level;
}

inline bool shouldLog(ChainAPILogLevel _expected) { return _expected >= chainAPILogLevel(); }

inline void setError(PrecompiledExecResult::Ptr const& _callResult, int _errorCode)
{
    getErrorCodeOut(_callResult->mutableExecResult(), _errorCode);
}

inline int mapVendorError(CallVendorHE::Err e)
{
    switch (e)
    {
    case CallVendorHE::Err::OK:
        return 0;
    case CallVendorHE::Err::NOT_INIT:
        return -51610;
    case CallVendorHE::Err::SO_LOAD_FAIL:
        return -51611;
    case CallVendorHE::Err::SYM_LOAD_FAIL:
        return -51612;
    case CallVendorHE::Err::BAD_PARAM:
        return -51613;
    case CallVendorHE::Err::IO_FAIL:
        return -51614;
    case CallVendorHE::Err::CRYPTO_FAIL:
        return CODE_INVALID_CIPHERS;
    case CallVendorHE::Err::INTERNAL:
    default:
        return -51699;
    }
}
}  // namespace

ChainAPIPrecompiled::ChainAPIPrecompiled() : m_vendor(std::make_shared<CallVendorHE>())
{
    if (shouldLog(LOG_LEVEL_INFO))
    {
        PRECOMPILED_LOG(INFO) << LOG_BADGE("ChainAPIPrecompiled")
                              << LOG_DESC("init")
                              << LOG_KV("defaultLogLevel", "DEBUG")
                              << LOG_KV("env", "CHAIN_API_PRECOMPILED_LOG_LEVEL");
    }
    auto regMethod = [this](const char* _method) {
        name2Selector[_method] = getFuncSelector(_method);
        if (shouldLog(LOG_LEVEL_DEBUG))
        {
            PRECOMPILED_LOG(DEBUG) << LOG_BADGE("ChainAPIPrecompiled")
                                   << LOG_DESC("register selector")
                                   << LOG_KV("method", _method)
                                   << LOG_KV("selector", name2Selector[_method]);
        }
    };

    regMethod(CHAIN_API_GET);
    regMethod(CHAIN_API_SET);
    regMethod(CHAIN_API_V_SET_FILE_PATH);
    regMethod(CHAIN_API_V_LOAD_SK);
    regMethod(CHAIN_API_V_LOAD_PK);
    regMethod(CHAIN_API_V_LOAD_DICTIONARY);
    regMethod(CHAIN_API_V_ENCRYPT_PUB_INT);
    regMethod(CHAIN_API_V_DECRYPT_INT);
    regMethod(CHAIN_API_V_ENCRYPT_PUB_DOUBLE);
    regMethod(CHAIN_API_V_DECRYPT_DOUBLE);
    regMethod(CHAIN_API_V_ENCRYPT_PUB_STRING);
    regMethod(CHAIN_API_V_DECRYPT_STRING);
    regMethod(CHAIN_API_V_ADD_INT);
    regMethod(CHAIN_API_V_SUBSTRACT_INT);
    regMethod(CHAIN_API_V_ADD_DOUBLE);
    regMethod(CHAIN_API_V_SUBSTRACT_DOUBLE);
    regMethod(CHAIN_API_V_MULTIPLY_DOUBLE);
    regMethod(CHAIN_API_V_DIVIDE_DOUBLE);
    regMethod(CHAIN_API_V_COMPARE_DOUBLE);
    regMethod(CHAIN_API_V_CONCAT_STRING);
    regMethod(CHAIN_API_V_SUBSTRING);
    regMethod(CHAIN_API_V_LENGTH);
}

PrecompiledExecResult::Ptr ChainAPIPrecompiled::call(
    ExecutiveContext::Ptr, bytesConstRef _param, Address const&, Address const&)
{
    if (shouldLog(LOG_LEVEL_DEBUG))
    {
        PRECOMPILED_LOG(DEBUG) << LOG_BADGE("ChainAPIPrecompiled") << LOG_DESC("call")
                               << LOG_KV("paramSize", _param.size()) << LOG_KV("param", toHex(_param));
    }

    auto callResult = m_precompiledExecResultFactory->createPrecompiledResult();
    callResult->gasPricer()->setMemUsed(_param.size());

    auto func = getParamFunc(_param);
    auto data = getParamData(_param);
    dev::eth::ContractABI abi;
    if (shouldLog(LOG_LEVEL_DEBUG))
    {
        PRECOMPILED_LOG(DEBUG) << LOG_BADGE("ChainAPIPrecompiled") << LOG_DESC("dispatch")
                               << LOG_KV("selector", func) << LOG_KV("dataSize", data.size());
    }

    if (func == name2Selector[CHAIN_API_GET])
    {
        if (shouldLog(LOG_LEVEL_DEBUG))
        {
            PRECOMPILED_LOG(DEBUG) << LOG_BADGE("ChainAPIPrecompiled") << LOG_DESC("execute get")
                                   << LOG_KV("version", m_version);
        }
        callResult->setExecResult(abi.abiIn("", m_version));
        return callResult;
    }
    if (func == name2Selector[CHAIN_API_SET])
    {
        std::string version;
        abi.abiOut(data, version);
        if (shouldLog(LOG_LEVEL_DEBUG))
        {
            PRECOMPILED_LOG(DEBUG) << LOG_BADGE("ChainAPIPrecompiled") << LOG_DESC("execute set")
                                   << LOG_KV("newVersion", version);
        }
        m_version = version;
        setError(callResult, 0);
        return callResult;
    }

    if (func == name2Selector[CHAIN_API_V_SET_FILE_PATH])
    {
        std::string path;
        abi.abiOut(data, path);
        if (shouldLog(LOG_LEVEL_DEBUG))
        {
            PRECOMPILED_LOG(DEBUG) << LOG_BADGE("ChainAPIPrecompiled")
                                   << LOG_DESC("execute VSetFilePath") << LOG_KV("path", path);
        }
        setError(callResult, mapVendorError(m_vendor->setFilePath(path)));
        return callResult;
    }
    if (func == name2Selector[CHAIN_API_V_LOAD_SK])
    {
        std::string file;
        abi.abiOut(data, file);
        if (shouldLog(LOG_LEVEL_DEBUG))
        {
            PRECOMPILED_LOG(DEBUG) << LOG_BADGE("ChainAPIPrecompiled")
                                   << LOG_DESC("execute VLoadSK") << LOG_KV("file", file);
        }
        setError(callResult, mapVendorError(m_vendor->loadSK(file)));
        return callResult;
    }
    if (func == name2Selector[CHAIN_API_V_LOAD_PK])
    {
        std::string file;
        abi.abiOut(data, file);
        if (shouldLog(LOG_LEVEL_DEBUG))
        {
            PRECOMPILED_LOG(DEBUG) << LOG_BADGE("ChainAPIPrecompiled")
                                   << LOG_DESC("execute VLoadPK") << LOG_KV("file", file);
        }
        setError(callResult, mapVendorError(m_vendor->loadPK(file)));
        return callResult;
    }
    if (func == name2Selector[CHAIN_API_V_LOAD_DICTIONARY])
    {
        std::string file;
        abi.abiOut(data, file);
        if (shouldLog(LOG_LEVEL_DEBUG))
        {
            PRECOMPILED_LOG(DEBUG) << LOG_BADGE("ChainAPIPrecompiled")
                                   << LOG_DESC("execute VLoadDictionary") << LOG_KV("file", file);
        }
        setError(callResult, mapVendorError(m_vendor->loadDictionary(file)));
        return callResult;
    }

    auto call1 = [&](CallVendorHE::Err (CallVendorHE::*fn)(const std::string&, std::string&) const) {
        std::string input;
        std::string output;
        abi.abiOut(data, input);
        if (shouldLog(LOG_LEVEL_DEBUG))
        {
            PRECOMPILED_LOG(DEBUG) << LOG_BADGE("ChainAPIPrecompiled")
                                   << LOG_DESC("execute unary method") << LOG_KV("input", input);
        }
        auto error = (m_vendor.get()->*fn)(input, output);
        if (error != CallVendorHE::Err::OK)
        {
            PRECOMPILED_LOG(ERROR) << LOG_BADGE("ChainAPIPrecompiled")
                                   << LOG_DESC("unary method failed")
                                   << LOG_KV("error", static_cast<int>(error));
            setError(callResult, mapVendorError(error));
            return;
        }
        if (shouldLog(LOG_LEVEL_DEBUG))
        {
            PRECOMPILED_LOG(DEBUG) << LOG_BADGE("ChainAPIPrecompiled")
                                   << LOG_DESC("unary method done") << LOG_KV("output", output);
        }
        callResult->setExecResult(abi.abiIn("", output));
    };

    auto call2 = [&](CallVendorHE::Err (CallVendorHE::*fn)(
                     const std::string&, const std::string&, std::string&) const) {
        std::string a;
        std::string b;
        std::string output;
        abi.abiOut(data, a, b);
        if (shouldLog(LOG_LEVEL_DEBUG))
        {
            PRECOMPILED_LOG(DEBUG) << LOG_BADGE("ChainAPIPrecompiled")
                                   << LOG_DESC("execute binary method")
                                   << LOG_KV("left", a) << LOG_KV("right", b);
        }
        auto error = (m_vendor.get()->*fn)(a, b, output);
        if (error != CallVendorHE::Err::OK)
        {
            PRECOMPILED_LOG(ERROR) << LOG_BADGE("ChainAPIPrecompiled")
                                   << LOG_DESC("binary method failed")
                                   << LOG_KV("error", static_cast<int>(error));
            setError(callResult, mapVendorError(error));
            return;
        }
        if (shouldLog(LOG_LEVEL_DEBUG))
        {
            PRECOMPILED_LOG(DEBUG) << LOG_BADGE("ChainAPIPrecompiled")
                                   << LOG_DESC("binary method done") << LOG_KV("output", output);
        }
        callResult->setExecResult(abi.abiIn("", output));
    };

    if (func == name2Selector[CHAIN_API_V_ENCRYPT_PUB_INT])
    {
        call1(&CallVendorHE::encryptPubInt);
        return callResult;
    }
    if (func == name2Selector[CHAIN_API_V_DECRYPT_INT])
    {
        call1(&CallVendorHE::decryptInt);
        return callResult;
    }
    if (func == name2Selector[CHAIN_API_V_ENCRYPT_PUB_DOUBLE])
    {
        call1(&CallVendorHE::encryptPubDouble);
        return callResult;
    }
    if (func == name2Selector[CHAIN_API_V_DECRYPT_DOUBLE])
    {
        call1(&CallVendorHE::decryptDouble);
        return callResult;
    }
    if (func == name2Selector[CHAIN_API_V_ENCRYPT_PUB_STRING])
    {
        call1(&CallVendorHE::encryptPubString);
        return callResult;
    }
    if (func == name2Selector[CHAIN_API_V_DECRYPT_STRING])
    {
        call1(&CallVendorHE::decryptString);
        return callResult;
    }

    if (func == name2Selector[CHAIN_API_V_ADD_INT])
    {
        call2(&CallVendorHE::addInt);
        return callResult;
    }
    if (func == name2Selector[CHAIN_API_V_SUBSTRACT_INT])
    {
        call2(&CallVendorHE::substractInt);
        return callResult;
    }
    if (func == name2Selector[CHAIN_API_V_ADD_DOUBLE])
    {
        call2(&CallVendorHE::addDouble);
        return callResult;
    }
    if (func == name2Selector[CHAIN_API_V_SUBSTRACT_DOUBLE])
    {
        call2(&CallVendorHE::substractDouble);
        return callResult;
    }
    if (func == name2Selector[CHAIN_API_V_MULTIPLY_DOUBLE])
    {
        call2(&CallVendorHE::multiplyDouble);
        return callResult;
    }
    if (func == name2Selector[CHAIN_API_V_DIVIDE_DOUBLE])
    {
        call2(&CallVendorHE::divideDouble);
        return callResult;
    }
    if (func == name2Selector[CHAIN_API_V_COMPARE_DOUBLE])
    {
        call2(&CallVendorHE::compareDouble);
        return callResult;
    }
    if (func == name2Selector[CHAIN_API_V_CONCAT_STRING])
    {
        call2(&CallVendorHE::concatString);
        return callResult;
    }

    if (func == name2Selector[CHAIN_API_V_SUBSTRING])
    {
        std::string source;
        std::string start;
        std::string end;
        std::string output;
        abi.abiOut(data, source, start, end);
        if (shouldLog(LOG_LEVEL_DEBUG))
        {
            PRECOMPILED_LOG(DEBUG) << LOG_BADGE("ChainAPIPrecompiled")
                                   << LOG_DESC("execute VSubstring")
                                   << LOG_KV("source", source)
                                   << LOG_KV("start", start)
                                   << LOG_KV("end", end);
        }
        auto error = m_vendor->substring(source, start, end, output);
        if (error != CallVendorHE::Err::OK)
        {
            setError(callResult, mapVendorError(error));
            return callResult;
        }
        callResult->setExecResult(abi.abiIn("", output));
        if (shouldLog(LOG_LEVEL_DEBUG))
        {
            PRECOMPILED_LOG(DEBUG) << LOG_BADGE("ChainAPIPrecompiled")
                                   << LOG_DESC("VSubstring done") << LOG_KV("output", output);
        }
        return callResult;
    }

    if (func == name2Selector[CHAIN_API_V_LENGTH])
    {
        call1(&CallVendorHE::length);
        return callResult;
    }

    PRECOMPILED_LOG(ERROR) << LOG_BADGE("ChainAPIPrecompiled") << LOG_DESC("unknown function")
                           << LOG_KV("selector", func);
    setError(callResult, CODE_UNKNOW_FUNCTION_CALL);
    return callResult;
}
