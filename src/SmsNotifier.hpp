#pragma once

#include <string>


namespace antifurto {

/// This class sends SMS by using a web service
class SmsNotifier
{
public:
    void send(std::string const& msg, std::string const& dest);
};

} // namespace antifurto
