#include "dt_tag_entry.hpp"
#include "dt_tag_entry_list.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>

using net::coderodde::dt::TagEntry;

static size_t computeEditDistance(std::string const& string1,
                                  std::string const& string2,
                                  size_t index1,
                                  size_t index2) {
    if (index1 == 0) {
        return index2;
    } else if (index2 == 0) {
        return index1;
    }

    return std::min(
            std::min(
                computeEditDistance(string1, string2, index1 - 1, index2) + 1,
                computeEditDistance(string1, string2, index1, index2 - 1) + 1
            ),
                computeEditDistance(string1, 
                                    string2, 
                                    index1 - 1, 
                                    index2 - 1) +
                (string1[index1 - 1] != string2[index2 - 1] ? 1 : 0)
    );
}

static size_t computeEditDistance(std::string const& string1, std::string const& string2) {
    return computeEditDistance(string1,
                               string2,
                               string1.length(),
                               string2.length());
}

namespace net {
namespace coderodde {
namespace dt {

    void TagEntryList::operator<<(TagEntry const& tagEntry) {
        m_entries.push_back(tagEntry);
    }

    TagEntry TagEntryList::operator[](std::string const& tag) const {
        if (m_entries.empty()) {
            throw std::runtime_error("No entries available.");
        }

        TagEntry bestTagEntry;
        size_t bestEditDistance = std::numeric_limits<size_t>::max();

        for (TagEntry const& tagEntry : m_entries) {
            size_t currentEditDistance = computeEditDistance(tag, tagEntry.getTag());

            if (bestEditDistance > currentEditDistance) {
                bestEditDistance = currentEditDistance;
                bestTagEntry = tagEntry;
            }
        }

        return bestTagEntry;
    }

    std::size_t TagEntryList::size() const {
        return m_entries.size();
    }

    std::vector<TagEntry>::const_iterator TagEntryList::begin() const {
        return m_entries.cbegin();
    }

    std::vector<TagEntry>::const_iterator TagEntryList::end() const {
        return m_entries.cend();
    }

    void TagEntryList::sortByTags() {
        std::stable_sort(m_entries.begin(),
            m_entries.end(),
            [](TagEntry const& tagEntry1,
               TagEntry const& tagEntry2) {
            return tagEntry1.getTag() < tagEntry2.getTag();
        });
    }

    void TagEntryList::sortByDirectories() {
        std::stable_sort(m_entries.begin(),
            m_entries.end(),
            [](TagEntry const& tagEntry1,
               TagEntry const& tagEntry2) {
            return tagEntry1.getDirectory() < tagEntry2.getDirectory();
        });
    }

    bool TagEntryList::empty() const {
        return m_entries.empty();
    }

} // End of namespace 'net::coderodde::dt2'.
} // End of namespace 'net::coderodde'.
} // End of namespace 'net'.