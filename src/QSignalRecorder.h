#pragma once

#include <QObject>
#include <QVariant>
#include "shared_export.h"

QT_BEGIN_NAMESPACE

/**
 * @brief Represent single signal record
 * signalName is name of the signal was emitted (e.g. `countChanged`)
 * signalArguments are an arguments the signalName was emitted (e.g. {QVariant(int, 1)})
 */
struct SHARED_EXPORT SignalRecord
{
    QString signalName;
    QList<QVariant> signalArguments;
    bool operator==(const SignalRecord& other) const noexcept;
    bool operator!=(const SignalRecord& other) const noexcept;
};

class SHARED_EXPORT QSignalRecorder : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief Constructs a QSignalRecorder and sets `isValid` to true if got a correct object.
     * @arg recordedObject - object signals should be recorded.
     * Must be inherited from QObject and have at least one signal.
     * @arg parent - parent object.
     */
    template<typename ObjectT>
    QSignalRecorder(const ObjectT* recordedObject, QObject* parent = nullptr) noexcept;

    /**
     * @return records of currently emitted signals.
     */
    [[ nodiscard ]] std::vector<SignalRecord> records() const noexcept;
    /**
     * @return returns if constructed QSignalRecorder is valid.
     * @details if QSignalRecorder is not valid it does nothing.
     */
    [[ nodiscard ]] bool isValid() const noexcept;

    Q_DISABLE_COPY(QSignalRecorder)
    QSignalRecorder(QSignalRecorder&& other) noexcept;
    QSignalRecorder &operator=(QSignalRecorder &&other) noexcept;

private:
    bool initialize(const QObject *recordedObject, const int metaMethodsOffset) noexcept;
    QMetaMethod storeRecordSlotToMetaMethod() const;
    bool connectObjectSignals(const QObject* recordedObject, const QMetaMethod& metaMethodSignal, const int signalIndex) noexcept;

private Q_SLOTS:
    void storeRecord();

private:
    class SignalArgumentsStorage;
    std::map<QString, SignalArgumentsStorage*> mSignalStorages;
    std::vector<SignalRecord> mRecords;
    bool mIsValid = false;
};

template<typename ObjectT>
QSignalRecorder::QSignalRecorder(const ObjectT* recordedObject, QObject* parent) noexcept
    : QObject(parent)
{
    static_assert (std::is_base_of_v<QObject, ObjectT>, "ObjectT must be derived from QObject");
    mIsValid = initialize(recordedObject, ObjectT::staticMetaObject.methodOffset());
    if (!mIsValid) {
        mSignalStorages.clear();
    }
}

QT_END_NAMESPACE
