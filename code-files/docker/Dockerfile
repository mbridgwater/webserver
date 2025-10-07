### Build/test container ###
# Define builder stage
ARG PROJECT_ID=server-ihardlyknowher-456404
FROM gcr.io/${PROJECT_ID}/server-ihardlyknowher-base:latest AS builder
# FROM server-ihardlyknowher:base AS builder

# Copy project source files into the container
COPY . /usr/src/project

# Create app directories
RUN mkdir -p /app/static /app/quizzes \
 && cp -r /usr/src/project/src/app/* /app/static \
 && cp -r /usr/src/project/src/app/quizzes /app/quizzes


 
# Set working directory for build
WORKDIR /usr/src/project/build

# Build and test
RUN cmake ..
RUN make
RUN ctest --output-on_failure

### Deploy container ###
# Define deploy stage
FROM ubuntu:noble as deploy

# Copy server output binary and config file to container working directory
COPY --from=builder /usr/src/project/build/bin/webserver .
COPY --from=builder /usr/src/project/config/dkr_config .

# Copy static assets
COPY --from=builder /usr/src/project/src/app /app/static
COPY --from=builder /usr/src/project/src/app1 /app/static1
COPY --from=builder /usr/src/project/src/app2 /app/static2
COPY --from=builder /usr/src/project/src/app/quizzes /app/quizzes

# Expose port 80
EXPOSE 80

# Use ENTRYPOINT to specify the binary name
ENTRYPOINT ["./webserver"]

# Use CMD to pass Docker-specific config file as argument to ENTRYPOINT
CMD ["dkr_config"]
