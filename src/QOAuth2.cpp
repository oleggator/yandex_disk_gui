#include "QOAuth2.h"
#include <QNetworkAccessManager>

#include <QNetworkReply>

void QOAuth2::setToken(const QString &token) {
  QOAuth2AuthorizationCodeFlow::setToken(token);
  setStatus(QAbstractOAuth::Status::Granted);

}

QOAuth2::QOAuth2(QNetworkAccessManager *manager, QObject *parent)
  :QOAuth2AuthorizationCodeFlow{manager, parent}{
}

QOAuth2::QOAuth2(QObject *parent)
    : QOAuth2AuthorizationCodeFlow{new QNetworkAccessManager{}, parent} {
}

