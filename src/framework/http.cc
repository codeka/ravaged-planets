#include <functional>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>

#include <framework/http.h>
#include <framework/xml.h>
#include <framework/logging.h>
#include <framework/framework.h>
#include <framework/exception.h>
#include <framework/settings.h>

namespace fw {

static bool g_enable_debug = false;

void http::initialize() {
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

void http::destroy() {
  curl_global_cleanup();
}

http::http() :
    _is_finished(false), _handle(nullptr), _last_error(CURLE_OK), _verb(GET) {
}

http::~http() {
  if (_handle != nullptr) {
    curl_easy_cleanup(_handle);
  }
}

int http::write_debug(CURL *, curl_infotype type, char *buffer, size_t len, void *) {
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
  BOOST_FOREACH(std::string line, lines) {
    std::string trimmed = boost::trim_copy(line);
    if (trimmed != "") {
      debug << "  CURL " << type_name << trimmed << std::endl;
    }
  }
  return 0;
}

size_t http::write_data(void *buffer, size_t size, size_t nmemb, void *userp) {
  char *data = reinterpret_cast<char *>(buffer);
  std::string str(data, size * nmemb);

  http *me = reinterpret_cast<http *>(userp);
  me->_download_data << str;
  return nmemb;
}

std::shared_ptr<http> http::perform(http_verb verb, std::string const &url) {
  std::shared_ptr<http> request(new http());
  request->perform_action(verb, url);
  return request;
}

std::shared_ptr<http> http::perform(http_verb verb, std::string const &url, xml_element &xml) {
  std::shared_ptr<http> request(new http());
  request->perform_action(verb, url, xml);
  return request;
}

std::shared_ptr<http> http::perform(http_verb verb, std::string const &url, std::map<std::string, std::string> const &data) {
  std::shared_ptr<http> request(new http());
  request->perform_action(verb, url, data);
  return request;
}

void http::perform_action(http_verb verb, std::string const &url) {
  _url = url;
  _verb = verb;
  _upload_data.clear();
  _download_data.clear();

  // kick off the post in another thread, TODO: thread pool?
  _thread = std::thread(std::bind(&http::do_action, this));
}

void http::perform_action(http_verb verb, std::string const &url, fw::xml_element &xml) {
  _url = url;
  _verb = verb;
  _headers["Content-Type"] = "text/xml";
  _upload_data = xml.to_string();
  _download_data.clear();

  // kick off the post in another thread, TODO: thread pool?
  _thread = std::thread(std::bind(&http::do_action, this));
}

void http::perform_action(http_verb verb, std::string const &url, std::map<std::string, std::string> const &data) {
  _url = url;
  _verb = verb;
  _headers["Content-Type"] = "application/application/x-www-form-urlencoded";
  //TODO: _upload_data = data();
  _download_data.clear();

  // kick off the post in another thread, TODO: thread pool?
  _thread = std::thread(std::bind(&http::do_action, this));
}

bool http::is_finished() {
  std::unique_lock<std::mutex> lock(_mutex);
  return _is_finished;
}

// waits for the response to be complete. This will block the current thread
void http::wait() {
  // lock the mutex then wait for the condition to become notified
  std::unique_lock<std::mutex> lock(_mutex);
  while (!_is_finished) {
    _finished.wait(lock);
  }
}

std::string http::get_response() {
  if (!is_finished())
    return ""; // TODO: something better than just empty string?

  if (is_error()) {
    BOOST_THROW_EXCEPTION(fw::exception() << fw::message_error_info(get_error_msg()));
  }

  return _download_data.str();
}

// parses the response as XML and returns a reference to it. if no response has
// been received yet, an xml_element pointing to a NULL element is returned.
xml_element http::get_xml_response() {
  std::string response = get_response();
  if (response == "")
    return xml_element();

  return xml_element(response);
}

bool http::is_error() const {
  return (_last_error != CURLE_OK);
}

std::string http::get_error_msg() const {
  if (_last_error == 0)
    return "";

  return curl_easy_strerror(_last_error);
}

void http::check_error(CURLcode error, char const *fn) {
  if (error != CURLE_OK) {
    _last_error = error;
    debug << boost::format("WARN: error returned from libcurl call: %1%") % fn << std::endl;
    debug << boost::format("  [%1%] %2%") % _last_error % curl_easy_strerror(_last_error) << std::endl;
  }
}

#define CHECK(fn) \
  check_error(fn, #fn)

// once upload_data has been set, uploads the data
void http::do_action() {
  _handle = curl_easy_init();
  if (_handle == 0) {
    debug << "ERROR: Could not create curl handle, cannot post data!" << std::endl;
    _last_error = CURLE_FAILED_INIT;
  } else {
    if (g_enable_debug) {
      debug << boost::format("CURL begin: %1%") % _url << std::endl;
      CHECK(curl_easy_setopt(_handle, CURLOPT_VERBOSE, 1));
    }
    CHECK(curl_easy_setopt(_handle, CURLOPT_DEBUGFUNCTION, &write_debug));
    CHECK(curl_easy_setopt(_handle, CURLOPT_DEBUGDATA, this));
    CHECK(curl_easy_setopt(_handle, CURLOPT_WRITEFUNCTION, &write_data));
    CHECK(curl_easy_setopt(_handle, CURLOPT_WRITEDATA, this));
    CHECK(curl_easy_setopt(_handle, CURLOPT_URL, _url.c_str()));

    curl_slist *headers = nullptr;
    std::pair<std::string, std::string> header;
    BOOST_FOREACH(header, _headers) {
      headers = curl_slist_append(headers, (header.first + ": " + header.second).c_str());
    }

    switch (_verb) {
    case POST:
      CHECK(curl_easy_setopt(_handle, CURLOPT_POST, 1));
      CHECK(curl_easy_setopt(_handle, CURLOPT_POSTFIELDS, &_upload_data[0]));
      break;
    case PUT:
      CHECK(curl_easy_setopt(_handle, CURLOPT_PUT, 1));
      if (!_upload_data.empty())
        CHECK(curl_easy_setopt(_handle, CURLOPT_POSTFIELDS, &_upload_data[0]));
      break;
    case DELETE:
      CHECK(curl_easy_setopt(_handle, CURLOPT_CUSTOMREQUEST, "DELETE"));
      if (!_upload_data.empty())
        CHECK(curl_easy_setopt(_handle, CURLOPT_POSTFIELDS, &_upload_data[0]));
      break;
    case GET:
      CHECK(curl_easy_setopt(_handle, CURLOPT_HTTPGET, 1));
      break;
    }

    CHECK(curl_easy_setopt(_handle, CURLOPT_HTTPHEADER, headers));

    CHECK(curl_easy_perform(_handle));
    curl_slist_free_all(headers);
  }

  std::unique_lock<std::mutex> lock(_mutex);
  _is_finished = true;
  _finished.notify_all();
}

}
