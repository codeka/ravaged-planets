#include <functional>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#include <framework/http.h>
#include <framework/xml.h>
#include <framework/logging.h>
#include <framework/framework.h>
#include <framework/exception.h>
#include <framework/settings.h>

namespace fw {

static bool g_enable_debug = false;

void Http::initialize() {
  curl_global_init(0);

  curl_version_info_data *curl_version = curl_version_info(CURLVERSION_NOW);
  if (curl_version->age >= 0) {
    debug << boost::format("libcurl (%1%) initialized") % curl_version->version << std::endl;
  }

#if defined(_DEBUG)
  // debug mode always has debugging on...
  g_enable_debug = true;
#else
  fw::settings stg;
  g_enable_debug = stg.get_value<bool>("debug-libcurl");
#endif
}

void Http::destroy() {
  curl_global_cleanup();
}

Http::Http() :
    is_finished_(false), handle_(nullptr), last_error_(CURLE_OK), verb_(GET) {
}

Http::~Http() {
  if (handle_ != nullptr) {
    curl_easy_cleanup(handle_);
  }
}

int Http::write_debug(CURL *, curl_infotype type, char *buffer, size_t len, void *) {
  if (!g_enable_debug)
    return 0;

  std::string msg(buffer, len);

  std::string type_name;
  switch (type) {
  case CURLINFO_TEXT:
    type_name = "DEBUG : ";
    break;
  case CURLINFO_HEADER_IN:
    type_name = "HEDR << ";
    break;
  case CURLINFO_HEADER_OUT:
    type_name = "HEDR >> ";
    break;
  case CURLINFO_DATA_IN:
    type_name = "DATA << ";
    break;
  case CURLINFO_DATA_OUT:
    type_name = "DATA >> ";
    break;
  default:
    type_name = "UNKNOWN ";
  }

  std::vector<std::string> lines;
  boost::split(lines, msg, boost::algorithm::is_any_of("\r\n"), boost::algorithm::token_compress_on);
  for(std::string line : lines) {
    std::string trimmed = boost::trim_copy(line);
    if (trimmed != "") {
      debug << "  CURL " << type_name << trimmed << std::endl;
    }
  }
  return 0;
}

size_t Http::write_data(void *buffer, size_t size, size_t nmemb, void *userp) {
  char *data = reinterpret_cast<char *>(buffer);
  std::string str(data, size * nmemb);

  Http *me = reinterpret_cast<Http *>(userp);
  me->download_data_ << str;
  return nmemb;
}

std::shared_ptr<Http> Http::perform(HttpVerb verb, std::string const &url) {
  std::shared_ptr<Http> request(new Http());
  request->perform_action(verb, url);
  return request;
}

std::shared_ptr<Http> Http::perform(HttpVerb verb, std::string const &url, XmlElement &xml) {
  std::shared_ptr<Http> request(new Http());
  request->perform_action(verb, url, xml);
  return request;
}

std::shared_ptr<Http> Http::perform(HttpVerb verb, std::string const &url, std::map<std::string, std::string> const &data) {
  std::shared_ptr<Http> request(new Http());
  request->perform_action(verb, url, data);
  return request;
}

void Http::perform_action(HttpVerb verb, std::string const &url) {
  url_ = url;
  verb_ = verb;
  upload_data_.clear();
  download_data_.clear();

  // kick off the post in another thread, TODO: thread pool?
  thread_ = std::thread(std::bind(&Http::do_action, this));
}

void Http::perform_action(HttpVerb verb, std::string const &url, fw::XmlElement &xml) {
  url_ = url;
  verb_ = verb;
  headers_["Content-Type"] = "text/xml";
  upload_data_ = xml.to_string();
  download_data_.clear();

  // kick off the post in another thread, TODO: thread pool?
  thread_ = std::thread(std::bind(&Http::do_action, this));
}

void Http::perform_action(HttpVerb verb, std::string const &url, std::map<std::string, std::string> const &data) {
  url_ = url;
  verb_ = verb;
  headers_["Content-Type"] = "application/application/x-www-form-urlencoded";
  //TODO: upload_data_ = data();
  download_data_.clear();

  // kick off the post in another thread, TODO: thread pool?
  thread_ = std::thread(std::bind(&Http::do_action, this));
}

bool Http::is_finished() {
  std::unique_lock<std::mutex> lock(mutex_);
  return is_finished_;
}

// waits for the response to be complete. This will block the current thread
void Http::wait() {
  // lock the mutex then wait for the condition to become notified
  std::unique_lock<std::mutex> lock(mutex_);
  while (!is_finished_) {
    finished_.wait(lock);
  }
}

std::string Http::get_response() {
  if (!is_finished())
    return ""; // TODO: something better than just empty string?

  if (is_error()) {
    BOOST_THROW_EXCEPTION(fw::Exception() << fw::message_error_info(get_error_msg()));
  }

  return download_data_.str();
}

// parses the response as XML and returns a reference to it. if no response has
// been received yet, an XmlElement pointing to a NULL element is returned.
XmlElement Http::get_xml_response() {
  std::string response = get_response();
  if (response == "")
    return XmlElement();

  return XmlElement(response);
}

bool Http::is_error() const {
  return (last_error_ != CURLE_OK);
}

std::string Http::get_error_msg() const {
  if (last_error_ == 0)
    return "";

  return curl_easy_strerror(last_error_);
}

void Http::check_error(CURLcode error, char const *fn) {
  if (error != CURLE_OK) {
    last_error_ = error;
    debug << boost::format("WARN: error returned from libcurl call: %1%") % fn << std::endl;
    debug << boost::format("  [%1%] %2%") % last_error_ % curl_easy_strerror(last_error_) << std::endl;
  }
}

#define CHECK(fn) \
  check_error(fn, #fn)

// once upload_data has been set, uploads the data
void Http::do_action() {
  handle_ = curl_easy_init();
  if (handle_ == 0) {
    debug << "ERROR: Could not create curl handle, cannot post data!" << std::endl;
    last_error_ = CURLE_FAILED_INIT;
  } else {
    if (g_enable_debug) {
      debug << boost::format("CURL begin: %1%") % url_ << std::endl;
      CHECK(curl_easy_setopt(handle_, CURLOPT_VERBOSE, 1));
    }
    CHECK(curl_easy_setopt(handle_, CURLOPT_DEBUGFUNCTION, &write_debug));
    CHECK(curl_easy_setopt(handle_, CURLOPT_DEBUGDATA, this));
    CHECK(curl_easy_setopt(handle_, CURLOPT_WRITEFUNCTION, &write_data));
    CHECK(curl_easy_setopt(handle_, CURLOPT_WRITEDATA, this));
    CHECK(curl_easy_setopt(handle_, CURLOPT_URL, url_.c_str()));

    curl_slist *headers = nullptr;
    std::pair<std::string, std::string> header;
    for(auto& header : headers_) {
      headers = curl_slist_append(headers, (header.first + ": " + header.second).c_str());
    }

    switch (verb_) {
    case POST:
      CHECK(curl_easy_setopt(handle_, CURLOPT_POST, 1));
      CHECK(curl_easy_setopt(handle_, CURLOPT_POSTFIELDS, &upload_data_[0]));
      break;
    case PUT:
      CHECK(curl_easy_setopt(handle_, CURLOPT_PUT, 1));
      if (!upload_data_.empty())
        CHECK(curl_easy_setopt(handle_, CURLOPT_POSTFIELDS, &upload_data_[0]));
      break;
    case DELETE:
      CHECK(curl_easy_setopt(handle_, CURLOPT_CUSTOMREQUEST, "DELETE"));
      if (!upload_data_.empty())
        CHECK(curl_easy_setopt(handle_, CURLOPT_POSTFIELDS, &upload_data_[0]));
      break;
    case GET:
      CHECK(curl_easy_setopt(handle_, CURLOPT_HTTPGET, 1));
      break;
    }

    CHECK(curl_easy_setopt(handle_, CURLOPT_HTTPHEADER, headers));

    CHECK(curl_easy_perform(handle_));
    curl_slist_free_all(headers);
  }

  std::unique_lock<std::mutex> lock(mutex_);
  is_finished_ = true;
  finished_.notify_all();
}

}
