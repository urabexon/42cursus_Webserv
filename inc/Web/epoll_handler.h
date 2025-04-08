#ifndef EPOLLHANDLER_HPP
#define EPOLLHANDLER_HPP

#include <sys/epoll.h>
#include <set>

#include "../Web/http_server.h"
#include "../Web/event.h"
#include "../Web/client_connection.h"
#include "../Cgi/cgi_handler.h"

class HttpServer;
class CgiHandler;

class EpollHandler {
 public:
  static EpollHandler& Instance();

  void Init(int maxEvents);
  void RunEventLoop();

  bool RegisterEvent(Event* event_handler, uint32_t events);
  bool UpdateEvent(Event* event_handler, uint32_t events);
  bool UnregisterEvent(Event* event_handler);

  void AddServer(HttpServer* server);

  ClientConnection* FindClientByFd(int fd);
  const std::vector<HttpServer*>& GetServers() const;

  void ScheduleForDeletion(Event* event);
  void PerformDelayedDeletion();

  void InvalidateEvent(Event* event);
  bool IsEventValid(Event* event) const;

 private:
  EpollHandler();
  ~EpollHandler();
  EpollHandler(const EpollHandler&);
  EpollHandler& operator=(const EpollHandler&);

  int epoll_fd_;
  int max_events_;
  std::vector<HttpServer*> servers_;
  static const int EPOLL_TIMEOUT_MS = 100;

  std::set<Event*> to_be_deleted_;
  std::set<Event*> invalid_events_;

  void CleanupConnections();
  void HandleCgiTimeout(std::map<int, ClientConnection*>::iterator& it, std::map<int, ClientConnection*>& connections);
  void CloseAndRemoveConnection(std::map<int, ClientConnection*>::iterator& it, std::map<int, ClientConnection*>& connections);
  void DeleteConnection(std::map<int, ClientConnection*>::iterator& it, std::map<int, ClientConnection*>& connections);
  void ProcessEvents(const std::vector<epoll_event>& events, int nfds);
  bool IsCgiEvent(Event* event_handler);
  void HandleCgiEvent(Event* event_handler, uint32_t events);
  bool IsServerEvent(Event* event_handler);
};

#endif
