#include <QObject>

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QProcessEnvironment>
#include <QTest>

class AV : public QObject {
    Q_OBJECT

public:
    AV(QObject * parent = Q_NULLPTR) : QObject(parent), networkManager(Q_NULLPTR) { }

    void setNetworkAccessManager(QNetworkAccessManager * manager)
    {
        if (networkManager && networkManager->parent() == this) {
            networkManager->deleteLater();
        }
        networkManager = manager;
        connect(networkManager, &QNetworkAccessManager::finished, this, &AV::onReplyFinished);
    }

protected slots:
    void initTestCase()
    {
        if (!networkManager) {
            setNetworkAccessManager(new QNetworkAccessManager(this));
        }
        QJsonArray tests;
        for (int methodIndex = 0; methodIndex < metaObject()->methodCount(); ++methodIndex) {
            const QMetaMethod method = metaObject()->method(methodIndex);
            if (isValidSlot(method)) {
                tests.append(appveyorTest(method.name().data()));
            }
        }
        //submitTests(QJsonDocument(tests));
    }

    void init()
    {
        testStartTime = QDateTime::currentMSecsSinceEpoch();
        submitTests(QJsonDocument(appveyorTest(QTest::currentTestFunction(), "Runnnig")));
    }

    void cleanup()
    {
        //const qint64 duration = testStartTime - QDateTime::currentMSecsSinceEpoch();
        //const QString outcome(
        //    QTest::currentTestFailed() ? QStringLiteral("Failed") : QStringLiteral("Passed"));
        //submitTests(QJsonDocument(appveyorTest(QTest::currentTestFunction(), outcome, duration)));
    }

    void cleanupTestCase()
    {
        if (networkManager && networkManager->parent() == this) {
            QEventLoop loop;
            connect(this, &AV::networkRepliesChanged, &loop, &QEventLoop::quit);
            while (!networkReplies.isEmpty()) {
                qDebug() << "Waiting for" << networkReplies.count() << "network replies to finish";
                loop.exec();
            }
        }
    }

private:
    QNetworkAccessManager * networkManager;
    QSet<QNetworkReply *> networkReplies;
    qint64 testStartTime;

    static QJsonObject appveyorTest(const QString &name, const QString &outcome = QString(), const qint64 duration = -1)
    {
        QJsonObject test;
        test.insert(QStringLiteral("testName"), name);
        test.insert(QStringLiteral("testFramework"), "QtTest");
        test.insert(QStringLiteral("fileName"), QTest::currentAppName());
        test.insert(QStringLiteral("outcome"), outcome.isEmpty() ? QStringLiteral("None") : outcome);
        if (duration >= 0) test.insert(QStringLiteral("durationMilliseconds"), duration);
        return test;
    }

    static bool isValidSlot(const QMetaMethod &sl)
    {
        if (sl.access() != QMetaMethod::Private || sl.parameterCount() != 0
            || sl.returnType() != QMetaType::Void || sl.methodType() != QMetaMethod::Slot)
            return false;
        const QByteArray name = sl.name();
        return !(name.isEmpty() || name.endsWith("_data")
            || name == "initTestCase" || name == "cleanupTestCase"
            || name == "init" || name == "cleanup");
    }

    void submitTests(const QJsonDocument &jsonDocument)
    {
        Q_ASSERT(jsonDocument.isArray() || jsonDocument.isObject());
        static const QString baseUrl = QProcessEnvironment::systemEnvironment().value(QStringLiteral("APPVEYOR_API_URL"));
        const QUrl url = baseUrl.isEmpty() ? QUrl() : baseUrl +
            (jsonDocument.isArray() ? QStringLiteral("api/tests/batch") : QStringLiteral("api/tests"));
        if (url.isValid()) {
            qDebug().noquote() << "Submitting tests" << url << jsonDocument.toJson(QJsonDocument::Indented);
            QNetworkRequest request(url);
            request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
            networkReplies.insert(networkManager->post(request, jsonDocument.toJson(QJsonDocument::Compact)));
            emit networkRepliesChanged();
        }
    }

private slots:
    void onReplyFinished(QNetworkReply * reply)
    {
        if (reply->error()) {
            qWarning() << "Failed to submit tests to AppVeyor API" << reply->errorString();
        }
        qDebug() << networkReplies.count() << "known replies";
        qDebug() << reply->url() << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() << reply->readAll();
        networkReplies.remove(reply);
        reply->close();
        reply->deleteLater();
        qDebug() << networkReplies.count() << "replies left";
        emit networkRepliesChanged();
    }

signals:
    void networkRepliesChanged();

};

class TestExample : public AV {
    Q_OBJECT

private slots:
    void bad();

    void good();

    void goodWithData_data();
    void goodWithData();

    void skipMe();
};
