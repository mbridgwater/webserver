## Brief Overview
This web server dynamically dispatches HTTP requests to custom handlers based on path-matching rules defined in a configuration file. The server loads configuration at startup, initializes a route-to-handler registry, and spawns new request handlers per incoming request.

## Source Code Layout 
#### `build/`
Generated directory for compiled binaries and CMake artifacts. Not tracked in version control. 

#### `config/`
Houses configuration files, including the Nginx-style server config that specifies handler routes and server settings like port. 

#### `docker/`
Contains Dockerfile and related scripts for containerizing and running the server in a reproducible environment. 

#### `include/`
Header files for server components like request handlers, dispatcher, logger, session, server, config parsing, and utility classes.

#### `src/`
Source code implementation for core server logic, request handlers, and factory registration mechanisms. 

#### `tests/`
Unit and integration tests to validate handler logic, server responses, and routing behavior. 

#### `CMakeLists.txt/`
Build configuration file defining how source files, include paths, and tests are compiled and linked. 

#### `CodeCoverageReportConfig.cmake/`
Optional CMake script used to generate code coverage reports via tools like `gcov` or `lcov`. 

## How to Build, Test, & Run the Code
### Locally 
To run the code, use the following commands:
1. `cd build`
2. `cmake ..`
3. `make`
4. `./bin/webserver ../config/dkr_config`

To test the code, use the following commands:
1. `cd build`
2. `cmake ..`
3. `make`
4. `make test`

To check build coverage
1. `cd build`
2. `cmake -DCMAKE_BUILD_TYPE=Coverage ..`
3. `make`
4. `make coverage`

NOTE: if having issues, try running `make clean` after `cd build` in these commands

### Docker 
To run the code and view the server in the browser, use the following:
1. `docker build -t server-ihardlyknowher:base -f docker/base.Dockerfile .`
2. `docker build -t webserver:latest -f docker/Dockerfile .`
3. `docker run -p 8080:80 --name echo-server webserver:latest`
4. Open browser via Google Cloud Shell (click view preview in upper right hand corner and then preview on port 8080)

## How to Add a New Request Handler 
### Adding a New Handler
1. Define a class that inherits from RequestHandler and implements the `handle_request()` method.
2. Implement a static create() method that takes in an unordered map of typed arguments and returns a unique pointer to the request handler. 
    ```
    // Method definition 
    static std::unique_ptr<NewHandler> create(const std::unordered_map<std::string, std::string>& args);


    // Method implementation 
    std::unique_ptr<RequestHandler>  NewHandler::create(const std::unordered_map<std::string, std::string>& args) {
        auto it_arg = args.find("<arg>");
        if (it_mount != args.end() && it_root != args.end()) {
    return std::make_unique<NewHandler>(it_arg->second);
        }
        return nullptr;
    }
    ```
3. Register the handler using your RequestHandlerFactory in server_main.cc.
    ```
    #include "new_handler.h"
    ...
    factory.register_factory("<NewHandler>", &<NewHandler>::create);
    ```
4. In config_interpreter, implement support for parsing your handler in extract_handler_configs.
    ```
    ...
    else if (config.handler == "EchoHandler") {
        handler_configs.push_back(config);
    }
    else if (config.handler == "<NewHandler>") {
        // Use find_value_for_key to assign config.args
        config.args[<arg>] = find_value_for_key(statement->child_block_.get(), "<key>");

        handler_configs.push_back(config);
    }
    ...
    ```
5. Update your config file with a location block that uses the new handler name and required arguments.
    ```
    location /<location> <NewHandler> {
        <arg> <value>;
    }
    ```

6. Be sure to update CMakeLists.txt with the new .cc file.

If any step is unclear, refer to the implementation of StaticFileHandler. 

### Current Handlers 
* EchoHandler – Returns the incoming request data as-is in the response body.
* StaticHandler – Serves static files from a configured root directory.
* NotFoundHandler – Returns a simple 404 Not Found response for unmatched or invalid routes.

## Detailed File Information 
### Docker Files
`docker/base.Dockerfile`

Creates a base Docker image to provide a consistent environment for building and testing the project.

---


`docker/Dockerfile`

Defines a multi-stage Docker build to compile, test, and package the web server.

Builder Stage
* Starts from the prebuilt base image.
* Copies all project source files and static app assets.
* Builds the server using CMake and Make.
* Runs tests with ctest to validate functionality.

Deploy Stage
* Copies the compiled webserver binary and configuration into a clean Ubuntu image.
* Bundles static assets under /app/static* directories.
* Exposes port 80 and sets the entrypoint to run the server using a provided config file.

This image is used for production deployment and serves both dynamic content and static files.

---

`docker/coverage.Dockerfile`

Defines a multi-stage Docker build for generating code coverage reports for the project.
* Uses the prebuilt base image as a foundation.
* Configures and compiles the project with gcov coverage flags.
* Runs all unit and integration tests using ctest.
* Generates a detailed HTML code coverage report using gcovr, excluding server_main.cc and googletest directories.
* Produces a minimal deployable image containing the compiled webserver binary and config file.

This image is useful for validating test coverage in CI pipelines or previewing reports locally or in staging environments.

---

`docker/cloudbuild.yaml`

Automates the containerization workflow using Google Cloud Build.
* Pulls a cached base image if available to speed up builds.
* Builds and tags a custom base image using docker/base.Dockerfile.
* Pushes the base image to Google Container Registry for reuse.
* Builds the main application image from docker/Dockerfile.
* Builds a test coverage image from docker/coverage.Dockerfile.

The resulting images are pushed to gcr.io/$PROJECT_ID and can be reused in deployment or testing workflows.

---

### Header and Source Files
`include/nginx_config.h & src/nginx_config.cc`

Defines the data structures used to represent a parsed Nginx-style config file.
* `NginxConfigStatement`:
Stores a single config line and an optional nested block.
* `NginxConfig`:
Holds a sequence of statements, representing a full config block.

Both classes support recursive ToString() serialization for debugging or output.

---

`include/nginx_config_parser.h & src/nginx_config_parser.cc`

Declares and implements the NginxConfigParser class, which parses Nginx-style configuration files into a structured NginxConfig AST. Supports input from both file paths and streams, handling nested blocks, quoted strings, comments, and validation.
* `bool Parse(std::istream* input, NginxConfig* config)`
    * Parses configuration data from an input stream.
    * Returns: true if syntax is valid, false otherwise.
* `bool Parse(const char* file_name, NginxConfig* config)`
    * Opens and parses the config file.
    * Returns: true on success, false on failure or invalid syntax.
* `TokenType ParseToken(std::istream* input, std::string* value)`
    * Extracts and classifies the next token from the stream.
    * Returns: the corresponding TokenType.
* `const char* TokenTypeAsString(TokenType type)`
    * Converts a token type enum into a human-readable string for debugging/logging.

---

`include/config_interpreter.h` & `src/config_interpreter.cc`

Implements utility functions to process a parsed NginxConfig object and extract server configuration details such as port numbers and URI-to-handler mappings.

* `void process_config_file(std::ifstream& config_file, NginxConfig& config)`
    * Parses configuration from an input file stream using NginxConfigParser.
    * Throws: std::runtime_error if the file cannot be opened or parsed.
* `std::string find_value_for_key(const NginxConfig* config_block, const std::string& key)`
    * Recursively searches a config block (and its children) for a directive key.
    * Returns: The value of the first matching key.
    * Throws: std::runtime_error if the key is not found.
* `short find_listen_port(const NginxConfig* config_block)`
    * Extracts the port number from the listen directive using find_value_for_key.
    * Returns: Port number as a short.
    * Throws: If the directive is missing or the value is not a valid integer.
* `std::vector<ConfigStruct> extract_handler_configs(const NginxConfig* config_block)`
    * Parses all location blocks and extracts handler configuration data.
    * Returns: A vector of ConfigStruct objects containing uri, handler, and argument mappings.
    * Throws: If a location is misconfigured, has a trailing slash, or uses an unsupported handler.
* `std::map<std::string, ConfigStruct> create_uri_to_config_map(const std::vector<ConfigStruct>& handler_configs)`
    * Creates a map from URI strings to their corresponding ConfigStruct.
    * Returns: A std::map of URI-to-config mappings.
    * Throws: If duplicate URIs are detected.

---

`include/request.h`

Defines a request struct to hold the request method, URI, HTTP version, headers, and body. 

---

`include/response.h`

Defines a response struct to hold the response HTTP version, status code, reason phrase, headers, and body. 

---

`include/res_req_helpers.h` & `src/res_req_helpers.cc`

Define helper functions to work with the request and response struct.
* `request parse_request(const std::string& raw_request)`
    * Parses a raw HTTP request string into a request struct. 
* `std::string serialize_response(const response& res);`
    * Converts a response struct into a raw HTTP response string. 

---

`include/request_handler.h`

Defines an abstract base class for request handlers. 
* `using Factory = std::function<std::unique_ptr<RequestHandler>(const std::unordered_map<std::string, std::string>&)>`
    * Creates a callable function that takes in an unordered map and returns a unique pointer to a handler.
* `virtual ~RequestHandler() {}`
    * Virtual request handler constructor 
* `virtual std::unique_ptr<response> handle_request(const request& req)`
    * Handles an incoming HTTP request and returns a response

---

`include/request_handler_factory.h`

Defines a factory class for dynamically creating instances of RequestHandler-derived objects based on handler names and constructor arguments.

* `void register_factory(const std::string& handler_name, RequestHandler::Factory factory)`
    * Registers a factory function for a specific handler type. Used to associate a handler name (e.g., "StaticFileHandler") with its creation logic.
* `std::unique_ptr<RequestHandler> create_handler(const std::string& handler_name, const std::unordered_map<std::string, std::string>& args)`
    * Creates and returns a new handler instance by invoking the factory function associated with the given handler name. Arguments are passed as key-value pairs.
* `std::unordered_map<std::string, RequestHandler::Factory> factory_map_;`
    * Internal map that associates handler names with their corresponding factory functions.

---

`include/echo_handler.h & include/echo_handler.cc`

Returns the incoming request data as-is in the response body.
* `std::unique_ptr<response> handle_request(const request& req) override;`
    * Generates a plain-text response that echoes the full HTTP request.
* `static std::unique_ptr<RequestHandler> create(const std::unordered_map<std::string, std::string>& args);`
    * Factory method to create echo handler. 

---

`include/static_file_handler.h` & `include/static_file_handler.cc`
Serves static files from a configured root directory.
* `StaticFileHandler(const std::string& mount_point, const std::string& doc_root);`
    * Constructor that takes in the static file handlers mount point and document root. 
* `static std::unique_ptr<RequestHandler> create(const std::unordered_map<std::string, std::string>& args);`
    * Factory method to create a static file handler that parses arguments and calls constructor. 
* `std::unique_ptr<response> handle_request(const request& req) override;`
    * Serves a static file under mount_point_ based on the request URI.

---

`include/not_found_handler.h` & `include/not_found_handler.cc`

Handles unmatched or invalid URL requests by returning a basic 404 Not Found response. This makes sure that requests not mapped in the config file receive a valid HTTP response and do not crash the server.
* `class NotFoundHandler : public RequestHandler`
    * Inherits from the abstract `RequestHandler` base class and implements the `handle_request()` method.
* `static std::unique_ptr<RequestHandler> create(const std::unordered_map<std::string, std::string>& args);`
    * Static factory method used to instantiate the handler without any arguments.
* `std::unique_ptr<response> handle_request(const request& req) override;`
    * This returns a 404 response with a plain-text body showing that the requested URL was not found.
	
---

`include/logger.h` & `src/logger.cc`

Implements application-wide logging using Boost.Log, with output directed to both the console and a log file.
* Supports daily rotation or rotation at 10MB file size, whichever comes first.
* Log entries include timestamp, severity level, and thread ID for traceability.
* Useful for debugging, monitoring, and tracing execution across components.

Example usage:
	`LOG_<SEVERITY> << “<MESSAGE”;`

---

`include/server.h` & `src/server.cc`

Implements the main server class responsible for accepting and managing incoming client connections using Boost.Asio.
* `server(boost::asio::io_service& io_service, short port, std::map<std::string, ConfigStruct> uri_to_config_map, RequestHandlerFactory& factory);`
    * Constructor that initializes the acceptor on the given port and logs startup. 
* `void start_accept()`
    * Begins asynchronous accept loop, creating a new session for each incoming connection.
* `void handle_accept(session* new_session, const boost::system::error_code& error)`
    * Handles post-accept logic: logs the client IP, starts the session, and continues accepting new connections.

---

`include/session.h & src/session.cc`

Handles a single client connection on the server. Each session is responsible for reading an HTTP request, selecting the appropriate handler based on URI, generating a response, and writing it back to the client. Uses asynchronous I/O via Boost.Asio.
* `session(boost::asio::io_service&, std::map<std::string, ConfigStruct>, RequestHandlerFactory&)`
    * Constructor. Initializes the session socket and stores config and factory references.
* `tcp::socket& socket()`
    * Returns a reference to the session's socket.
* `void set_client_ip(const std::string& ip)`
    * Sets the client's IP address for logging and traceability.
* `void start()`
    * Begins asynchronous reading from the client socket.
* `void handle_read(const boost::system::error_code& error, size_t bytes_transferred)`
    * Handles incoming data from the client.
        * Parses HTTP request from buffer.
        * Matches the request URI to the best handler using longest-prefix matching.
        * Invokes the appropriate handler and generates a response.
        * Sends the response or continues reading if incomplete.
* `void handle_write(const boost::system::error_code& error)`
    * Finalizes the response by closing the socket and cleaning up the session.

---

`include/trie.h & src/trie.cc`

Implements a TRIE data structure to efficiently execute longest prefix matching of the URI paths against registered handler routes.
* `class TrieNode`
    * Represents a single node in the TRIE, storing child nodes and an optional config pointer for the handler.
* `void insert(const std::string& path, ConfigStruct* config_ptr);`
    * Tokenizes and inserts a new route path into the TRIE. Each segment of the path becomes a nested node. 
    * For example, adds a path like /static/images into the TRIE by breaking it into parts (static, images).
* `const ConfigStruct* match(const std::string& path) const;`
    * Traverses the TRIE using segments of the request path and returns the config associated with the longest matched prefix.

---

`src/server_main.cc`

Instantiates and runs the asynchronous server on the specified port.
* Initializes logging and validates command-line arguments.
* Parses the Nginx-style config file to extract the port and handler mappings.
* Registers supported handlers with the factory (EchoHandler, StaticFileHandler).
* Handles errors gracefully and logs all major events.
* Registers signal handlers (SIGINT, SIGTERM) for clean shutdown.


