#if !defined DOCUMENT_H
#define      DOCUMENT_H

#include <string>
#include <iosfwd>

#include "exception.h"
#include "rapidxml/rapidxml.hpp"

// An XML document, owning the text
class Document {
public:
  Document() : buffer(0) {}
  ~Document() {
    delete [] buffer;
  }

  void read(const std::string & filename);
  void read(std::istream & in);

  const rapidxml::xml_document<> & getTop() const { return doc; }

private:
  rapidxml::xml_document<> doc;
  char * buffer;
};

class DocumentError : public Exception {
public:
  DocumentError(const std::string & msg)
    : Exception("Error parsing XML: " + msg) {}
};

#endif
