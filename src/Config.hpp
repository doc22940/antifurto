#pragma once

#include "meta/SmartPtr.hpp"

#include <string>
#include <vector>

namespace antifurto {

/// This class represents the configuration for antifurto
class Configuration
{
public:
    using StringList = std::vector<std::string>;
    struct Whatsapp {
        std::string src;
        std::string pwd;
        StringList destinations;
    };
    struct Dropbox {
        std::string appKey;
        std::string appSecret;
        std::string oauthToken;
        std::string oauthTokenSecret;
    };

    Whatsapp whatsapp;
    Dropbox dropbox;
};


// fwd reference
class ConfigurationParserImpl;

/// This class fills a Configuration by parsing the command line and a
/// configuration file.
///
/// Note: call parseCmdLine and/or parseConfigFile in order of priority; in case
/// of conflicting options, the first is considered.
class ConfigurationParser
{
public:
    ConfigurationParser();

    void parseCmdLine(int argc, char* argv[]);
    void parseConfigFile(const char* configFile);
    Configuration getConfiguration();

private:
    meta::ErasedUniquePtr<ConfigurationParserImpl> impl_;
};

} // namespace antifurto
