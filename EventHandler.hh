#ifndef EVENTHANDLER_h
#define EVNETHANDLER_h

#include <functional>
#include "define.hh"
#include "State.hh"

namespace fsm{
  struct Message;
  
  using EventHandler = std::function<stateid_t(VState*, const Message&)>;
    
  namespae eh{
    ///Most generic event handling function; first arg needed for memfuncs
    
    using _RawEH     = stateid_t(*)(VState*, const Message&);
    using _Arg2Func  = stateid_t(*)(const Message&);
    using _NoArgFunc = stateid_t(*)();
    template<class T> using _EHMemFn = stateid_t(T::*)(VState*, const Message&);
    template<class T> using _Arg2MemFn = stateid_t(T::*)(const Message&);
    template<class T> using _NoArgMemFn = stateid_t(T::*)();
  
    template<class Func> struct _WrappedEHandler{
      Func func;
      inline stateid_t operator()(VState*st, const Message&m){
	return func(st, m);
      }
    };
  
    template<> struct _WrappedEHandler<_Arg2Func>{
      _Arg2Func func;
      inline stateid_t operator()(VState*,const Message& m){
	return func(m);
      }
    };

    template<> struct _WrappedEHandler<_NoArgFunc>{
      _NoArgFunc func;
      inline stateid_t operator()(VState*,const Message& m){
	return func();
      }
    };


    //stupid workaround for lack of partial function template specialization
    template<class T, bool> struct _getstateobj{};
    template<class T> struct _getstateobj<T,true>{
      static inline T* _(VState* st)
      { return static_cast<T*>(st); }
    };
    template<class T> struct _getstateobj<T, false>{
      static inline T* _(VState* st)
      { return &static_cast<TState<T>*>(st)->GetStateObj(); }
    };
  
    template<class T> struct _WrappedEHandler<_NoArgMemFn<T>>{
      _NoArgMemFn<T> func;
      inline stateid_t operator()(VState* st, const Message&){
	T* t = _getstateobj<T, std::is_base_of<VState,T>::value>::_(st);
	return (t->*func)();
      }
    };
  
    template<class T> struct _WrappedEHandler<_EHMemFn<T>>{
      _EHMemFn<T> func;
      inline stateid_t operator()(VState* st, const Message& m){
	T* t = _getstateobj<T, std::is_base_of<VState,T>::value>::_(st);
	return (t->*func)(st, m);
      }
    };
  
    template<class T> struct _WrappedEHandler<_Arg2MemFn<T>>{
      _Arg2MemFn<T> func;
      inline stateid_t operator()(VState* st, const Message& m){
	T* t = _getstateobj<T, std::is_base_of<VState,T>::value>::_(st);
	return (t->*func)(m);
      }
    };
  
  
    template<class T> inline EventHandler MakeEventHandler(T t)
    {
      return _WrappedEHandler<T>{t};
    }
    
    template<> inline EventHandler MakeEventHandler<_RawEH>(_RawEH eh)
    {
      return eh;
    }
    template<> inline EventHandler MakeEventHandler<EventHandler>(EventHandler eh)
    {
      return eh;
    }
    
  };
  
};


#endif
