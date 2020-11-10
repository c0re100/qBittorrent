#pragma once

#include <regex>

#include <libtorrent/torrent_info.hpp>

#include "base/net/geoipmanager.h"

#include "peer_filter_plugin.hpp"
#include "peer_logger.hpp"

/*
// bad peer
bool isBadPeer(const lt::peer_info& info)
{
  QString pid = QString::fromStdString(info.pid.to_string().substr(0, 8));
  QString client = QString::fromStdString(info.client);
  QRegExp IDFilter("-(XL|SD|XF|QD|BN|DL)(\\d+)-");
  QRegExp UAFilter(R"((\d+.\d+.\d+.\d+|cacao_torrent))");
  return IDFilter.exactMatch(pid) || UAFilter.exactMatch(client);
}

// Unknown Peer
bool isUnknownPeer(const lt::peer_info& info)
{
  QString client = QString::fromStdString(info.client);
  QString country = Net::GeoIPManager::instance()->lookup({info.ip.data(), info.ip.port()});
  return client.contains("Unknown") && country == "CN";
}

// Offline Downloader
bool isOfflineDownloader(const lt::peer_info& info)
{
  unsigned short port = info.ip.port();
  QString country = Net::GeoIPManager::instance()->lookup({info.ip.data(), info.ip.port()});
  QString client = QString::fromStdString(info.client);
  return port >= 65000 && country == "CN" && client.contains("Transmission");
}

// BitTorrent Media Player Peer
bool isBitTorrentMediaPlayer(const lt::peer_info& info)
{
  QString pid = QString::fromStdString(info.pid.to_string().substr(0, 8));
  QRegExp PlayerFilter("-(UW\\w{4})-");
  return PlayerFilter.exactMatch(pid);
}
*/

// bad peer filter
bool is_bad_peer(const lt::peer_info& info)
{
  std::string pid = info.pid.to_string().substr(0, 8);
  std::regex id_filter("-(XL|SD|XF|QD|BN|DL)(\\d+)-");
  std::regex ua_filter(R"((\d+.\d+.\d+.\d+|cacao_torrent))");
  return std::regex_match(pid, id_filter) || std::regex_match(info.client, ua_filter);
}

// Unknown Peer filter
bool is_unknown_peer(const lt::peer_info& info)
{
  QString country = Net::GeoIPManager::instance()->lookup(QHostAddress(info.ip.data()));
  return info.client.find("Unknown") != std::string::npos && country == QLatin1String("CN");
}

// Offline Downloader filter
bool is_offline_downloader(const lt::peer_info& info)
{
  unsigned short port = info.ip.port();
  QString country = Net::GeoIPManager::instance()->lookup(QHostAddress(info.ip.data()));
  return port >= 65000 && country == QLatin1String("CN") && info.client.find("Transmission") != std::string::npos;
}

// BitTorrent Media Player Peer filter
bool is_bittorrent_media_player(const lt::peer_info& info)
{
  std::string pid = info.pid.to_string().substr(0, 8);
  std::regex player_filter("-(UW\\w{4})-");
  return !!std::regex_match(pid, player_filter);
}


// drop connection action
void drop_connection(lt::peer_connection_handle ph)
{
  ph.disconnect(boost::asio::error::connection_refused, lt::operation_t::bittorrent, 0);
}


class peer_logger_singleton
{
public:
  static peer_logger_singleton& instance()
  {
    static peer_logger_singleton logger;
    return logger;
  }

  void log_peer(const lt::peer_info& info, const std::string& tag)
  {
    m_logger.log_peer(info, tag);
  }

protected:
  peer_logger_singleton()
    : m_logger(db_connection::instance().connection(), QStringLiteral("banned_peers"))
  {}

private:
  peer_logger m_logger;
};


template<typename F>
auto wrap_filter(F filter, const lt::torrent_handle& th, const std::string& tag)
{
  return [=](const lt::peer_info& info, bool) {
    // ignore private torrents
    if (th.torrent_file() && th.torrent_file()->priv())
      return false;

    bool matched = filter(info);
    if (matched)
      peer_logger_singleton::instance().log_peer(info, tag);
    return matched;
  };
}


// plugins factory functions

std::shared_ptr<lt::torrent_plugin> create_drop_bad_peers_plugin(lt::torrent_handle const& th, void* d)
{
  return peer_action_plugin_creator(wrap_filter(is_bad_peer, th, "bad peer"), drop_connection)(th, d);
}

std::shared_ptr<lt::torrent_plugin> create_drop_unknown_peers_plugin(lt::torrent_handle const& th, void* d)
{
  return peer_action_plugin_creator(wrap_filter(is_unknown_peer, th, "unknown peer"), drop_connection)(th, d);
}

std::shared_ptr<lt::torrent_plugin> create_drop_offline_downloader_plugin(lt::torrent_handle const& th, void* d)
{
  return peer_action_plugin_creator(wrap_filter(is_offline_downloader, th, "offline downloader"), drop_connection)(th, d);
}

std::shared_ptr<lt::torrent_plugin> create_drop_bittorrent_media_player_plugin(lt::torrent_handle const& th, void* d)
{
  return peer_action_plugin_creator(wrap_filter(is_bittorrent_media_player, th, "bittorrent media player"), drop_connection)(th, d);
}
