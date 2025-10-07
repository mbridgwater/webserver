### Build/test/coverage stage ###
ARG PROJECT_ID
FROM gcr.io/${PROJECT_ID}/server-ihardlyknowher-base:latest AS coverage

# Set working directory inside the container
WORKDIR /usr/src/project

# Copy the entire project into the container working directory
COPY . .

# Create build directory and configure with coverage flags
RUN mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Debug \
             -DCMAKE_CXX_FLAGS="--coverage" \
             -DCMAKE_EXE_LINKER_FLAGS="--coverage"

# Build everything
RUN cd build && make -j$(nproc)

# Run all tests
RUN cd build && ctest --output-on-failure

# Generate coverage report (HTML)
RUN cd build && \
    mkdir -p coverage-report && \
    gcovr \
      --root /usr/src/project \
      --filter '/usr/src/project/src' \
      --exclude '/usr/src/project/src/server_main\.cc' \
      --html --html-details \
      -o coverage-report/index.html \
      --print-summary \
      --verbose \
      --exclude-directories "googletest"

### Deploy stage ###
FROM ubuntu:noble AS deploy

# Copy built webserver binary and config file from previous stage
COPY --from=coverage /usr/src/project/build/bin/webserver .
COPY --from=coverage /usr/src/project/config/dkr_config .

# Expose port 80
EXPOSE 80

# Use ENTRYPOINT to specify the binary name
ENTRYPOINT ["./webserver"]

# Use CMD to pass Docker-specific config file as argument to ENTRYPOINT
CMD ["dkr_config"]