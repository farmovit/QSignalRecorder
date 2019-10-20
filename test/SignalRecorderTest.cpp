#include "SignalRecorderTest.h"
#include "QSignalRecorder.h"
#include <QtTest>

class GoodTestObject : public QObject
{
    Q_OBJECT
public:
    GoodTestObject() = default;
    void emitSignalWithoutArguments()
    {
        emit signalWithoutArguments();
    }
    void emitSignalWithSingleArgument(int value)
    {
        emit signalWithSingleArgument(value);
    }
    void emitSignalWithDoubleArguments(int first, int second)
    {
        emit signalWithDoubleArguments(first, second);
    }
    void emitSignalWithSingleDefaultArgument(int value = 0)
    {
        emit signalWithSingleDefaultArgument(value);
    }
signals:
    void signalWithoutArguments();
    void signalWithSingleArgument(int value);
    void signalWithDoubleArguments(int first, int second);
    void signalWithSingleDefaultArgument(int value = 0);
};

class GoodTestObjectChild : public GoodTestObject
{
    Q_OBJECT
public:
    GoodTestObjectChild() = default;
};

class BadTestObject : public QObject
{
    Q_OBJECT
public:
    BadTestObject() = default;
private Q_SLOTS:
    void onSomethingHappened() {}
};

class BadTestObject2 : public QObject
{
    Q_OBJECT
public:
    BadTestObject2() = default;
};

SignalRecorderTest::SignalRecorderTest(QObject *parent)
    : QObject(parent) {
}

void SignalRecorderTest::simpleRecordTest()
{
    GoodTestObject testObject;
    QSignalRecorder recorder(&testObject);
    QVERIFY(recorder.isValid());
    static const std::vector<SignalRecord> signalsScenario =
    {
        {"signalWithoutArguments", {}},
        {"signalWithSingleArgument", {1}},
        {"signalWithDoubleArguments", {1, 2}},
        {"signalWithSingleDefaultArgument", {0}},
        {"signalWithSingleDefaultArgument", {1}},
    };
    testObject.emitSignalWithoutArguments();
    testObject.emitSignalWithSingleArgument(1);
    testObject.emitSignalWithDoubleArguments(1, 2);
    testObject.emitSignalWithSingleDefaultArgument();
    testObject.emitSignalWithSingleDefaultArgument(1);
    QVERIFY(signalsScenario == recorder.records());
}

void SignalRecorderTest::nullptrObjectTest()
{
    BadTestObject* testObject = nullptr;
    QSignalRecorder recorder(testObject);
    QVERIFY(!recorder.isValid());
}

void SignalRecorderTest::noMetaMethodsObjectTest()
{
    BadTestObject2 testObject;
    QSignalRecorder recorder(&testObject);
    QVERIFY(!recorder.isValid());
}

void SignalRecorderTest::noSignalsObjectTest()
{
    BadTestObject testObject;
    QSignalRecorder recorder(&testObject);
    QVERIFY(!recorder.isValid());
}

void SignalRecorderTest::childObjectBadTest()
{
    GoodTestObjectChild testObject;
    QSignalRecorder recorder(&testObject);
    QVERIFY(!recorder.isValid());
}

void SignalRecorderTest::childObjectGoodTest()
{
    GoodTestObjectChild testObject;
    QSignalRecorder recorder(dynamic_cast<GoodTestObject *>(&testObject));
    QVERIFY(recorder.isValid());
}

QTEST_MAIN(SignalRecorderTest)

#include "SignalRecorderTest.moc"
