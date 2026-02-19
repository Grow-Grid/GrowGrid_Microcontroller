#pragma once

class OTAManager {
public:
  OTAManager(const char *hostname, const char *password);

  void begin();
  void handle();

private:
  const char *_hostname;
  const char *_password;

  void registerCallbacks();
};
