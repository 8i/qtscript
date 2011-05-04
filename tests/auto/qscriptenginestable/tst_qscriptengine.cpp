/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtScript/qscriptengine.h>
#include <QtScript/qscriptprogram.h>
#include <QtScript/qscriptsyntaxcheckresult.h>
#include <QtScript/qscriptvalue.h>
#include <QtTest/qtest.h>

class tst_QScriptEngine : public QObject {
    Q_OBJECT

public:
    tst_QScriptEngine() {}
    virtual ~tst_QScriptEngine() {}

public slots:
    void init() {}
    void cleanup() {}

private slots:
    void newFunction();
    void newObject();
    void globalObject();
    void evaluate();
    void collectGarbage();
    void reportAdditionalMemoryCost();
    void nullValue();
    void undefinedValue();
    void evaluateProgram();
    void checkSyntax_data();
    void checkSyntax();
    void toObject();
    void toObjectTwoEngines();
    void newArray();
    void uncaughtException();
};

/* Evaluating a script that throw an unhandled exception should return an invalid value. */
void tst_QScriptEngine::evaluate()
{
    QScriptEngine engine;
    QVERIFY2(engine.evaluate("1+1").isValid(), "the expression should be evaluated and an valid result should be returned");
    QVERIFY2(engine.evaluate("ping").isValid(), "Script throwing an unhandled exception should return an exception value");
}

static QScriptValue myFunction(QScriptContext*, QScriptEngine* eng)
{
    return eng->nullValue();
}

static QScriptValue myFunctionWithArg(QScriptContext*, QScriptEngine* eng, void* arg)
{
    int* result = reinterpret_cast<int*>(arg);
    return QScriptValue(eng, *result);
}

static QScriptValue myFunctionThatReturns(QScriptContext*, QScriptEngine* eng)
{
    return QScriptValue(eng, 42);
}

static QScriptValue myFunctionThatReturnsWithoutEngine(QScriptContext*, QScriptEngine*)
{
    return QScriptValue(1024);
}

static QScriptValue myFunctionThatReturnsWrongEngine(QScriptContext*, QScriptEngine*, void* arg)
{
    QScriptEngine* wrongEngine = reinterpret_cast<QScriptEngine*>(arg);
    return QScriptValue(wrongEngine, 42);
}

void tst_QScriptEngine::newFunction()
{
    QScriptEngine eng;
    {
        QScriptValue fun = eng.newFunction(myFunction);
        QCOMPARE(fun.isValid(), true);
        QCOMPARE(fun.isFunction(), true);
        QCOMPARE(fun.isObject(), true);
        QCOMPARE(fun.scriptClass(), (QScriptClass*)0);
        // a prototype property is automatically constructed
        {
            QScriptValue prot = fun.property("prototype", QScriptValue::ResolveLocal);
            QVERIFY(prot.isObject());
            QVERIFY(prot.property("constructor").strictlyEquals(fun));
            QCOMPARE(fun.propertyFlags("prototype"), QScriptValue::PropertyFlags(QScriptValue::Undeletable | QScriptValue::SkipInEnumeration));
            QCOMPARE(prot.propertyFlags("constructor"), QScriptValue::SkipInEnumeration);
        }
        // prototype should be Function.prototype
        QCOMPARE(fun.prototype().isValid(), true);
        QCOMPARE(fun.prototype().isFunction(), true);
        QCOMPARE(fun.prototype().strictlyEquals(eng.evaluate("Function.prototype")), true);

        QCOMPARE(fun.call().isNull(), true);
        QCOMPARE(fun.construct().isObject(), true);
    }
    // the overload that takes an extra argument
    {
        int expectedResult = 42;
        QScriptValue fun = eng.newFunction(myFunctionWithArg, reinterpret_cast<void*>(&expectedResult));
        QVERIFY(fun.isFunction());
        QCOMPARE(fun.scriptClass(), (QScriptClass*)0);
        // a prototype property is automatically constructed
        {
            QScriptValue prot = fun.property("prototype", QScriptValue::ResolveLocal);
            QVERIFY(prot.isObject());
            QVERIFY(prot.property("constructor").strictlyEquals(fun));
            QCOMPARE(fun.propertyFlags("prototype"), QScriptValue::PropertyFlags(QScriptValue::Undeletable | QScriptValue::SkipInEnumeration));
            QCOMPARE(prot.propertyFlags("constructor"), QScriptValue::SkipInEnumeration);
        }
        // prototype should be Function.prototype
        QCOMPARE(fun.prototype().isValid(), true);
        QCOMPARE(fun.prototype().isFunction(), true);
        QCOMPARE(fun.prototype().strictlyEquals(eng.evaluate("Function.prototype")), true);

        QScriptValue result = fun.call();
        QCOMPARE(result.isNumber(), true);
        QCOMPARE(result.toInt32(), expectedResult);
    }
    // the overload that takes a prototype
    {
        QScriptValue proto = eng.newObject();
        QScriptValue fun = eng.newFunction(myFunction, proto);
        QCOMPARE(fun.isValid(), true);
        QCOMPARE(fun.isFunction(), true);
        QCOMPARE(fun.isObject(), true);
        // internal prototype should be Function.prototype
        QCOMPARE(fun.prototype().isValid(), true);
        QCOMPARE(fun.prototype().isFunction(), true);
        QCOMPARE(fun.prototype().strictlyEquals(eng.evaluate("Function.prototype")), true);
        // public prototype should be the one we passed
        QCOMPARE(fun.property("prototype").strictlyEquals(proto), true);
        QCOMPARE(fun.propertyFlags("prototype"), QScriptValue::PropertyFlags(QScriptValue::Undeletable | QScriptValue::SkipInEnumeration));
        QCOMPARE(proto.property("constructor").strictlyEquals(fun), true);
        QCOMPARE(proto.propertyFlags("constructor"), QScriptValue::SkipInEnumeration);

        QCOMPARE(fun.call().isNull(), true);
        QCOMPARE(fun.construct().isObject(), true);
    }
    // whether the return value is correct
    {
        QScriptValue fun = eng.newFunction(myFunctionThatReturns);
        QCOMPARE(fun.isValid(), true);
        QCOMPARE(fun.isFunction(), true);
        QCOMPARE(fun.isObject(), true);

        QScriptValue result = fun.call();
        QCOMPARE(result.isNumber(), true);
        QCOMPARE(result.toInt32(), 42);
    }
    // whether the return value is assigned to the correct engine
    {
        QScriptValue fun = eng.newFunction(myFunctionThatReturnsWithoutEngine);
        QCOMPARE(fun.isValid(), true);
        QCOMPARE(fun.isFunction(), true);
        QCOMPARE(fun.isObject(), true);

        QScriptValue result = fun.call();
        QCOMPARE(result.engine(), &eng);
        QCOMPARE(result.isNumber(), true);
        QCOMPARE(result.toInt32(), 1024);
    }
    // whether the return value is undefined when returning a value with wrong engine
    {
        QScriptEngine wrongEngine;

        QScriptValue fun = eng.newFunction(myFunctionThatReturnsWrongEngine, reinterpret_cast<void*>(&wrongEngine));
        QCOMPARE(fun.isValid(), true);
        QCOMPARE(fun.isFunction(), true);
        QCOMPARE(fun.isObject(), true);

        QTest::ignoreMessage(QtWarningMsg, "QScriptValue::call(): Value from different engine returned from native function, returning undefined value instead.");
        QScriptValue result = fun.call();
        QCOMPARE(result.isValid(), true);
        QCOMPARE(result.isUndefined(), true);
    }
}

void tst_QScriptEngine::newObject()
{
    QScriptEngine engine;
    QScriptValue object = engine.newObject();
    QVERIFY(object.isObject());
    QVERIFY(object.engine() == &engine);
    QVERIFY(!object.isError());
    QVERIFY(!object.equals(engine.newObject()));
    QVERIFY(!object.strictlyEquals(engine.newObject()));
    QCOMPARE(object.toString(), QString::fromAscii("[object Object]"));
}

void tst_QScriptEngine::globalObject()
{
    QScriptEngine engine;
    QScriptValue global = engine.globalObject();
    QScriptValue self = engine.evaluate("this");
    QVERIFY(global.isObject());
    QVERIFY(engine.globalObject().equals(engine.evaluate("this")));
    QVERIFY(engine.globalObject().strictlyEquals(self));
}

/* Test garbage collection, at least try to not crash. */
void tst_QScriptEngine::collectGarbage()
{
    QScriptEngine engine;
    QScriptValue foo = engine.evaluate("( function foo() {return 'pong';} )");
    QVERIFY(foo.isFunction());
    QVERIFY(engine.newArray().isValid());
    QVERIFY(engine.nullValue().isValid());
    QVERIFY(engine.newObject().isValid());
    QVERIFY(engine.newQObject(this).isValid());
    QVERIFY(engine.newQObject(new QWidget(), QScriptEngine::ScriptOwnership).isValid());
    engine.collectGarbage();
    QCOMPARE(foo.call().toString(), QString::fromAscii("pong"));
}

void tst_QScriptEngine::reportAdditionalMemoryCost()
{
    QSKIP("FIXME: not Implemented yet", SkipAll);

    // There isn't any easy way to test the responsiveness of the GC;
    // just try to call the function a few times with various sizes.
    QScriptEngine eng;
    for (int i = 0; i < 1000; ++i) {
        eng.reportAdditionalMemoryCost(0);
        eng.reportAdditionalMemoryCost(10);
        eng.reportAdditionalMemoryCost(1000);
        eng.reportAdditionalMemoryCost(10000);
        eng.reportAdditionalMemoryCost(100000);
        eng.reportAdditionalMemoryCost(1000000);
        eng.reportAdditionalMemoryCost(10000000);
        eng.reportAdditionalMemoryCost(-1);
        eng.reportAdditionalMemoryCost(-1000);
        QScriptValue obj = eng.evaluate("new Object");
        eng.collectGarbage();
    }
}

void tst_QScriptEngine::nullValue()
{
    QScriptEngine engine;
    QScriptValue value = engine.nullValue();
    QVERIFY(value.isValid());
    QVERIFY(value.isNull());
}

void tst_QScriptEngine::undefinedValue()
{
    QScriptEngine engine;
    QScriptValue value = engine.undefinedValue();
    QVERIFY(value.isValid());
    QVERIFY(value.isUndefined());
}

void tst_QScriptEngine::evaluateProgram()
{
    QScriptEngine eng;
    {
        QString code("1 + 2");
        QString fileName("hello.js");
        int lineNumber = 123;
        QScriptProgram program(code, fileName, lineNumber);
        QVERIFY(!program.isNull());
        QCOMPARE(program.sourceCode(), code);
        QCOMPARE(program.fileName(), fileName);
        QCOMPARE(program.firstLineNumber(), lineNumber);

        QScriptValue expected = eng.evaluate(code);
        for (int x = 0; x < 10; ++x) {
            QScriptValue ret = eng.evaluate(program);
            QVERIFY(ret.equals(expected));
        }

        // operator=
        QScriptProgram sameProgram = program;
        QVERIFY(sameProgram == program);
        QVERIFY(eng.evaluate(sameProgram).equals(expected));

        // copy constructor
        QScriptProgram sameProgram2(program);
        QVERIFY(sameProgram2 == program);
        QVERIFY(eng.evaluate(sameProgram2).equals(expected));

        QScriptProgram differentProgram("2 + 3");
        QVERIFY(differentProgram != program);
        QVERIFY(!eng.evaluate(differentProgram).equals(expected));
    }

    // Program that accesses variable in the scope
    {
        QScriptProgram program("a");
        QVERIFY(!program.isNull());
        {
            QScriptValue ret = eng.evaluate(program);
            QVERIFY(ret.isError());
            QVERIFY(ret.toString().startsWith("ReferenceError"));
        }
        {
            QScriptValue ret = eng.evaluate(program);
            QVERIFY(ret.isError());
        }
        eng.evaluate("a = 456");
        {
            QScriptValue ret = eng.evaluate(program);
            QVERIFY(!ret.isError());
            QCOMPARE(ret.toNumber(), 456.0);
        }
    }

    // Program that creates closure
    {
        QScriptProgram program("(function() { var count = 0; return function() { return count++; }; })");
        QVERIFY(!program.isNull());
        QScriptValue createCounter = eng.evaluate(program);
        QVERIFY(createCounter.isFunction());
        QScriptValue counter = createCounter.call();
        QVERIFY(counter.isFunction());
        {
            QScriptValue ret = counter.call();
            QVERIFY(ret.isNumber());
        }
        QScriptValue counter2 = createCounter.call();
        QVERIFY(counter2.isFunction());
        QVERIFY(!counter2.equals(counter));
        {
            QScriptValue ret = counter2.call();
            QVERIFY(ret.isNumber());
        }
    }

    // Same program run in different engines
    {
        QString code("1 + 2");
        QScriptProgram program(code);
        QVERIFY(!program.isNull());
        double expected = eng.evaluate(program).toNumber();
        for (int x = 0; x < 2; ++x) {
            QScriptEngine eng2;
            for (int y = 0; y < 2; ++y) {
                double ret = eng2.evaluate(program).toNumber();
                QCOMPARE(ret, expected);
            }
        }
    }

    // No program
    {
        QScriptProgram program;
        QVERIFY(program.isNull());
        QScriptValue ret = eng.evaluate(program);
        // FIXME (Qt5) an empty program should be executed in the same way as an empty string.
        QVERIFY(!ret.isValid());
    }
}

void tst_QScriptEngine::checkSyntax_data()
{
    QTest::addColumn<QString>("code");
    QTest::addColumn<int>("expectedState");
    QTest::addColumn<int>("errorLineNumber");
    QTest::addColumn<int>("errorColumnNumber");
    QTest::addColumn<QString>("errorMessage");

    QTest::newRow("0")
        << QString("0") << int(QScriptSyntaxCheckResult::Valid)
        << -1 << -1 << "";
    QTest::newRow("if (")
        << QString("if (\n") << int(QScriptSyntaxCheckResult::Intermediate)
        << 1 << 4 << "";
    QTest::newRow("if else")
        << QString("\nif else") << int(QScriptSyntaxCheckResult::Error)
        << 2 << 3 << "SyntaxError";
    QTest::newRow("{if}")
            << QString("{\n{\nif\n}\n") << int(QScriptSyntaxCheckResult::Error)
        << 4 << 0 << "SyntaxError";
    QTest::newRow("foo[")
        << QString("foo[") << int(QScriptSyntaxCheckResult::Error)
        << 1 << 4 << "SyntaxError";
    QTest::newRow("foo['bar']")
        << QString("foo['bar']") << int(QScriptSyntaxCheckResult::Valid)
        << -1 << -1 << "";

    QTest::newRow("/*")
        << QString("/*") << int(QScriptSyntaxCheckResult::Intermediate)
        << 1 << 1 << "Unclosed comment at end of file";
    QTest::newRow("/*\nMy comment")
        << QString("/*\nMy comment") << int(QScriptSyntaxCheckResult::Intermediate)
        << 1 << 1 << "Unclosed comment at end of file";
    QTest::newRow("/*\nMy comment */\nfoo = 10")
        << QString("/*\nMy comment */\nfoo = 10") << int(QScriptSyntaxCheckResult::Valid)
        << -1 << -1 << "";
    QTest::newRow("foo = 10 /*")
        << QString("foo = 10 /*") << int(QScriptSyntaxCheckResult::Intermediate)
        << -1 << -1 << "";
    QTest::newRow("foo = 10; /*")
        << QString("foo = 10; /*") << int(QScriptSyntaxCheckResult::Intermediate)
        << 1 << 11 << "Expected `end of file'";
    QTest::newRow("foo = 10 /* My comment */")
        << QString("foo = 10 /* My comment */") << int(QScriptSyntaxCheckResult::Valid)
        << -1 << -1 << "";

    QTest::newRow("/=/")
        << QString("/=/") << int(QScriptSyntaxCheckResult::Valid) << -1 << -1 << "";
    QTest::newRow("/=/g")
        << QString("/=/g") << int(QScriptSyntaxCheckResult::Valid) << -1 << -1 << "";
    QTest::newRow("/a/")
        << QString("/a/") << int(QScriptSyntaxCheckResult::Valid) << -1 << -1 << "";
    QTest::newRow("/a/g")
        << QString("/a/g") << int(QScriptSyntaxCheckResult::Valid) << -1 << -1 << "";
}

void tst_QScriptEngine::checkSyntax()
{
    QFETCH(QString, code);
    QFETCH(int, expectedState);
    QFETCH(int, errorLineNumber);
    QFETCH(int, errorColumnNumber);
    QFETCH(QString, errorMessage);

    QScriptSyntaxCheckResult result = QScriptEngine::checkSyntax(code);

    // assignment
    {
        QScriptSyntaxCheckResult copy = result;
        QCOMPARE(copy.state(), result.state());
        QCOMPARE(copy.errorLineNumber(), result.errorLineNumber());
        QCOMPARE(copy.errorColumnNumber(), result.errorColumnNumber());
        QCOMPARE(copy.errorMessage(), result.errorMessage());
    }
    {
        QScriptSyntaxCheckResult copy(result);
        QCOMPARE(copy.state(), result.state());
        QCOMPARE(copy.errorLineNumber(), result.errorLineNumber());
        QCOMPARE(copy.errorColumnNumber(), result.errorColumnNumber());
        QCOMPARE(copy.errorMessage(), result.errorMessage());
    }

    QEXPECT_FAIL("/*", "QScriptSyntaxCheckResult::state() doesn't return the Intermediate state in most cases", Abort);
    QEXPECT_FAIL("/*\nMy comment", "QScriptSyntaxCheckResult::state() doesn't return the Intermediate state in most cases", Abort);
    QEXPECT_FAIL("foo = 10 /*", "QScriptSyntaxCheckResult::state() doesn't return the Intermediate state in most cases", Abort);
    QEXPECT_FAIL("foo = 10; /*", "QScriptSyntaxCheckResult::state() doesn't return the Intermediate state in most cases", Abort);
    QEXPECT_FAIL("foo[", "QScriptSyntaxCheckResult::state() doesn't return the Intermediate state in most cases", Abort);
    QCOMPARE(result.state(), QScriptSyntaxCheckResult::State(expectedState));
    QEXPECT_FAIL("if (", "QScriptSyntaxCheckResult::state() Intermediate state is broken", Abort);
    QCOMPARE(result.errorLineNumber(), errorLineNumber);
    QCOMPARE(result.errorColumnNumber(), errorColumnNumber);
    if (errorMessage.size())
        QVERIFY(result.errorMessage().contains(errorMessage));
    else
        QCOMPARE(result.errorMessage(), errorMessage);
}

void tst_QScriptEngine::toObject()
{
    QScriptEngine eng;
    QVERIFY(!eng.toObject(eng.undefinedValue()).isValid());
    QVERIFY(!eng.toObject(eng.nullValue()).isValid());
    QVERIFY(!eng.toObject(QScriptValue()).isValid());

    QScriptValue falskt(false);
    {
        QScriptValue tmp = eng.toObject(falskt);
        QVERIFY(tmp.isObject());
        QVERIFY(!falskt.isObject());
        QVERIFY(!falskt.engine());
        QCOMPARE(tmp.toNumber(), falskt.toNumber());
    }

    QScriptValue sant(true);
    {
        QScriptValue tmp = eng.toObject(sant);
        QVERIFY(tmp.isObject());
        QVERIFY(!sant.isObject());
        QVERIFY(!sant.engine());
        QCOMPARE(tmp.toNumber(), sant.toNumber());
    }

    QScriptValue number(123.0);
    {
        QScriptValue tmp = eng.toObject(number);
        QVERIFY(tmp.isObject());
        QVERIFY(!number.isObject());
        QVERIFY(!number.engine());
        QCOMPARE(tmp.toNumber(), number.toNumber());
    }

    QScriptValue str = QScriptValue(&eng, QString("ciao"));
    {
        QScriptValue tmp = eng.toObject(str);
        QVERIFY(tmp.isObject());
        QVERIFY(!str.isObject());
        QCOMPARE(tmp.toString(), str.toString());
    }

    QScriptValue object = eng.evaluate("new Object");
    {
        QScriptValue tmp = eng.toObject(object);
        QVERIFY(tmp.isObject());
        QVERIFY(object.isObject());
        QVERIFY(tmp.strictlyEquals(object));
    }
}

void tst_QScriptEngine::toObjectTwoEngines()
{
    QScriptEngine engine1;
    QScriptEngine engine2;

    {
        QScriptValue null = engine1.nullValue();
        QTest::ignoreMessage(QtWarningMsg, "QScriptEngine::toObject: cannot convert value created in a different engine");
        QVERIFY(!engine2.toObject(null).isValid());
        QVERIFY(null.isValid());
        QTest::ignoreMessage(QtWarningMsg, "QScriptEngine::toObject: cannot convert value created in a different engine");
        QVERIFY(engine2.toObject(null).engine() != &engine2);
    }
    {
        QScriptValue undefined = engine1.undefinedValue();
        QTest::ignoreMessage(QtWarningMsg, "QScriptEngine::toObject: cannot convert value created in a different engine");
        QVERIFY(!engine2.toObject(undefined).isValid());
        QVERIFY(undefined.isValid());
        QTest::ignoreMessage(QtWarningMsg, "QScriptEngine::toObject: cannot convert value created in a different engine");
        QVERIFY(engine2.toObject(undefined).engine() != &engine2);
    }
    {
        QScriptValue value = engine1.evaluate("1");
        QTest::ignoreMessage(QtWarningMsg, "QScriptEngine::toObject: cannot convert value created in a different engine");
        QVERIFY(engine2.toObject(value).engine() != &engine2);
        QVERIFY(!value.isObject());
    }
    {
        QScriptValue string = engine1.evaluate("'Qt'");
        QTest::ignoreMessage(QtWarningMsg, "QScriptEngine::toObject: cannot convert value created in a different engine");
        QVERIFY(engine2.toObject(string).engine() != &engine2);
        QVERIFY(!string.isObject());
    }
    {
        QScriptValue object = engine1.evaluate("new Object");
        QTest::ignoreMessage(QtWarningMsg, "QScriptEngine::toObject: cannot convert value created in a different engine");
        QVERIFY(engine2.toObject(object).engine() != &engine2);
        QVERIFY(object.isObject());
    }
}

void tst_QScriptEngine::newArray()
{
    QScriptEngine eng;
    QScriptValue array = eng.newArray();
    QCOMPARE(array.isValid(), true);
    QCOMPARE(array.isArray(), true);
    QCOMPARE(array.isObject(), true);
    QVERIFY(!array.isFunction());
    // QCOMPARE(array.scriptClass(), (QScriptClass*)0);

    // Prototype should be Array.prototype.
    QCOMPARE(array.prototype().isValid(), true);
    QCOMPARE(array.prototype().isArray(), true);
    QCOMPARE(array.prototype().strictlyEquals(eng.evaluate("Array.prototype")), true);

    QScriptValue arrayWithSize = eng.newArray(42);
    QCOMPARE(arrayWithSize.isValid(), true);
    QCOMPARE(arrayWithSize.isArray(), true);
    QCOMPARE(arrayWithSize.isObject(), true);
    QCOMPARE(arrayWithSize.property("length").toInt32(), 42);

    // task 218092
    {
        QScriptValue ret = eng.evaluate("[].splice(0, 0, 'a')");
        QVERIFY(ret.isArray());
        QCOMPARE(ret.property("length").toInt32(), 0);
    }
    {
        QScriptValue ret = eng.evaluate("['a'].splice(0, 1, 'b')");
        QVERIFY(ret.isArray());
        QCOMPARE(ret.property("length").toInt32(), 1);
    }
    {
        QScriptValue ret = eng.evaluate("['a', 'b'].splice(0, 1, 'c')");
        QVERIFY(ret.isArray());
        QCOMPARE(ret.property("length").toInt32(), 1);
    }
    {
        QScriptValue ret = eng.evaluate("['a', 'b', 'c'].splice(0, 2, 'd')");
        QVERIFY(ret.isArray());
        QCOMPARE(ret.property("length").toInt32(), 2);
    }
    {
        QScriptValue ret = eng.evaluate("['a', 'b', 'c'].splice(1, 2, 'd', 'e', 'f')");
        QVERIFY(ret.isArray());
        QCOMPARE(ret.property("length").toInt32(), 2);
    }
}

void tst_QScriptEngine::uncaughtException()
{
    QScriptEngine eng;
    QScriptValue fun = eng.evaluate("(function foo () { return null; });");
    QVERIFY(!eng.uncaughtException().isValid());
    QVERIFY(fun.isFunction());
    QScriptValue throwFun = eng.evaluate("( function() { throw new Error('Pong'); });");
    QVERIFY(throwFun.isFunction());
    {
        eng.evaluate("a = 10");
        QVERIFY(!eng.hasUncaughtException());
        QVERIFY(!eng.uncaughtException().isValid());
    }
    {
        eng.evaluate("1 = 2");
        QVERIFY(eng.hasUncaughtException());
        eng.clearExceptions();
        QVERIFY(!eng.hasUncaughtException());
    }
    {
        // Check if the call or toString functions can remove the last exception.
        QVERIFY(throwFun.call().isError());
        QVERIFY(eng.hasUncaughtException());
        QScriptValue exception = eng.uncaughtException();
        fun.call();
        exception.toString();
        QVERIFY(eng.hasUncaughtException());
        QVERIFY(eng.uncaughtException().strictlyEquals(exception));
    }
    eng.clearExceptions();
    {
        // Check if in the call function a new exception can override an existing one.
        throwFun.call();
        QVERIFY(eng.hasUncaughtException());
        QScriptValue exception = eng.uncaughtException();
        throwFun.call();
        QVERIFY(eng.hasUncaughtException());
        QVERIFY(!exception.strictlyEquals(eng.uncaughtException()));
    }
    {
        eng.evaluate("throwFun = (function foo () { throw new Error('bla') });");
        eng.evaluate("1;\nthrowFun();");
        QVERIFY(eng.hasUncaughtException());
        QCOMPARE(eng.uncaughtExceptionLineNumber(), 1);
        eng.clearExceptions();
        QVERIFY(!eng.hasUncaughtException());
    }
    for (int x = 1; x < 4; ++x) {
        QScriptValue ret = eng.evaluate("a = 10;\nb = 20;\n0 = 0;\n",
                                        QString::fromLatin1("FooScript") + QString::number(x),
                                        /* lineNumber */ x);
        QVERIFY(eng.hasUncaughtException());
        QCOMPARE(eng.uncaughtExceptionLineNumber(), x + 2);
        QVERIFY(eng.uncaughtException().strictlyEquals(ret));
        QVERIFY(eng.hasUncaughtException());
        QVERIFY(eng.uncaughtException().strictlyEquals(ret));
        QString backtrace = QString::fromLatin1("<anonymous>()@FooScript") + QString::number(x) + ":" + QString::number(x + 2);
        QEXPECT_FAIL("", "FIXME: not implemented", Continue);
        QCOMPARE(eng.uncaughtExceptionBacktrace().join(""), backtrace);
        QVERIFY(fun.call().isNull());
        QVERIFY(eng.hasUncaughtException());
        QCOMPARE(eng.uncaughtExceptionLineNumber(), x + 2);
        QVERIFY(eng.uncaughtException().strictlyEquals(ret));
        eng.clearExceptions();
        QVERIFY(!eng.hasUncaughtException());
        QCOMPARE(eng.uncaughtExceptionLineNumber(), -1);
        QVERIFY(!eng.uncaughtException().isValid());
        eng.evaluate("2 = 3");
        QVERIFY(eng.hasUncaughtException());
        QScriptValue ret2 = throwFun.call();
        QVERIFY(ret2.isError());
        QVERIFY(eng.hasUncaughtException());
        QVERIFY(eng.uncaughtException().strictlyEquals(ret2));
        QCOMPARE(eng.uncaughtExceptionLineNumber(), 1);
        eng.clearExceptions();
        QVERIFY(!eng.hasUncaughtException());
        eng.evaluate("1 + 2");
        QVERIFY(!eng.hasUncaughtException());
    }
}

QTEST_MAIN(tst_QScriptEngine)
#include "tst_qscriptengine.moc"
