#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

std::atomic<int> g_counter(0);

void clientThread(const char* ip, int port, int connNum)
{
    std::vector<int> socks(connNum);
    for(int i=0; i<connNum; ++i)
    {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in servAddr{};
        servAddr.sin_family = AF_INET;
        servAddr.sin_port = htons(port);
        inet_pton(AF_INET, ip, &servAddr.sin_addr);

        if(connect(sock, (sockaddr*)&servAddr, sizeof(servAddr)) < 0)
        {
            perror("connect");
            continue;
        }
        socks[i] = sock;
    }

    const char* msg = "ping";
    char buf[1024];
    while(true)
    {
        for(int sock : socks)
        {
            send(sock, msg, strlen(msg), 0);
            int n = recv(sock, buf, sizeof(buf), 0);
            if(n > 0)
                g_counter++;
        }
    }
}

int main()
{
    const char* serverIp = "127.0.0.1";
    int serverPort = 8000;
    int threadNum = 4;
    int connPerThread = 100;

    std::vector<std::thread> threads;
    for(int i=0; i<threadNum; ++i)
    {
        threads.emplace_back(clientThread, serverIp, serverPort, connPerThread);
    }

    while(true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        int qps = g_counter.exchange(0);
        std::cout << "QPS: " << qps << std::endl;
    }

    for(auto& t : threads) t.join();
}
