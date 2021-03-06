#include "LocalSyncMaster.h"
#include "PathConfigLoader.h"

//TODO: check api for nullptr
LocalSyncMaster::LocalSyncMaster(YaDRestApi *api, QObject *parent)
    : Configurable{parent, new PathConfigLoader{}},
      _api{api} {
  QObject::connect(_api, &YaDRestApi::error, [](const QString &message) {
    qDebug() << message;
  });
  QObject::connect(_api, &YaDRestApi::replyApiError, [](const QJsonObject &message) {
    qDebug() << qPrintable(QJsonDocument(message).toJson());
  });
  loadConfigVariables();
}

//TODO: implement as slot
void LocalSyncMaster::forceSync() {
  JsonReplyWrapper *remote_reply = getRemoteResources();
  getLocalResources();
  QObject::connect(remote_reply, &JsonReplyWrapper::finished, [this, remote_reply] {
    mergeResources();
    remote_reply->deleteLater();
  });

}

JsonReplyWrapper *LocalSyncMaster::getRemoteResources() {
  QUrlQuery params;
  params.addQueryItem("fields", "items,items.path,items.md5,items.modified");
  //TODO: set correct parent for file_lsit response
  JsonReplyWrapper *file_list = _api->getFileList(std::move(params));
  QObject::connect(file_list, &JsonReplyWrapper::jsonReply, this, &LocalSyncMaster::parseFileResourceList);
  return file_list;
}


void LocalSyncMaster::parseFileResourceList(const QJsonObject &jsonObject) {
  QJsonArray items = jsonObject["items"].toArray();
  QString remoteroot = _conf_vars.remote_root;
  QString localroot = _conf_vars.local_root;
  for (const auto &i : items) {
    auto object = i.toObject();
    auto path = object["path"].toString();
    path.replace(remoteroot, localroot);
    auto hash = object["md5"].toString().toLocal8Bit();
    auto modified = QDateTime::fromString(object["modified"].toString(), Qt::ISODate);
    _remote_resources.push_back({path, hash, modified});
  }
}

LocalSyncMaster::~LocalSyncMaster() {

}

//TODO: optimize it ( make it parallel or smth)
void LocalSyncMaster::getLocalResources() {
  auto directories = _conf_vars.watching_dirs;
  QCryptographicHash md5hasher{QCryptographicHash::Md5};
  for (const auto &i : directories) {
    QDir current_dir{i};
    auto files = current_dir.entryInfoList(QDir::AllEntries
                                           | QDir::NoDotAndDotDot
                                           | QDir::NoSymLinks,
                                           QDir::SortFlag::Name);
    for (const auto &j : files) {
      auto path = j.filePath();
      QFile file{path};
      file.open(QIODevice::ReadOnly);
      md5hasher.addData(&file);
      auto hash = md5hasher.result().toHex();
      md5hasher.reset();
      auto modified = j.lastModified();
      _local_resources.push_back({path, hash, modified});
    }
  }
}

//TODO: reduce memory usage, make _*_reources reusable
void LocalSyncMaster::mergeResources() {
  auto pathComparator = [](const auto &a, const auto &b) {
    return a.path < b.path;
  };
  qSort(_remote_resources.begin(), _remote_resources.end(), pathComparator);
  qSort(_local_resources.begin(), _local_resources.end(), pathComparator);
  QList<ResourceRow> toLocal;
  QList<ResourceRow> toRemote;
  for (const auto &local : _local_resources) {
    auto remote = qBinaryFind(_remote_resources.begin(), _remote_resources.end(), local,
                              pathComparator);
    if (remote != _remote_resources.end()) {
      if (remote->md5 != local.md5) {
        if (remote->last_modified > local.last_modified) {
          toRemote.push_back(local);
        } else if (remote->last_modified < local.last_modified) {
          toLocal.push_back(*remote);
        }
      }
      _remote_resources.erase(remote);
    } else {
      toRemote.push_back(local);
    }
  }
  for (auto &i : _remote_resources) {
    toLocal.push_back(i);
  }
  for (auto &i : toLocal) {
    qDebug() << i.path;
    auto target = i.path;
    QUrlQuery query;
    query.addQueryItem("path", target.replace(_conf_vars.local_root, _conf_vars.remote_root));
    auto rep = _api->downloadFile(i.path, query);
    QObject::connect(rep, &ReplyWrapper::finished, [rep] {
      rep->deleteLater();
    });
  }
}

void LocalSyncMaster::loadConfigVariables() {
  _conf_vars.watching_dirs = getConfValue("watchingDirs", _conf_vars.watching_dirs).toStringList();
  _conf_vars.local_root = getConfValue("localroot", _conf_vars.local_root).toString();
  _conf_vars.remote_root = getConfValue("remoteroot", _conf_vars.remote_root).toString();
}

void LocalSyncMaster::handleConfigChange(QSettings *new_config) {
  auto prevConf = _conf_vars;
  loadConfigVariables();
  bool isDifferentLocalRoots = _conf_vars.local_root != prevConf.local_root;
  if (isDifferentLocalRoots) {
    _local_resources.clear();
    getLocalResources();
  }
  if (_conf_vars.local_root != prevConf.local_root) {
    JsonReplyWrapper *remote_reply = getRemoteResources();
    QObject::connect(remote_reply, &JsonReplyWrapper::finished, [this] {
      mergeResources();
    });
  } else if (isDifferentLocalRoots) {
    mergeResources();
  }
}
