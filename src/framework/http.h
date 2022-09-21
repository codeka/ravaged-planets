#pragma once

#include <condition_variable>
#include <map>
#include <mutex>
#include <sstream>
#include <thread>
#include <curl/curl.h>

#include <framework/xml.h>

// this is defined in winnt.h... silly!!
#ifdef DELETE
#undef DELETE
#endif

namespace fw {

/**
 * Represents a single HTTP request/response. Use the \ref Http::perform() methods to initiate a request, then use
 * \ref Http::wait() to wait for it to complete.
 */
class Http {
public:
  enum HttpVerb {
    POST, PUT, DELETE, GET
  };

private:
  CURL *handle_;
  std::string url_;
  HttpVerb verb_;
  std::map<std::string, std::string> headers_;
  std::string upload_data_;
  std::ostringstream download_data_;
  std::mutex mutex_;
  std::condition_variable finished_;
  bool is_finished_;
  CURLcode last_error_;
  std::thread thread_;

  // constructor is called by the \ref perform() static function.
  Http();

  // this is called by libcurl for debugging
  static int write_debug(CURL *handle, curl_infotype type, char *buffer, size_t len, void *userp);

  // this is called by libcurl when data is received/sent
  static size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp);

  // this is called to check an error code. There's a macro in Http.cpp that'll automatically
  // populate the parameters for us
  void check_error(CURLcode err, char const *fn);

  // once upload_data has been set, uploads the data
  void do_action();
public:
  // this is called automatically by the Framework to initialize cURL
  static void initialize();
  static void destroy();

  ~Http();

  /** Constructs a new Http, performs the specified verb on the specified URL. */
  static std::shared_ptr<Http> perform(HttpVerb verb, std::string const &url);

  /* Constructs a new Http, performs the specified verb on the specified URL (with the specified XML data). */
  static std::shared_ptr<Http> perform(HttpVerb verb, std::string const &url, XmlElement &xml);

  /** Constructs a new Http, performs the specified verb on the specified URL (with the specified name/value data). */
  static std::shared_ptr<Http> perform(HttpVerb verb, std::string const &url,
      std::map<std::string, std::string> const &data);

  /** Perform the given HTTP on the given URL. Cannot be called while a request is already in progress. */
  void perform_action(HttpVerb verb, std::string const &url);

  /** Perform the given HTTP on the given URL. Cannot be called while a request is already in progress. */
  void perform_action(HttpVerb verb, std::string const &url, XmlElement &xml);

  /** Perform the given HTTP on the given URL. Cannot be called while a request is already in progress. */
  void perform_action(HttpVerb verb, std::string const &url, std::map<std::string, std::string> const &data);

  /** Gets a value which indicates whether we've finished downloading the response. */
  bool is_finished();

  /** Waits for the response to be complete. This will block the current thread. */
  void wait();

  /** Gets the response we got from the server. If no response has been received yet, an empty string is returned. */
  std::string get_response();

  /**
   * Parses the response as XML and returns a reference to it. if no response has been received yet, an XmlElement
   * pointing to a NULL element is returned.
   */
  XmlElement get_xml_response();

  /** Returns a value which indicates whether there was an error making the request. */
  bool is_error() const;

  /** If there was an error making the request, returns a string indicating what the error was. */
  std::string get_error_msg() const;
};

}
