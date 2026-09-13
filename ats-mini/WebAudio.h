#ifndef WEB_AUDIO_H
#define WEB_AUDIO_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <vector>

//
// Audio Output Pinout for ATS-Mini (using spare ESP32-S3 GPIOs)
//
#ifndef I2S_BCLK_PIN
#define I2S_BCLK_PIN 11  // Bit Clock (spare GPIO11)
#endif
#ifndef I2S_LRC_PIN
#define I2S_LRC_PIN  12  // Word Select / LR Clock (spare GPIO12)
#endif
#ifndef I2S_DOUT_PIN
#define I2S_DOUT_PIN 13  // Serial Data Out (spare GPIO13)
#endif

#define FAVORITES_FILE_PATH "/favorites.json"

struct FavoriteStation
{
  String name;
  String url;
  String lang;
};

class WebAudioManager
{
public:
  WebAudioManager();

  void init();
  void start();
  void stop();
  void loop();

  void setStation(int idx);
  void nextStation();
  void prevStation();
  void togglePlayPause();
  void setVolume(uint8_t vol); // Maps 0..63 from ATS-mini to 0..21 for Audio

  bool isRunning() const { return _running; }
  bool isPlaying() const { return _playing; }
  bool isConnecting() const { return _connecting; }

  int getCurrentStationIdx() const { return _stationIdx; }
  const char* getCurrentStationName() const;
  const char* getCurrentStationLanguage() const;
  const char* getCurrentStationUrl() const;
  const char* getStreamTitle() const { return _streamTitle; }
  const char* getBitrate() const { return _bitrate; }

  int getTotalStations() const;
  int getWifiRSSI();
  bool isWifiConnected();

  // Dynamic Favorites Management
  bool loadFavorites();
  bool saveFavorites();
  bool addFavorite(const String &name, const String &url, const String &lang = "");
  bool deleteFavorite(int index);
  void setStarterFavorites();
  const std::vector<FavoriteStation>& getFavorites() const { return _favorites; }

  // Internal callback updaters
  void updateStationInfo(const char *info);
  void updateStreamTitle(const char *info);
  void updateBitrate(const char *info);

private:
  bool _initialized;
  bool _running;
  bool _playing;
  bool _connecting;
  int  _stationIdx;
  char _streamTitle[96];
  char _bitrate[16];
  uint32_t _lastConnectAttempt;
  uint32_t _retryTimer;

  std::vector<FavoriteStation> _favorites;
};

extern WebAudioManager webAudio;

#endif // WEB_AUDIO_H
