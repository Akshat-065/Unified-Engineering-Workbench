#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <winhttp.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <array>
#include <string>

namespace {

std::wstring executable_directory() {
  std::array<wchar_t, 32768> buffer{};
  const DWORD length = GetModuleFileNameW(nullptr, buffer.data(),
                                          static_cast<DWORD>(buffer.size()));
  if (length == 0 || length == buffer.size()) return {};
  std::wstring path(buffer.data(), length);
  const auto separator = path.find_last_of(L"\\/");
  if (separator == std::wstring::npos) return {};
  path.resize(separator);
  return path;
}

int choose_local_port() {
  WSADATA data{};
  if (WSAStartup(MAKEWORD(2, 2), &data) != 0) return 0;
  SOCKET socket_handle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (socket_handle == INVALID_SOCKET) {
    WSACleanup();
    return 0;
  }
  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  address.sin_port = 0;
  int port = 0;
  if (bind(socket_handle, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0) {
    int address_length = sizeof(address);
    if (getsockname(socket_handle, reinterpret_cast<sockaddr*>(&address),
                    &address_length) == 0) {
      port = ntohs(address.sin_port);
    }
  }
  closesocket(socket_handle);
  WSACleanup();
  return port;
}

bool health_ready(int port) {
  HINTERNET session = WinHttpOpen(L"UnifiedEngineeringWorkbench/1.0",
                                  WINHTTP_ACCESS_TYPE_NO_PROXY,
                                  WINHTTP_NO_PROXY_NAME,
                                  WINHTTP_NO_PROXY_BYPASS, 0);
  if (!session) return false;
  WinHttpSetTimeouts(session, 500, 500, 500, 500);
  HINTERNET connection = WinHttpConnect(session, L"127.0.0.1",
                                         static_cast<INTERNET_PORT>(port), 0);
  HINTERNET request = connection
      ? WinHttpOpenRequest(connection, L"GET", L"/api/health", nullptr,
                           WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0)
      : nullptr;
  const bool sent = request && WinHttpSendRequest(
      request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
      WinHttpReceiveResponse(request, nullptr);
  DWORD status = 0;
  DWORD status_size = sizeof(status);
  const bool health_ok = sent && WinHttpQueryHeaders(
      request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
      WINHTTP_HEADER_NAME_BY_INDEX, &status, &status_size,
      WINHTTP_NO_HEADER_INDEX) && status == 200;
  if (request) WinHttpCloseHandle(request);
  HINTERNET root_request = connection
      ? WinHttpOpenRequest(connection, L"GET", L"/", nullptr,
                           WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0)
      : nullptr;
  const bool root_sent = root_request && WinHttpSendRequest(
      root_request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
      WinHttpReceiveResponse(root_request, nullptr);
  DWORD root_status = 0;
  DWORD root_status_size = sizeof(root_status);
  const bool root_ok = root_sent && WinHttpQueryHeaders(
      root_request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
      WINHTTP_HEADER_NAME_BY_INDEX, &root_status, &root_status_size,
      WINHTTP_NO_HEADER_INDEX) && root_status == 200;
  if (root_request) WinHttpCloseHandle(root_request);
  if (connection) WinHttpCloseHandle(connection);
  WinHttpCloseHandle(session);
  return health_ok && root_ok;
}

void show_error(const wchar_t* text) {
  MessageBoxW(nullptr, text, L"Unified Engineering Workbench",
              MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
}

}  // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
  const std::wstring root = executable_directory();
  if (root.empty()) {
    show_error(L"Could not locate the extracted application folder.");
    return 1;
  }
  const std::wstring backend = root + L"\\backend\\workbench_server.exe";
  const std::wstring frontend = root + L"\\frontend\\dist";
  const std::wstring backend_check = root + L"\\backend\\workbench_server.exe";
  const std::wstring index = frontend + L"\\index.html";
  if (GetFileAttributesW(backend_check.c_str()) == INVALID_FILE_ATTRIBUTES ||
      GetFileAttributesW(index.c_str()) == INVALID_FILE_ATTRIBUTES) {
    show_error(L"The package is incomplete. Extract the full ZIP and keep its folder structure.");
    return 1;
  }
  const int port = choose_local_port();
  if (port == 0) {
    show_error(L"Could not reserve a local port for the Workbench service.");
    return 1;
  }

  const std::wstring port_value = std::to_wstring(port);
  if (!SetEnvironmentVariableW(L"WORKBENCH_PORT", port_value.c_str()) ||
      !SetEnvironmentVariableW(L"WORKBENCH_WEB_ROOT", frontend.c_str())) {
    show_error(L"Could not configure the local Workbench service environment.");
    return 1;
  }
  HANDLE job = CreateJobObjectW(nullptr, nullptr);
  if (!job) {
    show_error(L"Could not create the Workbench service lifetime controller.");
    return 1;
  }
  JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
  limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
  if (!SetInformationJobObject(job, JobObjectExtendedLimitInformation,
                               &limits, sizeof(limits))) {
    CloseHandle(job);
    show_error(L"Could not configure the Workbench service lifetime controller.");
    return 1;
  }

  STARTUPINFOW startup{};
  startup.cb = sizeof(startup);
  startup.dwFlags = STARTF_USESHOWWINDOW;
  startup.wShowWindow = SW_HIDE;
  PROCESS_INFORMATION process{};
  std::wstring command = L"\"" + backend + L"\"";
  if (!CreateProcessW(backend.c_str(), command.data(), nullptr, nullptr, FALSE,
                      CREATE_NO_WINDOW, nullptr, root.c_str(), &startup, &process)) {
    CloseHandle(job);
    show_error(L"The bundled backend could not be started. The extracted package may be incomplete.");
    return 1;
  }
  CloseHandle(process.hThread);
  if (!AssignProcessToJobObject(job, process.hProcess)) {
    TerminateProcess(process.hProcess, 1);
    CloseHandle(process.hProcess);
    CloseHandle(job);
    show_error(L"Could not control the bundled backend process.");
    return 1;
  }

  bool ready = false;
  for (int attempt = 0; attempt < 60; ++attempt) {
    if (WaitForSingleObject(process.hProcess, 100) == WAIT_OBJECT_0) break;
    if (health_ready(port)) {
      ready = true;
      break;
    }
  }
  if (!ready) {
    CloseHandle(process.hProcess);
    CloseHandle(job);  // Stops any server that is still starting.
    show_error(L"The Workbench backend did not become ready. No browser was opened.");
    return 1;
  }

  std::array<wchar_t, 8> self_test{};
  const DWORD self_test_length = GetEnvironmentVariableW(
      L"UEW_LAUNCHER_SELF_TEST", self_test.data(),
      static_cast<DWORD>(self_test.size()));
  if (self_test_length == 1 && self_test[0] == L'1') {
    CloseHandle(process.hProcess);
    CloseHandle(job);  // Stops the health-checked service before exiting.
    return 0;
  }

  const std::wstring url = L"http://127.0.0.1:" + port_value + L"/";
  const auto browser_result = reinterpret_cast<INT_PTR>(
      ShellExecuteW(nullptr, L"open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL));
  if (browser_result <= 32) {
    CloseHandle(process.hProcess);
    CloseHandle(job);
    show_error(L"The backend is ready, but Windows could not open the default browser.");
    return 1;
  }

  MessageBoxW(nullptr,
      L"The Workbench is open in your default browser. Keep this small status window open while you use it.\n\nClick OK to stop the bundled backend and close this Workbench session.",
      L"Unified Engineering Workbench is running",
      MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND);
  CloseHandle(process.hProcess);
  CloseHandle(job);  // JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE ends the service cleanly.
  return 0;
}
