
#include "utility.h"
#include "rtc_base/ssl_adapter.h"
#include "rtc_base/logging.h"

namespace rigel {

void InitializeLogger() {
  // specify `LS_NONE` to silence console output
  rtc::LogMessage::LogToDebug(rtc::LS_INFO);
}

void InitializeSSL() {
  rtc::InitializeSSL();
}

void CleanupSSL() {
  rtc::CleanupSSL();
}

}  // namespace rigel
