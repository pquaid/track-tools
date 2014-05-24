#include "document.h"

#include <string>
#include <iostream>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include "util.h"

using namespace std;
using namespace rapidxml;

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

  buffer = new char[text.size()+1];
  memcpy(buffer, text.data(), text.size());
  buffer[text.size()] = 0;

  doc.parse<0>(buffer);
}

void Document::read(const std::string& filename) {
  Descriptor fd(::open(filename.c_str(), O_RDONLY));
  SystemException::check(fd.get(), "Opening " + filename);

  struct stat stats;
  int rc = ::fstat(fd.get(), &stats);
  SystemException::check(rc, "Getting statistics");

  buffer = new char[stats.st_size + 1];
  rc = ::read(fd.get(), buffer, stats.st_size);
  SystemException::check(rc, "Reading");
  buffer[stats.st_size] = 0;

  doc.parse<0>(buffer);
}
