/* This file contains the header for the YAML to JSON converter */
/*It's a part of the SLN2Code project*/
/*Created by Macintosh-MaiSensei on 2025/12/20*/
#pragma once
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <unordered_map>
#include <stack>
#include <set>
#include <functional>
#include <yaml-cpp/yaml.h>
#include <json.hpp>

namespace yaml2json {
    
    class YamlToJsonConverter {
    private:
        struct ConversionOptions {
            bool allowInfNan = false;
            bool preserveTags = false;
            bool strictMode = true;
            int maxDepth = 1000;
            bool convertBinary = true;
        };
        
        struct ConversionState {
            std::set<const void*> visitedNodes;
            int currentDepth = 0;
        };
        
        ConversionOptions options;
        std::unordered_map<std::string, nlohmann::json> anchors;
        
        nlohmann::json handleBinary(const std::string& data) {
            if (!options.convertBinary) {
                return data;
            }
            return nlohmann::json::binary(
                std::vector<std::uint8_t>(data.begin(), data.end())
            );
        }
        
        bool isOctalLiteral(const std::string& str) {
            return str.size() > 2 && str[0] == '0' && (str[1] == 'o' || str[1] == 'O');
        }
        
        bool isHexLiteral(const std::string& str) {
            return str.size() > 2 && str[0] == '0' && (str[1] == 'x' || str[1] == 'X');
        }
        
        bool isBinaryLiteral(const std::string& str) {
            return str.size() > 2 && str[0] == '0' && (str[1] == 'b' || str[1] == 'B');
        }
        
        nlohmann::json convertScalar(const YAML::Node& node) {
            std::string scalar = node.Scalar();
            
            if (scalar.empty()) {
                return "";
            }
            
            if (scalar == "true" || scalar == "false") {
                return scalar == "true";
            }
            
            if (scalar == "null" || scalar == "Null" || scalar == "NULL" || scalar == "~") {
                return nullptr;
            }
            
            if (node.Tag() == "tag:yaml.org,2002:binary") {
                return handleBinary(scalar);
            }
            
            if (scalar == ".inf" || scalar == ".Inf" || scalar == ".INF" || 
                scalar == "+.inf" || scalar == "+.Inf" || scalar == "+.INF") {
                if (options.allowInfNan) {
                    return std::numeric_limits<double>::infinity();
                }
                return scalar;
            }
            
            if (scalar == "-.inf" || scalar == "-.Inf" || scalar == "-.INF") {
                if (options.allowInfNan) {
                    return -std::numeric_limits<double>::infinity();
                }
                return scalar;
            }
            
            if (scalar == ".nan" || scalar == ".NaN" || scalar == ".NAN") {
                if (options.allowInfNan) {
                    return std::numeric_limits<double>::quiet_NaN();
                }
                return scalar;
            }
            
            if (node.Tag() == "tag:yaml.org,2002:str" || node.Tag() == "!") {
                if (scalar.size() >= 2 && scalar.front() == '"' && scalar.back() == '"') {
                    return scalar.substr(1, scalar.size() - 2);
                }
                if (scalar.size() >= 2 && scalar.front() == '\'' && scalar.back() == '\'') {
                    return scalar.substr(1, scalar.size() - 2);
                }
                return scalar;
            }
            
            if (node.Tag() == "tag:yaml.org,2002:int") {
                try {
                        if (isHexLiteral(scalar)) {
                            return std::stoll(scalar.substr(2), nullptr, 16);
                        }
                        if (isOctalLiteral(scalar)) {
                            return std::stoll(scalar.substr(2), nullptr, 8);
                        }
                        if (isBinaryLiteral(scalar)) {
                            return std::stoll(scalar.substr(2), nullptr, 2);
                        }
                        if (scalar.find(':') != std::string::npos) {
                            return scalar;
                        }
                        
                        long long val = std::stoll(scalar);
                        if (val >= std::numeric_limits<int32_t>::min() && 
                            val <= std::numeric_limits<int32_t>::max()) {
                            return static_cast<int32_t>(val);
                        }
                        return val;
                } catch (...) {
                    return scalar;
                }
            }
            
            if (node.Tag() == "tag:yaml.org,2002:float") {
                try {
                    double val = std::stod(scalar);
                    if (options.strictMode && !std::isfinite(val)) {
                        return scalar;
                    }
                    return val;
                } catch (...) {
                    return scalar;
                }
            }
            
            if (node.Tag() == "tag:yaml.org,2002:bool") {
                return scalar == "true" || scalar == "True" || scalar == "TRUE";
            }
            
            if (node.Tag() == "tag:yaml.org,2002:null") {
                return nullptr;
            }
            
            if (scalar.find_first_of(".:eE") != std::string::npos) {
                try {
                    double val = std::stod(scalar);
                    if (!std::isfinite(val)) {
                        if (options.allowInfNan) {
                            return val;
                        }
                        return scalar;
                    }
                    if (scalar.find('.') != std::string::npos || 
                        scalar.find('e') != std::string::npos || 
                        scalar.find('E') != std::string::npos) {
                        return val;
                    }
                } catch (...) {
                }
            }
            
            try {
                if (scalar.find_first_not_of("0123456789-") == std::string::npos) {
                    if (scalar.size() > 1 && scalar[0] == '0' && scalar.size() <= 20) {
                        return scalar;
                    }
                    long long val = std::stoll(scalar);
                    if (scalar == std::to_string(val)) {
                        if (val >= std::numeric_limits<int32_t>::min() && 
                            val <= std::numeric_limits<int32_t>::max()) {
                            return static_cast<int32_t>(val);
                        }
                        return val;
                    }
                }
            } catch (...) {
            }
            
            return scalar;
        }
        
        nlohmann::json convertSequence(ConversionState& state, const YAML::Node& node) {
            if (state.currentDepth >= options.maxDepth) {
                throw std::runtime_error("Maximum recursion depth exceeded");
            }
            
            state.currentDepth++;
            nlohmann::json result = nlohmann::json::array();
            for (const auto& child : node) {
                result.push_back(convertNode(state, child));
            }
            state.currentDepth--;
            return result;
        }
        
        nlohmann::json convertMap(ConversionState& state, const YAML::Node& node) {
            if (state.currentDepth >= options.maxDepth) {
                throw std::runtime_error("Maximum recursion depth exceeded");
            }
            
            state.currentDepth++;
            nlohmann::json result = nlohmann::json::object();
            
            for (const auto& kv : node) {
                std::string key = kv.first.as<std::string>();
                if (key.empty()) {
                    key = "";
                }
                result[key] = convertNode(state, kv.second);
            }
            
            state.currentDepth--;
            return result;
        }
        
        nlohmann::json convertNode(ConversionState& state, const YAML::Node& node) {
            if (!node.IsDefined() || node.IsNull()) {
                return nullptr;
            }
            
            if (node.IsScalar()) {
                return convertScalar(node);
            }
            
            if (node.IsSequence()) {
                return convertSequence(state, node);
            }
            
            if (node.IsMap()) {
                return convertMap(state, node);
            }
            
            return nullptr;
        }
        
    public:
        YamlToJsonConverter() = default;
        
        void setAllowInfNan(bool allow) { options.allowInfNan = allow; }
        void setPreserveTags(bool preserve) { options.preserveTags = preserve; }
        void setStrictMode(bool strict) { options.strictMode = strict; }
        void setMaxDepth(int depth) { options.maxDepth = depth; }
        void setConvertBinary(bool convert) { options.convertBinary = convert; }
        
        nlohmann::json convert(const YAML::Node& yaml) {
            ConversionState state;
            anchors.clear();
            return convertNode(state, yaml);
        }
        
        nlohmann::json parseString(const std::string& yamlStr) {
            try {
                YAML::Node yaml = YAML::Load(yamlStr);
                return convert(yaml);
            } catch (const YAML::Exception& e) {
                throw std::runtime_error(std::string("YAML parsing failed: ") + e.what());
            }
        }
        
        nlohmann::json parseFile(const std::string& filename) {
            try {
                YAML::Node yaml = YAML::LoadFile(filename);
                return convert(yaml);
            } catch (const YAML::Exception& e) {
                throw std::runtime_error(std::string("YAML file loading failed: ") + e.what());
            }
        }
        
        std::string toJsonString(const nlohmann::json& json, int indent = -1, bool escapeUnicode = false) {
            if (indent >= 0) {
                return json.dump(indent, ' ', escapeUnicode);
            }
            return json.dump(-1, ' ', escapeUnicode);
        }
        
        std::string toJsonStringFormatted(const nlohmann::json& json, int indent = 2) {
            return json.dump(indent);
        }
    };
    
    inline nlohmann::json convert(const YAML::Node& yaml, 
                                   bool allowInfNan = false, 
                                   bool strictMode = true,
                                   int maxDepth = 1000) {
        YamlToJsonConverter converter;
        converter.setAllowInfNan(allowInfNan);
        converter.setStrictMode(strictMode);
        converter.setMaxDepth(maxDepth);
        return converter.convert(yaml);
    }
    
    inline nlohmann::json convertString(const std::string& yamlStr, 
                                         bool allowInfNan = false, 
                                         bool strictMode = true,
                                         int maxDepth = 1000) {
        YamlToJsonConverter converter;
        converter.setAllowInfNan(allowInfNan);
        converter.setStrictMode(strictMode);
        converter.setMaxDepth(maxDepth);
        return converter.parseString(yamlStr);
    }
    
    inline nlohmann::json convertFile(const std::string& filename, 
                                       bool allowInfNan = false, 
                                       bool strictMode = true,
                                       int maxDepth = 1000) {
        YamlToJsonConverter converter;
        converter.setAllowInfNan(allowInfNan);
        converter.setStrictMode(strictMode);
        converter.setMaxDepth(maxDepth);
        return converter.parseFile(filename);
    }
    
    inline std::string toJsonString(const YAML::Node& yaml, 
                                     int indent = -1, 
                                     bool allowInfNan = false, 
                                     bool strictMode = true,
                                     bool escapeUnicode = false) {
        YamlToJsonConverter converter;
        converter.setAllowInfNan(allowInfNan);
        converter.setStrictMode(strictMode);
        nlohmann::json json = converter.convert(yaml);
        return converter.toJsonString(json, indent, escapeUnicode);
    }
}
