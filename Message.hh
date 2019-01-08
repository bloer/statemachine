#include <string>
#include "define.hh"

namespace fsm{

  struct Message{
    event_t event;
    size_t datasize = 0;
    const char* data = 0;
    
    Message(event_t evt) : event(evt) {}
  };
  
  struct StringMessage : public Message{
    std::string stringdata;
    
    StringMessage(event_t evt, const std::string& msg="") : Message(evt)
    {
      SetMessageString(msg);
    }
    
    void SetMessageString(const std::string& msg){
      data = &msg[0];
      datasize = msg.size() + 1;
    }
    
  };
}

