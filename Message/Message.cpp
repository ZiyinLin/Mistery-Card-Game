#include "Message.h"
#include <string>
using namespace std;

class Message{
public:
    string getType() const {
        return message_type;
    }

    string getContent() const {
        return message_content;
    }

private:
    string message_type;
    string message_content;



};