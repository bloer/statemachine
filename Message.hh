#ifndef MESSAGE_h
#define MESSAGE_h

#include <string>
#include <vector>
#include <algorithm>
#include "define.hh"

namespace fsm{

  class Message{
  public:
    ///Constructor with event label only
    Message(const event_t& evt) : event(evt) {}
    
    ///Constructor with a single string arg
    Message(const event_t& evt, const std::string& arg1)
    { AddArg(arg1); }
    
    ///Constructor with data
    Message(const event_t& evt, void* data, size_t datasize)
    { AddDataArg(data, datasize); }
    
    ///event type identifier
    event_t event;
    
    ///Get all of the arguments
    const std::vector<std::string>& GetArgs() const { return _args; }
    
    ///convert the message to printable format
    const std::string& ToString() const; 

    ///build the message by ostream
    template<class T> Message& operator<<(const T& t)
    { _ss<<t; return *this; }
    
    ///Use Message::SEP to end the argument
    enum SEP_ { SEP };
    Message& operator<<(SEP_){ AddArg(_ss.str()); _ss.str(""); return *this; }
    
    ///Add an argument
    void AddArg(const std::string& s){ _args.push_back(s); }
    void AddArg(std::string&& s){ _args.push_back(std::move(s)); }

    ///add some binary data
    void AddDataArg(void* data, size_t size)
    { _args.push_back(std::string((char*)data, size)); }
    
    
    ///Set a named parameter
    void SetParameter(const std::string& key, const std::string& val)
    { _params[key] = val; }
    
    ///set all parameters
    void SetParameters(const std::map<std::string, std::string>& par)
    { _params = par; }
    void SetParameters(std::map<std::string, std::string>&& par)
    { _params = std::move(par); }
    
    ///See if we have a parameter
    bool HasParameter(const std::string& key) const{ return _params.count(key);}

    ///Get a parameter
    const std::string& GetParameter(const std::string& key) const 
    {
      const auto iter = _params.find(key);
      if(iter == _params.end()){
        static const std::string s;
        return s;
      }
      return iter->second; 
    }
    
  private:
    std::vector<std::string> _args;
    std::map<std::string, std::string> _params;
    std::stringstream _ss;

  public:
    Message& operator=(const Message& right){
      event = right.event;
      _args = right._args;
      _params = right._params;
      _ss.str(right._ss.str());
      return *this;
    }

    Message(const Message& right){
      *this = right;
    }
  };
};

inline std::ostream& operator<<(std::ostream& os, const fsm::Message& msg)
{ return os << msg.ToString(); }

#endif
