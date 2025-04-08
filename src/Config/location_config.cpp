#include "../../inc/Config/location_config.h"


LocationConfig::LocationConfig()
    : path_(""),
      redirect_("", -1),
      script_filename_(""),
      cgi_read_timeout_(60000),
      keepalive_timeout_(-1),
      keepalive_timeout_set_(false)
{
    accepted_methods_.push_back("GET");
    accepted_methods_.push_back("POST");
    accepted_methods_.push_back("DELETE");
}

LocationConfig::LocationConfig(const LocationConfig &other)
    : BaseConfig(other),
      path_(other.path_),
      accepted_methods_(other.accepted_methods_),
      redirect_(other.redirect_),
      script_filename_(other.script_filename_),
      cgi_executors_(other.cgi_executors_),
      cgi_read_timeout_(other.cgi_read_timeout_),
      upload_path_(other.upload_path_),
      keepalive_timeout_(other.keepalive_timeout_),
      keepalive_timeout_set_(other.keepalive_timeout_set_)
{
}

LocationConfig::LocationConfig(ServerConfig *server_config)
    : BaseConfig(*server_config),
      path_(""),
      redirect_(server_config->GetRedirect()),
      script_filename_(""),
      cgi_read_timeout_(60000),
      keepalive_timeout_(server_config->GetKeepaliveTimeout()),
      keepalive_timeout_set_(false)
{
    accepted_methods_.push_back("GET");
    accepted_methods_.push_back("POST");
    accepted_methods_.push_back("DELETE");
    autoindex_set_ = false;
}

LocationConfig::~LocationConfig() {}

void LocationConfig::SetPath(const std::string &path)
{
    path_ = path;
}

const std::string &LocationConfig::GetPath() const
{
    return path_;
}

void LocationConfig::AddAcceptedMethod(const std::string &method)
{
    if (std::find(accepted_methods_.begin(), accepted_methods_.end(), method) == accepted_methods_.end())
        accepted_methods_.push_back(method);
}

void LocationConfig::ClearAcceptedMethods()
{
    accepted_methods_.clear();
}

const std::vector<std::string> &LocationConfig::GetAcceptedMethods() const
{
    return accepted_methods_;
}

void LocationConfig::SetRedirect(const std::string &url, int code)
{
    redirect_ = std::make_pair(url, code);
}

const std::pair<std::string, int> &LocationConfig::GetRedirect() const
{
    return redirect_;
}

void LocationConfig::AddCgiExecutor(const std::string &extension, const std::string &executor)
{
    cgi_executors_[extension] = executor;
}

const std::map<std::string, std::string> &LocationConfig::GetCgiExecutors() const
{
    return cgi_executors_;
}

std::string LocationConfig::GetCgiExecutor(const std::string &extension) const
{
    std::map<std::string, std::string>::const_iterator it = cgi_executors_.find(extension);
    if (it != cgi_executors_.end())
    {
        return it->second;
    }
    return "";
}

void LocationConfig::SetScriptFilename(const std::string &filename)
{
    script_filename_ = filename;
}

const std::string &LocationConfig::GetScriptFilename() const
{
    return script_filename_;
}

void LocationConfig::SetCgiReadTimeout(int timeout)
{
    cgi_read_timeout_ = timeout;
}

int LocationConfig::GetCgiReadTimeout() const
{
    return cgi_read_timeout_;
}

void LocationConfig::SetUploadPath(const std::string &path)
{
    upload_path_ = path;
}

const std::string &LocationConfig::GetUploadPath() const
{
    return upload_path_;
}

void LocationConfig::SetKeepaliveTimeout(time_t timeout)
{
    keepalive_timeout_ = timeout;
    keepalive_timeout_set_ = true;
}

time_t LocationConfig::GetKeepaliveTimeout() const
{
    return keepalive_timeout_;
}

bool LocationConfig::IsKeepaliveTimeoutSet() const
{
    return keepalive_timeout_set_;
}
