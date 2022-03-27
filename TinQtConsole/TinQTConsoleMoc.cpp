/****************************************************************************
** Meta object code from reading C++ file 'TinQTConsole.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.3.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "TinQTConsole.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'TinQTConsole.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.3.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_CConsoleInput_t {
    QByteArrayData data[17];
    char stringdata[338];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_CConsoleInput_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_CConsoleInput_t qt_meta_stringdata_CConsoleInput = {
    {
QT_MOC_LITERAL(0, 0, 13),
QT_MOC_LITERAL(1, 14, 22),
QT_MOC_LITERAL(2, 37, 0),
QT_MOC_LITERAL(3, 38, 20),
QT_MOC_LITERAL(4, 59, 24),
QT_MOC_LITERAL(5, 84, 15),
QT_MOC_LITERAL(6, 100, 23),
QT_MOC_LITERAL(7, 124, 19),
QT_MOC_LITERAL(8, 144, 19),
QT_MOC_LITERAL(9, 164, 18),
QT_MOC_LITERAL(10, 183, 19),
QT_MOC_LITERAL(11, 203, 21),
QT_MOC_LITERAL(12, 225, 22),
QT_MOC_LITERAL(13, 248, 15),
QT_MOC_LITERAL(14, 264, 23),
QT_MOC_LITERAL(15, 288, 25),
QT_MOC_LITERAL(16, 314, 23)
    },
    "CConsoleInput\0OnButtonConnectPressed\0"
    "\0OnAutoConnectClicked\0OnConnectIPReturnPressed\0"
    "OnReturnPressed\0OnFileEditReturnPressed\0"
    "OnButtonStopPressed\0OnButtonExecPressed\0"
    "OnButtonRunPressed\0OnButtonStepPressed\0"
    "OnButtonStepInPressed\0OnButtonStepOutPressed\0"
    "OnFindEditFocus\0OnFindEditReturnPressed\0"
    "OnUnhashEditReturnPressed\0"
    "OnFunctionAssistPressed"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_CConsoleInput[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
      15,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   89,    2, 0x0a /* Public */,
       3,    0,   90,    2, 0x0a /* Public */,
       4,    0,   91,    2, 0x0a /* Public */,
       5,    0,   92,    2, 0x0a /* Public */,
       6,    0,   93,    2, 0x0a /* Public */,
       7,    0,   94,    2, 0x0a /* Public */,
       8,    0,   95,    2, 0x0a /* Public */,
       9,    0,   96,    2, 0x0a /* Public */,
      10,    0,   97,    2, 0x0a /* Public */,
      11,    0,   98,    2, 0x0a /* Public */,
      12,    0,   99,    2, 0x0a /* Public */,
      13,    0,  100,    2, 0x0a /* Public */,
      14,    0,  101,    2, 0x0a /* Public */,
      15,    0,  102,    2, 0x0a /* Public */,
      16,    0,  103,    2, 0x0a /* Public */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void CConsoleInput::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        CConsoleInput *_t = static_cast<CConsoleInput *>(_o);
        switch (_id) {
        case 0: _t->OnButtonConnectPressed(); break;
        case 1: _t->OnAutoConnectClicked(); break;
        case 2: _t->OnConnectIPReturnPressed(); break;
        case 3: _t->OnReturnPressed(); break;
        case 4: _t->OnFileEditReturnPressed(); break;
        case 5: _t->OnButtonStopPressed(); break;
        case 6: _t->OnButtonExecPressed(); break;
        case 7: _t->OnButtonRunPressed(); break;
        case 8: _t->OnButtonStepPressed(); break;
        case 9: _t->OnButtonStepInPressed(); break;
        case 10: _t->OnButtonStepOutPressed(); break;
        case 11: _t->OnFindEditFocus(); break;
        case 12: _t->OnFindEditReturnPressed(); break;
        case 13: _t->OnUnhashEditReturnPressed(); break;
        case 14: _t->OnFunctionAssistPressed(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObject CConsoleInput::staticMetaObject = {
    { &QLineEdit::staticMetaObject, qt_meta_stringdata_CConsoleInput.data,
      qt_meta_data_CConsoleInput,  qt_static_metacall, 0, 0}
};


const QMetaObject *CConsoleInput::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CConsoleInput::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_CConsoleInput.stringdata))
        return static_cast<void*>(const_cast< CConsoleInput*>(this));
    return QLineEdit::qt_metacast(_clname);
}

int CConsoleInput::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QLineEdit::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 15)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 15;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 15)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 15;
    }
    return _id;
}
struct qt_meta_stringdata_CConsoleOutput_t {
    QByteArrayData data[6];
    char stringdata[68];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_CConsoleOutput_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_CConsoleOutput_t qt_meta_stringdata_CConsoleOutput = {
    {
QT_MOC_LITERAL(0, 0, 14),
QT_MOC_LITERAL(1, 15, 6),
QT_MOC_LITERAL(2, 22, 0),
QT_MOC_LITERAL(3, 23, 22),
QT_MOC_LITERAL(4, 46, 16),
QT_MOC_LITERAL(5, 63, 4)
    },
    "CConsoleOutput\0Update\0\0HandleTextEntryClicked\0"
    "QListWidgetItem*\0item"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_CConsoleOutput[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   24,    2, 0x0a /* Public */,
       3,    1,   25,    2, 0x0a /* Public */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 4,    5,

       0        // eod
};

void CConsoleOutput::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        CConsoleOutput *_t = static_cast<CConsoleOutput *>(_o);
        switch (_id) {
        case 0: _t->Update(); break;
        case 1: _t->HandleTextEntryClicked((*reinterpret_cast< QListWidgetItem*(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObject CConsoleOutput::staticMetaObject = {
    { &QListWidget::staticMetaObject, qt_meta_stringdata_CConsoleOutput.data,
      qt_meta_data_CConsoleOutput,  qt_static_metacall, 0, 0}
};


const QMetaObject *CConsoleOutput::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CConsoleOutput::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_CConsoleOutput.stringdata))
        return static_cast<void*>(const_cast< CConsoleOutput*>(this));
    return QListWidget::qt_metacast(_clname);
}

int CConsoleOutput::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QListWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 2)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 2;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
