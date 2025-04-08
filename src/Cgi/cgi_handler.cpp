#include "../../inc/Cgi/cgi_handler.h"

CgiHandler::CgiHandler()
    : childPid(-1), exitStatus(0), isCompleted(false), isRegistered(false), response(NULL), clientFd(-1), executor(), pid(-1), startTime(std::time(NULL)), timeout(60000), state_(CGI_IDLE)
{
}

void CgiHandler::OnEvent(uint32_t events)
{
    if (state_ != CGI_READING) return;

    if (events & (EPOLLIN | EPOLLRDHUP | EPOLLHUP))
    {
        ReadOutputPipes();
        CheckChildProcessStatus();

        if (isTimedOut())
        {
            setState(CGI_TIMEOUT);
            handleCgiCompletion();
        }
        else if (isComplete() || getFd() == -1)
        {
            setState(CGI_COMPLETED);
            handleCgiCompletion();
        }
    }
}

void CgiHandler::ReadOutputPipes()
{
    ReadFromOutputPipe();
    ReadFromErrorPipe();
}

void CgiHandler::ReadFromOutputPipe()
{
    char buffer[4096];
    ssize_t n;

    if (getFd() != -1)
    {
        while ((n = read(getFd(), buffer, sizeof(buffer) - 1)) > 0)
        {
            if (isTimedOut())
            {
                handleCgiCompletion();
                return;
            }
            buffer[n] = '\0';
            outputContent.append(buffer, n);
            usleep(10000);
        }

        if (n == 0 || (n == -1))
        {
            outputPipeRead_.closeIfValid();
        }
        if (isRegisteredToEpoll())
        {
            EpollHandler::Instance().UnregisterEvent(this);
            isRegistered = false;
        }
    }
}

void CgiHandler::ReadFromErrorPipe()
{
    char buffer[4096];
    ssize_t n;

    if (getErrorPipe() != -1)
    {
        while ((n = read(getErrorPipe(), buffer, sizeof(buffer) - 1)) > 0)
        {
            buffer[n] = '\0';
            errorContent.append(buffer, n);
        }

        if (n == 0 || (n == -1))
        {
            errorPipeRead_.closeIfValid();
        }
        if (isRegisteredToEpoll())
        {
            EpollHandler::Instance().UnregisterEvent(this);
            isRegistered = false;
        }
    }
}

void CgiHandler::CheckChildProcessStatus()
{
    if (childPid > 0)
    {
        int status;
        pid_t result = waitpid(childPid, &status, WNOHANG);

        if (result == childPid)
        {
            exitStatus = 0;
            ProcessChildExitStatus(status);
            childPid = -1;
        }
    }
}

void CgiHandler::ProcessChildExitStatus(int status)
{
    if (WIFEXITED(status))
    {
        exitStatus = WEXITSTATUS(status);
        if (exitStatus != 0 && errorContent.empty())
        {
            std::stringstream ss;
            ss << "CGI process exited with non-zero status: " << exitStatus;
            errorContent = ss.str();
        }
    }
    else if (WIFSIGNALED(status))
    {
        int signalNum = WTERMSIG(status);
        std::stringstream ss;
        ss << "CGI process terminated by signal: " << signalNum;
        errorContent = ss.str();
    }
}

void CgiHandler::handleCgiCompletion()
{
    std::string responseBody;
    ClientConnection *client = EpollHandler::Instance().FindClientByFd(clientFd);
    bool hasError = false;

    PrepareResponseBody(responseBody, hasError);
    if (client)
    {
        client->handleCgiResponse(responseBody);
        client->SetCgiPid(-1);
    }

    UnregisterAndCleanup();

    if (client)
    {
        EpollHandler::Instance().InvalidateEvent(this);
        EpollHandler::Instance().ScheduleForDeletion(this);
        client->setCgiHandler(NULL);
    }
}

void CgiHandler::PrepareResponseBody(std::string &responseBody, bool &hasError)
{
    if (isTimedOut() && !isCompleted)
    {
        HandleTimeoutResponse(hasError);
    }
    else if (!errorContent.empty())
    {
        HandleErrorResponse(hasError);
    }
    else if (outputContent.empty())
    {
        HandleEmptyOutputResponse(hasError);
    }
    else
    {
        if (state_ != CGI_TIMEOUT && state_ != CGI_ERROR)
        {
            setState(CGI_COMPLETED);
        }
        responseBody = outputContent;
    }
}

void CgiHandler::HandleTimeoutResponse(bool &hasError)
{
    setState(CGI_TIMEOUT);

    if (response)
    {
        response->SetStatus(504, "Gateway Timeout");
        response->SetIsCgiProcessed(true);
        response->SetHeader("Connection", "close");
    }
    hasError = true;

    if (childPid > 0)
    {
        terminateChildProcess(childPid);
        ClientConnection *client = EpollHandler::Instance().FindClientByFd(clientFd);
        if (client)
        {
            client->SetCgiPid(-1);
        }
        childPid = -1;
    }
}

void CgiHandler::HandleErrorResponse(bool &hasError)
{
    setState(CGI_ERROR);

    if (response)
    {
        response->SetStatus(500, "Internal Server Error");
        response->SetIsCgiProcessed(true);
        response->SetHeader("Connection", "close");
    }
    hasError = true;
}

void CgiHandler::HandleEmptyOutputResponse(bool &hasError)
{
    setState(CGI_ERROR);

    if (response)
    {
        response->SetStatus(500, "Internal Server Error");
        response->SetIsCgiProcessed(true);
        response->SetHeader("Connection", "close");
    }
    hasError = true;
}

void CgiHandler::HandleClientResponse(ClientConnection *client, const std::string &responseBody, bool hasError)
{
    if (client)
    {
        if ((isTimedOut() && !isCompleted) || !errorContent.empty() || outputContent.empty())
        {
            if (response)
            {
                client->handleCgiResponse(responseBody);
            }
        }
        else
        {
            client->handleCgiResponse(responseBody);
        }

        if (hasError && response)
        {
            response->SetHeader("Connection", "close");
        }

        client->SetCgiPid(-1);
        client->setCgiHandler(NULL);
    }
}

void CgiHandler::UnregisterAndCleanup()
{
    if (isRegistered)
    {
        EpollHandler::Instance().UnregisterEvent(this);
        isRegistered = false;
    }

    cleanup();
}

int CgiHandler::getFd() const
{
    return outputPipeRead_.get();
}

void CgiHandler::executeCgi(const ServerConfig &server, const HttpRequest &request,
                            const std::string &scriptPath)
{
    if (executor.empty())
    {
        throw std::runtime_error("CGI executor not set");
    }

    this->scriptPath = scriptPath;
    this->method = request.GetMethod();
    this->requestBody = request.GetBody();

    setState(CGI_IDLE);
    createPipes();
    setupEnvironment(server, request, scriptPath);

    if (!execute())
    {
        throw std::runtime_error("Failed to execute CGI script");
    }
}

bool CgiHandler::execute()
{
    setState(CGI_EXECUTING);

    childPid = fork();
    if (childPid == -1)
    {
        errorContent = "Fork failed: Could not create process";
        setState(CGI_ERROR);
        return false;
    }

    if (childPid == 0)
    {
        if(childPid % 2 == 0)
            usleep(10000);
        setupChildProcess();
        std::exit(1);
    }

    pid = childPid;

    ClientConnection *client = EpollHandler::Instance().FindClientByFd(clientFd);
    if (client)
    {
        client->SetCgiPid(childPid);
    }

    if (!setupParentProcess())
    {
        errorContent = "Failed to setup parent process";
        terminateChildProcess(childPid);
        childPid = -1;
        setState(CGI_ERROR);
        return false;
    }

    return true;
}

void CgiHandler::setupChildProcess()
{
    dup2(inputPipeRead_.get(), STDIN_FILENO);
    dup2(outputPipeWrite_.get(), STDOUT_FILENO);
    dup2(errorPipeWrite_.get(), STDERR_FILENO);

    closeAllPipes();

    char **env = prepareEnvironment();

    if (executor.empty())
    {
        std::exit(1);
    }

    char *args[] = {(char *)executor.c_str(), (char *)scriptPath.c_str(), NULL};
    execve(executor.c_str(), args, env);
    std::cout << "execve failed" << std::endl;
    std::exit(1);
}

bool CgiHandler::setupParentProcess()
{
    inputPipeRead_.closeIfValid();
    outputPipeWrite_.closeIfValid();
    errorPipeWrite_.closeIfValid();

    fcntl(outputPipeRead_.get(), F_SETFL, O_NONBLOCK);
    fcntl(errorPipeRead_.get(), F_SETFL, O_NONBLOCK);
    fcntl(inputPipeWrite_.get(), F_SETFL, O_NONBLOCK);

    if (!writeRequestBody())
    {
        return false;
    }

    inputPipeWrite_.closeIfValid();
    setState(CGI_READING);
    return true;
}

bool CgiHandler::terminateChildProcess(pid_t pid)
{
    if (pid <= 0)
        return true;

    if (waitpid(pid, &exitStatus, WNOHANG) == 0)
    {
        int status;
        pid_t result = waitpid(pid, &status, WNOHANG);
        if (result == pid)
            return true;
        }

    if (kill(pid, SIGKILL) == 0)
    {
        int status;
        pid_t result = waitpid(pid, &status, 0);
        return (result == pid);
    }

    return false;
}


CgiHandler::~CgiHandler()
{
    if (isRegistered)
    {
        EpollHandler::Instance().UnregisterEvent(this);
        isRegistered = false;
    }

    if (childPid > 0)
    {
        terminateChildProcess(childPid);
        childPid = -1;
    }

}

void CgiHandler::createPipes()
{

    int inputPipe[2];
    int outputPipe[2];
    int errorPipe[2];

    if (pipe(inputPipe) < 0 || pipe(outputPipe) < 0 || pipe(errorPipe) < 0)
    {
        throw std::runtime_error("Failed to create pipes");
    }

    inputPipeRead_.reset(inputPipe[0]);
    inputPipeWrite_.reset(inputPipe[1]);
    outputPipeRead_.reset(outputPipe[0]);
    outputPipeWrite_.reset(outputPipe[1]);
    errorPipeRead_.reset(errorPipe[0]);
    errorPipeWrite_.reset(errorPipe[1]);

    if (fcntl(inputPipeWrite_.get(), F_SETFL, O_NONBLOCK) < 0 ||
        fcntl(outputPipeRead_.get(), F_SETFL, O_NONBLOCK) < 0 ||
        fcntl(errorPipeRead_.get(), F_SETFL, O_NONBLOCK) < 0)
    {
        throw std::runtime_error("Failed to set pipes to non-blocking mode");
    }

    fcntl(inputPipeRead_.get(), F_SETFD, FD_CLOEXEC);
    fcntl(inputPipeWrite_.get(), F_SETFD, FD_CLOEXEC);
    fcntl(outputPipeRead_.get(), F_SETFD, FD_CLOEXEC);
    fcntl(outputPipeWrite_.get(), F_SETFD, FD_CLOEXEC);
    fcntl(errorPipeRead_.get(), F_SETFD, FD_CLOEXEC);
    fcntl(errorPipeWrite_.get(), F_SETFD, FD_CLOEXEC);
}

void CgiHandler::cleanupPipes()
{
    inputPipeRead_.closeIfValid();
    inputPipeWrite_.closeIfValid();
    outputPipeRead_.closeIfValid();
    outputPipeWrite_.closeIfValid();
    errorPipeRead_.closeIfValid();
    errorPipeWrite_.closeIfValid();
}

void CgiHandler::setupEnvironment(const ServerConfig &server, const HttpRequest &request, const std::string &scriptPath)
{
    envVars.clear();
    SetupBasicEnvironment(request, scriptPath);
    SetupServerVariables(server);
    SetupRequestVariables(request);
    SetupContentVariables(request);
}

void CgiHandler::SetupBasicEnvironment(const HttpRequest &request, const std::string &scriptPath)
{
    envVars["GATEWAY_INTERFACE"] = "CGI/1.1";
    envVars["SERVER_PROTOCOL"] = request.GetVersion();
    envVars["REQUEST_METHOD"] = request.GetMethod();
    envVars["SCRIPT_FILENAME"] = scriptPath;
    envVars["REDIRECT_STATUS"] = "200";
    envVars["SERVER_SOFTWARE"] = "johnx/1.0.0";

    if (request.GetHeaders().find("host") != request.GetHeaders().end())
    {
        envVars["SERVER_NAME"] = request.GetHeaders().at("host");
    }
    else
    {
        envVars["SERVER_NAME"] = "localhost";
    }
}

void CgiHandler::SetupServerVariables(const ServerConfig &server)
{
    const std::vector<ListenDirective> &directives = server.GetListenDirectives();
    if (!directives.empty())
    {
        std::stringstream ss;
        ss << directives[0].port;
        envVars["SERVER_PORT"] = ss.str();
        envVars["REMOTE_ADDR"] = directives[0].host;
    }
}

void CgiHandler::SetupRequestVariables(const HttpRequest &request)
{
    envVars["SCRIPT_NAME"] = request.GetPath();
    envVars["QUERY_STRING"] = request.GetQueryString();
    envVars["REQUEST_URI"] = request.GetPath();
}

void CgiHandler::SetupContentVariables(const HttpRequest &request)
{
    const std::map<std::string, std::string> &headers = request.GetHeaders();

    for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it)
    {
        std::string lowerHeaderName = it->first;
        lowerHeaderName = libft::FT_ToLower(lowerHeaderName);

        if (lowerHeaderName == "content-type")
        {
            envVars["CONTENT_TYPE"] = it->second;
        }
        else if (lowerHeaderName == "content-length")
        {
            envVars["CONTENT_LENGTH"] = it->second;
        }
    }

    if (request.GetMethod() == "POST" && envVars.find("CONTENT_LENGTH") == envVars.end())
    {
        std::stringstream ss;
        ss << request.GetBody().length();
        envVars["CONTENT_LENGTH"] = ss.str();
    }
}

int CgiHandler::getExitStatus() const
{
    return exitStatus;
}

bool CgiHandler::writeRequestBody()
{
    if (requestBody.empty())
    {
        return true;
    }

    size_t total = 0;
    size_t left = requestBody.length();

    while (total < requestBody.length())
    {
        if (!WriteChunk(total, left))
            return false;

        if (total > 8192)
            break;
    }

    return true;
}

bool CgiHandler::WriteChunk(size_t &total, size_t &left)
{
    ssize_t written = write(inputPipeWrite_.get(), requestBody.c_str() + total, left);
    if (written > 0)
    {
        total += written;
        left -= written;
        return true;
    }
    else if (written == -1)
    {
        return true;
    }
    else
    {
        HandleWriteFailure();
        return false;
    }
}

void CgiHandler::HandleWriteFailure()
{
    terminateChildProcess(childPid);
    childPid = -1;
    cleanupPipes();
}

void CgiHandler::closeAllPipes()
{
    cleanupPipes();
}

int CgiHandler::getErrorPipe() const
{
    return errorPipeRead_.get();
}

pid_t CgiHandler::getChildPid() const
{
    return childPid;
}

char **CgiHandler::prepareEnvironment()
{
    envVars["GATEWAY_INTERFACE"] = "CGI/1.1";
    envVars["SERVER_PROTOCOL"] = "HTTP/1.1";
    envVars["REQUEST_METHOD"] = method;
    envVars["SCRIPT_FILENAME"] = scriptPath;
    envVars["REDIRECT_STATUS"] = "200";
    envVars["SERVER_SOFTWARE"] = "johnx/1.0.0";

    char **env = new char *[envVars.size() + 1];
    int i = 0;
    for (std::map<std::string, std::string>::const_iterator it = envVars.begin();
         it != envVars.end(); ++it)
    {
        std::string entry = it->first + "=" + it->second;
        env[i] = new char[entry.length() + 1];
        std::strncpy(env[i], entry.c_str(), entry.length());
        env[i][entry.length()] = '\0';
        ++i;
    }
    env[i] = NULL;
    return env;
}

bool CgiHandler::isTimedOut() const
{
    time_t elapsed = std::time(NULL) - startTime;
    return elapsed * 1000 >= timeout;
}

void CgiHandler::setTimeout(time_t milliseconds)
{
    timeout = milliseconds;
}

void CgiHandler::cleanup()
{
    cleanupPipes();

    if (childPid > 0)
    {
        int status;
        pid_t result = waitpid(childPid, &status, WNOHANG);
        if (result == 0)
        {
            bool terminated = terminateChildProcess(childPid);
            if (!terminated)
            {
                errorContent += "\nWarning: Failed to terminate CGI process completely";
                if (state_ == CGI_IDLE || state_ == CGI_EXECUTING || state_ == CGI_READING)
                {
                    setState(CGI_ERROR);
                }
            }
        }
        else if(result == childPid)
        {
            ProcessChildExitStatus(status);
            if (state_ != CGI_TIMEOUT && state_ != CGI_ERROR)
            {
                setState(CGI_COMPLETED);
            }
        }
        else
        {
            errorContent += "\nWarning: Failed to terminate CGI process completely";
            if (state_ == CGI_IDLE || state_ == CGI_EXECUTING || state_ == CGI_READING)
            {
                setState(CGI_ERROR);
            }
        }

        ClientConnection *client = EpollHandler::Instance().FindClientByFd(clientFd);
        if (client)
        {
            client->SetCgiPid(-1);
        }
        childPid = -1;
    }

    isCompleted = true;
}

bool CgiHandler::isComplete() const
{
    return isCompleted || state_ == CGI_COMPLETED || state_ == CGI_ERROR || state_ == CGI_TIMEOUT;
}

bool CgiHandler::isRegisteredToEpoll() const
{
    return isRegistered;
}

void CgiHandler::setRegistered(bool registered)
{
    isRegistered = registered;
}

void CgiHandler::setResponse(HttpResponse *resp)
{
    response = resp;
}

HttpResponse *CgiHandler::getResponse() const
{
    return response;
}

void CgiHandler::setClientFd(int fd)
{
    clientFd = fd;
}

int CgiHandler::getClientFd() const
{
    return clientFd;
}

void CgiHandler::setExecutor(const std::string &exec)
{
    executor = exec;
}

const std::string &CgiHandler::getExecutor() const
{
    return executor;
}

CgiState CgiHandler::getState() const
{
    return state_;
}

void CgiHandler::setState(CgiState s)
{
    state_ = s;
}
