#pragma once

#include <libtorrent/peer_info.hpp>

#include <QSqlDatabase>
#include <QSqlQuery>


class db_connection
{
public:
  static db_connection& instance()
  {
    static db_connection c;
    return c;
  }

  void init(const QString& db_path)
  {
    m_db.setDatabaseName(db_path);
    m_db.open();
  }

  QSqlDatabase connection() const
  {
    return m_db;
  }

protected:
  db_connection()
    : m_db(QSqlDatabase::addDatabase("QSQLITE"))
  {}

  ~db_connection()
  {
    m_db.close();
  }

private:
  QSqlDatabase m_db;
};


class peer_logger
{
public:
  explicit peer_logger(QSqlDatabase db, QString table)
    : m_db(db)
    , m_table(table)
  {
    if (!db.tables().contains(table)) {
      db.exec(QString(
                "CREATE TABLE '%1' ("
                "    'id'      INTEGER PRIMARY KEY,"
                "    'ip'      TEXT NOT NULL UNIQUE,"
                "    'client'  TEXT NOT NULL,"
                "    'pid'     BLOB NOT NULL,"
                "    'tag'     TEXT"
                ");").arg(table));
      db.commit();
    }
  }

  bool log_peer(const lt::peer_info& info, const std::string& tag = {})
  {
    QSqlQuery q(m_db);
    q.prepare(QString("INSERT INTO '%1' (ip, client, pid, tag) VALUES (?, ?, ?, ?)").arg(m_table));
    q.addBindValue(QString::fromStdString(info.ip.address().to_string()));
    q.addBindValue(QString::fromStdString(info.client));
    q.addBindValue(QString::fromStdString(info.pid.to_string().substr(0, 8)));
    q.addBindValue(QString::fromStdString(tag));
    return q.exec();
  }

private:
  QSqlDatabase m_db;
  QString m_table;
};
