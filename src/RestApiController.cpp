#include "RestApiController.h"


RestApiController::RestApiController(QObject *parent)
  :worker{new RestApiWorker(nullptr)}{
  qRegisterMetaType<RequestType>();
  worker->moveToThread(&worker_thread);
  QObject::connect(&worker_thread, &QThread::finished,
                   worker,&RestApiWorker::deleteLater);
  QObject::connect(this,&RestApiController::proxyRequest,
                   worker,&RestApiWorker::processRequest);
  worker_thread.start();
}

RestApiController::~RestApiController() {
  worker_thread.quit();
  worker_thread.wait();
}

void RestApiController::sendGetRequest(QObject *reciever,
                                       const QByteArray &recieve_slot,
                                       QNetworkRequest *request) {
  emit proxyRequest(reciever,recieve_slot,request,RequestType::GET);
}

void RestApiController::sendPostRequest(QObject *reciever,
                                        const QByteArray &recieve_slot,
                                        QNetworkRequest *request,
                                        QByteArray *data) {
  emit proxyRequest(reciever,recieve_slot,request,RequestType::POST, data);
}

void RestApiController::sendPutRequest(QObject *reciever,
                                       const QByteArray &recieve_slot,
                                       QNetworkRequest *request,
                                       QByteArray *data) {
  emit proxyRequest(reciever,recieve_slot,request,RequestType::PUT, data);
}