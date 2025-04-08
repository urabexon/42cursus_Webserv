#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include <sys/wait.h>

#include "../Web/client_connection.h"
#include "../Web/event.h"
#include "../Util/SocketFd.h"

class ClientConnection;

enum CgiState {
    CGI_IDLE,
    CGI_EXECUTING,
    CGI_READING,
    CGI_COMPLETED,
    CGI_TIMEOUT,
    CGI_ERROR
};

class CgiHandler : public Event {
private:
    SocketFd inputPipeRead_;
    SocketFd inputPipeWrite_;
    SocketFd outputPipeRead_;
    SocketFd outputPipeWrite_;
    SocketFd errorPipeRead_;
    SocketFd errorPipeWrite_;
    pid_t childPid;
    int exitStatus;
    std::map<std::string, std::string> envVars;

    std::string scriptPath;
    std::string queryString;
    std::string method;
    std::string requestBody;
    bool isCompleted;
    bool isRegistered;
    HttpResponse* response;
    int clientFd;
    std::string executor;
    pid_t pid;
    std::string outputContent;
    std::string errorContent;
    time_t startTime;
    time_t timeout;
    CgiState state_;

    void createPipes();
    void cleanupPipes();
    void setupEnvironment(const ServerConfig &server, const HttpRequest &request, const std::string &scriptPath);
    bool terminateChildProcess(pid_t pid);
    void setupChildProcess();
    char** prepareEnvironment();
    bool setupParentProcess();
    bool writeRequestBody();
    void handleCgiCompletion();

    void ReadOutputPipes();
    void ReadFromOutputPipe();
    void ReadFromErrorPipe();
    void CheckChildProcessStatus();
    void ProcessChildExitStatus(int status);

    void PrepareResponseBody(std::string& responseBody, bool& hasError);
    void HandleTimeoutResponse(bool& hasError);
    void HandleErrorResponse(bool& hasError);
    void HandleEmptyOutputResponse(bool& hasError);
    void HandleClientResponse(ClientConnection* client, const std::string& responseBody, bool hasError);
    void UnregisterAndCleanup();

    void SetupBasicEnvironment(const HttpRequest &request, const std::string &scriptPath);
    void SetupServerVariables(const ServerConfig &server);
    void SetupRequestVariables(const HttpRequest &request);
    void SetupContentVariables(const HttpRequest &request);

    bool WriteChunk(size_t& total, size_t& left);
    void HandleWriteFailure();

public:
    CgiHandler();
    ~CgiHandler();

    void executeCgi(const ServerConfig &server, const HttpRequest &request,
                    const std::string &scriptPath);
    bool isComplete() const;
    int getExitStatus() const;

    bool execute();
    void closeAllPipes();

    int getErrorPipe() const;
    pid_t getChildPid() const;

    void OnEvent(uint32_t events);
    int getFd() const;

    bool isRegisteredToEpoll() const;
    void setRegistered(bool registered);

    void setResponse(HttpResponse* resp);
    HttpResponse* getResponse() const;

    void setClientFd(int fd);
    int getClientFd() const;

    void setExecutor(const std::string& exec);
    const std::string& getExecutor() const;

    void setTimeout(time_t milliseconds);
    bool isTimedOut() const;

    void cleanup();

    CgiState getState() const;
    void setState(CgiState s);
};

#endif
