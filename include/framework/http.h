//
// Copyright (c) 2008-2011, Dean Harding. All rights reserved.
//
#pragma once

#include <curl/curl.h>

// this is defined in winnt.h... silly!!
#ifdef DELETE
#undef DELETE
#endif

namespace fw {
	class xml_element;

	// this is a helper class that wraps the functionality of libcurl into
	// something that's a bit simpler for the way we work.
	class http
	{
	public:
		enum http_verb
		{
			POST,
			PUT,
			DELETE,
			GET
		};

	private:
		CURL *_handle;
		std::string _url;
		http_verb _verb;
		std::map< std::string, std::string> _headers;
		std::string _upload_data;
		std::ostringstream _download_data;
		boost::mutex _mutex;
		boost::condition_variable _finished;
		bool _is_finished;
		CURLcode _last_error;
		boost::thread _thread;

		// constructor is called by the post_xml() static function.
		http();

		// this is called by libcurl for debugging
		static int write_debug(CURL *handle, curl_infotype type, char *buffer, size_t len, void *userp);

		// this is called by libcurl when data is received/sent
		static size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp);

		// this is called to check an error code. There's a macro in http.cpp that'll automatically
		// populate the parameters for us
		void check_error(CURLcode err, char const *fn);

		// once upload_data has been set, uploads the data
		void do_action();
	public:
		// this is called automatically by the framework to initialise cURL
		static void initialise();
		static void destroy();

		~http();

		// constructs a new http, performs the specified verb on the specified URL (with the specified data)
		static shared_ptr<http> perform(http_verb verb, std::string const &url);

		// constructs a new http, performs the specified verb on the specified URL (with the specified data)
		static shared_ptr<http> perform(http_verb verb, std::string const &url, xml_element &xml);

		// constructs a new http, performs the specified verb on the specified URL (with the specified name/value data)
		static shared_ptr<http> perform(http_verb verb, std::string const &url, std::map<std::string, std::string> const &data);

		void perform_action(http_verb verb, std::string const &url);
		void perform_action(http_verb verb, std::string const &url, xml_element &xml);
		void perform_action(http_verb verb, std::string const &url, std::map<std::string, std::string> const &data);

		// gets a value which indicates whether we've finished downloading the response.
		bool is_finished();

		// waits for the response to be complete. This will block the current thread
		void wait();

		// gets the response we got from the server. if no response has been received
		// yet, an empty string is returned.
		std::string get_response();

		// parses the response as XML and returns a reference to it. if no response has
		// been received yet, an xml_element pointing to a NULL element is returned.
		xml_element get_xml_response();

		// error handling, rather than throw exceptions from get_response (which is what
		// would happen if you call get_response on a connection that's in error), you
		// can call these to determine ahead of time if there was an error.
		bool is_error() const;
		std::string get_error_msg() const;
	};

}