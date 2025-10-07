#include "file_system.h"

namespace fs = std::filesystem;
namespace uuids = boost::uuids;

FileSystem::FileSystem(const std::string& data_path)
    : data_path_(data_path) {
    fs::path root_directory(data_path_);
    if (!fs::exists(root_directory)) {
        fs::create_directories(root_directory);
    }
}

// Creates a new entity file inside a named entity directory and returns a generated UUID.
// If the directory does not exist, it is created.
std::pair<bool, std::string> FileSystem::create_entity(const std::string &name) {
    // Create entity directory if it doesn't exist
    fs::path directory = fs::path(data_path_) / name;
    if (!fs::exists(directory)) {
        fs::create_directories(directory);
    }

    // Generate UUID for entity and create the entity file
    uuids::uuid uuid = uuids::random_generator()();
    std::string id = to_string(uuid);
    fs::path file = directory / id;
    std::ofstream out(file);
    if (!out) {
        return {false, "File cannot be opened"};        
    }
    out.close();
    return {true, id};
}

// Reads the contents of a specific entity file given by name and id.
// Returns the file's content as a string if it exists and can be read.
std::pair<bool, std::string> FileSystem::read_entity(const std::string &name, const std::string &id) const {
    fs::path directory = fs::path(data_path_) / name;
    if (!fs::exists(directory)) {
        return {false, ""};
    }
    fs::path file = directory / id;
    if (!fs::exists(file)) {
        return {false, ""};
    }
    std::ifstream in(file);
    if (!in) {
        return {false, ""};
    }

    std::stringstream buffer;
    buffer << in.rdbuf();
    return {true, buffer.str()};
}

// Writes data to an existing entity file identified by name and id.
// Returns false if the directory or file doesn't exist, or if writing fails.
bool FileSystem::write_entity(const std::string &name, const std::string &id, const std::string &data) {
    // Check if entity directory already exists
    fs::path directory = fs::path(data_path_) / name;
    if (!fs::exists(directory)) {
        return false;
    }
    
    // Check if entity file with given ID already exists
    fs::path file = directory / id;
    if (!fs::exists(file)) {
        return false;
    }
    
    // Check if the file has opened to write the data inside
    std::ofstream out(file);
    if (!out) {
        return false;
    }
    out << data;
    out.close();
    return true;
}

// Implement delete_entity method
bool FileSystem::delete_entity(const std::string &name, const std::string &id) {
    fs::path directory = fs::path(data_path_) / name;
    if (!fs::exists(directory)) {
        return false;
    }
    
    fs::path file = directory / id;
    if (!fs::exists(file)) {
        return false;
    }
    
    return fs::remove(file);
}

// Lists all entity file IDs under a given entity type (directory name).
// Returns a list of filenames (IDs) found in the directory.
std::pair<bool, std::vector<std::string>> FileSystem::list_entities(const std::string &name) const {
    fs::path entity_dir = fs::path(data_path_) / name;
    std::vector<std::string> ids;

    if (!fs::exists(entity_dir) || !fs::is_directory(entity_dir)) {
        return {false, ids};
    }

    for (const auto& entry : fs::directory_iterator(entity_dir)) {
        if (entry.is_regular_file()) {
            ids.push_back(entry.path().filename().string());
        }
    }

    return {true, ids};
}

const std::string& FileSystem::get_data_path() const {
    return data_path_;
}

bool FileSystem::exists(const std::string& entity, const std::string& id) const {
    std::filesystem::path file_path = std::filesystem::path(data_path_) / entity / id;
    return std::filesystem::exists(file_path);
}
