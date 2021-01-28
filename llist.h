#ifndef LLIST_H
#define LLIST_H

#include "lockfree.h"

template<typename T>
class LList {
public:
  class Item {
    friend class LList;
  public:
    Item(T &i):item(i){}
    T item;
  private:
    Item *next = 0;
  };

  LList() {}
  virtual ~LList(){
    clear();
  }

  LList(const LList &other) {copy(other);}
  LList(LList &&other) {copy(other);}

  void clear() {
    while(_root) {
      Item *tmp = _root->next;
      delete _root;
      _root = tmp;
    }
    m_count = 0;
  }

  void append(T item) {
    if(_root) {
      _end->next = new Item(item);
      _end = _end->next;
    } else {
      _root = new Item(item);
      _end = _root;
    }
    m_count++;
  }

  T get(int index) {
    if(m_count == 0) return 0;
    if(index > m_count-1) return 0;
    Item *tmp = _root;
    while(index--) {
        tmp = tmp->next;
        if(tmp == 0) return tmp->item;
      }
    return tmp->item;
  }

  int exist(T it) {
    if(m_count == 0) return 0;
    Item *tmp = _root;
    while(tmp) {
        if(tmp->item == it) return 1;
        tmp = tmp->next;
      }
    return 0;
  }

  void remove(int index) {
    if(index >= m_count) return;
    Item *tmp = _root;
    if(index == 0) {
      tmp = _root->next;
      delete _root;
      _root = tmp;
      m_count--;
      if(m_count == 0)
        _end = 0;
      return;
    }
    for(int i=0; i<index-1; i++, tmp = tmp->next);
    if(index == m_count-1) {
      _end = tmp;
      delete _end->next;
      _end->next = 0;
      m_count--;
      return;
    }
    Item *p = tmp->next;
    tmp->next = tmp->next->next;
    delete p;
    return;
  }

  void remove(T item) {
    if(m_count == 0) return;
    Item *tmp = _root;
    uint32_t idx = 0;
    while(tmp) {
      if(tmp->item == item) break;
      idx++;
      tmp = tmp->next;
    }
    if(tmp) remove(idx);
  }

  int count() {
    return m_count;
  }

  Item &first() {
    return *_root;
  }

  /*------------------------------------------ITERATOR---------------------------------*/
  class iterator
  {
  public:
    iterator(const LList<T>& c, LList<T>::Item *idx)
      : m_container(c), i(idx)
    {}
    // Required
    bool operator!=(const iterator& other)
    {
      if(i == 0) return 0;
      return (i != other.i->next);
    }
    // Required
    const iterator& operator++()
    {
      i = i->next;
      return *this;
    }
    // Required
    Item& operator*() const
    {
      return *i;
    }
  private:
    const LList<T>& m_container;
    LList<T>::Item* i;
  };

  LList<T>::iterator begin() noexcept
  { return iterator(*this, _root); }

  LList<T>::iterator end() noexcept
  { return iterator(*this, _end); }
  /******************************************************************************************/

  void lock()
  {
    m_lock.lock();
  }

  void free()
  {
    m_lock.free();
  }
private:
  void copy(LList &other) {
    Item *tmp = other._root;
    while(tmp) {
      append(tmp->item);
      tmp = tmp->next;
    }
  }
  LockFree m_lock;
  Item *_root = 0;
  Item *_end = 0;
  int m_count = 0;
};

#endif // LLIST_H
