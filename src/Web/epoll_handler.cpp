#include "../../inc/Web/epoll_handler.h"

EpollHandler::EpollHandler() : epoll_fd_(-1), max_events_(0) {}

EpollHandler::~EpollHandler()
{
  if (epoll_fd_ >= 0)
  {
    close(epoll_fd_);
  }

  PerformDelayedDeletion();
}

EpollHandler &EpollHandler::Instance()
{
  static EpollHandler instance;
  return instance;
}

void EpollHandler::Init(int maxEvents)
{
  max_events_ = maxEvents;
  epoll_fd_ = epoll_create(max_events_);
  if (epoll_fd_ < 0)
  {
    throw std::runtime_error("epoll_create failed");
  }
}

void EpollHandler::RunEventLoop()
{
  std::vector<epoll_event> events(max_events_);

  while (true)
  {
    int nfds = epoll_wait(epoll_fd_, &events[0], max_events_, EPOLL_TIMEOUT_MS);
    if (nfds == -1)
    {
      if (errno == EINTR)
        continue;
      throw std::runtime_error("epoll_wait failed");
    }

    ProcessEvents(events, nfds);
    CleanupConnections();
    PerformDelayedDeletion();
  }
}

void EpollHandler::ScheduleForDeletion(Event *event)
{
  if (event)
  {
    InvalidateEvent(event);
    to_be_deleted_.insert(event);
  }
}

void EpollHandler::PerformDelayedDeletion()
{
  std::set<Event*> deleted_events = to_be_deleted_;

  for (std::set<Event *>::iterator it = to_be_deleted_.begin(); it != to_be_deleted_.end(); ++it)
  {
    errno = 0;
    if (EpollHandler::Instance().UnregisterEvent(*it))
    {
      delete *it;
    }
  }
  to_be_deleted_.clear();
  while(waitpid(-1, NULL, WNOHANG) > 0);
}

void EpollHandler::CleanupConnections()
{
  for (size_t i = 0; i < servers_.size(); ++i)
  {
    std::map<int, ClientConnection *> &connections = servers_[i]->GetConnections();
    std::map<int, ClientConnection *>::iterator it = connections.begin();

    while (it != connections.end())
    {
      ClientConnection *conn = it->second;

      if (!conn)
      {
        ++it;
        continue;
      }

      if (conn->IsTimedOut() && !conn->IsClosed())
      {
        conn->Close();
        ++it;
      }
      else if (conn->ShouldDelete())
      {
        if (!conn->IsClosed())
        {
          UnregisterEvent(conn);
        }
        DeleteConnection(it, connections);
      }
      else
      {
        ++it;
      }
    }
  }
}

void EpollHandler::HandleCgiTimeout(std::map<int, ClientConnection *>::iterator &it, std::map<int, ClientConnection *> &connections)
{
  try
  {
    CgiHandler *handler = it->second->getCgiHandler();
    if (handler && handler->isRegisteredToEpoll())
    {
      UnregisterEvent(handler);
      handler->setRegistered(false);
    }

    it->second->OnEvent(EPOLLOUT);
    it->second->KillCgiProcess();
    it->second->setCgiHandler(NULL);
    ++it;
  }
  catch (const std::exception &e)
  {
    CloseAndRemoveConnection(it, connections);
  }
}

void EpollHandler::CloseAndRemoveConnection(std::map<int, ClientConnection *>::iterator &it, std::map<int, ClientConnection *> &connections)
{
  it->second->Close();
  DeleteConnection(it, connections);
}

void EpollHandler::DeleteConnection(std::map<int, ClientConnection *>::iterator &it, std::map<int, ClientConnection *> &connections)
{
  ScheduleForDeletion(it->second);
  std::map<int, ClientConnection *>::iterator temp = it++;
  connections.erase(temp);
}

void EpollHandler::ProcessEvents(const std::vector<epoll_event> &events, int nfds)
{
  for (int i = 0; i < nfds; i++)
  {
    const epoll_event &ev = events[i];
    Event *event_handler = static_cast<Event *>(ev.data.ptr);

    if (!event_handler || !IsEventValid(event_handler))
    {
      continue;
    }

    int fd = event_handler->getFd();
    if (fd < 0)
    {
      continue;
    }

    if (!IsCgiEvent(event_handler) && !IsServerEvent(event_handler))
    {
      ClientConnection *conn = dynamic_cast<ClientConnection *>(event_handler);
      if (conn)
      {
        if (conn->IsClosed() || conn->ShouldDelete())
        {
          continue;
        }

        if (!FindClientByFd(fd))
        {
          continue;
        }
      }
    }

    if (IsCgiEvent(event_handler))
    {
      HandleCgiEvent(event_handler, ev.events);
      continue;
    }

    try
    {
      event_handler->OnEvent(ev.events);
    }
    catch (const std::exception &e)
    {
      if (!IsServerEvent(event_handler))
      {
        ClientConnection *conn = dynamic_cast<ClientConnection *>(event_handler);
        if (conn && !conn->IsClosed())
        {
          conn->Close();
        }
      }
    }
    catch (...)
    {
      if (!IsServerEvent(event_handler))
      {
        ClientConnection *conn = dynamic_cast<ClientConnection *>(event_handler);
        if (conn && !conn->IsClosed())
        {
          conn->Close();
        }
      }
    }
  }
}

bool EpollHandler::IsCgiEvent(Event *event_handler)
{
  if (event_handler == NULL)
  {
    return false;
  }
  return dynamic_cast<CgiHandler *>(event_handler) != NULL;
}

void EpollHandler::HandleCgiEvent(Event *event_handler, uint32_t events)
{
  if (!event_handler)
  {
    return;
  }

  CgiHandler *cgi = dynamic_cast<CgiHandler *>(event_handler);
  if (!cgi)
  {
    return;
  }

  try
  {
    cgi->OnEvent(events);
  }
  catch (const std::exception &e)
  {
    if (cgi && cgi->isRegisteredToEpoll())
    {
      cgi->setRegistered(false);
    }
  }
  catch (...)
  {
    if (cgi && cgi->isRegisteredToEpoll())
    {
      cgi->setRegistered(false);
    }
  }
}

bool EpollHandler::IsServerEvent(Event *event_handler)
{
  for (size_t j = 0; j < servers_.size(); ++j)
  {
    if (event_handler == servers_[j])
    {
      return true;
    }
  }
  return false;
}

bool EpollHandler::RegisterEvent(Event *event_handler, uint32_t events)
{
  if (event_handler == NULL || !IsEventValid(event_handler))
    return false;

  epoll_event ev;
  ev.events = events | EPOLLRDHUP | EPOLLERR | EPOLLHUP;
  ev.data.ptr = event_handler;
  return epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, event_handler->getFd(), &ev) == 0;
}

bool EpollHandler::UpdateEvent(Event *event_handler, uint32_t events)
{
  if (event_handler == NULL || !IsEventValid(event_handler))
    return false;

  epoll_event ev;
  ev.events = events | EPOLLRDHUP | EPOLLERR | EPOLLHUP;
  ev.data.ptr = event_handler;
  return epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, event_handler->getFd(), &ev) == 0;
}

bool EpollHandler::UnregisterEvent(Event *event_handler)
{
  if (event_handler == NULL || !IsEventValid(event_handler))
    return false;

  return epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, event_handler->getFd(), NULL) == 0;
}

void EpollHandler::AddServer(HttpServer *server) { servers_.push_back(server); }

ClientConnection *EpollHandler::FindClientByFd(int fd)
{
  if (fd < 0)
    return NULL;

  for (size_t i = 0; i < servers_.size(); ++i)
  {
    std::map<int, ClientConnection *> &connections = servers_[i]->GetConnections();
    std::map<int, ClientConnection *>::iterator it = connections.find(fd);
    if (it != connections.end() && it->second && !it->second->IsClosed() && !it->second->ShouldDelete())
    {
      return it->second;
    }
  }
  return NULL;
}

const std::vector<HttpServer *> &EpollHandler::GetServers() const
{
  return servers_;
}

void EpollHandler::InvalidateEvent(Event *event)
{
  if (event)
  {
    invalid_events_.insert(event);
  }
}

bool EpollHandler::IsEventValid(Event *event) const
{
  return event && invalid_events_.find(event) == invalid_events_.end();
}
