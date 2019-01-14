#include <type_traits>
#include <typeindex>
#include <cassert>

using stateid_t = std::type_index;
template<class T> inline stateid_t GetStateID() { 
  return typeid(typename std::remove_pointer<T>::type);
}

template<class T> inline stateid_t GetStateID(const T&){ 
  return GetStateID<T>();
}

template<class T> inline stateid_t GetStateID(T* t){ 
  return typeid(*t);
}

struct A{ virtual stateid_t GetID(){ return GetStateID(this);} };
struct X : public A{};
int main()
{
  A a;
  const A b;
  A& c = a;
  X x;
  A* y = &x;
  
  assert(GetStateID<A>() == GetStateID(a));
  assert(GetStateID<A>() == GetStateID<A&>());
  assert(GetStateID(a) == GetStateID(&a));
  assert(GetStateID(a) == GetStateID(b));
  assert(GetStateID(a) == GetStateID(c));
  assert(GetStateID<A>() != GetStateID<X>());
  assert(GetStateID(a) != GetStateID(x));
  assert(GetStateID(a) != GetStateID(y));
  assert(GetStateID(x) == GetStateID(y));
  assert(a.GetID() != x.GetID());
  return 0;
}
