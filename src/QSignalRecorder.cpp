#include "QSignalRecorder.h"
#include <QMetaMethod>
#include <QVector>

QT_BEGIN_NAMESPACE

bool SignalRecord::operator==(const SignalRecord &other) const noexcept
{
    return signalName == other.signalName
            && signalArguments == other.signalArguments;
}

bool SignalRecord::operator!=(const SignalRecord &other) const noexcept
{
    return !(*this == other);
}

// NB: This class duplicates some functionality of QSignalSpy.
// But we dont want to have dependency from QtTest.
class QSignalRecorder::SignalArgumentsStorage: public QObject, public QList<QVariantList>
{
public:
    explicit SignalArgumentsStorage(const QObject *obj, const QMetaMethod &signalMetaMethod,
                                    int signalIndex, QObject* parent = nullptr)
        : QObject(parent)
    {
        if (!QMetaObject::connect(obj, signalIndex, this, QObject::staticMetaObject.methodCount(),
                                  Qt::DirectConnection)) {
            qWarning("SignalArgumentsStorage: QMetaObject::connect returned false. Unable to connect.");
        } else {
            initArgs(signalMetaMethod, obj);
            mIsValid = true;
        }
    }

    bool isValid() const noexcept
    {
        return mIsValid;
    }

    int qt_metacall(QMetaObject::Call call, int methodId, void **a) override
    {
        methodId = QObject::qt_metacall(call, methodId, a);
        if (methodId < 0) {
            return methodId;
        }
        if (call == QMetaObject::InvokeMetaMethod) {
            if (methodId == 0) {
                appendArgs(a);
            }
            --methodId;
        }
        return methodId;
    }

    void initArgs(const QMetaMethod &member, const QObject *obj)
    {
        mArgumentTypes.reserve(member.parameterCount());
        for (int i = 0; i < member.parameterCount(); ++i) {
            int parameterType = member.parameterType(i);
            if (parameterType == QMetaType::UnknownType && obj) {
                void *argv[] = { &parameterType, &i };
                QMetaObject::metacall(const_cast<QObject*>(obj),
                                      QMetaObject::RegisterMethodArgumentMetaType,
                                      member.methodIndex(), argv);
                if (parameterType == -1) {
                    parameterType = QMetaType::UnknownType;
                }
            }
            if (parameterType == QMetaType::UnknownType) {
                qWarning("SignalArgumentsStorage: Unable to handle parameter '%s' of type '%s' of method '%s',"
                         " use qRegisterMetaType to register it.",
                         member.parameterNames().at(i).constData(),
                         member.parameterTypes().at(i).constData(),
                         member.name().constData());
            }
            mArgumentTypes << parameterType;
        }
    }
    template<class Args>
    void appendArgs(Args args)
    {
        QList<QVariant> list;
        list.reserve(mArgumentTypes.count());
        for (int i = 0; i < mArgumentTypes.count(); ++i) {
            const QMetaType::Type metaType = static_cast<QMetaType::Type>(mArgumentTypes[i]);
            if (metaType == QMetaType::QVariant)
                list << *static_cast<QVariant *>(args[i + 1]);
            else
                list << QVariant(metaType, args[i + 1]);
        }
        append(list);
    }

private:
    QVector<int> mArgumentTypes;
    bool mIsValid = false;
};

std::vector<SignalRecord> QSignalRecorder::records() const noexcept
{
    return mRecords;
}

bool QSignalRecorder::isValid() const noexcept
{
    return mIsValid;
}

QSignalRecorder::QSignalRecorder(QSignalRecorder &&other) noexcept
    : mSignalStorages(std::move(other.mSignalStorages))
    , mRecords(std::move(other.mRecords))
{
}

QSignalRecorder &QSignalRecorder::operator=(QSignalRecorder &&other) noexcept
{
    mSignalStorages = std::move(other.mSignalStorages);
    mRecords = std::move(other.mRecords);
    return *this;
}

bool QSignalRecorder::initialize(const QObject *recordedObject, const int metaMethodsOffset) noexcept
{
    if (recordedObject == nullptr) {
        return false;
    }
    const auto* recordedMetaObject = recordedObject->metaObject();
    const int metaMethodsCount = recordedMetaObject->methodCount();
    if (metaMethodsOffset >= metaMethodsCount) {
        qWarning("QSignalRecorder: provided object does not contain any meta methods");
        return false;
    }
    for (int i = metaMethodsOffset; i < metaMethodsCount; ++i) {
        auto metaMethod = recordedMetaObject->method(i);
        if (metaMethod.methodType() == QMetaMethod::Signal
                && !connectObjectSignals(recordedObject, metaMethod, i)) {
            return false;
        }
    }
    if (mSignalStorages.empty()) {
        qWarning("QSignalRecorder: provided object does not contain any signals");
        return false;
    }
    return true;
}

QMetaMethod QSignalRecorder::storeRecordSlotToMetaMethod() const
{
    const auto* selfMetaObject = metaObject();
    const auto lastMetaMethodIndex = selfMetaObject->methodCount() - 1;
    auto lastMetaMethod = selfMetaObject->method(lastMetaMethodIndex);
    if (QString::fromLatin1(lastMetaMethod.name()) == QLatin1String("storeRecord")) {
        return lastMetaMethod;
    }
    qWarning("QSignalRecorder: cannot find storeRecord() meta method. Did you change QSignalRecorder realization?");
    return {};
}

bool QSignalRecorder::connectObjectSignals(const QObject *recordedObject, const QMetaMethod &metaMethodSignal, const int signalIndex) noexcept
{
    try {
        if (const auto signalStorageIt = mSignalStorages.find(metaMethodSignal.name());
                signalStorageIt == mSignalStorages.end()) {
            auto signalStorage = new SignalArgumentsStorage(recordedObject, metaMethodSignal, signalIndex, this);
            if (!signalStorage->isValid()) {
                return false;
            }
            mSignalStorages.emplace(QString::fromLatin1(metaMethodSignal.name()), std::move(signalStorage));
        }
        const auto storeRecordMetaMethod = storeRecordSlotToMetaMethod();
        if (!storeRecordMetaMethod.isValid()) {
            return false;
        }
        connect(recordedObject, metaMethodSignal, this, storeRecordMetaMethod, Qt::UniqueConnection);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

void QSignalRecorder::storeRecord()
{
    const auto* recordedObject = sender();
    if (recordedObject == nullptr) {
        qWarning("QSignalRecorder: storeRecord() slot must be call only by recorded object");
        return;
    }
    int senderSignalIdx = senderSignalIndex();
    if (senderSignalIdx == -1) {
        qWarning("QSignalRecorder: cannot find recorded object signal index");
        return;
    }
    const auto methodName = QString::fromLatin1(recordedObject->metaObject()->method(senderSignalIdx).name());
    const auto signalStorageIt = mSignalStorages.find(methodName);
    if (signalStorageIt != mSignalStorages.end()) {
        const auto signalStorage = signalStorageIt->second;
        QList<QVariant> signalArguments;
        if (!signalStorage->empty()) {
            signalArguments = signalStorage->takeLast();
        }
        SignalRecord record{methodName, signalArguments};
        mRecords.emplace_back(std::move(record));
    }
}

QT_END_NAMESPACE
