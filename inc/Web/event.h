#ifndef EVENT_H
#define EVENT_H

#include <stdint.h>

class Event {
 public:
  Event() {}
  virtual ~Event() {}

  virtual void OnEvent(uint32_t events) = 0;
  virtual int getFd() const = 0;
};

#endif
