#include <linux/limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pwd.h>
#include "dt_tag_entry.hpp"
#include "dt_tag_entry_list.hpp"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

const std::string FLAG_LIST_TAGS_AND_DIRECTORIES                = "-L";
const std::string FLAG_LIST_TAGS_NO_DIRECTORIES                 = "-l";
const std::string FLAG_LIST_TAGS_AND_DIRECTORIES_SORTED         = "-S";
const std::string FLAG_LIST_TAGS_NO_DIRECTORIES_SORTED          = "-s";
const std::string FLAG_LIST_TAGS_AND_DIRECTORIES_SORTED_BY_DIRS = "-d";
const std::string TAG_ENTRY_LIST_FILE_DIRECTORY                 = "/.dt";
const std::string TAG_ENTRY_LIST_FILE_NAME                      = "/tags";
const std::string TAG_LINE                                      = "tag";
const std::string OPERATION_DESCRIPTOR_SWITCH_DIRECTORY         = "switch_directory";
const std::string OPERATION_DESCRIPTOR_SHOW_TAG_ENTRY_LIST      = "show_tag_entry_list";
const std::string OPERATION_DESCRIPTOR_MESSAGE                  = "message";
const std::string LIST_LINE                                     = "list";
const size_t LINE_BUFFER_CAPACITY = 1024;

#define RETURN_VALUE_CD_SUCCESS 10
#define RETURN_VALUE_CD_FAILURE 11
#define RETURN_VALUE_WRONG_ARGUMENTS 12
#define RETURN_VALUE_NO_TAGS 13

////// https://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
        return !std::isspace(ch);
    }));
}

static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

inline void trim(std::string &s) {
    ltrim(s);
    rtrim(s);
}

using net::coderodde::dt::TagEntry;
using net::coderodde::dt::TagEntryList;

static void operator>>(std::ifstream& inputFileStream, 
                       TagEntryList& tagEntryList) {
    char lineBuffer[LINE_BUFFER_CAPACITY];

    while (!inputFileStream.eof() && !inputFileStream.bad() && !inputFileStream.fail()) {
        inputFileStream.getline(lineBuffer, LINE_BUFFER_CAPACITY);
        std::stringstream ss;
        std::string tag;
        std::string directory;
        ss << lineBuffer;
        ss >> tag;
        char directoryBuffer[PATH_MAX];
        ss.getline(directoryBuffer, PATH_MAX);
        directory = directoryBuffer;
        trim(directory);

        if (!tag.empty() && !directory.empty()) {
                TagEntry tagEntry(tag, directory);
                tagEntryList << tagEntry;
        }
    }
}

static size_t getMaximumTagLength(TagEntryList const& tagEntryList) {
    auto const& maximumLengthTagIter =
        std::max_element(tagEntryList.begin(),
                tagEntryList.end(),
                [](TagEntry const& tagEntry1,
                        TagEntry const& tagEntry2) {
        return tagEntry1.getTag().length() <
                tagEntry2.getTag().length();
    });

    return maximumLengthTagIter->getTag().length();
}

static void listTags(TagEntryList const& tagEntryList) {
    std::cout << OPERATION_DESCRIPTOR_SHOW_TAG_ENTRY_LIST
              << '\n';
    
    for (TagEntry const& tagEntry : tagEntryList) {
        std::cout << tagEntry.getTag() << "\n";
    }
}

static void listTagsAndDirectories(TagEntryList const& tagEntryList) {
    if (tagEntryList.empty()) {
        // getMaximumTagLength assumes that the tagEntryList
        // is not empty. (It would dereference end().
        std::cout << OPERATION_DESCRIPTOR_MESSAGE 
                  << "\nTag list is empty.\n";
        return;
    }

    size_t maximumTagLength = getMaximumTagLength(tagEntryList);
    std::cout << OPERATION_DESCRIPTOR_SHOW_TAG_ENTRY_LIST
              << '\n';
    
    for (TagEntry const& tagEntry : tagEntryList) {
        std::cout << std::setw(maximumTagLength) 
                  << std::left 
                  << tagEntry.getTag()
                  << " "
                  << tagEntry.getDirectory() 
                  << "\n";
    }
}

static std::string matchTag(TagEntryList const& tagEntryList, std::string const& tag) {
    try {
        TagEntry bestTagEntry = tagEntryList[tag];
        return bestTagEntry.getDirectory();
    }
    catch (std::runtime_error const& err) {
        std::exit(1);
    }
}

static std::string getUserHomeDirectoryName() {
    struct passwd* pw = getpwuid(getuid());
    std::string name{pw->pw_dir}; 
    return name;
}

static void createTagFile(char* fileName) {
    int fd = open(fileName, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    close(fd);
}

static TagEntryList loadTagFile(std::string const& tagFileName) {
    std::ifstream ifs;
    ifs.open(tagFileName.c_str(), std::ifstream::in);
    TagEntryList tagEntryList;
    ifs >> tagEntryList;
    ifs.close();
    return tagEntryList;
}

static void toggleDirectory(std::string const& tagFileName) {
    TagEntryList tagEntryList = loadTagFile(tagFileName);
    std::string path;
    
    try {
        path = tagEntryList["prev"].getDirectory();
    } catch (...) {
        std::cout << OPERATION_DESCRIPTOR_MESSAGE 
                  << "\nNo 'prev' tag in the tag file.\n";
        return;
    }
    
    std::cout << OPERATION_DESCRIPTOR_SWITCH_DIRECTORY
              << '\n'
              << path;
}

static std::string getTagFilePath() {
    std::string homeDirectoryName = getUserHomeDirectoryName();
    std::string directoryTaggerDirectory = 
            homeDirectoryName + TAG_ENTRY_LIST_FILE_DIRECTORY;
    std::string tagFilePath =
            directoryTaggerDirectory + TAG_ENTRY_LIST_FILE_NAME;
    return tagFilePath;
}

static std::string omitTilde(std::string& dir,
                             std::string const& homeDirectoryPath) {
    if (dir[0] == '~') {
        return homeDirectoryPath + dir.substr(1);
    } else {
        return dir;
    }
}

int main(int argc, char* argv[]) {
    if (argc > 3) {
        std::cout << OPERATION_DESCRIPTOR_MESSAGE
                  << "\nInvalid number of arguments: "
                  << argc - 1
                  << '\n';
        
        return EXIT_FAILURE;
    }
    
    std::string tagFilePath = getTagFilePath(); 
    createTagFile((char*) tagFilePath.c_str()); // Make sure the tag file exists.
    
    if (argc == 1) {
        toggleDirectory(tagFilePath);
        return EXIT_SUCCESS;
    }
    
    TagEntryList tagEntryList = loadTagFile(tagFilePath);
    
    if (argc == 3) {
        std::string flag = argv[1];
        std::string path = argv[2];

        if (std::string{"--update-prev"}.compare(flag) != 0) {
            std::cout << OPERATION_DESCRIPTOR_MESSAGE 
                      << " \n--update_prev expected, but "
                      << flag
                      << " was received.\n";
            
            return EXIT_FAILURE;
        }

        // Now rewrite the file.
        std::ofstream ofs;
        ofs.open(tagFilePath.c_str(), 
                 std::ofstream::out);
        
        bool prevTagIsPresent = false;
        
        for (auto iter = tagEntryList.begin();
            iter != tagEntryList.end();
            iter++) {
            TagEntry currentTagEntry = *iter;
            std::string tag = currentTagEntry.getTag();
            std::string dir = currentTagEntry.getDirectory();
            trim(dir);
            // Reset the string to the entry:
            currentTagEntry.setDirectory(dir);

            if (currentTagEntry.getTag().compare(std::string{"prev"}) == 0) {
                currentTagEntry.setDirectory(path);
                prevTagIsPresent = true;
            }
            
            /*if (currentTagEntry.getDirectory()[0] == '~') {
                std::string newDir = 
                        homedir + currentTagEntry.getDirectory().substr(1);
                std::cerr << newDir << " heah\n";
                currentTagEntry.setDirectory(newDir);
                std::exit(23);
            }*/
            
            ofs << currentTagEntry.getTag() 
                << " "
                << currentTagEntry.getDirectory()
                << '\n';
//            if (!currentTagEntry.getTag().empty()) {
//                std::string aux = getUserHomeDirectoryName();
//                aux = currentTagEntry.getDirectory()[0] == '~' ?
//                    aux :
//                    currentTagEntry.getDirectory();
//                ofs << currentTagEntry.getTag()
//                    << " "
//                    << currentTagEntry.getDirectory()
//                    << "\n";
//            }
        }
        
        if (!prevTagIsPresent) {
            tagEntryList << TagEntry("prev", path);
            std::cout << OPERATION_DESCRIPTOR_MESSAGE 
                      << "\nNo 'prev' tag.\n";
        }

        ofs.close();
        return 0;
    } 

    std::string flag{argv[1]};
    
    if (flag == FLAG_LIST_TAGS_AND_DIRECTORIES_SORTED
            || flag == FLAG_LIST_TAGS_NO_DIRECTORIES_SORTED) {
        tagEntryList.sortByTags();
    }
    else if (flag == FLAG_LIST_TAGS_AND_DIRECTORIES_SORTED_BY_DIRS) {
        tagEntryList.sortByDirectories();
    }

    if (flag == FLAG_LIST_TAGS_AND_DIRECTORIES
            || flag == FLAG_LIST_TAGS_AND_DIRECTORIES_SORTED
            || flag == FLAG_LIST_TAGS_AND_DIRECTORIES_SORTED_BY_DIRS) {
        listTagsAndDirectories(tagEntryList);
    }
    else if (flag == FLAG_LIST_TAGS_NO_DIRECTORIES
            || flag == FLAG_LIST_TAGS_NO_DIRECTORIES_SORTED) {
        listTags(tagEntryList);
    }
    else {
        std::string targetDirectory = tagEntryList[argv[1]].getDirectory();
        targetDirectory = 
                targetDirectory[0] == '~' ?
                    getUserHomeDirectoryName() + targetDirectory.substr(1) :
                    targetDirectory;
        
        std::cout << OPERATION_DESCRIPTOR_SWITCH_DIRECTORY
                  << '\n'
                  << targetDirectory;
    }
    
    return RETURN_VALUE_CD_SUCCESS;
}