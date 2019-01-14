#include <string>
#include <vector>
#include <algorithm>
#include "define.hh"

namespace fsm{

  class Message{
  public:

    ///Constructor with event label only
    Message(const event_t& evt) : event(evt) {}
    
    ///Constructor with size of owned data
    Message(const event_t& evt, size_t bufsize) : 
      event(evt), _datasize(bufsize), _owned_data(bufsize){}
    
    ///Constructor pointing to a remote block of data
    Message(const event_t& evt, void* data, size_t datasize, bool copy=false) :
      event(evt), _data(data), _datasize(datasize){
      if(copy){
	_owned_data.resize(datasize);
	std::copy_n((char*)data, datasize, _owned_data.begin());
	_data = nullptr;
      }
    }
    
    ///event type identifier
    event_t event;
    
    ///pointer to the data location
    void* GetData()
    { return _owned_data.empty() ? _data : _owned_data.data(); }

    ///const access to the data pointer
    const void* GetData() const 
    { return _owned_data.empty() ? _data : _owned_data.data(); }

    ///get the size of pointed data
    size_t GetDataSize() const { return _datasize; }

    ///format the data as a string. May not be a valid c-string though!
    const char* GetDataString() const 
    { return _data ? (const char*)GetData() : ""; }

  private:
    void* _data = nullptr;
    size_t _datasize = 0;
    std::vector<char> _owned_data;
  };

};

