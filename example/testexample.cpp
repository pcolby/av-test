#include <testexample.h>

#include <QTest>

void TestExample::bad()
{
    QVERIFY(false);
}

void TestExample::good()
{
    QVERIFY(true);
}

void TestExample::goodWithData_data()
{
    QTest::addColumn<int>("i");
    QTest::addRow("one") << 1;
}

void TestExample::goodWithData()
{
    QFETCH(int, i);
    QCOMPARE(i, i);
}

void TestExample::skipMe()
{
    QSKIP("demo");
}
