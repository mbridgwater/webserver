#ifndef FILE_SYSTEM_INTERFACE_H
#define FILE_SYSTEM_INTERFACE_H

class FileSystemInterface {
public:
    virtual ~FileSystemInterface() = default;

    virtual std::pair<bool, std::string> create_entity(const std::string &name) = 0;
    virtual std::pair<bool, std::string> read_entity(const std::string &name, const std::string &id) const = 0;
    virtual bool write_entity(const std::string &name, const std::string &id, const std::string &data) = 0;
    virtual bool delete_entity(const std::string &name, const std::string &id) = 0;
    virtual std::pair<bool, std::vector<std::string>> list_entities(const std::string &name) const = 0;
    virtual const std::string& get_data_path() const = 0;
    virtual bool exists(const std::string& entity, const std::string& id) const = 0;
};

#endif