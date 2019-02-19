#include "dt_tag_entry.hpp"
#include <string>
#include <utility>

namespace net {
namespace coderodde {
namespace dt {

    TagEntry::TagEntry(std::string const& tag,
                       std::string const& directory)
        :
        m_tag(tag),
        m_directory(directory) {

    }

    TagEntry::TagEntry()
        :
        m_tag(""),
        m_directory("") {

    }

    TagEntry::TagEntry(TagEntry const& other)
        :
        m_tag(other.m_tag),
        m_directory(other.m_directory) {

    }

    TagEntry& TagEntry::operator=(TagEntry&& other) {
        m_tag = std::move(other.m_tag);
        m_directory = std::move(other.m_directory);
        return *this;
    }

    TagEntry& TagEntry::operator=(TagEntry const& other) {
        m_tag = other.m_tag;
        m_directory = other.m_directory;
        return *this;
    }

    std::string const& TagEntry::getTag() const {
        return m_tag;
    }

    std::string const& TagEntry::getDirectory() const {
        return m_directory;
    }

    void TagEntry::setDirectory(std::string& directory) {
        m_directory = directory;
    }

} // End of namespace 'net::coderodde::dt2'.
} // End of namespace 'net::coderodde'.
} // End of namespace 'net'.