<div align="center">

  [![Status](https://img.shields.io/badge/status-active-success.svg)]()
  [![GitHub Issues](https://img.shields.io/github/issues/farmovit/QSignalRecorder](https://github.com/farmovit/QSignalRecorder/issues)
  [![GitHub Pull Requests](https://img.shields.io/github/issues-pr/farmovit/QSignalRecorder)](https://github.com/farmovit/QPointerGrabber/pulls)
  [![License](https://img.shields.io/github/license/farmovit/QSignalRecorder)](/LICENSE)
	[![Build status](https://img.shields.io/appveyor/ci/farmovit/QSignalRecorder)](https://ci.appveyor.com/project/farmovit/qsignalrecorder)

</div>

---

### QSignalRecorder
A small library that allows you to record all signals with parameters emitted by QObject. Might be useful for testing, logging and some runtime checks.

#### Requirements
- C++17
- Qt 5.X

#### Usage
Let's assume you implemented some QObject class that represents your business logic and emits some signals:

```c++
class BusinessLogic: public QObject
{
Q_OBJECT
public:
// ...
	void doSomeBusinessLogic(); // emits firstEmittedSignal, secondEmittedSignal, thirdEmittedSignal
// ...
signals:
	void firstEmittedSignal();
	void secondEmittedSignal(int i);
	void thirdEmittedSignal(int j = 0);
// ...
};
```
It is important to you to have that signals emitted in a certain order. So you now want to test your `BusinessLogic`. Qt provides a helpful mechanism to do it named `QSignalSpy` that should be created for each signal of your object and each of these spies should be then tested. E.g.
```c++
void test()
{
	BusinessLogic logic;
	QSignalSpy spy1(&logic, &BusinessLogic::firstEmittedSignal);
	QSignalSpy spy2(&logic, &BusinessLogic::secondEmittedSignal);
	QSignalSpy spy3(&logic, &BusinessLogic::thirdEmittedSignal);
	//... do something with logic
	QCOMPARE(spy1.count(), 0);
	QCOMPARE(spy2.count(), 1);
	const auto arguments = spy2.takeFirst();
	QVERIFY(arguments.at(0).toInt() == 2);
	QCOMPARE(spy3.count(), 1);
	arguments = spy3.takeFirst();
	QVERIFY(arguments.at(0).toInt() == 3);
}
```
The approach above does not allow you to test the order of signal emission and looks overcomplicated for that needs. Also it is not representative for code readers who wants to now how the code works. Moreover it is bug prone. So instead of that you can use my library
```c++
void test()
{
	BusinessLogic logic;
	QSignalRecorder recorder(&logic);
	static std::vector<SignalRecords> expected_signals =
	{
		{"firstEmittedSignal", {}},
		{"secondEmittedSignal", {2}},
		{"thirdEmittedSignal", {3}},
	}
	//... do something with logic
	QVERIFY(expected_signals == recorder.records());
}
```
Also feel free to look the [test](https://github.com/farmovit/QSignalRecorder/blob/master/test/SignalRecorderTest.cpp) to get more information.