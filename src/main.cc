#include <sys/resource.h>
#include <sys/time.h>

#include <cerrno>
#include <chrono>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

#include "http_message.h"
#include "http_server.h"
#include "uri.h"

#include "curl/curl.h"
#include <fstream> 
#include <vector> 
#include <iterator>
#include <filesystem>

using simple_http_server::HttpMethod;
using simple_http_server::HttpRequest;
using simple_http_server::HttpResponse;
using simple_http_server::HttpServer;
using simple_http_server::HttpStatusCode;
using simple_http_server::Uri;

void ensure_enough_resource(int resource, std::uint32_t soft_limit,
                            std::uint32_t hard_limit) {
  rlimit new_limit, old_limit;

  new_limit.rlim_cur = soft_limit;
  new_limit.rlim_max = hard_limit;
  getrlimit(resource, &old_limit);

  std::cout << "Old limit: " << old_limit.rlim_cur << " (soft limit), "
            << old_limit.rlim_cur << " (hard limit)." << std::endl;
  std::cout << "New limit: " << new_limit.rlim_cur << " (soft limit), "
            << new_limit.rlim_cur << " (hard limit)." << std::endl;

  if (setrlimit(resource, &new_limit)) {
    std::cerr << "Warning: Could not update resource limit ";
    std::cerr << "(" << strerror(errno) << ")." << std::endl;
    std::cerr << "Consider setting the limit manually with ulimit" << std::endl;
    exit(-1);
  }
}

int main(void) {
  std::string host = "0.0.0.0";
  int port = 8080;
  HttpServer server(host, port);

  // Register a few endpoints for demo and benchmarking
  auto say_hello = [](const HttpRequest& request) -> HttpResponse {
    HttpResponse response(HttpStatusCode::Ok);
    response.SetHeader("Content-Type", "text/plain");
    response.SetContent("Hello, world\n");
    return response;
  };
 auto downloadOnServer = [](const HttpRequest& request) -> HttpResponse {

    std::string content = request.content();
    const Uri uri(content);

    const std::string name = "annual-enterprise-survey-2021-financial-year-provisional-csv.csv";
    const std::string path = "Download/";
    const std::string local = path + name;
    std::string url = "https://www.stats.govt.nz/assets/Uploads/Annual-enterprise-survey/Annual-enterprise-survey-2021-financial-year-provisional/Download-data/annual-enterprise-survey-2021-financial-year-provisional-csv.csv";

    request.download(url, local);

    HttpResponse response(HttpStatusCode::Ok);

    return response;
  };
  auto downloadOnClient = [](const HttpRequest& request) -> HttpResponse {
    const std::string filePath = "Download/annual-enterprise-survey-2021-financial-year-provisional-csv.csv";

    std::ifstream file(filePath);
    if (!file.is_open()) {
        HttpResponse response(HttpStatusCode::NotFound);
        return response;
    }

    // Чтение содержимого CSV-файла в строку
    std::string fileContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    // Создание HTTP-ответа с содержимым CSV-файла
    HttpResponse response(HttpStatusCode::Ok);
    response.SetContent(fileContent);

    return response;
  };
  // auto downloadOnClient = [](const HttpRequest& request) -> HttpResponse {
  //   // std::string filePath = "Download/annual-enterprise-survey-2021-financial-year-provisional-csv.csv";
    
  //   // std::ifstream file(filePath);
  //   // if (!file.is_open()) {
  //   //     HttpResponse response(HttpStatusCode::NotFound);
  //   //     return response;
  //   // }

  // };
  auto send_html = [](const HttpRequest& request) -> HttpResponse {
    HttpResponse response(HttpStatusCode::Ok);
    std::string content;
    content += "<!doctype html>\n";
    content += "<html>\n<body>\n\n";
    content += "<h1>Hello, world in an Html page</h1>\n";
    content += "<p>A Paragraph</p>\n\n";
    content += "</body>\n</html>\n";

    response.SetHeader("Content-Type", "text/html");
    response.SetContent(content);
    return response;
  };

  server.RegisterHttpRequestHandler("/", HttpMethod::HEAD, say_hello);
  server.RegisterHttpRequestHandler("/", HttpMethod::GET, say_hello);
  server.RegisterHttpRequestHandler("/hello.html", HttpMethod::HEAD, send_html);
  server.RegisterHttpRequestHandler("/hello.html", HttpMethod::GET, send_html);
  server.RegisterHttpRequestHandler("/", HttpMethod::DOWNLOAD, downloadOnServer);
  server.RegisterHttpRequestHandler("/", HttpMethod::SAVEAS, downloadOnClient);

  try {
    // установки для ограничения ресурсов (расскомментируйте при необходимости)

    // std::cout << "Установка новых лимитов для количества файловых дескрипторов.." << std::endl;
    // ensure_enough_resource(RLIMIT_NOFILE, 15000, 15000);

    // std::cout << "Установка новых лимитов для количества потоков.." << std::endl;
    // ensure_enough_resource(RLIMIT_NPROC, 60000, 60000);

    std::cout << "Запуск веб-сервера.." << std::endl;
    server.Start();
    std::cout << "Сервер слушает на " << host << ":" << port << std::endl;

    std::cout << "Введите [quit], чтобы остановить сервер" << std::endl;
    std::string command;
    while (std::cin >> command, command != "quit") {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cout << "Введена команда 'quit'. Остановка веб-сервера.." << std::endl;
    server.Stop();
    std::cout << "Сервер остановлен" << std::endl;
  } catch (std::exception& e) {
    std::cerr << "Произошла ошибка: " << e.what() << std::endl;
    return -1;
  }

  return 0;
}
