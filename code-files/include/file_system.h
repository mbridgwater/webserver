#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include <fstream>
#include <sstream>
#include <filesystem>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "file_system_interface.h"

class FileSystem : public FileSystemInterface {
public:
    FileSystem(const std::string& data_path);

    // CRUD operations on entity
    std::pair<bool, std::string> create_entity(const std::string &name) override;

    std::pair<bool, std::string> read_entity(const std::string &name, const std::string &id) const override; 
    
    bool write_entity(const std::string &name, const std::string &id, const std::string &data) override;
    
    bool delete_entity(const std::string &name, const std::string &id) override;
    
    std::pair<bool, std::vector<std::string>> list_entities(const std::string &name) const override;

    bool exists(const std::string& entity, const std::string& id) const override;

    // Getter for data_path_
    const std::string& get_data_path() const override;

private:
    std::string data_path_;
};

#endif