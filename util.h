#if !defined UTIL_H
#define      UTIL_H

#include <unistd.h>
#include <string>

// Make a class un-copyable by inheriting from this
class NoCopy {
 public:
  NoCopy() {}
 private:
  NoCopy(const NoCopy&);
  NoCopy& operator=(const NoCopy&);
};

// Wrap a descriptor, like a socket or file
class Descriptor {
 public:
  Descriptor(int d = -1) : descriptor(d) {}
  ~Descriptor() { close(); }

  int release() {
    int r = descriptor;
    descriptor = -1;
    return r;
  }

  int get() const { return descriptor; }

  void set(int d) {
    close();
    descriptor = d;
  }

  void close() {
    if (descriptor != -1) ::close(descriptor);
    descriptor = -1;
  }

 private:
  int descriptor;
};

class Util {
 public:
  // Does 'str' end with 'suffix'?
  static bool endsWith(const std::string & str, const std::string & suffix);

  // Some simple conversions
  static double meters_to_miles(double meters);
  static double meters_to_feet(double meters);

  // Return the shortest reasonable string for the time,
  // something like: [n days, [HH:[MM:[SS]]]]
  static std::string asTime(time_t seconds);
};

#endif
