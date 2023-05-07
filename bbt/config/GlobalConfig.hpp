#pragma once
#include <map>
#include <unordered_map>
#include <vector>
#include <memory>
#include <bbt/template_util/noncopyable.hpp>
#include "bbt/config/Define.hpp"

namespace bbt::config
{

class GlobalConfig : bbt::noncopyable
{
/**
 * @brief 系统初始化开始时,先设置配置信息.系统开始初始化则根据GlobalConfig获取配置数据
 */
BBT_IMPL_CLASS DynamicConfig
{
public:
    // static DynamicConfig* GetInstance();
    template<typename Entry>
    Entry* GetEntry(const std::string& name);

    template<typename Entry>
    bool SetEntry(const std::string& name,Entry* config);
private:
    std::unordered_map<std::string,void*> m_config_entry;    
};

public:
    static GlobalConfig* GetInstance();
    GlobalConfig();
    std::unique_ptr<DynamicConfig>& GetDynamicCfg()
    { return m_dynamic_cfg; }
private:
    std::unique_ptr<DynamicConfig> m_dynamic_cfg;
};

#include "GlobalConfig_detail.hpp"
}