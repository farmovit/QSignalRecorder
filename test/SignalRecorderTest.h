#include "QSignalRecorder.h"
#include <QtTest>

class SignalRecorderTest : public QObject
{
    Q_OBJECT
public:
    explicit SignalRecorderTest(QObject *parent = nullptr);
private slots:
    void simpleRecordTest();
    void nullptrObjectTest();
    void noMetaMethodsObjectTest();
    void noSignalsObjectTest();
    void childObjectBadTest();
    void childObjectGoodTest();
};
