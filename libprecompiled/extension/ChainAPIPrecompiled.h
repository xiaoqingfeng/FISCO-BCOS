#pragma once

#include "CallVendorHE.h"
#include <libblockverifier/ExecutiveContext.h>
#include <libprecompiled/Common.h>
#include <memory>
#include <string>

namespace dev
{
namespace precompiled
{
class ChainAPIPrecompiled : public dev::precompiled::Precompiled
{
public:
    typedef std::shared_ptr<ChainAPIPrecompiled> Ptr;
    ChainAPIPrecompiled();
    virtual ~ChainAPIPrecompiled(){};

    PrecompiledExecResult::Ptr call(std::shared_ptr<dev::blockverifier::ExecutiveContext> context,
        bytesConstRef param, Address const& origin = Address(),
        Address const& sender = Address()) override;

private:
    std::shared_ptr<CallVendorHE> m_vendor;
    std::string m_version = "ChainAPI-HE-1.0";
};

}  // namespace precompiled
}  // namespace dev
