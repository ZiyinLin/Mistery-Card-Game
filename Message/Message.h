#ifndef MESSAGE_H
#define MESSAGE_H
#include <string>

class Message{
public:
    Message(const std::string& type, const std::string& content)
     :message_type(type), message_content(content){}

    std::string getType();
    std::string getContent();

private:
    std::string message_type;
    std::string message_content;



};

#endif
