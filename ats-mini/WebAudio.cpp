#include "WebAudio.h"
#include "Common.h"
#include "Audio.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <esp_wifi.h>
#include "mbedtls/platform.h"

// Custom mbedtls allocator using 8MB Octal PSRAM from Golden "AllIsWell" version
static void* psram_mbedtls_calloc(size_t n, size_t size) {
    void* ptr = heap_caps_calloc(n, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!ptr) {
        ptr = heap_caps_calloc(n, size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    }
    return ptr;
}

static void psram_mbedtls_free(void* ptr) {
    free(ptr);
}

static Audio audio;
static WiFiMulti webWifiMulti;

WebAudioManager webAudio;

WebAudioManager::WebAudioManager()
  : _initialized(false),
    _running(false),
    _playing(false),
    _connecting(false),
    _stationIdx(0),
    _lastConnectAttempt(0),
    _retryTimer(0)
{
  _streamTitle[0] = '\0';
  _bitrate[0] = '\0';
}

void WebAudioManager::init()
{
  if (_initialized) return;

  if (psramFound())
  {
    mbedtls_platform_set_calloc_free(psram_mbedtls_calloc, psram_mbedtls_free);
  }

  // Configure I2S audio output pins (spare GPIOs)
  audio.setPinout(I2S_BCLK_PIN, I2S_LRC_PIN, I2S_DOUT_PIN);
  audio.setVolume(18); // Default volume (0..21)

  // Load user favorites from LittleFS
  loadFavorites();

  // Pre-load known Wi-Fi networks from preferences
  Preferences prefs;
  prefs.begin("network", true, "nvs");
  for (int j = 0; j < 3; j++)
  {
    char nameSSID[16], namePASS[16];
    snprintf(nameSSID, sizeof(nameSSID), "wifissid%d", j + 1);
    snprintf(namePASS, sizeof(namePASS), "wifipass%d", j + 1);
    String ssid = prefs.getString(nameSSID, "");
    String pass = prefs.getString(namePASS, "");
    if (ssid.length() > 0)
    {
      webWifiMulti.addAP(ssid.c_str(), pass.c_str());
    }
  }
  prefs.end();

  // Add default home networks as fallback
  webWifiMulti.addAP("suresh2.4gExt", "alangium");
  webWifiMulti.addAP("suresh", "alangium");

  _initialized = true;
}

void WebAudioManager::setStarterFavorites()
{
  _favorites.clear();
  _favorites.push_back({ "Akashvani Thrissur", "https://airrelay.onrender.com/thrissur.mp3", "Malayalam" });
  _favorites.push_back({ "Vividh Bharati", "https://radio.wavespb.com/live/146ed6ec6dea5a24/146ed6ec6dea5a24.m3u8", "Hindi" });
  _favorites.push_back({ "FM Gold Delhi", "https://airhlspush.pc.cdn.bitgravity.com/httppush/hlspbaudio005/hlspbaudio005_Auto.m3u8", "Hindi" });
  _favorites.push_back({ "FM Rainbow Delhi", "https://airhlspush.pc.cdn.bitgravity.com/httppush/hlspbaudio004/hlspbaudio004_Auto.m3u8", "Hindi" });
  _favorites.push_back({ "Raagam Classical", "https://airhlspush.pc.cdn.bitgravity.com/httppush/hlspbaudioragam/hlspbaudioragam_Auto.m3u8", "Classical" });
  _favorites.push_back({ "AIR Malayalam", "https://radio.wavespb.com/live/b2bc21f834d989f6/b2bc21f834d989f6.m3u8", "Malayalam" });
  _favorites.push_back({ "BBC World Service", "https://stream.live.vc.bbcmedia.co.uk/bbc_world_service", "English" });
}

bool WebAudioManager::loadFavorites()
{
  _favorites.clear();

  if (!LittleFS.exists(FAVORITES_FILE_PATH))
  {
    setStarterFavorites();
    saveFavorites();
    return true;
  }

  File f = LittleFS.open(FAVORITES_FILE_PATH, "r");
  if (!f)
  {
    setStarterFavorites();
    return false;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();

  if (err || !doc.is<JsonArray>())
  {
    setStarterFavorites();
    return false;
  }

  JsonArray arr = doc.as<JsonArray>();
  for (JsonObject obj : arr)
  {
    const char *name = obj["name"] | "";
    const char *url  = obj["url"]  | "";
    const char *lang = obj["lang"] | "";
    if (strlen(name) > 0 && strlen(url) > 0)
    {
      _favorites.push_back({ String(name), String(url), String(lang) });
    }
  }

  if (_favorites.empty())
  {
    setStarterFavorites();
    saveFavorites();
  }
  return true;
}

bool WebAudioManager::saveFavorites()
{
  File f = LittleFS.open(FAVORITES_FILE_PATH, "w");
  if (!f) return false;

  JsonDocument doc;
  JsonArray arr = doc.to<JsonArray>();
  for (const auto &st : _favorites)
  {
    JsonObject obj = arr.add<JsonObject>();
    obj["name"] = st.name;
    obj["url"]  = st.url;
    obj["lang"] = st.lang;
  }

  serializeJson(doc, f);
  f.close();
  return true;
}

bool WebAudioManager::addFavorite(const String &name, const String &url, const String &lang)
{
  if (name.length() == 0 || url.length() == 0) return false;
  _favorites.push_back({ name, url, lang });
  saveFavorites();
  return true;
}

bool WebAudioManager::deleteFavorite(int index)
{
  if (index < 0 || index >= (int)_favorites.size()) return false;
  _favorites.erase(_favorites.begin() + index);
  if (_favorites.empty())
  {
    setStarterFavorites();
  }
  saveFavorites();
  if (_stationIdx >= (int)_favorites.size())
  {
    _stationIdx = _favorites.size() - 1;
  }
  return true;
}

void WebAudioManager::start()
{
  if (!_initialized) init();
  if (_running) return;

  _running = true;
  _streamTitle[0] = '\0';
  _bitrate[0] = '\0';

  // Power on Wi-Fi in Station mode
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false); // Zero-stutter active Wi-Fi

  _connecting = true;
  _lastConnectAttempt = millis();

  // Attempt initial quick connection
  webWifiMulti.run();

  // Start stream
  setStation(_stationIdx);
}

void WebAudioManager::stop()
{
  if (!_running) return;

  _running = false;
  _playing = false;
  _connecting = false;

  // Stop audio decoder
  audio.stopSong();

  // Shut down Wi-Fi to eliminate RF hash on broadcast bands
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  esp_wifi_stop();

  _streamTitle[0] = '\0';
  _bitrate[0] = '\0';
}

void WebAudioManager::loop()
{
  if (!_running) return;

  // Run audio streaming engine
  audio.loop();

  // Monitor Wi-Fi connection
  if (WiFi.status() != WL_CONNECTED)
  {
    _connecting = true;
    if (millis() - _lastConnectAttempt > 4000)
    {
      _lastConnectAttempt = millis();
      webWifiMulti.run();
    }
  }
  else
  {
    if (_connecting)
    {
      _connecting = false;
      // Re-trigger playback if connection just restored
      if (!_playing || !audio.isRunning())
      {
        setStation(_stationIdx);
      }
    }
  }

  // Auto-recovery if stream stopped unexpectedly while running
  if (_running && !_connecting && !audio.isRunning())
  {
    if (_retryTimer == 0)
    {
      _retryTimer = millis();
    }
    else if (millis() - _retryTimer > 3500)
    {
      _retryTimer = 0;
      setStation(_stationIdx);
    }
  }
  else
  {
    _retryTimer = 0;
  }
}

void WebAudioManager::setStation(int idx)
{
  if (_favorites.empty()) loadFavorites();
  if (_favorites.empty()) return;

  if (idx < 0) idx = _favorites.size() - 1;
  if (idx >= (int)_favorites.size()) idx = 0;

  _stationIdx = idx;
  _streamTitle[0] = '\0';
  _bitrate[0] = '\0';

  if (!_running) return;

  audio.stopSong();

  const char *url = _favorites[_stationIdx].url.c_str();
  if (url && strlen(url) > 0)
  {
    audio.connecttohost(url);
    _playing = true;
  }
}

void WebAudioManager::nextStation()
{
  setStation(_stationIdx + 1);
}

void WebAudioManager::prevStation()
{
  setStation(_stationIdx - 1);
}

void WebAudioManager::togglePlayPause()
{
  if (!_running) return;

  if (audio.isRunning())
  {
    audio.pauseResume();
    _playing = audio.isRunning();
  }
  else
  {
    setStation(_stationIdx);
  }
}

void WebAudioManager::setVolume(uint8_t vol)
{
  // ATS-Mini volume: 0..63
  // ESP32-audioI2S volume: 0..21
  uint8_t mappedVol = (vol > 63) ? 21 : ((uint16_t)vol * 21 / 63);
  audio.setVolume(mappedVol);
}

int WebAudioManager::getTotalStations() const
{
  return _favorites.size();
}

const char* WebAudioManager::getCurrentStationName() const
{
  if (_stationIdx >= 0 && _stationIdx < (int)_favorites.size())
  {
    return _favorites[_stationIdx].name.c_str();
  }
  return "No Station";
}

const char* WebAudioManager::getCurrentStationLanguage() const
{
  if (_stationIdx >= 0 && _stationIdx < (int)_favorites.size())
  {
    return _favorites[_stationIdx].lang.c_str();
  }
  return "";
}

const char* WebAudioManager::getCurrentStationUrl() const
{
  if (_stationIdx >= 0 && _stationIdx < (int)_favorites.size())
  {
    return _favorites[_stationIdx].url.c_str();
  }
  return "";
}

int WebAudioManager::getWifiRSSI()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    return WiFi.RSSI();
  }
  return -100;
}

bool WebAudioManager::isWifiConnected()
{
  return (WiFi.status() == WL_CONNECTED);
}

void WebAudioManager::updateStationInfo(const char *info)
{
}

void WebAudioManager::updateStreamTitle(const char *info)
{
  if (info && strlen(info) > 0)
  {
    snprintf(_streamTitle, sizeof(_streamTitle), "%s", info);
  }
}

void WebAudioManager::updateBitrate(const char *info)
{
  if (info && strlen(info) > 0)
  {
    snprintf(_bitrate, sizeof(_bitrate), "%s", info);
  }
}

//
// Global Audio Callbacks for ESP32-audioI2S
//
void audio_showstation(const char *info)
{
  webAudio.updateStationInfo(info);
}

void audio_showstreamtitle(const char *info)
{
  webAudio.updateStreamTitle(info);
}

void audio_bitrate(const char *info)
{
  webAudio.updateBitrate(info);
}

void audio_eof_mp3(const char *info)
{
}
