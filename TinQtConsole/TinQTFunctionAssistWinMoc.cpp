/****************************************************************************
** Meta object code from reading C++ file 'TinQTFunctionAssistWin.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.3.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "TinQTFunctionAssistWin.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'TinQTFunctionAssistWin.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.3.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_CDebugFunctionAssistWin_t {
    QByteArrayData data[5];
    char stringdata[92];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_CDebugFunctionAssistWin_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_CDebugFunctionAssistWin_t qt_meta_stringdata_CDebugFunctionAssistWin = {
    {
QT_MOC_LITERAL(0, 0, 23),
QT_MOC_LITERAL(1, 24, 21),
QT_MOC_LITERAL(2, 46, 0),
QT_MOC_LITERAL(3, 47, 21),
QT_MOC_LITERAL(4, 69, 22)
    },
    "CDebugFunctionAssistWin\0OnButtonMethodPressed\0"
    "\0OnButtonBrowsePressed\0OnButtonCreatedPressed"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_CDebugFunctionAssistWin[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   29,    2, 0x0a /* Public */,
       3,    0,   30,    2, 0x0a /* Public */,
       4,    0,   31,    2, 0x0a /* Public */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void CDebugFunctionAssistWin::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        CDebugFunctionAssistWin *_t = static_cast<CDebugFunctionAssistWin *>(_o);
        switch (_id) {
        case 0: _t->OnButtonMethodPressed(); break;
        case 1: _t->OnButtonBrowsePressed(); break;
        case 2: _t->OnButtonCreatedPressed(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObject CDebugFunctionAssistWin::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_CDebugFunctionAssistWin.data,
      qt_meta_data_CDebugFunctionAssistWin,  qt_static_metacall, 0, 0}
};


const QMetaObject *CDebugFunctionAssistWin::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CDebugFunctionAssistWin::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_CDebugFunctionAssistWin.stringdata))
        return static_cast<void*>(const_cast< CDebugFunctionAssistWin*>(this));
    return QWidget::qt_metacast(_clname);
}

int CDebugFunctionAssistWin::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 3;
    }
    return _id;
}
struct qt_meta_stringdata_CFunctionAssistInput_t {
    QByteArrayData data[1];
    char stringdata[21];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_CFunctionAssistInput_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_CFunctionAssistInput_t qt_meta_stringdata_CFunctionAssistInput = {
    {
QT_MOC_LITERAL(0, 0, 20)
    },
    "CFunctionAssistInput"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_CFunctionAssistInput[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

void CFunctionAssistInput::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

const QMetaObject CFunctionAssistInput::staticMetaObject = {
    { &QLineEdit::staticMetaObject, qt_meta_stringdata_CFunctionAssistInput.data,
      qt_meta_data_CFunctionAssistInput,  qt_static_metacall, 0, 0}
};


const QMetaObject *CFunctionAssistInput::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CFunctionAssistInput::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_CFunctionAssistInput.stringdata))
        return static_cast<void*>(const_cast< CFunctionAssistInput*>(this));
    return QLineEdit::qt_metacast(_clname);
}

int CFunctionAssistInput::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QLineEdit::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
struct qt_meta_stringdata_CFunctionAssistList_t {
    QByteArrayData data[5];
    char stringdata[64];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_CFunctionAssistList_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_CFunctionAssistList_t qt_meta_stringdata_CFunctionAssistList = {
    {
QT_MOC_LITERAL(0, 0, 19),
QT_MOC_LITERAL(1, 20, 9),
QT_MOC_LITERAL(2, 30, 0),
QT_MOC_LITERAL(3, 31, 16),
QT_MOC_LITERAL(4, 48, 15)
    },
    "CFunctionAssistList\0OnClicked\0\0"
    "QTreeWidgetItem*\0OnDoubleClicked"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_CFunctionAssistList[] = {

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
       1,    1,   24,    2, 0x0a /* Public */,
       4,    1,   27,    2, 0x0a /* Public */,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 3,    2,
    QMetaType::Void, 0x80000000 | 3,    2,

       0        // eod
};

void CFunctionAssistList::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        CFunctionAssistList *_t = static_cast<CFunctionAssistList *>(_o);
        switch (_id) {
        case 0: _t->OnClicked((*reinterpret_cast< QTreeWidgetItem*(*)>(_a[1]))); break;
        case 1: _t->OnDoubleClicked((*reinterpret_cast< QTreeWidgetItem*(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObject CFunctionAssistList::staticMetaObject = {
    { &QTreeWidget::staticMetaObject, qt_meta_stringdata_CFunctionAssistList.data,
      qt_meta_data_CFunctionAssistList,  qt_static_metacall, 0, 0}
};


const QMetaObject *CFunctionAssistList::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CFunctionAssistList::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_CFunctionAssistList.stringdata))
        return static_cast<void*>(const_cast< CFunctionAssistList*>(this));
    return QTreeWidget::qt_metacast(_clname);
}

int CFunctionAssistList::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QTreeWidget::qt_metacall(_c, _id, _a);
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
struct qt_meta_stringdata_CFunctionParameterList_t {
    QByteArrayData data[1];
    char stringdata[23];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_CFunctionParameterList_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_CFunctionParameterList_t qt_meta_stringdata_CFunctionParameterList = {
    {
QT_MOC_LITERAL(0, 0, 22)
    },
    "CFunctionParameterList"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_CFunctionParameterList[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

void CFunctionParameterList::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

const QMetaObject CFunctionParameterList::staticMetaObject = {
    { &QTreeWidget::staticMetaObject, qt_meta_stringdata_CFunctionParameterList.data,
      qt_meta_data_CFunctionParameterList,  qt_static_metacall, 0, 0}
};


const QMetaObject *CFunctionParameterList::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CFunctionParameterList::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_CFunctionParameterList.stringdata))
        return static_cast<void*>(const_cast< CFunctionParameterList*>(this));
    return QTreeWidget::qt_metacast(_clname);
}

int CFunctionParameterList::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QTreeWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
QT_END_MOC_NAMESPACE
