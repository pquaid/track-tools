#include "document.h"

#include <iostream>
#include <string>

#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "util.h"

using namespace rapidxml;
using namespace std;

void Document::read(istream& in) {
  // I don't know how large the stream is, so I need to read it
  // into a growable buffer (eg a string), then copy it into a
  // modifiable char buffer
  string text;

  while (in.good() && !in.eof()) {
    char buffer[10000];

    in.read(buffer, sizeof(buffer));
    text.append(buffer, in.gcount());
  }

  buffer.reset(new char[text.size()+1]);
  memcpy(buffer.get(), text.data(), text.size());
  buffer[text.size()] = 0;

  doc.parse<0>(buffer.get());
}

void Document::read(const std::string& filename) {
  Descriptor fd(::open(filename.c_str(), O_RDONLY));
  SystemException::check(fd.get(), "Opening " + filename);

  struct stat stats;
  int rc = ::fstat(fd.get(), &stats);
  SystemException::check(rc, "Getting statistics");

  buffer.reset(new char[stats.st_size + 1]);
  rc = ::read(fd.get(), buffer.get(), stats.st_size);
  SystemException::check(rc, "Reading");
  buffer[stats.st_size] = 0;

  doc.parse<0>(buffer.get());
}
