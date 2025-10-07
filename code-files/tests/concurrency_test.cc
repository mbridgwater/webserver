#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdlib>

using namespace std::chrono;

double run_curl_and_time(const std::string& url) {
    auto start = high_resolution_clock::now();
    std::string cmd = "curl -s " + url + " > /dev/null";
    int ret = std::system(cmd.c_str());
    auto end = high_resolution_clock::now();
    duration<double> elapsed = end - start;
    return elapsed.count();
}

TEST(ConcurrentRequestTest, HandlesSimultaneousRequests) {
    pid_t pid = fork();

    if (pid == 0) {
        // launch request to sleep in this process
        double time_taken = run_curl_and_time("http://localhost:8080/sleep");
        exit(0);
    } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        double time_taken = run_curl_and_time("http://localhost:8080/echo");
        waitpid(pid, nullptr, 0);

        // making sure result came back in less than the time it took for your sleep amount, sleep amount is 3
        EXPECT_LT(time_taken, 3.0);
    }
}
