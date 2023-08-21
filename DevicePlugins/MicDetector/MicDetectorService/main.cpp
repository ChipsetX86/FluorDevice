#include <QCoreApplication>
#include <QObject>
#include <QCommandLineParser>

#include <json_rpc_tcp_server.h>
#include <json_rpc_debug_logger.h>
#include <json_rpc_logger.h>

#include "../Common.h"

#include "MicDetectorWrapper.h"
#include "DetectorService.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QCommandLineParser parser;
    QCommandLineOption portOption(QStringLiteral("p"),
                                  QStringLiteral("Port listen json server"),
                                  QStringLiteral("0"));
    parser.addOption(portOption);
    parser.process(app);

    if (int port = parser.value(portOption).toInt()) {
        auto detector = new MicDetectorWrapper;
        if (detector->loadMicLibrary()) {
            auto rpcServer = new jcon::JsonRpcTcpServer(nullptr, std::make_shared<jcon::JsonRpcDebugLogger>());
            rpcServer->registerServices(QObjectList{new DetectorService(detector)});
            rpcServer->listen(QHostAddress::LocalHost, port);
        } else {
            qDebug() << "Error load MIC library";
            return EXIT_FAILURE;
        }
    } else {
        qDebug() << "Port is not set in -p param";
        return EXIT_FAILURE;
    }

    return app.exec();
}
